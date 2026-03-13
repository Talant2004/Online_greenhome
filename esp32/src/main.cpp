/**
 * ESP32 — мониторинг агроклиматических параметров
 * DHT11, датчик почвы, LCD 1602A I2C
 * Отправка данных на сервер Telegram-бота
 */

#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <DHT.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <time.h>

// ============ НАСТРОЙКИ ============
#define WIFI_SSID "AgroWiFi"
#define WIFI_PASS "1234509876+"
#define SERVER_URL "https://online-greenhome.onrender.com"
#define API_SECRET "agro_secret_2026"

#define DHT_PIN 4
#define SOIL_PIN 34
#define MEASURE_INTERVAL 45000
#define LCD_UPDATE_MS 2000
#define WIFI_RETRY_MS 5000
#define SEND_REPORT_HOUR_1 8
#define SEND_REPORT_HOUR_2 20

// LCD I2C 1602A: SDA=GPIO21, SCL=GPIO22, адрес обычно 0x27 (или 0x3F)
#define LCD_ADDR 0x27
#define LCD_COLS 16
#define LCD_ROWS 2

// ============ ДАТЧИКИ ============
#define DHT_TYPE DHT11
DHT dht(DHT_PIN, DHT_TYPE);
LiquidCrystal_I2C lcd(LCD_ADDR, LCD_COLS, LCD_ROWS);

bool wifiWasConnected = false;
unsigned long lastMeasure = 0;
unsigned long lastLcdUpdate = 0;
unsigned long lastReportSent = -1;
bool dhtOk = true;
bool lastSendOk = true;

void setup() {
  Serial.begin(115200);
  Wire.begin(21, 22);
  dht.begin();

  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("Online Greenhome");
  lcd.setCursor(0, 1);
  lcd.print("Starting...");

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
    lcd.setCursor(0, 1);
    lcd.print("WiFi...          ");
    delay(WIFI_RETRY_MS);
    connectWiFi();
    return;
  }

  if (wifiWasConnected == false) {
    sendNotification("wifi_restored", "Соединение WiFi восстановлено");
    wifiWasConnected = true;
  }

  unsigned long now = millis();

  if (now - lastLcdUpdate >= LCD_UPDATE_MS) {
    lastLcdUpdate = now;
    updateLcd();
  }

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

  delay(500);
}

void updateLcd() {
  float t = dht.readTemperature();
  float h = dht.readHumidity();
  int soil = analogRead(SOIL_PIN);

  lcd.setCursor(0, 0);
  if (!isnan(t) && !isnan(h)) {
    lcd.print("T:");
    lcd.print(t, 1);
    lcd.print((char)223);
    lcd.print("C H:");
    lcd.print(h, 0);
    lcd.print("%  ");
  } else {
    lcd.print("DHT error    ");
  }

  lcd.setCursor(0, 1);
  lcd.print("Soil:");
  lcd.print(soil);
  lcd.print(" W");
  lcd.print(WiFi.status() == WL_CONNECTED ? "1" : "0");
  lcd.print(" C");
  lcd.print(lastSendOk ? "1  " : "0  ");
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
    Serial.println("\nWiFi: " + WiFi.localIP().toString());
  } else {
    Serial.println("\nОшибка WiFi");
  }
}

void sendSensorData() {
  float temp = 0, humidity = 0;
  int soil = analogRead(SOIL_PIN);

  dhtOk = true;
  float t = dht.readTemperature();
  float h = dht.readHumidity();
  if (isnan(t) || isnan(h)) {
    dhtOk = false;
    sendNotification("sensor_error", "Ошибка датчика DHT11");
    return;
  }
  temp = t;
  humidity = h;

  HTTPClient http;
  String url = String(SERVER_URL) + "/api/sensors";
  http.begin(url);
  http.addHeader("Content-Type", "application/json");
  http.addHeader("X-API-Secret", API_SECRET);

  StaticJsonDocument<128> doc;
  doc["temperature"] = round(temp * 10) / 10.0;
  doc["humidity"] = round(humidity * 10) / 10.0;
  doc["soil"] = soil;
  doc["light"] = 0;

  String body;
  serializeJson(doc, body);

  int code = http.POST(body);
  http.end();

  lastSendOk = (code == 200);
  if (lastSendOk) {
    Serial.println("OK: T=" + String(temp) + " H=" + String(humidity) + " Soil=" + String(soil));
  } else {
    Serial.println("Ошибка: " + String(code));
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
  http.POST(body);
  http.end();
}
