#include <WiFi.h>
#include <Firebase_ESP_Client.h>
#include "addons/TokenHelper.h"
#include "addons/RTDBHelper.h"
#include <time.h>

// Config
#define WIFI_SSID       "WIFI"
#define WIFI_PASSWORD   "PASSWORD"
#define FIREBASE_HOST   "embedded-cc90e-default-rtdb.asia-southeast1.firebasedatabase.app"
#define FIREBASE_SECRET "wBhPfRAptoio9WFNrICysJCq4b99L96pOekdvWV3"

#define NODE_ID "indoor" // outdoor

// Firebase paths
#define PATH_LATEST      "/sensors/" NODE_ID "/latest"
#define PATH_HISTORY     "/sensors/" NODE_ID "/history"
#define PATH_CONTROL     "/control/" NODE_ID
#define PATH_CALIBRATION "/calibration/" NODE_ID

FirebaseData   fbdo;
FirebaseAuth   auth;
FirebaseConfig config;

struct CalOffsets {
  float pm25 = 0, pm10 = 0;
} cal;

unsigned long lastSend    = 0;
unsigned long lastHistory = 0;
unsigned long lastControl = 0;
unsigned long lastCal     = 0;

long long getTimestampMs() {
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    Serial.println("[NTP] not synced yet — using millis()");
    return (long long)millis();
  }
  time_t now = mktime(&timeinfo);
  return (long long)now * 1000LL;
}

void sendSensorData(float pm25, float pm10, float curr) {
  if (!Firebase.ready()) {
    Serial.println("Firebase not ready");
    return;
  }

  FirebaseJson json;
  json.set("pm25",        pm25 + cal.pm25);
  json.set("pm10",        pm10 + cal.pm10);
  json.set("current",     curr);
  json.set("node_id",     NODE_ID);
  json.set("timestamp",   getTimestampMs());

  if (Firebase.RTDB.setJSON(&fbdo, PATH_LATEST, &json)) {
    struct tm timeinfo;
    getLocalTime(&timeinfo);
    char buf[32];
    strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &timeinfo);
    Serial.printf("[%s] ✓ sent at %s\n", NODE_ID, buf);
  } else {
    Serial.printf("[%s] %s\n", NODE_ID, fbdo.errorReason().c_str());
  }

  if (millis() - lastHistory >= 30000) {
    lastHistory = millis();
    if (Firebase.RTDB.pushJSON(&fbdo, PATH_HISTORY, &json)) {
      Serial.printf("[%s] history: %s\n", NODE_ID, fbdo.pushName().c_str());
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
    // if (cmd == "sleep")   { esp_deep_sleep(30 * 1000000ULL); }
    if (cmd == "sync")    { lastSend = 0; } // force send ทันที

    Firebase.RTDB.setString(&fbdo, String(PATH_CONTROL) + "/command", "");
  }
}

// void fetchCalibration() {
//   if (!Firebase.ready()) return;

//   FirebaseJson     result;
//   FirebaseJsonData data;

//   if (!Firebase.RTDB.getJSON(&fbdo, PATH_CALIBRATION, &result)) return;

//   auto getF = [&](const char* key, float& dst) {
//     result.get(data, key);
//     if (data.success) dst = data.floatValue;
//   };

//   getF("pm25_offset", cal.pm25);
//   getF("pm10_offset", cal.pm10);

//   Serial.printf("[%s] Cal loaded: pm25+%.1f temp+%.1f\n",
//                 NODE_ID, cal.pm25);
// }

void setup() {
  Serial.begin(115200);
  Serial.printf("\n=== AIRNODE [%s] ===\n", NODE_ID);

  // WiFi
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

   // ── NTP sync ────────────────────────────────────────────
  // configTime(timezone_offset_sec, daylight_offset_sec, ntp_server)
  // GMT+7 = 7 * 3600 = 25200
  configTime(25200, 0, "pool.ntp.org", "time.google.com");
  Serial.print("NTP sync");
  struct tm timeinfo;
  {
    unsigned long t0 = millis();
    while (!getLocalTime(&timeinfo) && millis() - t0 < 10000) {
      Serial.print(".");
      delay(500);
    }
    if (getLocalTime(&timeinfo)) {
      char buf[32];
      strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &timeinfo);
      Serial.printf("\n✓ Time: %s (GMT+7)\n", buf);
    } else {
      Serial.println("\n✗ NTP timeout — will retry");
    }
  }

  // Firebase
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
  Serial.printf("\n%s\n", Firebase.ready() ? "Firebase ready" : "Firebase timeout");

  // fetchCalibration();
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
    // float temp = 28.5;
    // float hum  = 65.0;
    // float pres = 1010.2;
    float curr = 1.24;

    sendSensorData(pm25, pm10, curr);
  }

  if (now - lastControl >= 2000) {
    lastControl = now;
    pollControl();
  }

  // if (now - lastCal >= 60000) {
  //   lastCal = now;
    // fetchCalibration();
  // }
}