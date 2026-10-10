#!/usr/bin/env python3
"""Validate ONLY a synthetic Qwen red/blue image reply, never upload CAD data.

Turkish dotless i is not a Unicode combining mark. NFD alone keeps the
character in "kırmızı", so a naive ASCII-only regex can reject a valid answer.
This validator is independent of the deployment script for unit testing.
"""
import json
import re
import sys
import unicodedata
from pathlib import Path


def normalize(value: str) -> str:
    plain = unicodedata.normalize("NFD", value.casefold())
    return "".join(ch for ch in plain if unicodedata.category(ch) != "Mn").replace("ı", "i")


def extract_answer(payload: dict) -> str:
    result = payload.get("result") or payload
    choices = result.get("choices") or []
    message = choices[0].get("message") or {} if choices else {}
    content = message.get("content")
    if isinstance(content, str):
        return content.strip()
    if isinstance(content, list):
        return " ".join(
            item.get("text", "") for item in content if isinstance(item, dict)
        ).strip()
    return ""


def validate_synthetic_vision(payload: dict) -> tuple[bool, str]:
    answer = extract_answer(payload)
    if not answer:
        return False, "EMPTY_CONTENT"
    plain = normalize(answer)
    left = bool(re.search(r"\b(?:kirmizi|red)\b", plain))
    right = bool(re.search(r"\b(?:mavi|blue)\b", plain))
    if not (left and right):
        # Safe diagnostics: only boolean markers, no raw provider responses.
        return False, "COLOR_MISMATCH_red_{}_blue_{}".format(int(left), int(right))
    return True, "PASS"


def main() -> int:
    if len(sys.argv) != 2:
        raise SystemExit("Usage: validate_qwen_vision_result.py <response.json>")
    try:
        payload = json.loads(Path(sys.argv[1]).read_text(encoding="utf-8"))
        ok, code = validate_synthetic_vision(payload)
    except (ValueError, OSError, TypeError, KeyError):
        ok, code = False, "INVALID_JSON"
    if not ok:
        print("::error::Qwen vision readiness failed [" + code + "]; no deployment.")
        return 1
    print("Qwen synthetic-image readiness PASS; response redacted, no user CAD uploaded.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
