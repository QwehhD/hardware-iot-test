#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>

// --- KONFIGURASI WIFI & API ---
const char *ssid = "Bunda 1 Lantai 2 New";
const char *password = "bismillahlancar";

// URL diarahkan langsung ke ID yang mau di-update (misal ID 1)
const char *serverUrl = "https://iot-test-one.vercel.app/api/sensor?id=eq.1";
const char *myApiKey = "kobong";

const int potPin = 1;
int lastPersen = -1; // Untuk menyimpan nilai terakhir agar tidak spam data yang sama

void setup()
{
  // S3 menggunakan USB CDC, delay sebentar agar Serial Monitor sempat terbuka
  Serial.begin(115200);
  delay(2000);

  Serial.println("\n--- ESP32-S3 POTENTIOMETER UPDATE MODE ---");

  WiFi.begin(ssid, password);
  Serial.print("Menghubungkan ke WiFi");

  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nWiFi Terhubung!");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());
}

void loop()
{
  // 1. Baca Potensiometer
  int nilaiPot = analogRead(potPin);

  // Konversi ke persen (0-100)
  // Gunakan map atau rumus (nilaiPot * 100 / 4095)
  int persen = map(nilaiPot, 0, 4095, 0, 100);

  // 2. Cek apakah nilai berubah dari sebelumnya
  // Kita kirim data HANYA jika nilainya berubah (biar lebih efisien)
  if (persen != lastPersen)
  {
    Serial.printf("Nilai Berubah! Raw: %d | Persen: %d%%\n", nilaiPot, persen);

    if (WiFi.status() == WL_CONNECTED)
    {
      WiFiClientSecure *client = new WiFiClientSecure;

      if (client)
      {
        client->setInsecure(); // Bypass verifikasi SSL untuk Vercel
        HTTPClient http;

        // Gunakan metode PATCH untuk mengupdate data yang sudah ada
        http.begin(*client, serverUrl);
        http.addHeader("Content-Type", "application/json");
        http.addHeader("x-api-key", myApiKey);

        // Payload JSON hanya mengirim nilai yang mau diupdate
        String jsonPayload = "{\"value\":" + String(persen) + "}";

        Serial.println("Mengupdate data di Dashboard...");

        // PENTING: Gunakan metode PATCH
        int httpResponseCode = http.PATCH(jsonPayload);

        if (httpResponseCode > 0)
        {
          Serial.printf("Berhasil Update! Code: %d\n", httpResponseCode);
          lastPersen = persen; // Simpan nilai terakhir jika sukses
        }
        else
        {
          Serial.printf("Gagal Update! Error: %s\n", http.errorToString(httpResponseCode).c_str());
        }

        http.end();
        delete client;
      }
    }
  }
  else
  {
    // Jika nilai tidak berubah, kita tampilkan di Serial saja tanpa kirim ke internet
    // Serial.println("Nilai stabil, tidak mengirim data...");
  }

  delay(1000); // Cek perubahan setiap 1 detik
}