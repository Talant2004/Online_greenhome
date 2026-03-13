#!/usr/bin/env bash
set -e
# Render: venv может быть в .venv или /opt/render/project/src/.venv
for py in .venv/bin/python /opt/render/project/src/.venv/bin/python; do
    if [ -f "$py" ]; then
        exec "$py" main.py
    fi
done
exec python3 main.py
