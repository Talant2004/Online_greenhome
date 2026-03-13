"""Обработчики команд Telegram-бота."""
from telegram import Update, InlineKeyboardButton, InlineKeyboardMarkup, ReplyKeyboardMarkup, KeyboardButton
from telegram.ext import ContextTypes

from config import ALLOWED_CHAT_IDS
from data_store import format_status, format_temp, format_soil
from storage import get_last_measurements
from live_messages import set_live, clear_live, get_live


def get_keyboard():
    """Клавиатура с кнопками."""
    return ReplyKeyboardMarkup(
        [
            [KeyboardButton("📊 Status"), KeyboardButton("🔄 Обновить")],
            [KeyboardButton("📜 Data"), KeyboardButton("🌡 Temp"), KeyboardButton("🌱 Soil")],
            [KeyboardButton("⏹ Стоп")],
        ],
        resize_keyboard=True,
    )


def get_refresh_keyboard():
    """Кнопка обновить под сообщением."""
    return InlineKeyboardMarkup([
        [InlineKeyboardButton("🔄 Обновить", callback_data="refresh")],
    ])


def is_allowed(chat_id: int) -> bool:
    return not ALLOWED_CHAT_IDS or chat_id in ALLOWED_CHAT_IDS


async def cmd_start(update: Update, context: ContextTypes.DEFAULT_TYPE):
    """Команда /start — описание и кнопки."""
    chat_id = update.effective_chat.id
    text = f"""🌱 **Online Greenhome**

Используйте кнопки внизу — не нужно вводить команды.

Ваш Chat ID: `{chat_id}`"""
    await update.message.reply_text(text, parse_mode="Markdown", reply_markup=get_keyboard())


async def cmd_status(update: Update, context: ContextTypes.DEFAULT_TYPE):
    """Один раз — текущие значения."""
    if not is_allowed(update.effective_chat.id):
        return
    await update.message.reply_text(format_status())


async def cmd_temp(update: Update, context: ContextTypes.DEFAULT_TYPE):
    if not is_allowed(update.effective_chat.id):
        return
    await update.message.reply_text(format_temp())


async def cmd_soil(update: Update, context: ContextTypes.DEFAULT_TYPE):
    if not is_allowed(update.effective_chat.id):
        return
    await update.message.reply_text(format_soil())


async def cmd_stop(update: Update, context: ContextTypes.DEFAULT_TYPE):
    """Остановить режим Live."""
    if not is_allowed(update.effective_chat.id):
        return
    chat_id = update.effective_chat.id
    clear_live(chat_id)
    await update.message.reply_text("⏹ Режим Live остановлен")


async def cmd_live(update: Update, context: ContextTypes.DEFAULT_TYPE):
    """Сообщение с кнопкой Обновить — обновляется только по нажатию."""
    if not is_allowed(update.effective_chat.id):
        return
    chat_id = update.effective_chat.id
    clear_live(chat_id)
    msg = await update.message.reply_text(
        format_status(),
        reply_markup=get_refresh_keyboard(),
    )
    set_live(chat_id, msg.message_id)


async def cmd_data(update: Update, context: ContextTypes.DEFAULT_TYPE):
    if not is_allowed(update.effective_chat.id):
        return
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


async def button_press(update: Update, context: ContextTypes.DEFAULT_TYPE):
    """Обработка нажатий кнопок под сообщением."""
    query = update.callback_query
    await query.answer()
    chat_id = query.message.chat_id
    msg_id = query.message.message_id
    if not is_allowed(chat_id):
        return

    if query.data == "refresh":
        if get_live(chat_id) == msg_id:
            try:
                await query.edit_message_text(
                    format_status(),
                    reply_markup=get_refresh_keyboard(),
                )
            except Exception:
                pass


async def text_message(update: Update, context: ContextTypes.DEFAULT_TYPE):
    """Обработка текста от кнопок клавиатуры."""
    if not update.message or not update.message.text:
        return
    chat_id = update.effective_chat.id
    if not is_allowed(chat_id):
        return

    text = update.message.text.strip()
    if text == "📊 Status":
        await update.message.reply_text(format_status())
    elif text == "🔄 Обновить":
        await cmd_live(update, context)
    elif text == "📜 Data":
        await cmd_data(update, context)
    elif text == "🌡 Temp":
        await update.message.reply_text(format_temp())
    elif text == "🌱 Soil":
        await update.message.reply_text(format_soil())
    elif text == "⏹ Стоп":
        await cmd_stop(update, context)
