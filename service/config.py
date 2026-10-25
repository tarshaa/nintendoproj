"""Local-only credentials. Never print, serve, or copy these to the SD card."""
import json
import os
from pathlib import Path

PRIVATE = Path(__file__).resolve().parent / ".private"


def settings():
    path = PRIVATE / "settings.json"
    config = json.loads(path.read_text()) if path.exists() else {}
    return {
        "api_key": os.environ.get("OPENAI_API_KEY", config.get("api_key", "")),
        "model": os.environ.get("PHOTO_WORLDS_MODEL", config.get("model", "gpt-4.1-mini")),
        "device_token": os.environ.get("PHOTO_WORLDS_DEVICE_TOKEN", config.get("device_token", "")),
    }


def private_write(path, content):
    path.parent.mkdir(mode=0o700, parents=True, exist_ok=True)
    os.chmod(path.parent, 0o700)
    tmp = path.with_suffix(path.suffix + ".tmp")
    fd = os.open(tmp, os.O_WRONLY | os.O_CREAT | os.O_TRUNC, 0o600)
    with os.fdopen(fd, "wb") as stream:
        stream.write(content)
    os.chmod(tmp, 0o600)
    os.replace(tmp, path)
