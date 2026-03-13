# Telegram-бот мониторинга агроклиматических параметров

Бот получает данные с ESP32 (DHT11, датчик почвы) и отображает их в Telegram.

## Быстрый старт

### 1. Настройка бота

```bash
cd c:\telegrambot
python -m venv venv
venv\Scripts\activate
pip install -r requirements.txt
```

Скопируйте `.env.example` в `.env` и заполните:

```
TELEGRAM_BOT_TOKEN=токен_от_@BotFather
ALLOWED_CHAT_IDS=123456789
API_SECRET=случайная_строка_для_защиты
```

**Как узнать Chat ID:** запустите бота, отправьте `/start` — он покажет ваш Chat ID.

### 2. Запуск

```bash
python main.py
```

### 3. Настройка ESP32

**Вариант A — через Render (сервер):**  
`esp32/src/main.cpp` → `SERVER_URL`, `API_SECRET`

**Вариант B — автономно (без сервера):**  
`esp32/src/main_standalone.cpp` → `BOT_TOKEN`, `ALLOWED_CHAT_ID`  
Сборка: `pio run -e standalone`

Соберите и загрузите прошивку через PlatformIO.

## Команды бота

| Команда   | Описание                     |
| --------- | ---------------------------- |
| /start    | Описание системы, ваш Chat ID |
| /status   | Текущие значения датчиков    |
| /data     | Последние 10 измерений       |

## API для ESP32

- `POST /api/sensors` — отправить данные датчиков (JSON: temperature, humidity, soil, light)
- `POST /api/notification` — отправить уведомление (JSON: type, message)
- Заголовок `X-API-Secret` обязателен

## Расписание

Автоматические отчёты отправляются в **08:00** и **20:00**.

## Хранение данных

Измерения сохраняются в `data/measurements.csv` (формат: Дата, Время, Температура, Влажность, Почва, Свет).

---

## Развёртывание на Render

1. **New → Web Service** → подключи репозиторий `Online_greenhome`
2. **Settings → Environment:** смени **Python** на **Docker** (или создай сервис заново с **Docker**)
3. **Environment Variables:**
   | Key | Value |
   |-----|-------|
   | TELEGRAM_BOT_TOKEN | твой_токен_от_BotFather |
   | ALLOWED_CHAT_IDS | -5198577449 |
   | API_SECRET | agro_secret_2026 |
4. **Region:** Oregon (US West) | **Plan:** Free

После деплоя получишь URL вида `https://online-greenhome.onrender.com`. Обнови в ESP32 `SERVER_URL` на этот адрес (вместо `http://192.168.1.100:5000`).

⚠️ **Free-план:** сервис засыпает после 15 мин без запросов. ESP32 при отправке данных «разбудит» сервис (задержка ~30 сек).

# Online_greenhome
