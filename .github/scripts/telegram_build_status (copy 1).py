#!/usr/bin/env python3
"""Send build-status messages to Telegram.

Usage:
  TOKEN=... CHAT_ID=... COMMIT_NAME=... python telegram_build_status.py start

The start action sends exactly:
  {commit name} - building...
then pins that message. It does not edit/update the message later.
"""
import json
import os
import sys
import urllib.parse
import urllib.request


def telegram_call(token: str, method: str, fields: dict, required: bool = True) -> dict:
    data = urllib.parse.urlencode(fields).encode("utf-8")
    req = urllib.request.Request(
        f"https://api.telegram.org/bot{token}/{method}",
        data=data,
        headers={"Content-Type": "application/x-www-form-urlencoded"},
    )
    try:
        with urllib.request.urlopen(req, timeout=60) as resp:
            payload = resp.read().decode("utf-8", "replace")
    except Exception as exc:
        if required:
            print(f"Telegram {method} failed: {exc}", file=sys.stderr)
            raise SystemExit(1)
        print(f"Telegram {method} warning: {exc}", file=sys.stderr)
        return {"ok": False, "description": str(exc)}

    try:
        out = json.loads(payload)
    except Exception:
        if required:
            print(f"Telegram {method} returned non-JSON: {payload}", file=sys.stderr)
            raise SystemExit(1)
        print(f"Telegram {method} returned non-JSON: {payload}", file=sys.stderr)
        return {"ok": False, "description": payload}

    if required and not out.get("ok"):
        print(f"Telegram {method} returned error: {payload}", file=sys.stderr)
        raise SystemExit(1)
    if not out.get("ok"):
        print(f"Telegram {method} warning: {payload}", file=sys.stderr)
    return out


def main() -> int:
    action = sys.argv[1] if len(sys.argv) > 1 else "start"
    token = os.environ.get("TOKEN", "").strip()
    chat_id = os.environ.get("CHAT_ID", "").strip()

    # Pull requests from forks usually do not have secrets. Do not kill builds for that.
    if not token or not chat_id:
        print("Telegram build status skipped: missing TOKEN or CHAT_ID.")
        return 0

    if action != "start":
        print(f"Unknown telegram build-status action: {action}", file=sys.stderr)
        return 2

    commit = os.environ.get("COMMIT_NAME", "").strip() or os.environ.get("GITHUB_REF_NAME", "").strip() or "PexCraft"
    # Telegram message requested by the user. Keep it plain text and do not edit it later.
    text = f"{commit} - building..."

    sent = telegram_call(
        token,
        "sendMessage",
        {
            "chat_id": chat_id,
            "text": text,
            "disable_web_page_preview": "true",
        },
        required=True,
    )
    message_id = sent.get("result", {}).get("message_id")
    if message_id is not None:
        telegram_call(
            token,
            "pinChatMessage",
            {
                "chat_id": chat_id,
                "message_id": str(message_id),
                "disable_notification": "true",
            },
            required=False,
        )
        print(f"Sent and requested pin for Telegram message {message_id}: {text}")
    else:
        print(f"Telegram sendMessage response had no message_id: {sent}", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
