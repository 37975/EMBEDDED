#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>
#include <MQTT.h>
#include <Firebase_ESP_Client.h>
#include "addons/TokenHelper.h"
#include "addons/RTDBHelper.h"
#include <time.h>
#include <Wire.h>
#include "INA226.h"
#include "driver/uart.h"

// ─── Config ───────────────────────────────────────────────
#define WIFI_SSID       "immwinter"
#define WIFI_PASSWORD   "haerinnarak"
#define FIREBASE_HOST   "embedded-cc90e-default-rtdb.asia-southeast1.firebasedatabase.app"
#define FIREBASE_SECRET "wBhPfRAptoio9WFNrICysJCq4b99L96pOekdvWV3"
#define NODE_ID         "outdoor"

// ─── MQTT ─────────────────────────────────────────────────
const char mqtt_broker[]    = "broker.hivemq.com";
const char mqtt_client_id[] = "airnode_" NODE_ID;
const int  MQTT_PORT        = 1883;
const char topic_mode[]     = "airnode/" NODE_ID "/mode";

// ─── ESP-NOW struct ───────────────────────────────────────
typedef struct {
  uint8_t type;
  char cmd[10];
  int  fanSpeed;
} IndoorCmd;

IndoorCmd rxCmd;

// ─── Soft-start state ─────────────────────────────────────
int           softStartTarget  = 0;
int           softStartCurrent = 0;
unsigned long lastSoftStep     = 0;
#define SOFT_STEP_MS 10

// ─── Firebase paths ───────────────────────────────────────
#define PATH_LATEST  "/sensors/" NODE_ID "/latest"
#define PATH_HISTORY "/sensors/" NODE_ID "/history"

// ─── RGB LED pins ─────────────────────────────────────────
#define R1_PIN 32
#define G1_PIN 25
#define B1_PIN 33
#define R2_PIN 26
#define G2_PIN 27
#define B2_PIN 14

// ─── LED common cathode ───────────────────────────────────
#define LED_ON  HIGH
#define LED_OFF LOW

// ─── Fan ──────────────────────────────────────────────────
#define FAN_PIN   12
#define FAN_FREQ  25000
#define FAN_RES   8
#define FAN_SLEEP 80

// ─── PMS3003 UART ─────────────────────────────────────────
#define UART_PORT UART_NUM_2
#define RX_PIN    16
#define TX_PIN    17

// ─── Current Feedback ───────────────────────────────────── // สรุปมี bugs/ไม่ work
#define BASELINE_CURRENT  4200.0f       // 4.2A → mA
#define CALIB_FACTOR      0.785239192f  // calibrate factor จากการวัดจริง
#define OFFSET_A          0.0f          
#define CLOG_THRESHOLD    0.90f
#define MIN_PWM_FOR_CHECK 200

// ─── Objects ──────────────────────────────────────────────
INA226     ina(0x40);
WiFiClient net;
MQTTClient mqtt;
FirebaseData   fbdo;
FirebaseAuth   auth;
FirebaseConfig config;

// ─── State ────────────────────────────────────────────────
String currentMode      = "auto";
String prevMode         = "auto";
int    currentPWM       = 0;
int    lastPM25         = 0;
int    lastPM10         = 0;
int    remotePM25       = 0;
int    manualFanSpeed   = 0;
bool   filterClogged    = false;
bool   sleepLocked      = false;

// ─── Timers ───────────────────────────────────────────────
unsigned long lastSend    = 0;
unsigned long lastHistory = 0;

// ═══════════════════════════════════════════════════════════
//  INA226 Init 
// ═══════════════════════════════════════════════════════════
void initINA() {
  if (!ina.begin()) {
    Serial.println("[INA226] ✗ not found!");
  } else {
    ina.setAverage(7);                   
    ina.setBusVoltageConversionTime(7); 
    ina.setShuntVoltageConversionTime(4);
    Serial.println("[INA226] ✓ ready");
  }
}

// ═══════════════════════════════════════════════════════════
//  RGB LED — common cathode
// ═══════════════════════════════════════════════════════════
void ledsOff() {
  digitalWrite(R1_PIN, LED_OFF);
  digitalWrite(G1_PIN, LED_OFF);
  digitalWrite(B1_PIN, LED_OFF);
  digitalWrite(R2_PIN, LED_OFF);
  digitalWrite(G2_PIN, LED_OFF);
  digitalWrite(B2_PIN, LED_OFF);
}

