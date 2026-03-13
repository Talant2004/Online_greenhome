"""Главный модуль — запуск бота, API и планировщика."""
import logging
import threading

from config import TELEGRAM_BOT_TOKEN, SERVER_HOST, SERVER_PORT, ALLOWED_CHAT_IDS
from bot_handlers import cmd_start, cmd_status, cmd_data
from bot_instance import set_bot
from api import app as api_app
from scheduler import setup_jobs

logging.basicConfig(
    format="%(asctime)s - %(name)s - %(levelname)s - %(message)s",
    level=logging.INFO,
)
logger = logging.getLogger(__name__)


def run_flask():
    """Запуск Flask в отдельном потоке."""
    api_app.run(host=SERVER_HOST, port=SERVER_PORT, use_reloader=False)


def main():
    if not TELEGRAM_BOT_TOKEN:
        logger.error("Укажите TELEGRAM_BOT_TOKEN в .env")
        return

    if not ALLOWED_CHAT_IDS:
        logger.warning("ALLOWED_CHAT_IDS пуст — уведомления не будут отправляться")

    from telegram import Bot
    from telegram.ext import Application, CommandHandler

    bot = Bot(token=TELEGRAM_BOT_TOKEN)
    set_bot(bot)

    application = (
        Application.builder()
        .token(TELEGRAM_BOT_TOKEN)
        .build()
    )

    application.add_handler(CommandHandler("start", cmd_start))
    application.add_handler(CommandHandler("status", cmd_status))
    application.add_handler(CommandHandler("data", cmd_data))

    setup_jobs(application)

    flask_thread = threading.Thread(target=run_flask, daemon=True)
    flask_thread.start()
    logger.info(f"API слушает на http://{SERVER_HOST}:{SERVER_PORT}")

    application.run_polling(allowed_updates=["message"])


if __name__ == "__main__":
    main()
