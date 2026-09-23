# Smart_Energy_Meter

This project is a smart energy meter built during Fablab Rwanda's TechUp Skills Program as a group project. It monitors a household's electricity usage in real time and helps prevent unexpected power loss when a purchased electricity budget runs low.

## What it does

- Tracks voltage, current, power, and cumulative energy consumption (V/I/P/E) in real time
- Displays live readings on an LCD screen
- Warns the user when their purchased electricity budget is close to running out
- Automatically shuts off high-power devices through a relay before the budget is fully exhausted, preventing an unexpected blackout

## Hardware

- ESP32 microcontroller
- Current/voltage sensing for real-time power and energy calculation
- LCD display for live V/I/P/E readout
- Relay module for automatic load disconnection (demonstrated in testing using a domestic bulb as a switchable high-power load)

## Why this matters

Many households on prepaid electricity plans in East Africa lose power unexpectedly when their balance runs out mid-use. This project explores a low-cost way to give households advance warning and automatically protect high-power appliances before that happens.

## Team

Built as part of Fablab Rwanda's TechUp Skills Program, a group project focused on applying embedded systems skills to real household energy challenges.
