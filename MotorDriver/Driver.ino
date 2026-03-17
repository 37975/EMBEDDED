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
#define WIFI_SSID       "Realme_12"
#define WIFI_PASSWORD   "123456789012"
#define FIREBASE_HOST   "embedded-cc90e-default-rtdb.asia-southeast1.firebasedatabase.app"
#define FIREBASE_SECRET "wBhPfRAptoio9WFNrICysJCq4b99L96pOekdvWV3"
#define NODE_ID         "outdoor"

// ─── MQTT ─────────────────────────────────────────────────
const char mqtt_broker[]    = "broker.hivemq.com";
const char mqtt_client_id[] = "airnode_" NODE_ID;
const int  MQTT_PORT        = 1883;
const char topic_mode[]     = "airnode/" NODE_ID "/mode";

// ─── ESP-NOW struct ───────────────────────────────────────
typedef struct __attribute__((packed)) {
  uint8_t type;
  char cmd[10];
  int  pm25;       
  int  fanSpeed;   
} IndoorCmd;

IndoorCmd rxCmd; 

// ─── Soft-start state ─────────────────────────────────────
int    softStartTarget  = 0;
int    softStartCurrent = 0;
unsigned long lastSoftStep = 0;
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
#define MIN_PWM   100  

// ─── PMS3003 UART ─────────────────────────────────────────
#define UART_PORT UART_NUM_2
#define RX_PIN    16
#define TX_PIN    17

// ─── Objects ──────────────────────────────────────────────
INA226 ina(0x40);
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
bool   manualFanPending = false;
bool   filterClogged    = false;
bool   sleepLocked      = false;
float prev_curr = 0;

// ─── Timers ───────────────────────────────────────────────
unsigned long lastSend    = 0;
unsigned long lastHistory = 0;

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
  if      (pm < 12)  setRGB1(0, 1, 0);
  else if (pm < 35)  setRGB1(1, 1, 0);
  else if (pm < 75)  setRGB1(1, 1, 0);
  else if (pm < 150) setRGB1(1, 0, 0);
  else               setRGB1(1, 0, 1);
}

// ═══════════════════════════════════════════════════════════
//  Current Feedback & Filter Health
// ═══════════════════════════════════════════════════════════
void checkFilterHealth(float curr_A, int pwm) {
  if (currentMode == "sleep" || pwm < MIN_PWM) {
    setRGB2(0, 1, 0);
    return;
  }
  
  if (curr_A > 5.0f) {
    setRGB2(1, 0, 0); 
    if (!filterClogged) {
      filterClogged = true;
      Serial.printf("[Filter] ⚠ DANGER! curr=%.2f A\n", curr_A);
    }
  } else if (curr_A > 4.3f) {
    setRGB2(1, 1, 0); 
    if (!filterClogged) {
      filterClogged = true; 
      Serial.printf("[Filter] ⚠ WARNING! curr=%.2f A\n", curr_A);
    }
  } else {
    setRGB2(0, 1, 0); 
    if (filterClogged) {
      filterClogged = false;
      Serial.println("[Filter] ✓ OK");
    }
  }
}

// ═══════════════════════════════════════════════════════════
//  Fan PWM — Soft-start
// ═══════════════════════════════════════════════════════════
void setFan(int target) {
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

  if (abs(softStartCurrent - softStartTarget) <= 2) {
    softStartCurrent = softStartTarget;
  }
  
  ledcWrite(FAN_PIN, softStartCurrent);
  currentPWM = softStartCurrent;
}

int autoFanSpeed(int pm) {
  float t = constrain(pm, 0, 150) / 150.0f;
  float curve = t * t;
  return constrain((int)(MIN_PWM + curve * (255 - MIN_PWM)), MIN_PWM, 255);
}