void setRGB1(int r, int g, int b) {
  digitalWrite(R1_PIN, r ? LED_ON : LED_OFF);
  digitalWrite(G1_PIN, g ? LED_ON : LED_OFF);
  digitalWrite(B1_PIN, b ? LED_ON : LED_OFF);
}
void setRGB2(int r, int g, int b) {
  digitalWrite(R2_PIN, r ? LED_ON : LED_OFF);
  digitalWrite(G2_PIN, g ? LED_ON : LED_OFF);
  digitalWrite(B2_PIN, b ? LED_ON : LED_OFF);
}

void updateAirLED(int pm) {
  if      (pm < 12)  setRGB1(0, 1, 0);   // เขียว
  else if (pm < 35)  setRGB1(1, 1, 0);   // เหลือง
  else if (pm < 75)  setRGB1(1, 1, 0);   // เหลือง
  else if (pm < 150) setRGB1(1, 0, 0);   // แดง
  else               setRGB1(1, 0, 1);   // ม่วง
}

// ═══════════════════════════════════════════════════════════
//  Current Feedback
// ═══════════════════════════════════════════════════════════
void checkFilterHealth(float curr, int pwm) {
  if (currentMode == "sleep") return;
  if (pwm < MIN_PWM_FOR_CHECK) return;

  // calibrate กระแส (curr มาเป็น mA)
  float curr_cal = curr * CALIB_FACTOR - OFFSET_A;
  if (curr_cal < 0) curr_cal = 0;

  float ratio = curr_cal / BASELINE_CURRENT;

  if (ratio < CLOG_THRESHOLD) {
    if (!filterClogged) {
      filterClogged = true;
      Serial.printf("[Filter] ⚠ CLOGGED! curr=%.1f mA | cal=%.1f mA | baseline=%.0f mA | ratio=%.0f%%\n",
                    curr, curr_cal, BASELINE_CURRENT, ratio * 100);
    }
    setRGB2(1, 0, 0);
  } else {
    if (filterClogged) { filterClogged = false; Serial.println("[Filter] ✓ OK"); }
    setRGB2(0, 1, 0);
  }
}

// ═══════════════════════════════════════════════════════════
//  Fan PWM — Non-blocking Soft-start
// ═══════════════════════════════════════════════════════════
void setFan(int target) {
  if (currentMode == "sleep" && target != FAN_SLEEP) return;
  softStartTarget = constrain(target, 0, 255);
}

void updateFan() {
  if (softStartCurrent == softStartTarget) return;

  unsigned long now = millis();
  if (now - lastSoftStep < SOFT_STEP_MS) return;
  lastSoftStep = now;

  if (softStartCurrent < softStartTarget) softStartCurrent += 2;
  else                                    softStartCurrent -= 2;

  softStartCurrent = constrain(softStartCurrent, 0, 255);

  if (abs(softStartCurrent - softStartTarget) <= 2)
    softStartCurrent = softStartTarget;

  ledcWrite(FAN_PIN, softStartCurrent);
  currentPWM = softStartCurrent;
}

int autoFanSpeed(int pm) {
  float t = constrain(pm, 0, 150) / 150.0f;
  float curve = t * t;
  return constrain((int)(80 + curve * (255 - 80)), 80, 255);
}

// ═══════════════════════════════════════════════════════════
//  ESP-NOW callback
// ═══════════════════════════════════════════════════════════
void onReceive(const esp_now_recv_info_t *info, const uint8_t *incomingData, int len) {
  if (len != sizeof(IndoorCmd)) return;

  memcpy(&rxCmd, incomingData, sizeof(IndoorCmd));

  if (rxCmd.type == 1) {
    currentMode = String(rxCmd.cmd);
    Serial.printf("[ESP-NOW] Mode change → %s\n", currentMode.c_str());

    if (currentMode == "sleep") {
      sleepLocked = true;
      setFan(FAN_SLEEP);
      ledsOff();
    } else {
      sleepLocked = false;
      if (rxCmd.fanSpeed > 0) {
        remotePM25 = rxCmd.fanSpeed;
      }
      if (currentMode == "manual") {
        manualFanSpeed = constrain(rxCmd.fanSpeed, 0, 255);
      }
    }
  }
  else if (rxCmd.type == 0) {
    if (currentMode != "sleep") {
      sleepLocked = false;
    }
    if (sleepLocked) return;

    if (currentMode == "auto") {
      if (rxCmd.fanSpeed > 0) {
        remotePM25 = rxCmd.fanSpeed;
      }
    }
    else if (currentMode == "manual") {
      manualFanSpeed = constrain(rxCmd.fanSpeed, 0, 255);
    }
  }
}

void setupESPNOW() {
  if (esp_now_init() != ESP_OK) {
    Serial.println("[ESP-NOW] init failed");
    return;
  }
  esp_now_register_recv_cb(onReceive);
  Serial.println("[ESP-NOW] ✓ ready");
}

