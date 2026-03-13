"""Планировщик отправки данных по расписанию (08:00 и 20:00)."""
from telegram.ext import ContextTypes
from datetime import time

from data_store import format_status
from config import ALLOWED_CHAT_IDS


async def send_scheduled_report(context: ContextTypes.DEFAULT_TYPE):
    """Отправить регулярный отчёт в разрешённые чаты."""
    if not ALLOWED_CHAT_IDS:
        return
    text = "📊 Регулярный отчёт\n\n" + format_status()
    for chat_id in ALLOWED_CHAT_IDS:
        try:
            await context.bot.send_message(chat_id=chat_id, text=text)
        except Exception:
            pass


def setup_jobs(application):
    """Добавить задания в JobQueue бота."""
    from config import ENABLE_SCHEDULED_REPORTS
    if not ENABLE_SCHEDULED_REPORTS:
        return
    job_queue = application.job_queue
    if job_queue:
        job_queue.run_daily(send_scheduled_report, time=time(hour=8, minute=0))
        job_queue.run_daily(send_scheduled_report, time=time(hour=20, minute=0))
