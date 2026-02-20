#include <WiFi.h>
#include <esp_now.h>

typedef struct {
  char message[32];
} Message;

void onReceive(const uint8_t *macAddr, const uint8_t *incomingData, int len){
  Message myData;
  memcpy(&myData, incomingData , sizeof(myData));
  Serial.print("Received : ");
  Serial.prinln(myData.message); 
}

void setup(){
  Serial.begin(115200);4
  WiFi.mode(WIFI_STA);
  if (esp_now_init() != ESP_OK){
    Serial.println("ESP-NOW Initialization Failed");
    return;
  }
  esp_now_register_recv_cb(onReceive);
}

void loop(){
  
}