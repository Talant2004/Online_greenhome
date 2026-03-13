"""Обработчики команд Telegram-бота."""
from telegram import Update
from telegram.ext import ContextTypes

from data_store import format_status
from storage import get_last_measurements


async def cmd_start(update: Update, context: ContextTypes.DEFAULT_TYPE):
    """Команда /start — описание системы."""
    chat_id = update.effective_chat.id
    text = f"""🌱 **Система мониторинга агроклиматических параметров**

Ваш Chat ID: `{chat_id}`
Добавьте его в ALLOWED_CHAT_IDS в .env для получения уведомлений и отчётов.

Бот получает данные с микроконтроллера ESP32:
• DHT11 — температура и влажность воздуха
• BH1750 — освещённость
• Датчик влажности почвы

**Команды:**
/status — текущие значения датчиков
/data — последние измерения

Данные обновляются каждые 30–60 секунд и сохраняются в архив."""
    await update.message.reply_text(text, parse_mode="Markdown")


async def cmd_status(update: Update, context: ContextTypes.DEFAULT_TYPE):
    """Команда /status — текущие значения датчиков."""
    await update.message.reply_text(format_status())


async def cmd_data(update: Update, context: ContextTypes.DEFAULT_TYPE):
    """Команда /data — последние измерения."""
    rows = get_last_measurements(10)
    if not rows:
        await update.message.reply_text("Пока нет сохранённых измерений.")
        return

    lines = ["Последние измерения:\n"]
    for r in rows:
        lines.append(
            f"{r.get('Дата', '')} {r.get('Время', '')} — "
            f"T:{r.get('Температура', '')}°C H:{r.get('Влажность', '')}% "
            f"Почва:{r.get('Почва', '')} Свет:{r.get('Свет', '')}lx"
        )
    await update.message.reply_text("\n".join(lines))