// ═══════════════════════════════════════════════════════════
//  PMS3003
// ═══════════════════════════════════════════════════════════
void setupUART() {
  uart_config_t cfg = {
    .baud_rate  = 9600,
    .data_bits  = UART_DATA_8_BITS,
    .parity     = UART_PARITY_DISABLE,
    .stop_bits  = UART_STOP_BITS_1,
    .flow_ctrl  = UART_HW_FLOWCTRL_DISABLE,
  };
  uart_param_config(UART_PORT, &cfg);
  uart_set_pin(UART_PORT, TX_PIN, RX_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
  uart_driver_install(UART_PORT, 2048, 0, 0, NULL, 0);
}

int readPM_DMA() {
  uint8_t buffer[32];
  int len = uart_read_bytes(UART_PORT, buffer, sizeof(buffer), 20 / portTICK_PERIOD_MS);
  if (len == 32 && buffer[0] == 0x42 && buffer[1] == 0x4D) {
    int pm25 = buffer[12] * 256 + buffer[13];
    lastPM10 = buffer[10] * 256 + buffer[11];
    Serial.printf("[PMS] PM2.5=%d PM10=%d µg/m³\n", pm25, lastPM10);
    return pm25;
  }
  return -1;
}

// ═══════════════════════════════════════════════════════════
//  MQTT
// ═══════════════════════════════════════════════════════════
void messageReceived(String &topic, String &payload) {
  Serial.printf("[MQTT] mode → %s\n", payload.c_str());
  if (payload == "auto" || payload == "manual" || payload == "sleep") {
    currentMode = payload;
    if (currentMode == "sleep") {
      sleepLocked = true;
      setFan(FAN_SLEEP);
      ledsOff();
    } else {
      sleepLocked = false;
    }
  }
}

void mqttConnect() {
  Serial.print("MQTT");
  while (!mqtt.connect(mqtt_client_id)) { Serial.print("."); delay(1000); }
  mqtt.subscribe(topic_mode);
  Serial.printf("\n✓ subscribed: %s\n", topic_mode);
}

// ═══════════════════════════════════════════════════════════
//  NTP
// ═══════════════════════════════════════════════════════════
long long getTimestampMs() {
  struct tm t;
  if (!getLocalTime(&t)) return (long long)millis();
  return (long long)mktime(&t) * 1000LL;
}

// ═══════════════════════════════════════════════════════════
//  Firebase
// ═══════════════════════════════════════════════════════════
void sendSensorData(int pm25, float curr) {
  if (!Firebase.ready()) { Serial.println("Firebase not ready"); return; }

  float curr_cal = curr * CALIB_FACTOR - OFFSET_A;
  if (curr_cal < 0) curr_cal = 0;

  FirebaseJson json;
  json.set("pm25",        pm25);
  json.set("pm10",        lastPM10);
  json.set("remotePM25",  remotePM25);
  json.set("current_mA",  curr_cal);
  json.set("node_id",     NODE_ID);
  json.set("fan_pwm",     currentPWM);
  json.set("timestamp",   getTimestampMs());

  if (Firebase.RTDB.setJSON(&fbdo, PATH_LATEST, &json)) {
    struct tm t; char buf[32] = "—";
    if (getLocalTime(&t)) strftime(buf, sizeof(buf), "%H:%M:%S", &t);
    Serial.printf("[Firebase] ✓ %s | pm25=%d curr=%.1fmA pwm=%d\n",
                  buf, pm25, curr_cal, currentPWM);
  } else {
    Serial.printf("[Firebase] ✗ %s\n", fbdo.errorReason().c_str());
  }

  if (millis() - lastHistory >= 30000) {
    lastHistory = millis();
    Firebase.RTDB.pushJSON(&fbdo, PATH_HISTORY, &json);
    Serial.println("[Firebase] ✓ history pushed");
  }
}

// ═══════════════════════════════════════════════════════════
void setup() {
  Serial.begin(115200);
  Serial.printf("\n=== AIRNODE [%s] ===\n", NODE_ID);

  pinMode(R1_PIN, OUTPUT); pinMode(G1_PIN, OUTPUT); pinMode(B1_PIN, OUTPUT);
  pinMode(R2_PIN, OUTPUT); pinMode(G2_PIN, OUTPUT); pinMode(B2_PIN, OUTPUT);
  ledsOff();

  ledcAttach(FAN_PIN, FAN_FREQ, FAN_RES);

  Wire.begin();
  initINA();
  setupUART();

  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("WiFi");
  unsigned long t0 = millis();
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print("."); delay(300);
    if (millis() - t0 > 15000) { Serial.println("\n✗ WiFi FAILED"); break; }
  }
  if (WiFi.status() == WL_CONNECTED)
    Serial.printf("\n✓ IP: %s\n", WiFi.localIP().toString().c_str());

  setupESPNOW();

  configTime(25200, 0, "pool.ntp.org", "time.google.com");
  Serial.print("NTP");
  {
    struct tm t; unsigned long t0 = millis();
    while (!getLocalTime(&t) && millis() - t0 < 10000) { Serial.print("."); delay(500); }
    char buf[32] = "timeout";
    if (getLocalTime(&t)) strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &t);
    Serial.printf("\n✓ %s (GMT+7)\n", buf);
  }

  config.database_url               = FIREBASE_HOST;
  config.signer.tokens.legacy_token = FIREBASE_SECRET;
  config.token_status_callback      = tokenStatusCallback;
  Firebase.begin(&config, &auth);
  Firebase.reconnectNetwork(true);
  Serial.print("Firebase");
  { unsigned long t0 = millis(); while (!Firebase.ready() && millis()-t0 < 10000) { Serial.print("."); delay(300); } }
  Serial.printf("\n%s\n", Firebase.ready() ? "✓ ready" : "✗ timeout");

  mqtt.begin(mqtt_broker, MQTT_PORT, net);
  mqtt.onMessage(messageReceived);
  if (WiFi.status() == WL_CONNECTED) mqttConnect();

  setRGB2(0, 1, 0);
  Serial.println("=== RUNNING ===\n");
}

