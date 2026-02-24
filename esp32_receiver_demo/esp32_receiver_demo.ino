#include <WiFi.h>
#include <esp_now.h>

typedef struct {
  char message[32];
} Message;

// Note the updated "esp_now_recv_info" parameter here
void onReceive(const esp_now_recv_info *info, const uint8_t *incomingData, int len) {
  Message myData;
  memcpy(&myData, incomingData, sizeof(myData));
  
  Serial.print("Received from: ");
  // To get the MAC address now, you'd use info->src_addr
  for (int i = 0; i < 6; i++) {
    Serial.printf("%02X%s", info->src_addr[i], (i < 5) ? ":" : "");
  }
  
  Serial.print(" | Message: ");
  Serial.println(myData.message); // Fixed the 't' in println
}

void setup() {
  Serial.begin(115200); // Removed the extra '4'
  WiFi.mode(WIFI_STA);
  
  if (esp_now_init() != ESP_OK) {
    Serial.println("ESP-NOW Initialization Failed");
    return;
  }
  
  // Register the callback
  esp_now_register_recv_cb(onReceive);
}

void loop() {
  // Stay empty for receiver
}