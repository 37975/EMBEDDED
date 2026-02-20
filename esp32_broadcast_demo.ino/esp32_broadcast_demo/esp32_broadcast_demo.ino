#include <WiFi.h>
#include <esp_now.h>

uint8_t receiverAddress[] = { 
  0x24, 0x6F, 0x28, 0xAA, 0xBB, 0xCC
}; // Replace with the receiver's MAC address

typedef struct {
  char message[32];
} Message;

Message myData;

void onSent(const wifi_tx_info_t *info, esp_now_send_status_t status) {
  Serial.print("Delivery Status : ");
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Success" : "Fail");
}

void setup() {
  Serial.begin(115200);

  WiFi.mode(WIFI_STA);

  Serial.print("Sender MAC Address: ");
  Serial.println(WiFi.macAddress());

  if (esp_now_init() != ESP_OK) {
    Serial.println("ESP-NOW Initialization Failed");
    return;
  }

  esp_now_register_send_cb(onSent);

  esp_now_peer_info_t peerInfo = {};
  memcpy(peerInfo.peer_addr, receiverAddress, 6);
  peerInfo.channel = 0;
  peerInfo.encrypt = false;

  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    Serial.println("Failed to add peer");
    return;
  }
}

void loop() {
  strcpy(myData.message, "Hello ESP-NOW");

  esp_err_t result = esp_now_send(
    receiverAddress,
    (uint8_t *)&myData,
    sizeof(myData)
  );

  if (result != ESP_OK) {
    Serial.println("Send error");
  }

  delay(1000);
}