// ═══════════════════════════════════════════════════════════
//  ESP-NOW callback
// ═══════════════════════════════════════════════════════════
void onReceive(const esp_now_recv_info_t *info, const uint8_t *incomingData, int len) {
  if (len != sizeof(IndoorCmd)) return;
  memcpy(&rxCmd, incomingData, sizeof(IndoorCmd));

  if (rxCmd.pm25 >= 0 && rxCmd.pm25 <= 1000) { 
    remotePM25 = rxCmd.pm25;
  }

  if (rxCmd.type == 1) {
    currentMode = String(rxCmd.cmd);
    Serial.printf("[ESP-NOW] Mode change → %s\n", currentMode.c_str());
    if (currentMode == "sleep") {
      sleepLocked = true;
      setFan(0);
      ledsOff();
    } else {
      sleepLocked = false;
    }
  }
  
  if (currentMode != "sleep") sleepLocked = false;
  if (sleepLocked) return;

  if (currentMode == "manual") {
    int rawSpeed = rxCmd.fanSpeed;
    if (rawSpeed >= MIN_PWM && rawSpeed <= 255) {
      manualFanSpeed = rawSpeed;
    } else {
      manualFanSpeed = 0;
    }
    manualFanPending = true;
  }
}

void setupESPNOW() {
  if (esp_now_init() != ESP_OK) {
    Serial.println("[ESP-NOW] init failed");
    return;
  }
  esp_now_register_recv_cb(onReceive);
  Serial.println("[ESP-NOW] ready");
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
  
  // 🟢 จุดแก้ไขสำคัญ: เคลียร์บัฟเฟอร์ถ้าข้อมูลที่อ่านมาไม่สมบูรณ์
  if (len == 32 && buffer[0] == 0x42 && buffer[1] == 0x4D) {
    int pm25 = buffer[12] * 256 + buffer[13];
    lastPM10 = buffer[10] * 256 + buffer[11];
    return pm25;
  } else if (len > 0) {
    uart_flush_input(UART_PORT);
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
      setFan(0);
      ledsOff();
    } else {
      sleepLocked = false;
    }
  }
}

void mqttConnect() {
  Serial.print("MQTT");
  int retries = 0;
  
  // 🟢 ยอมให้พยายามต่อแค่ 3 ครั้ง จะได้ไม่บล็อกโปรแกรมหลัก
  while (!mqtt.connect(mqtt_client_id) && retries < 3) { 
    Serial.print("."); 
    delay(500); 
    retries++;
  }
  
  if (mqtt.connected()) {
    mqtt.subscribe(topic_mode);
    Serial.printf("\n✓ subscribed: %s\n", topic_mode);
  } else {
    Serial.println(" ✗ MQTT failed, will retry next loop");
  }
}

// ═══════════════════════════════════════════════════════════
//  NTP & Firebase
// ═══════════════════════════════════════════════════════════
long long getTimestampMs() {
  struct tm t;
  if (!getLocalTime(&t)) return (long long)millis();
  return (long long)mktime(&t) * 1000LL;
}

