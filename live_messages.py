"""Хранилище ID «живых» сообщений для редактирования на месте."""
from typing import Dict, Optional

# chat_id -> message_id
_live_messages: Dict[int, int] = {}


def set_live(chat_id: int, message_id: int):
    _live_messages[chat_id] = message_id


def get_live(chat_id: int) -> Optional[int]:
    return _live_messages.get(chat_id)


def clear_live(chat_id: int):
    if chat_id in _live_messages:
        del _live_messages[chat_id]


def get_all_chats() -> list[int]:
    return list(_live_messages.keys())
