# ESP32 + Firebase Android

This repository contains an ESP32 firmware sketch and an Android application that demonstrate using Firebase together with an ESP32 device.

Contents

- `ESP32_Firebase.ino` – the main Arduino/ESP32 sketch located at the repository root.
- `ESP32_Firebase/` – the Android project built with Gradle. The `app/` module holds the Android source and `app/google-services.json` (Firebase configuration) — that file is already included in the repo.

Purpose

- Example of connecting an ESP32 to Wi‑Fi and sending/receiving data with Firebase.
- An Android app to interact with Firebase data and/or manage the device.

Prerequisites

- Android: Java 11+ (check the project Gradle configuration), Android Studio (recommended) or use the Gradle wrapper.
- ESP32: Arduino IDE, PlatformIO, or Arduino CLI with ESP32 board support installed.
- ESP32 Firebase client libraries (for example `Firebase ESP Client` or `FirebaseESP32`) and `ArduinoJson` if the sketch uses JSON.

Notes

- `app/google-services.json` must contain the Firebase configuration for the Android application. If you change the applicationId (package name) or Firebase project, update this file accordingly.
- Importing the project into Android Studio is the easiest way to resolve dependencies automatically via Gradle.

ESP32 (firmware) instructions

1. Open `ESP32_Firebase.ino` in the Arduino IDE or PlatformIO.
2. Select the appropriate ESP32 board (for example: "ESP32 Dev Module") and the correct serial port.
3. Install required libraries used by the sketch (examples):
   - Firebase client for ESP32 (e.g. `Firebase ESP Client` by Mobizt or `FirebaseESP32`),
   - `ArduinoJson`,
   - `WiFi`.
4. Update configuration values in the sketch: Firebase credentials/endpoints (API key / project IDs) as needed.
5. Upload the sketch to the board and open the Serial Monitor to view logs (common baud rate: 115200).

