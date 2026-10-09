#!/usr/bin/env python3
"""Fail-closed guard for MusaCAD's existing Cloudflare license Worker.

Reads a temporary Cloudflare settings JSON file; NEVER prints variable values,
developer device IDs, API credentials, secrets or the full settings response.
This script only validates metadata and writes an ephemeral Wrangler config.
It does not deploy, mutate D1 or make network calls.
"""
import argparse
import json
import re
import sys
from pathlib import Path

WORKER = "musacad-trial-api"
SUPPORTED = {"plain_text", "secret_text", "d1"}
DEVICE = re.compile(r"^(?:MC-[0-9A-F]{8}-[0-9A-F]{8}-[0-9A-F]{8}|MC-FALLBACK-[0-9A-F-]{36})$")
DB_ID = re.compile(r"^[0-9a-fA-F-]{32,36}$")


class GuardError(Exception):
    pass


def settings_bindings(raw):
    if raw.get("success") is False or not isinstance(raw.get("result"), dict):
        raise GuardError("Cloudflare settings API did not succeed")
    bindings = raw["result"].get("bindings")
    if not isinstance(bindings, list) or not bindings:
        raise GuardError("Missing Worker binding metadata")
    by_name = {}
    for binding in bindings:
        if not isinstance(binding, dict):
            raise GuardError("Invalid Worker binding metadata")
        name, typ = binding.get("name"), binding.get("type")
        if not isinstance(name, str) or not name or not isinstance(typ, str):
            raise GuardError("Worker binding name/type is missing")
        if name in by_name:
            raise GuardError("Duplicate Worker binding name")
        if typ not in SUPPORTED:
            raise GuardError("Unsupported Worker binding type; refuse risky redeploy")
        by_name[name] = binding
    db = by_name.get("DB")
    if not db or db["type"] != "d1":
        raise GuardError("Existing production D1 binding DB is missing")
    if len([b for b in bindings if b["type"] == "d1"]) != 1:
        raise GuardError("Unexpected additional D1 database binding")
    db_id = db.get("database_id") or db.get("id")
    if not isinstance(db_id, str) or not DB_ID.fullmatch(db_id):
        raise GuardError("Existing D1 database ID is invalid")
    package = by_name.get("MUSACAD_PACKAGE_NAME")
    if not package or package["type"] != "plain_text" or package.get("text") != "com.musa.cad":
        raise GuardError("Existing production app package binding is missing or mismatched")
    developer = by_name.get("MUSACAD_DEVELOPER_DEVICE_IDS")
    if not developer or developer["type"] != "plain_text":
        raise GuardError("Production developer-device allowlist is missing")
    ids = developer.get("text")
    if not isinstance(ids, str) or not ids.strip():
        raise GuardError("Developer-device allowlist has no configured IDs")
    if not all(DEVICE.fullmatch(item.strip().upper()) for item in ids.split(",")):
        raise GuardError("Developer-device allowlist contains invalid identifiers")
    signing = by_name.get("MUSACAD_TRIAL_PRIVATE_KEY_PEM")
    if not signing or signing["type"] != "secret_text":
        raise GuardError("Existing trial private signing secret binding is missing")
    return by_name, db_id


def comparable(binding):
    typ = binding["type"]
    if typ == "plain_text":
        # Compare values in memory only; never log them.
        return (typ, binding.get("text"))
    if typ == "d1":
        return (typ, binding.get("database_id") or binding.get("id"))
    return (typ, None)


def run(argv=None):
    p = argparse.ArgumentParser()
    p.add_argument("--settings", required=True, type=Path)
    p.add_argument("--output", type=Path, help="Ephemeral output wrangler.toml for new code")
    p.add_argument("--compare", type=Path, help="Post-deploy Worker settings snapshot")
    a = p.parse_args(argv)
    original = json.loads(a.settings.read_text(encoding="utf-8"))
    before, db_id = settings_bindings(original)
    if a.compare:
        after, after_id = settings_bindings(json.loads(a.compare.read_text(encoding="utf-8")))
        if db_id != after_id or set(before) != set(after):
            raise GuardError("Production bindings were changed; manual review required")
        for name in before:
            if comparable(before[name]) != comparable(after[name]):
                raise GuardError("Production binding unexpectedly changed; manual review required")
        print("PASS: Existing D1, plain variables and secret binding names/types retained.")
    if a.output:
        a.output.parent.mkdir(parents=True, exist_ok=True)
        a.output.write_text(
            'name = "' + WORKER + '"\n'
            'main = "src/index.js"\n'
            'compatibility_date = "2026-09-01"\n\n'
            '[[d1_databases]]\n'
            'binding = "DB"\n'
            'database_name = "musacad-trials"\n'
            'database_id = "' + db_id + '"\n', encoding="utf-8"
        )
        print("PASS: Minimal config prepared for existing production D1.")
    print("PASS: Validated production license Worker metadata, "
          f"{len(before)} binding names; secret/device values were not logged.")


if __name__ == "__main__":
    try:
        run()
    except (GuardError, OSError, ValueError, KeyError, TypeError) as exc:
        # GuardError contains only static non-sensitive messages.
        msg = str(exc) if isinstance(exc, GuardError) else "Could not parse Worker settings metadata"
        print("BLOCKED: " + msg, file=sys.stderr)
        sys.exit(1)
