#include <ACS712.h>

// Define the pin for the ACS712 sensor
#define CURRENT_SENSOR_PIN A0 

// Define the pin for the Relay
#define RELAY_PIN 7 

// Define the pin for the Buzzer
#define BUZZER_PIN 9 // Digital Pin 9 for the buzzer

// Define the voltage supply for power calculation
#define VOLTAGE_SUPPLY 230.0

// Define the current threshold for "theft" detection in Amps
#define THEFT_THRESHOLD_AMPS 0.2 // 0.2 Amps (200 mA)

// ACS712 sensor instance (use the observed midpoint)
ACS712 ACS(CURRENT_SENSOR_PIN, 0.185, 512); 

// Variables for power and energy calculation
float accumulatedPowerWh = 0.0;
unsigned long previousMillis = 0; // For time tracking of energy accumulation

// New variables for non-blocking buzzer control
unsigned long previousBuzzerToggleMillis = 0; // Stores last time buzzer state was toggled
int buzzerState = LOW;                      // Current state of the buzzer (HIGH/LOW)
unsigned int buzzerOnDuration = 0;           // How long the buzzer stays ON (ms)
unsigned int buzzerOffDuration = 0;          // How long the buzzer stays OFF (ms)

// NEW: Variables for non-blocking serial print interval
unsigned long previousSerialPrintMillis = 0; // Stores last time data was printed
const long serialPrintInterval = 2000;       // Print every 2000 milliseconds (2 seconds)

void setup() {
  Serial.begin(9600);
  Serial.println("Smart Energy Meter - Virtual Terminal Display");

  Serial.print("Sensor MidPoint: ");
  Serial.println(ACS.getMidPoint()); 

  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, HIGH); // Ensure power is initially ON

  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW); // Ensure buzzer is initially OFF

  // Initialize previousMillis for accurate energy calculation from start
  previousMillis = millis(); 
  previousSerialPrintMillis = millis(); // Initialize print timer
}

void loop() {
  // --- Current, Power, Energy Measurement (runs continuously) ---
  float current_mA = ACS.mA_AC();
  float current_A = current_mA / 1000.0; 

  float power = abs(current_A) * VOLTAGE_SUPPLY; 

  unsigned long currentMillis = millis(); // Get current time once per loop for all timers
  
  // Calculate elapsed time for energy accumulation
  // This part remains crucial for accurate energy tracking, regardless of print speed.
  float timeElapsedHours = (float)(currentMillis - previousMillis) / 3600000.0; 
  accumulatedPowerWh += power * timeElapsedHours; 
  previousMillis = currentMillis; // Update for the next energy calculation

  float accumulatedEnergyKWh = accumulatedPowerWh / 1000.0; 

  // --- Print results to Virtual Terminal (NEW: Only print if enough time has passed) ---
  if (currentMillis - previousSerialPrintMillis >= serialPrintInterval) {
    previousSerialPrintMillis = currentMillis; // Reset the print timer

    Serial.print("Current: ");
    Serial.print(current_A, 3); 
    Serial.print("A | Power: ");
    Serial.print(power, 2); 
    Serial.print("W | Energy: ");
    Serial.print(accumulatedEnergyKWh, 5); 
    Serial.println("kWh");
  }

  // --- Theft Detection Logic and Relay Control ---
  bool theftDetected = false; 
  if (abs(current_A) > THEFT_THRESHOLD_AMPS) {
    // Only print "ALERT" message when current is over threshold and *if* it's time to print
    if (currentMillis - previousSerialPrintMillis >= serialPrintInterval) { // Check again for printing
      Serial.println("ALERT: High current detected! Power disconnected.");
    }
    digitalWrite(RELAY_PIN, LOW);   // Disconnect power
    theftDetected = true; 
  } else {
    digitalWrite(RELAY_PIN, HIGH);  // Keep power connected
    theftDetected = false; 
  }

  // --- Buzzer Control Logic (Non-Blocking) ---
  if (theftDetected) {
    buzzerOnDuration = 50; 
    buzzerOffDuration = 50; 
  } else {
    buzzerOnDuration = 100; 
    buzzerOffDuration = 900; 
  }

  if (buzzerState == LOW && (currentMillis - previousBuzzerToggleMillis >= buzzerOffDuration)) {
    digitalWrite(BUZZER_PIN, HIGH); 
    buzzerState = HIGH;
    previousBuzzerToggleMillis = currentMillis; 
  } else if (buzzerState == HIGH && (currentMillis - previousBuzzerToggleMillis >= buzzerOnDuration)) {
    digitalWrite(BUZZER_PIN, LOW); 
    buzzerState = LOW;
    previousBuzzerToggleMillis = currentMillis; 
  }

  // NO delay() here! The loop runs as fast as possible, and timers manage events.
}
