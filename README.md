# Smart Bin - IoT Waste Management System

This project implements an intelligent waste bin system using ESP32, ultrasonic sensors, and Firebase integration. The smart bin features automated lid control, waste level monitoring, gas detection, and real-time data tracking through a web interface.

## Features

- 🚀 Automated lid control with proximity detection
- 📊 Real-time waste level monitoring
- 🌡️ Gas level detection (MQ4 sensor)
- 🔥 Firebase Realtime Database integration
- 🌐 Web API for configuration and monitoring
- 💡 LED indicators for bin status
- ⚙️ Configurable parameters via API

## Hardware Requirements

- ESP32 microcontroller
- 2x HC-SR04 Ultrasonic sensors
  - One for bin level detection
  - One for user proximity detection
- MQ4 gas sensor
- Servo motor for lid control
- LED indicator
- Power supply

## Pin Configuration

```cpp
#define TRIG_BIN 5    // Bin level sensor trigger
#define ECHO_BIN 4    // Bin level sensor echo
#define TRIG_USER 18  // User detection sensor trigger
#define ECHO_USER 19  // User detection sensor echo
#define MQ4_PIN 34    // Gas sensor analog input
#define LED_DOOR 2    // LED indicator
// Servo motor on pin 13
```

## Setup Instructions

1. Install required Arduino libraries:
   - WiFi
   - Firebase ESP Client
   - WebServer
   - ArduinoJson
   - ESP32Servo

2. Configure WiFi and Firebase credentials in the code:
   ```cpp
   #define WIFI_SSID "your_wifi_ssid"
   #define WIFI_PASS "your_wifi_password"
   #define DB_URL "your_firebase_db_url"
   #define API_KEY "your_firebase_api_key"
   ```

3. Upload the code to your ESP32 board
4. The device will automatically connect to WiFi and Firebase
5. Access the web interface at the ESP32's IP address

## Web API Endpoints

### GET /
- Basic health check endpoint
- Returns: "Smart Bin Web Interface 🌍"

### GET /health
- System health status check
- Returns JSON with sensor and motor status
```json
{
  "bin_sensor": "OK/FAIL",
  "user_sensor": "OK/FAIL",
  "gas_sensor": "OK/FAIL",
  "servo_motor": "OK/FAIL"
}
```

### POST /update-door-time
- Update door closing delay time
- Request body:
```json
{
  "door_close_time": 5.0
}
```

### POST /update-bin-height
- Update bin depth configuration
- Request body:
```json
{
  "bin_depth": 27.29
}
```

### POST /update-motor-speed
- Update servo motor speed
- Request body:
```json
{
  "motor_speed": 15
}
```
- Valid range: 5-50 ms (lower = faster)

## Firebase Data Structure

### /SmartBin
```json
{
  "bin_precentage": 75.5,    // Fill percentage
  "remaining_cm": 10.5,      // Empty space in cm
  "userDetect": 1,          // User presence (1/0)
  "gas_level": 500,         // Gas sensor reading
  "isFull": 0              // Bin full status (1/0)
}
```

### /config
```json
{
  "door_close_time": 5.0,   // Seconds before auto-close
  "bin_depth": 27.29,      // Total bin depth in cm
  "motor_speed": 15        // Servo movement delay in ms
}
```

### /connection
```json
{
  "ip_address": "192.168.1.100",
  "ssid": "WiFi_Name",
  "password": "WiFi_Password"
}
```

## Operation

1. The system continuously monitors user proximity and bin levels
2. When a user approaches (within 50cm):
   - The lid automatically opens
   - LED indicator turns on
   - Door remains open for set duration
   - LED blinks during last 5 seconds
   - Lid closes automatically
3. Bin data updates every 5 seconds to Firebase
4. Gas levels are continuously monitored
5. System reports bin full status when level exceeds 85%

## Troubleshooting

- If sensors show "FAIL" in health check:
  - Verify wiring connections
  - Check power supply
  - Ensure proper pin configuration
- If Firebase updates fail:
  - Check internet connectivity
  - Verify Firebase credentials
  - Check serial monitor for error messages

## Contributing

Feel free to submit issues and enhancement requests!
