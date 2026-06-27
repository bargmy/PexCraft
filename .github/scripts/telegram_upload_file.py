#!/usr/bin/env python3
"""Upload one or more build outputs directly to Telegram.

This avoids GitHub Actions artifact storage entirely. Configure with:
  TOKEN, CHAT_ID, FILE_PATH or FILE_PATHS, BUILDER, COMMIT_NAME
"""
import mimetypes
import os
import sys
import uuid
import urllib.request


def fail(msg: str) -> None:
    print(msg, file=sys.stderr)
    raise SystemExit(1)


def iter_paths():
    raw = os.environ.get("FILE_PATHS") or os.environ.get("FILE_PATH") or ""
    for line in raw.replace(";", "\n").splitlines():
        path = line.strip().strip('"')
        if path:
            yield path


def content_type(path: str) -> str:
    name = os.path.basename(path).lower()
    guessed = mimetypes.guess_type(name)[0]
    if guessed:
        return guessed
    if name.endswith(".apk"):
        return "application/vnd.android.package-archive"
    if name.endswith(".ipk"):
        return "application/vnd.debian.binary-package"
    if name.endswith((".appx", ".appxbundle", ".msix", ".msixbundle")):
        return "application/vnd.ms-appx"
    if name.endswith(".zip"):
        return "application/zip"
    if name.endswith(".cer"):
        return "application/pkix-cert"
    return "application/octet-stream"


def multipart(fields, files):
    boundary = "----pexcraft-" + uuid.uuid4().hex
    body = []
    for name, value in fields.items():
        body.append(
            (
                f"--{boundary}\r\n"
                f"Content-Disposition: form-data; name=\"{name}\"\r\n\r\n"
                f"{value}\r\n"
            ).encode("utf-8")
        )
    for field_name, path in files:
        filename = os.path.basename(path)
        body.append(
            (
                f"--{boundary}\r\n"
                f"Content-Disposition: form-data; name=\"{field_name}\"; filename=\"{filename}\"\r\n"
                f"Content-Type: {content_type(path)}\r\n\r\n"
            ).encode("utf-8")
        )
        with open(path, "rb") as f:
            body.append(f.read())
        body.append(b"\r\n")
    body.append(f"--{boundary}--\r\n".encode("utf-8"))
    return boundary, b"".join(body)


def send_document(token: str, chat_id: str, path: str, caption: str) -> None:
    boundary, data = multipart(
        {
            "chat_id": chat_id,
            "caption": caption[:1024],
        },
        [("document", path)],
    )
    req = urllib.request.Request(
        f"https://api.telegram.org/bot{token}/sendDocument",
        data=data,
        headers={"Content-Type": f"multipart/form-data; boundary={boundary}"},
    )
    with urllib.request.urlopen(req, timeout=300) as resp:
        print(resp.read().decode("utf-8", "replace"))


def main() -> int:
    token = os.environ.get("TOKEN", "").strip()
    chat_id = os.environ.get("CHAT_ID", "").strip()
    if not token:
        fail("Missing Telegram bot token. Add repository secret TOKEN.")
    if not chat_id:
        fail("Missing Telegram chat id. Set CHAT_ID in the workflow env.")

    paths = list(iter_paths())
    if not paths:
        fail("No FILE_PATH or FILE_PATHS provided for Telegram upload.")

    builder = os.environ.get("BUILDER", "PexCraft build").strip() or "PexCraft build"
    commit = os.environ.get("COMMIT_NAME", "").strip()
    sha = os.environ.get("GITHUB_SHA", "")[:12]
    for i, path in enumerate(paths):
        if not os.path.exists(path):
            fail(f"Telegram upload file does not exist: {path}")
        caption = f"PexCraft build\nBuilder: {builder}\nCommit: {commit} - {sha}"
        if len(paths) > 1:
            caption += f"\nPart: {i + 1}/{len(paths)}"
        print(f"Uploading {path} to Telegram...")
        send_document(token, chat_id, path, caption)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