// ═══════════════════════════════════════════════════════════
void loop() {
  updateFan();

  if (WiFi.status() == WL_CONNECTED) {
    mqtt.loop();
    if (!mqtt.connected()) mqttConnect();
  }

  int pm25 = readPM_DMA();
  if (pm25 > 0) lastPM25 = pm25;

  // ── ตรวจ mode เปลี่ยน (sleep → อื่น) ────────────────────
  if (prevMode == "sleep" && currentMode != "sleep") {
    Serial.println("[Mode] ⬆ Waking from sleep — reinit INA226 + LED");
    Wire.end();
    delay(10);
    Wire.begin();
    initINA();
    int pmNow = remotePM25 > 0 ? remotePM25 : lastPM25;
    updateAirLED(pmNow);
    setRGB2(0, 1, 0);
  }
  prevMode = currentMode;

  float curr = ina.getCurrent_mA();

  // ── Fan control ───────────────────────────────────────────
  if (currentMode == "auto") {
    int pmForFan  = remotePM25 > 0 ? remotePM25 : lastPM25;
    int targetPWM = autoFanSpeed(pmForFan);
    setFan(targetPWM);
    updateAirLED(pmForFan);
    checkFilterHealth(curr, currentPWM);

  } else if (currentMode == "manual") {
    setFan(manualFanSpeed);
    updateAirLED(remotePM25 > 0 ? remotePM25 : lastPM25);
    checkFilterHealth(curr, currentPWM);

  } else if (currentMode == "sleep") {
    setFan(FAN_SLEEP);
    ledsOff();
  }

  // ── Firebase & Serial log ทุก 5 วิ ───────────────────────
  unsigned long now = millis();
  if (now - lastSend >= 5000) {
    lastSend = now;
    sendSensorData(lastPM25, curr);

    float curr_cal = curr * CALIB_FACTOR - OFFSET_A;
    if (curr_cal < 0) curr_cal = 0;
    int pmForFan = remotePM25 > 0 ? remotePM25 : lastPM25;

    Serial.println("─────────────────────────────");
    Serial.printf("  MODE         : %s\n",  currentMode.c_str());
    Serial.printf("  PM2.5 remote : %d µg/m³\n", remotePM25);
    Serial.printf("  PM2.5 local  : %d µg/m³\n", lastPM25);
    Serial.printf("  PM2.5 ใช้งาน : %d µg/m³\n", pmForFan);
    Serial.printf("  PM10         : %d µg/m³\n", lastPM10);
    Serial.printf("  Current raw  : %.1f mA\n", curr);
    Serial.printf("  Current cal  : %.1f mA (x%.3f)\n", curr_cal, CALIB_FACTOR);
    Serial.printf("  Baseline     : %.0f mA\n", BASELINE_CURRENT);
    Serial.printf("  Ratio        : %.0f%%  Filter: %s\n",
                  (curr_cal / BASELINE_CURRENT) * 100,
                  filterClogged ? "⚠ CLOGGED" : "✓ OK");
    Serial.printf("  Fan PWM      : %d / 255\n", currentPWM);
    Serial.printf("  Manual speed : %d / 255\n", manualFanSpeed);
    Serial.printf("  Sleep locked : %s\n", sleepLocked ? "yes" : "no");
    Serial.println("─────────────────────────────");
  }
}