void sendSensorData(int pm25, float curr) {
  if (!Firebase.ready()) return;
  FirebaseJson json;
  if(curr != prev_curr) prev_curr = curr;
  json.set("pm25",      pm25);
  json.set("pm10",      lastPM10);
  json.set("current",   prev_curr);
  json.set("node_id",   NODE_ID);
  json.set("fan_pwm",   currentPWM);
  json.set("timestamp", getTimestampMs());
  if (Firebase.RTDB.setJSON(&fbdo, PATH_LATEST, &json)) {
    struct tm t; char buf[32] = "—";
    if (getLocalTime(&t)) strftime(buf, sizeof(buf), "%H:%M:%S", &t);
    Serial.printf("[Firebase] ✓ %s | pm25=%d curr=%.2f pwm=%d\n",
                  buf, pm25, curr, currentPWM);
  }

  if (millis() - lastHistory >= 30000) {
    lastHistory = millis();
    Firebase.RTDB.pushJSON(&fbdo, PATH_HISTORY, &json);
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
  // ina.begin();
  setupUART();

  if (!ina.begin()) {
    Serial.println("Error: INA226 not found!");
    while(1); // หยุดทำงานถ้าหาเซนเซอร์ไม่เจอ
  } else {
    // ปรับค่าเฉลี่ยให้สูงที่สุด (Mode 7) เพื่อลดสัญญาณรบกวนจากมอเตอร์ 4.2A
    ina.setAverage(7); 
    ina.setBusVoltageConversionTime(7);
    ina.setShuntVoltageConversionTime(4);
    Serial.println("INA226 Ready.");
  }




  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("WiFi");
  unsigned long t0 = millis();
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print("."); delay(300);
    if (millis() - t0 > 15000) { Serial.println("\n✗ WiFi FAILED");
      break; }
  }
  if (WiFi.status() == WL_CONNECTED)
    Serial.printf("\n✓ IP: %s\n", WiFi.localIP().toString().c_str());

  setupESPNOW();
  configTime(25200, 0, "pool.ntp.org", "time.google.com");
  Serial.print("NTP");
  {
    struct tm t; unsigned long t1 = millis();
    while (!getLocalTime(&t) && millis() - t1 < 10000) { Serial.print("."); delay(500); }
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
  { 
    unsigned long t2 = millis(); 
    while (!Firebase.ready() && millis()-t2 < 10000) { Serial.print("."); delay(300);
    } 
  }
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

  if (prevMode == "sleep" && currentMode != "sleep") {
    Serial.println("[Mode] ⬆ Waking from sleep — reinit INA226 + LED");
    Wire.end();
    delay(10);
    Wire.begin();
    if (ina.begin()) {
      ina.setAverage(7); 
      ina.setBusVoltageConversionTime(7);
      ina.setShuntVoltageConversionTime(4);
    }
    int pmNow = remotePM25 > 0 ? remotePM25 : lastPM25;
    updateAirLED(pmNow);
    setRGB2(0, 1, 0);
  }
  prevMode = currentMode;
  
  float shunt_mV = ina.getShuntVoltage_mV();
  float curr_A = (shunt_mV / 10.0f)/1.29911839;

  // 🟢 กรองค่า NaN หรือค่าที่ติดลบผิดปกติ
  if (isnan(curr_A) || curr_A < 0) {
      curr_A = 0.00;
  }

  if (currentMode == "auto") {
    int pmForFan  = remotePM25 > 0 ? remotePM25 : lastPM25;
    int targetPWM = autoFanSpeed(pmForFan);
    setFan(targetPWM);
    updateAirLED(pmForFan);
    checkFilterHealth(curr_A, currentPWM);
  } else if (currentMode == "manual") {
    setFan(manualFanSpeed);
    updateAirLED(remotePM25 > 0 ? remotePM25 : lastPM25);
    checkFilterHealth(curr_A, currentPWM);
  } else if (currentMode == "sleep") {
    setFan(125);
    ledsOff();
  }

  unsigned long now = millis();
  if (now - lastSend >= 5000) {
    lastSend = now;
    sendSensorData(lastPM25, curr_A);
    int pmForFan = remotePM25 > 0 ? remotePM25 : lastPM25;
    
    Serial.println("─────────────────────────────");
    Serial.printf("  MODE         : %s\n",  currentMode.c_str());
    Serial.printf("  PM2.5 remote : %d µg/m³  ← จาก indoor\n",   remotePM25);
    Serial.printf("  PM2.5 local  : %d µg/m³  ← sensor ตัวเอง\n", lastPM25);
    Serial.printf("  PM2.5 ใช้งาน : %d µg/m³  ← ใช้ปรับพัดลม\n",  pmForFan);
    Serial.printf("  Current      : %.2f A\n", curr_A);
    Serial.printf("  Filter State : %s\n", filterClogged ? "⚠ CLOGGED / WARNING" : "✓ OK");
    Serial.printf("  Fan PWM      : %d / 255\n", currentPWM);
    Serial.printf("  Manual target: %d / 255\n", manualFanSpeed);
    Serial.printf("  Sleep locked : %s\n", sleepLocked ? "yes" : "no");
    Serial.println("─────────────────────────────");
  }
}