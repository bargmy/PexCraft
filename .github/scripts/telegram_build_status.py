#!/usr/bin/env python3
"""Handle one shared Telegram build-status message for GitHub Actions."""

import json
import os
import sys
import urllib.parse
import urllib.request

LOADING_EMOJI = "⏳"
SUCCESS_EMOJI = "✅"
FAILED_EMOJI = "❌"

TASKS = [
    ("Windows x86 MinGW", {"default", "win"}),
    ("Windows x64 MinGW", {"default", "win"}),
    ("Windows arm64 Clang/MinGW", {"default", "win"}),
    ("Linux SDL2/OpenGL", {"default", "linux"}),
    ("LG webOS armv7", {"default", "lg"}),
    ("Android APK", {"default", "android"}),
    ("Android TV APK", {"default", "androidtv"}),
    ("PSP", {"console", "psp"}),
    ("Wii", {"console", "wii"}),
    ("Xbox One UWP Dev Mode", {"console", "xbox"}),
]


def mode_from_text(text: str) -> str:
    text = (text or "").lower()
    if "(console-only)" in text:
        return "console"
    for mode, marker in [
        ("win", "(win-only)"),
        ("linux", "(linux-only)"),
        ("lg", "(lg-only)"),
        ("android", "(android-only)"),
        ("androidtv", "(androidtv-only)"),
        ("psp", "(psp-only)"),
        ("wii", "(wii-only)"),
        ("xbox", "(xbox-only)"),
    ]:
        if marker in text:
            return mode
    return "default"


def selected_tasks() -> list[str]:
    mode = mode_from_text(os.environ.get("REF_TEXT", ""))
    return [task for task, modes in TASKS if mode in modes]


def build_text(tasks: list[str] | None = None, emoji: str = LOADING_EMOJI) -> str:
    tasks = tasks or ["PexCraft build"]
    lines = ["Building..."]
    lines.extend(f"# {emoji} - {task}" for task in tasks)
    lines.append("..")
    return "\n".join(lines)


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


def append_github_output(name: str, value: str) -> None:
    output_path = os.environ.get("GITHUB_OUTPUT")
    if output_path:
        with open(output_path, "a", encoding="utf-8") as handle:
            handle.write(f"{name}={value}\n")


def start_all() -> int:
    chat_id = os.environ.get("CHAT_ID", "").strip()
    if not chat_id:
        print("CHAT_ID is missing; skipping Telegram status message.")
        append_github_output("message_id", "")
        return 0

    parsed = telegram_call("sendMessage", {"chat_id": chat_id, "text": build_text(selected_tasks())})
    try:
        message_id = str(parsed["result"]["message_id"]) if parsed else ""
    except Exception:
        message_id = ""

    append_github_output("message_id", message_id)
    if message_id:
        append_github_env("TG_STATUS_MESSAGE_ID", message_id)
    return 0


def delete() -> int:
    chat_id = os.environ.get("CHAT_ID", "").strip()
    message_id = os.environ.get("TG_STATUS_MESSAGE_ID", "").strip()
    if not chat_id or not message_id:
        print("No Telegram status message to delete; skipping.")
        return 0
    telegram_call("deleteMessage", {"chat_id": chat_id, "message_id": message_id})
    return 0


# Backwards-compatible single-task actions. They now edit/delete only when a shared id exists.
def start() -> int:
    return start_all()


def finish() -> int:
    chat_id = os.environ.get("CHAT_ID", "").strip()
    message_id = os.environ.get("TG_STATUS_MESSAGE_ID", "").strip()
    task = os.environ.get("TASK_NAME", "PexCraft build").strip() or "PexCraft build"
    result = os.environ.get("RESULT", "failure").strip().lower()
    emoji = SUCCESS_EMOJI if result in {"success", "ok", "passed", "1", "true"} else FAILED_EMOJI

    if not chat_id or not message_id:
        print("No Telegram status message to edit; skipping.")
        return 0

    telegram_call("editMessageText", {"chat_id": chat_id, "message_id": message_id, "text": build_text([task], emoji)})
    return 0


if __name__ == "__main__":
    action = sys.argv[1] if len(sys.argv) > 1 else ""
    if action in {"start-all", "start_all"}:
        raise SystemExit(start_all())
    if action == "delete":
        raise SystemExit(delete())
    if action == "start":
        raise SystemExit(start())
    if action == "finish":
        raise SystemExit(finish())
    print("usage: telegram_build_status.py start-all|delete", file=sys.stderr)
    raise SystemExit(2)
