"""Обработчики команд Telegram-бота."""
from telegram import Update
from telegram.ext import ContextTypes

from data_store import format_status, format_temp, format_soil
from storage import get_last_measurements
from live_messages import set_live, clear_live


async def cmd_start(update: Update, context: ContextTypes.DEFAULT_TYPE):
    """Команда /start — описание системы."""
    chat_id = update.effective_chat.id
    text = f"""🌱 **Online Greenhome**

Ваш Chat ID: `{chat_id}`
Добавьте его в ALLOWED_CHAT_IDS для уведомлений.

Бот получает данные с ESP32 (DHT11, датчик почвы).

**Команды:**
/live — обновляемое сообщение (без спама)
/status — текущие значения
/temp — температура
/soil — влажность почвы
/data — последние 10 измерений

Данные обновляются каждые 30 сек."""
    await update.message.reply_text(text, parse_mode="Markdown")


async def cmd_status(update: Update, context: ContextTypes.DEFAULT_TYPE):
    """Команда /status — текущие значения датчиков."""
    await update.message.reply_text(format_status())


async def cmd_temp(update: Update, context: ContextTypes.DEFAULT_TYPE):
    """Команда /temp — только температура."""
    await update.message.reply_text(format_temp())


async def cmd_soil(update: Update, context: ContextTypes.DEFAULT_TYPE):
    """Команда /soil — влажность почвы."""
    await update.message.reply_text(format_soil())


async def cmd_live(update: Update, context: ContextTypes.DEFAULT_TYPE):
    """Команда /live — создать сообщение, обновляемое на месте."""
    chat_id = update.effective_chat.id
    clear_live(chat_id)
    msg = await update.message.reply_text(format_status())
    set_live(chat_id, msg.message_id)
    await update.message.reply_text("✅ Обновляется автоматически при данных от ESP32")


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
