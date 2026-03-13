"""Хранилище последних данных датчиков в памяти."""
from dataclasses import dataclass
from datetime import datetime
from typing import Optional


@dataclass
class SensorData:
    temperature: float
    humidity: float
    soil: int
    light: int
    timestamp: datetime


_last_data: Optional[SensorData] = None


def update(temperature: float, humidity: float, soil: int, light: int):
    """Обновить последние данные."""
    global _last_data
    _last_data = SensorData(
        temperature=temperature,
        humidity=humidity,
        soil=soil,
        light=light,
        timestamp=datetime.now(),
    )


def get() -> Optional[SensorData]:
    """Получить последние данные."""
    return _last_data


def format_status() -> str:
    """Форматировать данные для сообщения."""
    data = get()
    if not data:
        return "📭 Данные ещё не получены от ESP32.\nОжидайте первое измерение."

    return f"""📊 Состояние системы

🌡 Температура: {data.temperature} °C
💧 Влажность воздуха: {data.humidity} %
🌱 Влажность почвы: {data.soil}
💡 Освещённость: {data.light} lux

Обновлено: {data.timestamp.strftime('%d.%m.%Y %H:%M:%S')}"""


def format_temp() -> str:
    """Только температура."""
    data = get()
    if not data:
        return "📭 Нет данных"
    return f"🌡 Температура: {data.temperature} °C"


def format_soil() -> str:
    """Только влажность почвы."""
    data = get()
    if not data:
        return "📭 Нет данных"
    return f"🌱 Влажность почвы: {data.soil}"
