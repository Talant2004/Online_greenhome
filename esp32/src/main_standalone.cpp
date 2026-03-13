/**
 * ESP32 — автономный Telegram-бот (БЕЗ Render/сервера)
 * DHT11, датчик почвы
 * Прямое подключение к Telegram API
 *
 * Команды: /start /status /live /temp /soil
 * /live — создаёт ОДНО сообщение, которое обновляется на месте (без спама)
 * Авто-отправка по расписанию (08:00, 20:00)
 */

#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <DHT.h>
#include <time.h>

// ============ НАСТРОЙКИ ============
#define WIFI_SSID "AgroWiFi"
#define WIFI_PASS "1234509876+"
#define BOT_TOKEN "ТВОЙ_ТОКЕН_ОТ_BotFather"
#define ALLOWED_CHAT_ID "123456789"

#define DHT_PIN 4
#define SOIL_PIN 34
#define MEASURE_INTERVAL 30000
#define TELEGRAM_POLL_MS 2000
#define WIFI_RETRY_MS 5000
#define SEND_REPORT_HOUR_1 8
#define SEND_REPORT_HOUR_2 20

#define TG_API "https://api.telegram.org/bot" BOT_TOKEN

// ============ ДАТЧИКИ ============
#define DHT_TYPE DHT11
DHT dht(DHT_PIN, DHT_TYPE);

bool wifiWasConnected = false;
unsigned long lastMeasure = 0;
unsigned long lastTgPoll = 0;
unsigned long lastReportSent = -1;
long updateOffset = 0;
long liveMessageId = 0;

void setup() {
  Serial.begin(115200);
  dht.begin();

  WiFi.setAutoReconnect(true);
  configTime(3 * 3600, 0, "pool.ntp.org", "time.nist.gov");
  connectWiFi();
}

