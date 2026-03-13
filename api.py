"""HTTP API для приёма данных от ESP32."""
from flask import Flask, request, jsonify

from config import API_SECRET, ALLOWED_CHAT_IDS
from data_store import update as update_data, format_status
from storage import save_measurement
from bot_instance import get_bot

app = Flask(__name__)


def check_auth():
    """Проверка секрета в заголовке."""
    return request.headers.get("X-API-Secret") == API_SECRET


def send_to_all_chats(text: str):
    """Отправить сообщение всем разрешённым чатам."""
    bot = get_bot()
    if not bot or not ALLOWED_CHAT_IDS:
        return
    for chat_id in ALLOWED_CHAT_IDS:
        try:
            bot.send_message(chat_id=chat_id, text=text)
        except Exception:
            pass


@app.route("/api/sensors", methods=["POST"])
def receive_sensors():
    """Принять данные датчиков от ESP32."""
    if not check_auth():
        return jsonify({"error": "Unauthorized"}), 401

    data = request.get_json() or {}
    temp = float(data.get("temperature", 0))
    humidity = float(data.get("humidity", 0))
    soil = int(data.get("soil", 0))
    light = int(data.get("light", 0))

    update_data(temp, humidity, soil, light)
    save_measurement(temp, humidity, soil, light)

    return jsonify({"ok": True})


@app.route("/api/notification", methods=["POST"])
def receive_notification():
    """Принять уведомление от ESP32 (WiFi lost, sensor error и т.д.)."""
    if not check_auth():
        return jsonify({"error": "Unauthorized"}), 401

    data = request.get_json() or {}
    msg_type = data.get("type", "info")
    message = data.get("message", "")

    if msg_type == "wifi_lost":
        text = "⚠️ Внимание\nСоединение WiFi потеряно"
    elif msg_type == "wifi_restored":
        text = "✅ Соединение WiFi восстановлено"
    elif msg_type == "sensor_error":
        text = "❌ Ошибка датчика"
    else:
        text = message or "Уведомление от ESP32"

    send_to_all_chats(text)
    return jsonify({"ok": True})


@app.route("/")
def index():
    """Главная страница."""
    return jsonify({
        "service": "Online Greenhome",
        "status": "ok",
        "endpoints": {
            "health": "/health",
            "api_sensors": "POST /api/sensors",
            "api_notification": "POST /api/notification",
        },
    })


@app.route("/health", methods=["GET"])
def health():
    """Проверка работоспособности."""
    return jsonify({"status": "ok"})
