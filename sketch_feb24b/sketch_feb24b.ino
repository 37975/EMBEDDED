#include <WiFi.h>
#include <Firebase_ESP_Client.h>
#include "addons/TokenHelper.h"
#include "addons/RTDBHelper.h"

// config
#define WIFI_SSID       "WIFI"
#define WIFI_PASSWORD   "PASSWORD"
#define FIREBASE_HOST   "embedded-cc90e-default-rtdb.asia-southeast1.firebasedatabase.app"
#define FIREBASE_SECRET "wBhPfRAptoio9WFNrICysJCq4b99L96pOekdvWV3"

#define NODE_ID "indoor"   // "indoor" หรือ "outdoor"

#define PATH_LATEST      "/sensors/" NODE_ID "/latest"
#define PATH_HISTORY     "/sensors/" NODE_ID "/history"
#define PATH_CONTROL     "/control/" NODE_ID
#define PATH_CALIBRATION "/calibration/" NODE_ID

FirebaseData   fbdo;
FirebaseAuth   auth;
FirebaseConfig config;

struct SensorValues {
  float pm25        = 35.0;
  float pm10        = 72.0;
  float temperature = 28.5;
  float humidity    = 65.0;
  float pressure    = 1010.0;
  float current     = 1.24;
} sensor;

unsigned long lastSend    = 0;
unsigned long lastHistory = 0;
unsigned long lastControl = 0;


void updateMockSensors() {
  sensor.pm25        += (random(-20, 20)) / 10.0;
  sensor.pm10        += (random(-30, 30)) / 10.0;
  sensor.temperature += (random(-5,   5)) / 10.0;
  sensor.humidity    += (random(-10, 10)) / 10.0;
  sensor.pressure    += (random(-5,   5)) / 10.0;
  sensor.current     += (random(-10, 10)) / 100.0;

  sensor.pm25        = constrain(sensor.pm25,        0.0,   300.0);
  sensor.pm10        = constrain(sensor.pm10,        0.0,   500.0);
  sensor.temperature = constrain(sensor.temperature, 15.0,  45.0);
  sensor.humidity    = constrain(sensor.humidity,    20.0,  95.0);
  sensor.pressure    = constrain(sensor.pressure,    990.0, 1030.0);
  sensor.current     = constrain(sensor.current,     0.0,   5.0);
}

void sendToFirebase() {
  if (!Firebase.ready()) {
    Serial.println("Firebase not ready — skip");
    return;
  }

  FirebaseJson json;
  json.set("pm25",        sensor.pm25);
  json.set("pm10",        sensor.pm10);
  json.set("temperature", sensor.temperature);
  json.set("humidity",    sensor.humidity);
  json.set("pressure",    sensor.pressure);
  json.set("current",     sensor.current);
  json.set("node_id",     NODE_ID);
  json.set("timestamp",   (int)millis());

  Serial.println("─────────────────────────────────────");
  Serial.printf(" [%s] /sensors/%s/latest\n", NODE_ID, NODE_ID);
  Serial.println("─────────────────────────────────────");
  Serial.printf("  pm25        : %.1f µg/m³\n", sensor.pm25);
  Serial.printf("  pm10        : %.1f µg/m³\n", sensor.pm10);
  Serial.printf("  temperature : %.1f °C\n",    sensor.temperature);
  Serial.printf("  humidity    : %.1f %%\n",     sensor.humidity);
  Serial.printf("  pressure    : %.1f hPa\n",   sensor.pressure);
  Serial.printf("  current     : %.2f A\n",     sensor.current);
  Serial.printf("  timestamp   : %d ms\n",      (int)millis());
  Serial.println("─────────────────────────────────────");

  if (Firebase.RTDB.setJSON(&fbdo, PATH_LATEST, &json)) {
    Serial.printf("[%s] latest sent\n", NODE_ID);
  } else {
    Serial.printf("[%s] %s\n", NODE_ID, fbdo.errorReason().c_str());
  }

  if (millis() - lastHistory >= 30000) {
    lastHistory = millis();
    if (Firebase.RTDB.pushJSON(&fbdo, PATH_HISTORY, &json)) {
      Serial.printf("[%s]  history: %s\n", NODE_ID, fbdo.pushName().c_str());
    } else {
      Serial.printf("[%s]  history: %s\n", NODE_ID, fbdo.errorReason().c_str());
    }
  }
}

void pollControl() {
  if (!Firebase.ready()) return;

  FirebaseJson     result;
  FirebaseJsonData data;

  if (!Firebase.RTDB.getJSON(&fbdo, PATH_CONTROL, &result)) return;

  result.get(data, "fan_speed");
  if (data.success)
    Serial.printf("[%s] fan_speed = %d\n", NODE_ID, data.intValue);

  result.get(data, "mode");
  if (data.success)
    Serial.printf("[%s] mode = %s\n", NODE_ID, data.stringValue.c_str());

  result.get(data, "command");
  if (data.success && data.stringValue.length() > 0) {
    String cmd = data.stringValue;
    Serial.printf("[%s] command = %s\n", NODE_ID, cmd.c_str());
    if (cmd == "restart") { delay(500); ESP.restart(); }
    if (cmd == "sleep")   { esp_deep_sleep(30 * 1000000ULL); }
    if (cmd == "sync")    { lastSend = 0; }
    Firebase.RTDB.setString(&fbdo, String(PATH_CONTROL) + "/command", "");
  }
}

void setup() {
  Serial.begin(115200);
  delay(500);
  randomSeed(analogRead(0));

  Serial.println("\n╔══════════════════════════════════╗");
  Serial.printf( "║  AIRNODE MOCK — NODE: %-10s║\n", NODE_ID);
  Serial.println("╚══════════════════════════════════╝\n");

  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(300);
  }
  Serial.printf("\n✓ IP: %s\n", WiFi.localIP().toString().c_str());

  //Firebase
  config.database_url               = FIREBASE_HOST;
  config.signer.tokens.legacy_token = FIREBASE_SECRET;
  config.token_status_callback      = tokenStatusCallback;

  Firebase.begin(&config, &auth);
  Firebase.reconnectNetwork(true);

  Serial.print("Firebase");
  unsigned long t0 = millis();
  while (!Firebase.ready() && millis() - t0 < 10000) {
    Serial.print(".");
    delay(300);
  }
  Serial.printf("\n%s\n\n",
    Firebase.ready() ? "✓ Firebase ready" : "✗ Firebase timeout");

  Serial.println("Mock data ทุก 5 วิ → Firebase");
  Serial.println("History   ทุก 30 วิ → Firebase");
  Serial.println("Control   poll ทุก 2 วิ\n");
}


void loop() {
  unsigned long now = millis();

  // ── ส่ง mock data ทุก 5 วิ ────────────────────────────────
  if (now - lastSend >= 5000) {
    lastSend = now;
    updateMockSensors();
    sendToFirebase();
  }

  // ── poll control ทุก 2 วิ ──────────────────────────────────
  if (now - lastControl >= 2000) {
    lastControl = now;
    pollControl();
  }
}