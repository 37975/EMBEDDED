#include <WiFi.h>
#include <Firebase_ESP_Client.h>
#include "addons/TokenHelper.h"
#include "addons/RTDBHelper.h"

// ─── Config ───────────────────────────────────────────────
#define WIFI_SSID       "WIFI"
#define WIFI_PASSWORD   "PASSWORD"
#define FIREBASE_HOST   "embedded-cc90e-default-rtdb.asia-southeast1.firebasedatabase.app"
#define FIREBASE_SECRET "wBhPfRAptoio9WFNrICysJCq4b99L96pOekdvWV3"

#define NODE_ID "indoor" // outdoor

// ── Firebase paths (สร้างอัตโนมัติจาก NODE_ID) ────────────
#define PATH_LATEST      "/sensors/" NODE_ID "/latest"
#define PATH_HISTORY     "/sensors/" NODE_ID "/history"
#define PATH_CONTROL     "/control/" NODE_ID
#define PATH_CALIBRATION "/calibration/" NODE_ID

FirebaseData   fbdo;
FirebaseAuth   auth;
FirebaseConfig config;

struct CalOffsets {
  float pm25 = 0, pm10 = 0, temp = 0, hum = 0, pres = 0;
} cal;

unsigned long lastSend    = 0;
unsigned long lastHistory = 0;
unsigned long lastControl = 0;
unsigned long lastCal     = 0;


void sendSensorData(float pm25, float pm10,
                    float temp,  float hum,
                    float pres,  float curr) {
  if (!Firebase.ready()) {
    Serial.println("Firebase not ready");
    return;
  }

  FirebaseJson json;
  json.set("pm25",        pm25 + cal.pm25);
  json.set("pm10",        pm10 + cal.pm10);
  json.set("temperature", temp + cal.temp);
  json.set("humidity",    hum  + cal.hum);
  json.set("pressure",    pres + cal.pres);
  json.set("current",     curr);
  json.set("node_id",     NODE_ID);
  json.set("timestamp",   (int)millis());

  if (Firebase.RTDB.setJSON(&fbdo, PATH_LATEST, &json)) {
    Serial.printf("[%s] ✓ latest sent\n", NODE_ID);
  } else {
    Serial.printf("[%s] ✗ %s\n", NODE_ID, fbdo.errorReason().c_str());
  }

  if (millis() - lastHistory >= 30000) {
    lastHistory = millis();
    if (Firebase.RTDB.pushJSON(&fbdo, PATH_HISTORY, &json)) {
      Serial.printf("[%s] ✓ history: %s\n", NODE_ID, fbdo.pushName().c_str());
    }
  }
}

void pollControl() {
  if (!Firebase.ready()) return;

  FirebaseJson     result;
  FirebaseJsonData data;

  if (!Firebase.RTDB.getJSON(&fbdo, PATH_CONTROL, &result)) return;

  result.get(data, "fan_speed");
  if (data.success) {
    int speed = data.intValue;
    Serial.printf("[%s] fan_speed = %d\n", NODE_ID, speed);
  }

  // mode
  result.get(data, "mode");
  if (data.success) {
    Serial.printf("[%s] mode = %s\n", NODE_ID, data.stringValue.c_str());
  }

  result.get(data, "command");
  if (data.success && data.stringValue.length() > 0) {
    String cmd = data.stringValue;
    Serial.printf("[%s] command = %s\n", NODE_ID, cmd.c_str());

    if (cmd == "restart") { delay(500); ESP.restart(); }
    if (cmd == "sleep")   { esp_deep_sleep(30 * 1000000ULL); }
    if (cmd == "sync")    { lastSend = 0; } // force send ทันที

    Firebase.RTDB.setString(&fbdo, String(PATH_CONTROL) + "/command", "");
  }
}

void fetchCalibration() {
  if (!Firebase.ready()) return;

  FirebaseJson     result;
  FirebaseJsonData data;

  if (!Firebase.RTDB.getJSON(&fbdo, PATH_CALIBRATION, &result)) return;

  auto getF = [&](const char* key, float& dst) {
    result.get(data, key);
    if (data.success) dst = data.floatValue;
  };

  getF("pm25_offset", cal.pm25);
  getF("pm10_offset", cal.pm10);
  getF("temp_offset", cal.temp);
  getF("hum_offset",  cal.hum);
  getF("pres_offset", cal.pres);

  Serial.printf("[%s] Cal loaded: pm25+%.1f temp+%.1f\n",
                NODE_ID, cal.pm25, cal.temp);
}

void setup() {
  Serial.begin(115200);
  Serial.printf("\n=== AIRNODE [%s] ===\n", NODE_ID);

  // ── WiFi ──────────────────────────────────────────────────
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(300);
  }
  Serial.printf("\n✓ IP: %s  MAC: %s\n",
    WiFi.localIP().toString().c_str(),
    WiFi.macAddress().c_str());

  // ── Firebase ──────────────────────────────────────────────
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
  Serial.printf("\n%s\n", Firebase.ready() ? "✓ Firebase ready" : "✗ Firebase timeout");

  fetchCalibration();
  pollControl();

  Serial.printf("=== RUNNING ===\n\n");
}


void loop() {
  unsigned long now = millis();

  if (now - lastSend >= 5000) {
    lastSend = now;

    //ให้แก้รับค่าจาก sensor
    float pm25 = 35.2;
    float pm10 = 72.4;
    float temp = 28.5;
    float hum  = 65.0;
    float pres = 1010.2;
    float curr = 1.24;

    sendSensorData(pm25, pm10, temp, hum, pres, curr);
  }

  if (now - lastControl >= 2000) {
    lastControl = now;
    pollControl();
  }

  if (now - lastCal >= 60000) {
    lastCal = now;
    fetchCalibration();
  }
}