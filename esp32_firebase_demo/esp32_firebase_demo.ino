#include <WiFi.h>
#include <MQTT.h>
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

// MQTT
const char mqtt_broker[]    = "broker.hivemq.com";
const char mqtt_client_id[] = "airnode_" NODE_ID;
const int  MQTT_PORT        = 1883;

const char topic_mode[] = "airnode/" NODE_ID "mode";
// State
String currentMode = "auto"; // auto | manual | sleep

WiFiClient net;
MQTTClient mqtt;

// Firebase paths
#define PATH_LATEST      "/sensors/" NODE_ID "/latest"
#define PATH_HISTORY     "/sensors/" NODE_ID "/history"
#define PATH_CONTROL     "/control/" NODE_ID
// #define PATH_CALIBRATION "/calibration/" NODE_ID

FirebaseData   fbdo;
FirebaseAuth   auth;
FirebaseConfig config;

struct CalOffsets {
  float pm25 = 0, pm10 = 0;
} cal;

unsigned long lastSend    = 0;
unsigned long lastHistory = 0;

void messageReceived(String &topic, String &payload){
  Serial.printf("[MQTT] mode → %s\n", payload.c_str());
  if (payload == "auto" || payload == "manual" || payload == "sleep"){
    currentMode = payload;

    if( currentMode == "auto" ){
      Serial.printf("MODE : AUTO \n");
    }else if (currentMode == "manual"){
      Serial.printf("MODE : MANUAL \n");
    }else{
      Serial.printf("MODE : SLEEP \n");
    }
  }
}

void mqttConnect(){
  Serial.print("MQTT");
  while(!mqtt.connect(mqtt_client_id)) { 
    Serial.print("."); 
    delay(1000); 
  }
  mqtt.subscribe(topic_mode);
  Serial.printf("\n SUBSCRIBED: %s\n", topic_mode);
}

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
  mqtt.begin(mqtt_broker, MQTT_PORT, net);
  mqtt.onMessage(messageReceived);
  mqttConnect();

  Serial.printf("=== RUNNING ===\n\n");
}


void loop() {
  unsigned long now = millis();

  mqtt.loop();
  if(!mqtt.connected()) mqttConnect();

  if(currentMode == "auto"){
    printf(currentMode.c_str());
  }

  if (now - lastSend >= 5000) {
    lastSend = now;

    //ให้แก้รับค่าจาก sensor
    float pm25 = 35.2;
    float pm10 = 72.4;
    float curr = 1.24;

    sendSensorData(pm25, pm10, curr);
  }
}