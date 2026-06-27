#!/usr/bin/env python3
"""Send/edit a tiny per-build Telegram status message for GitHub Actions."""

import json
import os
import sys
import urllib.parse
import urllib.request

LOADING_EMOJI = "⏳"
SUCCESS_EMOJI = "✅"
FAILED_EMOJI = "❌"


def build_text(emoji: str, task: str) -> str:
    return f"Building...\n# {emoji} - {task}\n.."


def telegram_call(method: str, payload: dict):
    token = os.environ.get("TOKEN", "").strip()
    if not token:
        print("Telegram TOKEN is missing; skipping Telegram status update.")
        return None

    data = urllib.parse.urlencode(payload).encode("utf-8")
    request = urllib.request.Request(f"https://api.telegram.org/bot{token}/{method}", data=data)
    try:
        with urllib.request.urlopen(request, timeout=45) as response:
            raw = response.read().decode("utf-8", "replace")
            print(raw)
            parsed = json.loads(raw)
            if not parsed.get("ok"):
                return None
            return parsed
    except Exception as exc:  # Keep Telegram failures from breaking the build itself.
        print(f"Telegram status update skipped: {exc}")
        return None


def append_github_env(name: str, value: str) -> None:
    env_path = os.environ.get("GITHUB_ENV")
    if env_path:
        with open(env_path, "a", encoding="utf-8") as handle:
            handle.write(f"{name}={value}\n")


def start() -> int:
    chat_id = os.environ.get("CHAT_ID", "").strip()
    task = os.environ.get("TASK_NAME", "PexCraft build").strip() or "PexCraft build"
    if not chat_id:
        print("CHAT_ID is missing; skipping Telegram status message.")
        return 0

    parsed = telegram_call("sendMessage", {"chat_id": chat_id, "text": build_text(LOADING_EMOJI, task)})
    try:
        message_id = str(parsed["result"]["message_id"]) if parsed else ""
    except Exception:
        message_id = ""
    if message_id:
        append_github_env("TG_STATUS_MESSAGE_ID", message_id)
    return 0


def finish() -> int:
    chat_id = os.environ.get("CHAT_ID", "").strip()
    message_id = os.environ.get("TG_STATUS_MESSAGE_ID", "").strip()
    task = os.environ.get("TASK_NAME", "PexCraft build").strip() or "PexCraft build"
    result = os.environ.get("RESULT", "failure").strip().lower()
    emoji = SUCCESS_EMOJI if result in {"success", "ok", "passed", "1", "true"} else FAILED_EMOJI

    if not chat_id or not message_id:
        print("No Telegram status message to edit; skipping.")
        return 0

    telegram_call(
        "editMessageText",
        {"chat_id": chat_id, "message_id": message_id, "text": build_text(emoji, task)},
    )
    return 0


if __name__ == "__main__":
    action = sys.argv[1] if len(sys.argv) > 1 else ""
    if action == "start":
        raise SystemExit(start())
    if action == "finish":
        raise SystemExit(finish())
    print("usage: telegram_build_status.py start|finish", file=sys.stderr)
    raise SystemExit(2)
