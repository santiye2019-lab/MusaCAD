#!/usr/bin/env python3
"""Validate DXF evidence produced from pinned LibreDWG sample fixtures."""
import argparse
from pathlib import Path


def pairs_from_dxf(path):
    lines = Path(path).read_text(encoding="latin-1").replace("\r", "").split("\n")
    return [(lines[i].strip(), lines[i + 1].strip())
            for i in range(0, len(lines) - 1, 2)]


def check(path, expect_ole):
    pairs = pairs_from_dxf(path)
    if not pairs or ("0", "SECTION") not in pairs or ("0", "EOF") not in pairs:
        raise SystemExit(f"Invalid/incomplete ASCII DXF: {path}")
    if not any(k == "2" and v == "ENTITIES" for k, v in pairs):
        raise SystemExit(f"Missing ENTITIES section: {path}")
    if expect_ole:
        in_ole = False
        ole_seen = False
        payload_seen = False
        for group, value in pairs:
            if group == "0":
                in_ole = (value.upper() == "OLE2FRAME")
                ole_seen |= in_ole
            elif in_ole and group == "310" and value:
                payload_seen = True
        if not ole_seen or not payload_seen:
            raise SystemExit(f"OLE2FRAME or group 310 bytes lost: {path}")
    print(f"PASS: {path} (DXF sections/EOF" +
          (", OLE2FRAME and group 310 payload)" if expect_ole else ")"))


if __name__ == "__main__":
    cli = argparse.ArgumentParser()
    cli.add_argument("file")
    cli.add_argument("--ole", action="store_true")
    args = cli.parse_args()
    check(args.file, args.ole)
