#include <WiFi.h>
#include <Firebase_ESP_Client.h>
#include <WebServer.h>
#include <ArduinoJson.h>
#include <ESP32Servo.h>

#include <addons/TokenHelper.h>
#include <addons/RTDBHelper.h>

// WiFi & Firebase creds
#define WIFI_SSID "<WIFI SSID>"
#define WIFI_PASS "<WIFI PASSWORD>"
#define DB_URL "<FIREBASE DB URL>"
#define API_KEY " <FIREBASE API KEY>"

// Pin setup
#define TRIG_BIN 5
#define ECHO_BIN 4
#define TRIG_USER 18
#define ECHO_USER 19
#define MQ4_PIN 34
#define LED_DOOR 2

FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig cfg;
WebServer server(80);

Servo doorServo; // Create a Servo object for the bin door

unsigned long lastUpdate = 0;
const int UPDATE_INTERVAL = 5000;

float doorCloseTime = 5;
float binDepth = 27.29;

int speedDelay = 15;
int currentAngle = 135;
int targetAngle = 135;

void moveServoSmoothly(int target)
{
  int step = (target > currentAngle) ? 1 : -1;
  while (currentAngle != target)
  {
    currentAngle += step;
    doorServo.write(currentAngle);
    delay(speedDelay);
  }
}

void updateConnectionInfo()
{
  FirebaseJson json;
  json.set("ip_address", WiFi.localIP().toString());
  json.set("ssid", WIFI_SSID);
  json.set("password", WIFI_PASS);

  if (Firebase.RTDB.setJSON(&fbdo, "/connection", &json))
  {
    Serial.println("📡 Connection info updated in Firebase");
  }
  else
  {
    Serial.print("❌ Failed to update connection info: ");
    Serial.println(fbdo.errorReason());
  }
}

void setup()
{
  Serial.begin(115200);

  pinMode(TRIG_BIN, OUTPUT);
  pinMode(ECHO_BIN, INPUT);
  pinMode(TRIG_USER, OUTPUT);
  pinMode(ECHO_USER, INPUT);
  pinMode(MQ4_PIN, INPUT);
  pinMode(LED_DOOR, OUTPUT);
  digitalWrite(LED_DOOR, LOW);

  doorServo.attach(13);
  doorServo.write(135);

  WiFi.begin(WIFI_SSID, WIFI_PASS);
  Serial.print("WiFi");
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }
  Serial.println(" ✅ Connected!");

  cfg.api_key = API_KEY;
  cfg.database_url = DB_URL;
  Firebase.signUp(&cfg, &auth, "", "");
  Firebase.begin(&cfg, &auth);
  Firebase.reconnectWiFi(true);

  // Update connection info in Firebase
  updateConnectionInfo();

  server.on("/", handleRoot);
  server.on("/health", handleHealth);
  server.on("/update-door-time", handleUpdateDoorTime);
  server.on("/update-bin-height", handleUpdateBinHeight);
  server.on("/update-motor-speed", handleUpdateMotorSpeed);
  server.begin();
  Serial.println("🌐 WebServer started on port 80");

  // Fetch current config from Firebase
  fetchConfig();
}

float readDistance(int trig, int echo)
{
  digitalWrite(trig, LOW);
  delayMicroseconds(2);
  digitalWrite(trig, HIGH);
  delayMicroseconds(10);
  digitalWrite(trig, LOW);
  long d = pulseIn(echo, HIGH, 30000); // timeout 30ms
  return (d == 0) ? -1 : (d * 0.0343) / 2.0;
}

float binLevel()
{
  const float sensorHeightOffset = 2.0;
  const float emptyBinThreshold = 2.0;

  float distance = readDistance(TRIG_BIN, ECHO_BIN);

  if (distance < 0 || distance > (binDepth + sensorHeightOffset))
  {
    return binDepth;
  }

  float adjustedDistance = distance - sensorHeightOffset;

  // Treat distances close to binDepth as an empty bin
  if (adjustedDistance >= (binDepth - emptyBinThreshold))
  {
    return binDepth; // Bin is empty
  }

  return adjustedDistance < 0 ? 0 : adjustedDistance;
}

bool userNear()
{
  float d = readDistance(TRIG_USER, ECHO_USER);
  return (d > 0 && d < 50);
}

