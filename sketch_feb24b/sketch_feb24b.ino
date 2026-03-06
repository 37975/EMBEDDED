#define ENABLE_USER_AUTH
#define ENABLE_DATABASE

#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <FirebaseClient.h>

#define WIFI_SSID ""
#define WIFI_PASSWORD ""

#define Web_API_KEY ""
#define DATABASE_URL ""
#define  USER_EMAIL ""
#define USER_PASS ""

void processData(AsyncResult &aResult);

// Authentication
UserAuth user_auth(Web_API_KEY, USER_EMAIL, USER_PASS);

FirebaseApp app;
WiFiClientSecure ssl_client;
using AsyncClient = AsyncClientClass;
AsyncClient aClient(ssl_client);
RealtimeDatabase Database;

unsigned long lastSendTime = 0;
const unsigned long sendInterval = 10000; // 10 seconds

//Variables to send to the database;
int intValue = 0;
float floatValue = 0.01;
String stringValue = "";


void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);

  //Connect to Wi - Fi
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to Wi-Fi");
  while (WiFi.status() != WL_CONNECTED){
    Serial.print(".");
    delay(300);
  }
  Serial.println();

  ssl_client.setInseure();
  ssl_client.setConnectionTimeout(1000);
  ssl_client.setHandshakeTImeout(5);

  // initialize Firebase
  initializeApp(aClient, app, getAuth(user_auth), processData, "authTask");
  app.getApp<RealtimeDatabase>(Database);
  Database.url(DATABASE_URL);
}

void loop() {
  // put your main code here, to run repeatedly:

}
