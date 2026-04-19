#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>
#include <Adafruit_NeoPixel.h>

const char *ssid = "Bunda 1 Lantai 2 New";
const char *password = "bismillahlancar";

const char *serverUrlPot = "https://iot-test-one.vercel.app/api/sensor?id=eq.1";
const char *serverUrlRgb = "https://iot-test-one.vercel.app/api/actuators?name=eq.RGB_ESP32-S3";
const char *myApiKey = "kobong";

const int potPin = 1;
const int RGB_PIN = 48; 
const int NUM_PIXELS = 1;

Adafruit_NeoPixel pixel(NUM_PIXELS, RGB_PIN, NEO_GRB + NEO_KHZ800);

int lastPersen = -1;

void setup() {
  Serial.begin(115200);
  delay(2000);

  pixel.begin();
  pixel.setBrightness(100);
  pixel.setPixelColor(0, pixel.Color(255, 255, 255)); 
  pixel.show();

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  
  pixel.setPixelColor(0, pixel.Color(0, 255, 0));
  pixel.show();
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
        http.begin(*client, serverUrlPot);
        http.addHeader("Content-Type", "application/json");
        http.addHeader("x-api-key", myApiKey);
        String jsonPayload = "{\"value\":" + String(persen) + "}";
        int httpResponseCode = http.PATCH(jsonPayload);
        if (httpResponseCode > 0) lastPersen = persen;
        http.end();
      }

      http.begin(*client, serverUrlRgb);
      http.addHeader("x-api-key", myApiKey);
      int httpCode = http.GET();

      if (httpCode == 200) {
        String payload = http.getString();
        
        DynamicJsonDocument doc(2048);
        DeserializationError error = deserializeJson(doc, payload);
        
        if (!error) {
          JsonArray root = doc.as<JsonArray>();
          JsonObject firstItem = root[0];

          int r = firstItem["red_val"] | 0;
          int g = firstItem["green_val"] | 0;
          int b = firstItem["blue_val"] | 0;

          pixel.setPixelColor(0, pixel.Color(r, g, b));
          pixel.show();
          
        }
      }
      http.end();
      delete client;
    }
  }
  delay(1000);
}