int getGasAvg()
{
  int sum = 0;
  for (int i = 0; i < 10; i++)
  {
    sum += analogRead(MQ4_PIN);
    delay(5);
  }
  return sum / 10;
}

void sendData(float rem, float pct, bool isLidOpen, int gas)
{
  FirebaseJson json;
  json.set("bin_precentage", pct);
  json.set("remaining_cm", rem);
  json.set("userDetect", isLidOpen ? 1 : 0);
  json.set("gas_level", gas);
  json.set("isFull", pct >= 85 ? 1 : 0);

  if (Firebase.RTDB.setJSON(&fbdo, "/SmartBin", &json))
  {
    Serial.println("📤 Firebase updated");
  }
  else
  {
    Serial.print("❌ Firebase fail: ");
    Serial.println(fbdo.errorReason());
  }
}

void handleRoot()
{
  server.send(200, "text/plain", "Smart Bin Web Interface 🌍");
}

void handleHealth()
{
  DynamicJsonDocument doc(256);

  float bin = readDistance(TRIG_BIN, ECHO_BIN);
  float user = readDistance(TRIG_USER, ECHO_USER);
  int gas = getGasAvg();

  doc["bin_sensor"] = (bin > 0 && bin < 100) ? "OK" : "FAIL";
  doc["user_sensor"] = (user > 0 && user < 100) ? "OK" : "FAIL";
  doc["gas_sensor"] = (gas > 100 && gas < 4095) ? "OK" : "FAIL";
  doc["servo_motor"] = doorServo.attached() ? "OK" : "FAIL";

  String json;
  serializeJson(doc, json);
  server.send(200, "application/json", json);

  Serial.println("📡 /health requested:");
  Serial.println(json);
}

void fetchConfig()
{
  FirebaseData configData;
  FirebaseJson json;

  if (Firebase.RTDB.getJSON(&configData, "/config", &json))
  {
    // These lines extract values from FirebaseJson safely
    FirebaseJsonData result;

    if (json.get(result, "door_close_time") && result.success)
    {
      doorCloseTime = result.to<float>();
    }

    if (json.get(result, "bin_depth") && result.success)
    {
      binDepth = result.to<float>();
    }

    if (json.get(result, "motor_speed") && result.success)
    {
      speedDelay = result.to<int>();
    }

    Serial.printf("Fetched config - Door Close Time: %.2f, Bin Depth: %.2f, Motor Speed: %d\n", doorCloseTime, binDepth, speedDelay);
  }
  else
  {
    Serial.println("Failed to fetch config from Firebase");
  }
}

void handleUpdateDoorTime()
{
  if (server.method() == HTTP_POST)
  {
    DynamicJsonDocument doc(256);
    DeserializationError error = deserializeJson(doc, server.arg("plain"));

    if (error)
    {
      Serial.println("❌ Failed to parse JSON");
      server.send(400, "text/plain", "Invalid JSON");
      return;
    }

    if (doc.containsKey("door_close_time"))
    {
      doorCloseTime = doc["door_close_time"];

      FirebaseJson configJson;
      configJson.set("door_close_time", doorCloseTime);
      configJson.set("bin_depth", binDepth);
      configJson.set("motor_speed", speedDelay);

      if (Firebase.RTDB.setJSON(&fbdo, "/config", &configJson))
      {
        Serial.println("✅ Door close time updated");
        server.send(200, "text/plain", "Door close time updated");
      }
      else
      {
        Serial.println("❌ Failed to update door close time in Firebase");
        server.send(500, "text/plain", "Failed to update door close time in Firebase");
      }
    }
    else
    {
      server.send(400, "text/plain", "Missing door_close_time");
    }
  }
  else
  {
    server.send(405, "text/plain", "Method Not Allowed");
  }
}

