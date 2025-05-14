# Smart Bin Web Server API Documentation

This document describes the available API endpoints for interacting with the Smart Bin Web Server. The server allows you to monitor and control the smart bin's functionality via HTTP requests.

## Base URL

The base URL for the web server is the IP address of the ESP32 module. All endpoints are relative to this IP address.

Example: `http://<ESP32_IP>`

---

## Endpoints

### 1. `/`

**GET**  
Returns a simple message indicating that the web server is running.

Response:

```txt
Smart Bin Web Interface 🌍
```

---

### 2. `/health`

**GET**  
Returns the status of all sensors in the system.

Response:

```json
{
  "bin_sensor": "OK",
  "user_sensor": "OK",
  "gas_sensor": "OK",
  "servo_motor": "OK"
}
```

---

### 3. `/update-bin-height`

**POST**  
This endpoint allows you to update the maximum bin height (`bin_depth`) parameter.

Parameters:

- `bin_depth` (float): The new bin depth value.

Request body:

```json
{
  "bin_depth": 30.0
}
```

Response:

```txt
Bin depth updated!
```

---

### 4. `/update-door-time`

**POST**  
This endpoint allows you to update the door close time (`door_close_time`) parameter.

Parameters:

- `door_close_time` (float): The new door close time in seconds.

Request body:

```json
{
  "door_close_time": 10.0
}
```

Response:

```txt
Door close time updated!
```

---

### 5. `/update-motor-speed`

**POST**  
This endpoint allows you to update the speed of the servo motor.

Parameters:

- `motor_speed` (int): The new motor speed value (range: 0-255).

Request body:

```json
{
  "motor_speed": 150
}
```

Response:

```txt
Motor speed updated!
```

---

## Example Usage

1. **Update Bin Depth**  
   Send a POST request to `http://<ESP32_IP>/update-bin-height` with the JSON body:

   ```json
   {
     "bin_depth": 30.0
   }
   ```

2. **Update Door Close Time**  
   Send a POST request to `http://<ESP32_IP>/update-door-time` with the JSON body:
   ```json
   {
     "door_close_time": 10.0
   }
   ```

---

## Firebase Integration

The server is integrated with Firebase, and the values for `bin_depth` and `door_close_time` are stored in Firebase under the `/config` node. You can update these values dynamically via the provided API endpoints, and they will be reflected across all connected devices.
