#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>

const char *ssid = "Bunda 1 Lantai 2 New";
const char *password = "bismillahlancar";
const char *serverUrl = "https://iot-test-one.vercel.app/api/sensor?id=eq.1";
const char *myApiKey = "kobong";

const int potPin = 1;
const int pinR = 4; 
const int pinG = 5; 
const int pinB = 6; 

int lastPersen = -1;

void setup() {
  Serial.begin(115200);
  delay(2000);

  pinMode(pinR, OUTPUT);
  pinMode(pinG, OUTPUT);
  pinMode(pinB, OUTPUT);

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
}

void loop() {
  int nilaiPot = analogRead(potPin);
  int persen = map(nilaiPot, 0, 4095, 0, 100);

  if (WiFi.status() == WL_CONNECTED) {
    WiFiClientSecure *client = new WiFiClientSecure;
    if (client) {
      client->setInsecure();
      HTTPClient http;

      if (persen != lastPersen) {
        http.begin(*client, serverUrl);
        http.addHeader("Content-Type", "application/json");
        http.addHeader("x-api-key", myApiKey);
        String jsonPayload = "{\"value\":" + String(persen) + "}";
        int httpResponseCode = http.PATCH(jsonPayload);
        
        if (httpResponseCode > 0) {
          lastPersen = persen;
        }
        http.end();
      }

      http.begin(*client, serverUrl);
      http.addHeader("x-api-key", myApiKey);
      int httpCode = http.GET();

      if (httpCode == 200) {
        String payload = http.getString();
        DynamicJsonDocument doc(1024);
        deserializeJson(doc, payload);
        
        int r = doc[0]["red_val"];
        int g = doc[0]["green_val"];
        int b = doc[0]["blue_val"];

        analogWrite(pinR, r);
        analogWrite(pinG, g);
        analogWrite(pinB, b);
      }
      http.end();
      delete client;
    }
  }
  delay(1000);
}