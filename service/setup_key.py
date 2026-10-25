#!/usr/bin/env python3
"""User-run private credential prompt; does not make a paid request."""
import getpass
import json
import secrets
import sys
from config import PRIVATE, private_write, settings


def main():
    if not sys.stdin.isatty():
        raise SystemExit("Open the setup command in Terminal; do not enter your key in chat.")
    print("Photo Worlds — API setup\n")
    print("Create a key at https://platform.openai.com/api-keys")
    print("Paste it below. Input is hidden. This step does not generate a game or spend API credit.\n")
    key = getpass.getpass("OpenAI API key: ").strip()
    if not key.startswith("sk-") or len(key) < 24 or any(c.isspace() for c in key):
        raise SystemExit("That does not look like a complete API key. Nothing was saved.")
    old = settings()
    config = {"api_key": key, "model": old["model"],
              "device_token": old["device_token"] or secrets.token_hex(32)}
    private_write(PRIVATE / "settings.json", (json.dumps(config) + "\n").encode())
    print("\nSaved privately on this Mac. You can tell Codex: API setup is ready.")
    print("The key will stay on the server; it will not go on the DSi or into the game.")
    input("Press Return to close. ")


if __name__ == "__main__":
    main()
