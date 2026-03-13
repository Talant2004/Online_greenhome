"""Конфигурация бота."""
import os

# Загрузка .env локально (без внешних зависимостей)
_env_path = os.path.join(os.path.dirname(__file__), ".env")
if os.path.exists(_env_path):
    with open(_env_path, encoding="utf-8") as f:
        for line in f:
            line = line.strip()
            if line and not line.startswith("#") and "=" in line:
                k, _, v = line.partition("=")
                os.environ.setdefault(k.strip(), v.strip().strip('"').strip("'"))

TELEGRAM_BOT_TOKEN = os.getenv("TELEGRAM_BOT_TOKEN", "")
ALLOWED_CHAT_IDS = [
    int(x.strip()) for x in os.getenv("ALLOWED_CHAT_IDS", "").split(",") if x.strip()
]
API_SECRET = os.getenv("API_SECRET", "default_secret_change_me")
SERVER_HOST = os.getenv("SERVER_HOST", "0.0.0.0")
# Render предоставляет PORT — используем его в облаке
SERVER_PORT = int(os.getenv("PORT", os.getenv("SERVER_PORT", "5000")))
DATA_FILE = os.getenv("DATA_FILE", "data/measurements.csv")
