/**
 * ESP32 — мониторинг агроклиматических параметров
 * Отправка данных на сервер Telegram-бота
 */

#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <DHT.h>
#include <Wire.h>
#include <BH1750.h>
#include <time.h>

// ============ НАСТРОЙКИ ============
#define WIFI_SSID "AgroWiFi"
#define WIFI_PASS "1234509876+"
#define SERVER_URL "http://192.168.1.100:5000"  // IP ПК с ботом в локальной сети
#define API_SECRET "default_secret_change_me"   // Должен совпадать с .env

#define DHT_PIN 4
#define SOIL_PIN 34
#define MEASURE_INTERVAL 45000   // 45 секунд
#define WIFI_RETRY_MS 5000
#define SEND_REPORT_HOUR_1 8
#define SEND_REPORT_HOUR_2 20

// ============ ДАТЧИКИ ============
#define DHT_TYPE DHT11
DHT dht(DHT_PIN, DHT_TYPE);
BH1750 lightMeter;

// Состояние
bool wifiWasConnected = false;
unsigned long lastMeasure = 0;
unsigned long lastReportSent = -1;
bool dhtOk = true;
bool bh1750Ok = true;

void setup() {
  Serial.begin(115200);
  Wire.begin(21, 22);
  dht.begin();
  lightMeter.begin(BH1750::CONTINUOUS_HIGH_RES_MODE_2);

  configTime(3 * 3600, 0, "pool.ntp.org", "time.nist.gov");
  connectWiFi();
}

void loop() {
  bool wifiConnected = WiFi.status() == WL_CONNECTED;

  if (!wifiConnected) {
    if (wifiWasConnected) {
      sendNotification("wifi_lost", "Соединение WiFi потеряно");
      wifiWasConnected = false;
    }
    delay(WIFI_RETRY_MS);
    connectWiFi();
    return;
  }

  if (wifiWasConnected == false) {
    sendNotification("wifi_restored", "Соединение WiFi восстановлено");
    wifiWasConnected = true;
  }

  unsigned long now = millis();
  if (now - lastMeasure >= MEASURE_INTERVAL) {
    lastMeasure = now;
    sendSensorData();
  }

  struct tm timeinfo;
  if (getLocalTime(&timeinfo)) {
    int hour = timeinfo.tm_hour;
    if ((hour == SEND_REPORT_HOUR_1 || hour == SEND_REPORT_HOUR_2) &&
        lastReportSent != (unsigned long)(hour * 60 + timeinfo.tm_min)) {
      lastReportSent = hour * 60 + timeinfo.tm_min;
      sendSensorData();
    }
  }

  delay(1000);
}

void connectWiFi() {
  if (WiFi.status() == WL_CONNECTED) return;

  Serial.print("Подключение к WiFi ");
  WiFi.begin(WIFI_SSID, WIFI_PASS);

  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 30) {
    delay(500);
    Serial.print(".");
    attempts++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nWiFi подключен: " + WiFi.localIP().toString());
  } else {
    Serial.println("\nОшибка WiFi");
  }
}

void sendSensorData() {
  float temp = 0, humidity = 0;
  int soil = 0, light = 0;

  dhtOk = true;
  float t = dht.readTemperature();
  float h = dht.readHumidity();
  if (isnan(t) || isnan(h)) {
    dhtOk = false;
    sendNotification("sensor_error", "Ошибка датчика DHT11");
  } else {
    temp = t;
    humidity = h;
  }

  soil = analogRead(SOIL_PIN);

  bh1750Ok = true;
  float lx = lightMeter.readLightLevel();
  if (lx < 0) {
    bh1750Ok = false;
  } else {
    light = (int)lx;
  }

  if (!dhtOk && !bh1750Ok) {
    return;
  }

  HTTPClient http;
  String url = String(SERVER_URL) + "/api/sensors";
  http.begin(url);
  http.addHeader("Content-Type", "application/json");
  http.addHeader("X-API-Secret", API_SECRET);

  StaticJsonDocument<128> doc;
  doc["temperature"] = round(temp * 10) / 10.0;
  doc["humidity"] = round(humidity * 10) / 10.0;
  doc["soil"] = soil;
  doc["light"] = light;

  String body;
  serializeJson(doc, body);

  int code = http.POST(body);
  http.end();

  if (code == 200) {
    Serial.println("Данные отправлены: T=" + String(temp) + " H=" + String(humidity) +
                   " Почва=" + String(soil) + " Свет=" + String(light));
  } else {
    Serial.println("Ошибка отправки: " + String(code));
  }
}

void sendNotification(const char* type, const char* message) {
  if (WiFi.status() != WL_CONNECTED) return;

  HTTPClient http;
  String url = String(SERVER_URL) + "/api/notification";
  http.begin(url);
  http.addHeader("Content-Type", "application/json");
  http.addHeader("X-API-Secret", API_SECRET);

  StaticJsonDocument<128> doc;
  doc["type"] = type;
  doc["message"] = message;

  String body;
  serializeJson(doc, body);

  int code = http.POST(body);
  http.end();

  Serial.println("Уведомление " + String(type) + ": " + String(code));
}