void handleUpdateBinHeight()
{
  if (server.method() == HTTP_POST)
  {
    DynamicJsonDocument doc(256);
    DeserializationError error = deserializeJson(doc, server.arg("plain"));

    if (error)
    {
      Serial.println("❌ Failed to parse JSON");
      server.send(400, "text/plain", "Invalid JSON");
      return;
    }

    if (doc.containsKey("bin_depth"))
    {
      binDepth = doc["bin_depth"];

      FirebaseJson configJson;
      configJson.set("door_close_time", doorCloseTime);
      configJson.set("bin_depth", binDepth);
      configJson.set("motor_speed", speedDelay);

      if (Firebase.RTDB.setJSON(&fbdo, "/config", &configJson))
      {
        Serial.println("✅ Bin depth updated");
        server.send(200, "text/plain", "Bin depth updated");
      }
      else
      {
        Serial.println("❌ Failed to update bin depth in Firebase");
        server.send(500, "text/plain", "Failed to update bin depth in Firebase");
      }
    }
    else
    {
      server.send(400, "text/plain", "Missing bin_depth");
    }
  }
  else
  {
    server.send(405, "text/plain", "Method Not Allowed");
  }
}

void handleUpdateMotorSpeed()
{
  if (server.method() == HTTP_POST)
  {
    DynamicJsonDocument doc(256);
    DeserializationError error = deserializeJson(doc, server.arg("plain"));

    if (error)
    {
      Serial.println("❌ Failed to parse JSON");
      server.send(400, "text/plain", "Invalid JSON");
      return;
    }

    if (doc.containsKey("motor_speed"))
    {
      int motorSpeed = doc["motor_speed"];

      if (motorSpeed < 5 || motorSpeed > 50)
      {
        server.send(400, "text/plain", "Invalid motor speed value. Must be between 5 and 50.");
        return;
      }

      speedDelay = motorSpeed; // Update the speed delay

      FirebaseJson configJson;
      configJson.set("door_close_time", doorCloseTime);
      configJson.set("bin_depth", binDepth);
      configJson.set("motor_speed", speedDelay);

      if (Firebase.RTDB.setJSON(&fbdo, "/config", &configJson))
      {
        Serial.printf("✅ Motor speed updated to delay: %d ms\n", speedDelay);
        server.send(200, "text/plain", "Motor speed updated!");
      }
      else
      {
        Serial.println("❌ Failed to update motor speed in Firebase");
        server.send(500, "text/plain", "Failed to update motor speed in Firebase");
      }
    }
    else
    {
      server.send(400, "text/plain", "Missing motor_speed");
    }
  }
  else
  {
    server.send(405, "text/plain", "Method Not Allowed");
  }
}

void loop()
{
  server.handleClient();

  bool user = userNear();
  int gas = getGasAvg();

  if (user)
  {
    Serial.println("🟢 Door open start");
    digitalWrite(LED_DOOR, HIGH);
    targetAngle = 0; // Open the door
    moveServoSmoothly(targetAngle);

    Serial.println("🟡 Skipping Firebase update, door opened");
    sendData(-1, -1, true, gas);

    delay(5000);

    Serial.println("⚠️ Door closing soon...");
    for (int i = doorCloseTime; i > 0; i--)
    {
      Serial.printf("⏳ Closing in %d...\n", i);

      if (i <= 5) // Blink the light in the last 5 seconds
      {
        digitalWrite(LED_DOOR, LOW);
        delay(250);
        digitalWrite(LED_DOOR, HIGH);
        delay(250);
      }
      else
      {
        delay(1000);
      }
    }

    Serial.println("🔴 Door closed");
    digitalWrite(LED_DOOR, LOW);
    targetAngle = 135;
    moveServoSmoothly(targetAngle);

    // Update bin level after the door is closed
    float raw = binLevel();
    float rem = raw > binDepth ? 0 : raw;
    float pct = ((binDepth - rem) / binDepth) * 100.0;
    pct = constrain(pct, 0, 100);

    sendData(rem, pct, false, gas);
    delay(1000);
  }
  else
  {
    digitalWrite(LED_DOOR, LOW);
  }

  if (millis() - lastUpdate > UPDATE_INTERVAL)
  {
    lastUpdate = millis();
    float raw = binLevel();
    float rem = raw > binDepth ? 0 : raw;
    float pct = ((binDepth - rem) / binDepth) * 100.0;
    pct = constrain(pct, 0, 100);

    Serial.printf("📊 Bin: %.1fcm (%.1f%%) | User: %d | Gas: %d | Door Close Time: %.2f | Bin Depth: %.2f\n", rem, pct, user, gas, doorCloseTime, binDepth);
    sendData(rem, pct, false, gas);
  }

  delay(100);
}
