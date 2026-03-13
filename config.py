"""Конфигурация бота."""
import os
from dotenv import load_dotenv

load_dotenv()

TELEGRAM_BOT_TOKEN = os.getenv("TELEGRAM_BOT_TOKEN", "")
ALLOWED_CHAT_IDS = [
    int(x.strip()) for x in os.getenv("ALLOWED_CHAT_IDS", "").split(",") if x.strip()
]
API_SECRET = os.getenv("API_SECRET", "default_secret_change_me")
SERVER_HOST = os.getenv("SERVER_HOST", "0.0.0.0")
# Render предоставляет PORT — используем его в облаке
SERVER_PORT = int(os.getenv("PORT", os.getenv("SERVER_PORT", "5000")))
DATA_FILE = os.getenv("DATA_FILE", "data/measurements.csv")