void loop() {
  bool wifiConnected = WiFi.status() == WL_CONNECTED;

  if (!wifiConnected) {
    if (wifiWasConnected) {
      wifiWasConnected = false;
    }
    delay(WIFI_RETRY_MS);
    connectWiFi();
    return;
  }

  if (!wifiWasConnected) {
    sendTgMessage(ALLOWED_CHAT_ID, "✅ Соединение WiFi восстановлено");
    liveMessageId = 0;
    wifiWasConnected = true;
  }

  unsigned long now = millis();

  if (now - lastTgPoll >= TELEGRAM_POLL_MS) {
    lastTgPoll = now;
    processTelegram();
  }

  if (now - lastMeasure >= MEASURE_INTERVAL) {
    lastMeasure = now;
    updateLiveMessage(ALLOWED_CHAT_ID);
  }

  struct tm timeinfo;
  if (getLocalTime(&timeinfo)) {
    int hour = timeinfo.tm_hour;
    if ((hour == SEND_REPORT_HOUR_1 || hour == SEND_REPORT_HOUR_2) &&
        lastReportSent != (unsigned long)(hour * 60 + timeinfo.tm_min)) {
      lastReportSent = hour * 60 + timeinfo.tm_min;
      updateLiveMessage(ALLOWED_CHAT_ID);
    }
  }

  delay(100);
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

long sendTgMessage(const char* chatId, const char* text) {
  WiFiClientSecure client;
  client.setInsecure();

  HTTPClient http;
  http.setTimeout(10000);
  String url = String(TG_API) + "/sendMessage";
  http.begin(client, url);
  http.addHeader("Content-Type", "application/json");

  StaticJsonDocument<1024> doc;
  doc["chat_id"] = chatId;
  doc["text"] = text;
  String body;
  serializeJson(doc, body);

  int code = http.POST(body);
  String payload = http.getString();
  http.end();

  if (code == 200) {
    StaticJsonDocument<256> resp;
    if (deserializeJson(resp, payload) == DeserializationError::Ok) {
      return resp["result"]["message_id"].as<long>();
    }
    return 1;
  }
  Serial.println("TG err: " + String(code));
  return 0;
}

bool editTgMessage(const char* chatId, long msgId, const char* text) {
  if (msgId <= 0) return false;
  WiFiClientSecure client;
  client.setInsecure();

  HTTPClient http;
  http.setTimeout(10000);
  String url = String(TG_API) + "/editMessageText";
  http.begin(client, url);
  http.addHeader("Content-Type", "application/json");

  StaticJsonDocument<1024> doc;
  doc["chat_id"] = chatId;
  doc["message_id"] = msgId;
  doc["text"] = text;
  String body;
  serializeJson(doc, body);

  int code = http.POST(body);
  http.end();

  if (code == 200) {
    return true;
  }
  return false;
}

void processTelegram() {
  WiFiClientSecure client;
  client.setInsecure();

  HTTPClient http;
  http.setTimeout(5000);
  String url = String(TG_API) + "/getUpdates?offset=" + String(updateOffset) + "&timeout=1";
  http.begin(client, url);

  int code = http.GET();
  String payload = http.getString();
  http.end();

  if (code != 200) return;

  StaticJsonDocument<4096> doc;
  DeserializationError err = deserializeJson(doc, payload);
  if (err) return;

  JsonArray results = doc["result"];
  for (JsonObject update : results) {
    updateOffset = update["update_id"].as<long>() + 1;

    if (update.containsKey("message")) {
      JsonObject msg = update["message"];
      String text = msg["text"].as<String>();
      long chatIdNum = msg["chat"]["id"];
      String chatId = String(chatIdNum);

      if (text == "/start") {
        String reply = "🌱 Online Greenhome\n\n"
                       "Команды:\n"
                       "/live — обновляемое сообщение (без спама)\n"
                       "/status — все показания\n"
                       "/temp — температура\n"
                       "/soil — влажность почвы\n\n"
                       "Chat ID: " + chatId + "\n(добавьте в ALLOWED_CHAT_ID)";
        sendTgMessage(chatId.c_str(), reply.c_str());
      } else if (chatId == ALLOWED_CHAT_ID &&
                 (text == "/live" || text == "/status" || text == "/temp" || text == "/soil")) {
        handleCommand(chatId.c_str(), text);
      }
    }
  }
}

void handleCommand(const char* chatId, const String& cmd) {
  float t = dht.readTemperature();
  float h = dht.readHumidity();
  int soil = analogRead(SOIL_PIN);

  String reply;
  if (cmd == "/live") {
    liveMessageId = 0;
    updateLiveMessage(chatId);
    return;
  } else if (cmd == "/status") {
    if (!isnan(t) && !isnan(h)) {
      reply = "🌡 Температура: " + String(t, 1) + "°C\n";
      reply += "💧 Влажность: " + String(h, 0) + "%\n";
    } else {
      reply = "❌ Ошибка DHT11\n";
    }
    reply += "🌱 Почва: " + String(soil);
  } else if (cmd == "/temp") {
    reply = isnan(t) ? "❌ Ошибка датчика" : "🌡 " + String(t, 1) + "°C";
  } else if (cmd == "/soil") {
    reply = "🌱 Почва: " + String(soil);
  }

  if (reply.length() > 0) {
    sendTgMessage(chatId, reply.c_str());
  }
}

String buildStatusText(float t, float h, int soil) {
  String msg = "📊 Состояние системы\n\n";
  if (!isnan(t) && !isnan(h)) {
    msg += "🌡 Температура: " + String(t, 1) + "°C\n";
    msg += "💧 Влажность: " + String(h, 0) + "%\n";
  } else {
    msg += "❌ Ошибка DHT11\n";
  }
  msg += "🌱 Почва: " + String(soil);
  return msg;
}

long updateLiveMessage(const char* chatId) {
  float t = dht.readTemperature();
  float h = dht.readHumidity();
  int soil = analogRead(SOIL_PIN);
  String msg = buildStatusText(t, h, soil);

  if (liveMessageId > 0) {
    if (editTgMessage(chatId, liveMessageId, msg.c_str())) {
      Serial.println("OK (edit): T=" + String(t) + " H=" + String(h) + " Soil=" + String(soil));
      return liveMessageId;
    }
    liveMessageId = 0;
  }

  long mid = sendTgMessage(chatId, msg.c_str());
  if (mid > 0) {
    liveMessageId = mid;
    Serial.println("OK (new): T=" + String(t) + " H=" + String(h) + " Soil=" + String(soil));
  } else if (!isnan(t) && !isnan(h)) {
    sendTgMessage(chatId, "❌ Ошибка датчика DHT11");
  }
  return mid;
}
