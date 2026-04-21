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

// Smoothing: moving average filter
const int SAMPLE_SIZE = 10;
int samples[SAMPLE_SIZE] = {0};
int sampleIndex = 0;

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

  if (String(topic) == "monitor/actuator/rgb") {
    int r = doc["red_val"] | 0;
    int g = doc["green_val"] | 0;
    int b = doc["blue_val"] | 0;
    pixel.setPixelColor(0, pixel.Color(r, g, b));
    pixel.show();
  }
}

void reconnect() {
  while (!client.connected()) {
    if (client.connect("ESP32S3", mqtt_user, mqtt_pass)) {
      client.subscribe("monitor/actuator/rgb");
      pixel.setPixelColor(0, pixel.Color(0, 255, 0));
      pixel.show();
    } else {
      delay(5000);
    }
  }
}

// Smoothing function - moving average to reduce noise
int readSmoothedPot() {
  samples[sampleIndex] = analogRead(potPin);
  sampleIndex = (sampleIndex + 1) % SAMPLE_SIZE;
  
  int sum = 0;
  for(int i = 0; i < SAMPLE_SIZE; i++) {
    sum += samples[i];
  }
  return sum / SAMPLE_SIZE;
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

  int smoothedValue = readSmoothedPot();
  int persen = map(smoothedValue, 0, 4095, 0, 100);

  // Threshold increased from 1 to 5%, delay increased from 500ms to 1000ms
  if (abs(persen - lastPersen) > 5) { 
    if (millis() - lastMsg > 1000) { 
      lastMsg = millis();
      lastPersen = persen;

      StaticJsonDocument<64> doc;
      doc["value"] = persen;
      char buffer[64];
      serializeJson(doc, buffer);
      
      client.publish("monitor/sensor/pot", buffer);
      Serial.print("Published potentiometer: ");
      Serial.println(persen);
    }
  }
}