/*
  Smart Energy Meter — ESP32
  Fablab Rwanda — TechUp Skills Program (Group Project)

  What this does:
  - Reads AC voltage and current using a voltage sensing module and a CT
    (current transformer) clamp sensor
  - Computes real-time power (W) and accumulates energy (kWh)
  - Displays live Voltage / Current / Power / Energy on a 16x2 I2C LCD
  - Mirrors those same readings to a Blynk cloud dashboard in real time
  - Lets the user set a "baseline" (purchased electricity budget, in kWh)
    from the Blynk app
  - Warns and automatically disconnects a high-power load via relay when
    the accumulated energy gets close to / reaches that budget
  - Allows manual on/off override of the load from the Blynk app

  NOTE ON CALIBRATION:
  The constants marked CALIBRATE below depend on your exact CT sensor,
  voltage sensing circuit, and burden resistor. The values here are
  reasonable starting points, not measured values — you'll need to tune
  them against a known load (e.g. a lamp of known wattage) before the
  readings are accurate.
*/

#define BLYNK_TEMPLATE_ID   "YOUR_TEMPLATE_ID"      // from Blynk.Console
#define BLYNK_TEMPLATE_NAME "Smart Energy Meter"
#define BLYNK_AUTH_TOKEN    "YOUR_AUTH_TOKEN"        // from Blynk.Console

#include <WiFi.h>
#include <BlynkSimpleEsp32.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// ---------- WiFi credentials ----------
char ssid[] = "YOUR_WIFI_SSID";
char pass[] = "YOUR_WIFI_PASSWORD";

// ---------- Pin assignments ----------
const int VOLTAGE_SENSOR_PIN = 34;   // ADC pin for voltage sensing module
const int CURRENT_SENSOR_PIN = 35;   // ADC pin for CT current sensor
const int RELAY_PIN          = 26;   // Relay controlling the load

// ---------- Blynk virtual pins ----------
#define VPIN_VOLTAGE   V0
#define VPIN_CURRENT   V1
#define VPIN_POWER     V2
#define VPIN_ENERGY    V3
#define VPIN_BASELINE  V4   // Purchased budget (kWh), set from the app
#define VPIN_ONOFF     V5   // Manual on/off switch from the app

// ---------- LCD setup (adjust address if needed: common are 0x27 or 0x3F) ----------
LiquidCrystal_I2C lcd(0x27, 16, 2);

// ---------- Calibration constants (CALIBRATE for your hardware) ----------
const float VOLTAGE_CALIBRATION = 220.0 / 2048.0; // ADC midpoint -> ~220V reference
const float CURRENT_CALIBRATION = 30.0 / 2048.0;  // e.g. for a 30A CT clamp

// ---------- State ----------
float baselineKWh   = 5.0;   // default purchased budget until user sets one
float energyKWh     = 0.0;   // accumulated energy since last reset
bool  loadIsOn       = true;
bool  budgetWarned   = false;

unsigned long lastSampleMillis = 0;
const unsigned long SAMPLE_INTERVAL_MS = 1000; // 1 reading per second

BlynkTimer timer;

// Reads the voltage sensor pin and converts to an RMS voltage estimate
float readVoltageRMS() {
  int raw = analogRead(VOLTAGE_SENSOR_PIN);
  float voltage = raw * VOLTAGE_CALIBRATION;
  return voltage;
}

// Reads the CT current sensor pin and converts to an RMS current estimate
float readCurrentRMS() {
  int raw = analogRead(CURRENT_SENSOR_PIN);
  float current = raw * CURRENT_CALIBRATION;
  return current;
}

void setLoad(bool on) {
  loadIsOn = on;
  digitalWrite(RELAY_PIN, on ? HIGH : LOW);
  Blynk.virtualWrite(VPIN_ONOFF, on ? 1 : 0);
}

// Called whenever the user moves the on/off switch in the Blynk app
BLYNK_WRITE(VPIN_ONOFF) {
  int value = param.asInt();
  setLoad(value == 1);
}

// Called whenever the user updates the baseline (budget) field in the app
BLYNK_WRITE(VPIN_BASELINE) {
  baselineKWh = param.asFloat();
  budgetWarned = false; // allow a fresh warning cycle for the new budget
}

void takeMeasurementAndUpdate() {
  float voltage = readVoltageRMS();
  float current = readCurrentRMS();
  float power   = voltage * current; // simplified real power (assumes power factor ~1)

  // Integrate power over the sample interval to accumulate energy
  float hoursElapsed = SAMPLE_INTERVAL_MS / 3600000.0;
  energyKWh += (power * hoursElapsed) / 1000.0;

  // ---- Update LCD ----
  lcd.setCursor(0, 0);
  lcd.print("V:");
  lcd.print(voltage, 1);
  lcd.print(" I:");
  lcd.print(current, 2);

  lcd.setCursor(0, 1);
  lcd.print("P:");
  lcd.print(power, 1);
  lcd.print("W E:");
  lcd.print(energyKWh, 2);

  // ---- Push readings to the Blynk dashboard ----
  Blynk.virtualWrite(VPIN_VOLTAGE, voltage);
  Blynk.virtualWrite(VPIN_CURRENT, current);
  Blynk.virtualWrite(VPIN_POWER, power);
  Blynk.virtualWrite(VPIN_ENERGY, energyKWh);

  // ---- Budget check: warn, then auto-shutoff ----
  float percentUsed = (energyKWh / baselineKWh) * 100.0;

  if (percentUsed >= 90.0 && percentUsed < 100.0 && !budgetWarned) {
    Blynk.logEvent("low_budget", "Warning: electricity budget almost exhausted");
    budgetWarned = true;
  }

  if (percentUsed >= 100.0 && loadIsOn) {
    setLoad(false); // automatically disconnect the high-power load
    Blynk.logEvent("budget_exhausted", "Budget exhausted: load automatically switched off");
  }
}

void setup() {
  Serial.begin(115200);

  pinMode(RELAY_PIN, OUTPUT);
  setLoad(true); // load starts on

  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("Smart Energy");
  lcd.setCursor(0, 1);
  lcd.print("Meter starting...");

  Blynk.begin(BLYNK_AUTH_TOKEN, ssid, pass);

  // Sync the baseline value already stored on the server, if any
  Blynk.syncVirtual(VPIN_BASELINE);

  timer.setInterval(SAMPLE_INTERVAL_MS, takeMeasurementAndUpdate);

  delay(1000);
  lcd.clear();
}

void loop() {
  Blynk.run();
  timer.run();
}
