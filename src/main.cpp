#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <Adafruit_NeoPixel.h>
#include "secrets.h"

const char *ssid = SECRET_SSID;
const char *password = SECRET_PASS;
const char *mqtt_server = SECRET_MQTT_SERVER;
const int mqtt_port = 8883;
const char *mqtt_user = SECRET_MQTT_USER;
const char *mqtt_pass = SECRET_MQTT_PASS;

const int potPin = 1;
const int RGB_PIN = 48;
const int NUM_PIXELS = 1;

Adafruit_NeoPixel pixel(NUM_PIXELS, RGB_PIN, NEO_GRB + NEO_KHZ800);
WiFiClientSecure espClient;
PubSubClient client(espClient);

int lastPersen = -1;
unsigned long lastMsg = 0;

void setup_wifi() {
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
  }
  espClient.setInsecure();
}

void callback(char *topic, byte *payload, unsigned int length) {
  StaticJsonDocument<200> doc;
  deserializeJson(doc, payload, length);

  if (String(topic) == "oldynew/actuator/rgb") {
    int r = doc["red_val"] | 0;
    int g = doc["green_val"] | 0;
    int b = doc["blue_val"] | 0;
    pixel.setPixelColor(0, pixel.Color(r, g, b));
    pixel.show();
  }
}

void reconnect() {
  while (!client.connected()) {
    if (client.connect("ESP32S3_OldyNew", mqtt_user, mqtt_pass)) {
      client.subscribe("oldynew/actuator/rgb");
      pixel.setPixelColor(0, pixel.Color(0, 255, 0));
      pixel.show();
    } else {
      delay(5000);
    }
  }
}

void setup() {
  Serial.begin(115200);
  pixel.begin();
  pixel.setBrightness(100);
  pixel.setPixelColor(0, pixel.Color(255, 255, 255));
  pixel.show();

  setup_wifi();
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  int nilaiPot = analogRead(potPin);
  int persen = map(nilaiPot, 0, 4095, 0, 100);

  if (abs(persen - lastPersen) > 1) { 
    if (millis() - lastMsg > 500) { 
      lastMsg = millis();
      lastPersen = persen;

      StaticJsonDocument<64> doc;
      doc["value"] = persen;
      char buffer[64];
      serializeJson(doc, buffer);
      
      client.publish("oldynew/sensor/pot", buffer);
    }
  }
}