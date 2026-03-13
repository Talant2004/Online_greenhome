"""Хранение данных измерений в CSV."""
import csv
import os
from datetime import datetime
from typing import Optional

from config import DATA_FILE


def ensure_data_dir():
    """Создать директорию для данных при необходимости."""
    os.makedirs(os.path.dirname(DATA_FILE) or ".", exist_ok=True)


def init_csv():
    """Инициализировать CSV-файл с заголовками."""
    ensure_data_dir()
    if not os.path.exists(DATA_FILE):
        with open(DATA_FILE, "w", newline="", encoding="utf-8") as f:
            writer = csv.writer(f)
            writer.writerow(["Дата", "Время", "Температура", "Влажность", "Почва", "Свет"])


def save_measurement(temperature: float, humidity: float, soil: int, light: int):
    """Сохранить измерение в таблицу."""
    init_csv()
    now = datetime.now()
    with open(DATA_FILE, "a", newline="", encoding="utf-8") as f:
        writer = csv.writer(f)
        writer.writerow([
            now.strftime("%Y-%m-%d"),
            now.strftime("%H:%M:%S"),
            temperature,
            humidity,
            soil,
            light,
        ])


def get_last_measurements(count: int = 10) -> list[dict]:
    """Получить последние N измерений."""
    if not os.path.exists(DATA_FILE):
        return []
    try:
        with open(DATA_FILE, "r", encoding="utf-8") as f:
            reader = csv.DictReader(f, fieldnames=["Дата", "Время", "Температура", "Влажность", "Почва", "Свет"])
            rows = list(reader)[1:]  # skip header
            return rows[-count:][::-1]
    except Exception:
        return []
