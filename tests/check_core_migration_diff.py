"""PR source gate: engine-semantic Java changes must arrive with shared core code & tests.

The manifest's "needs_core_audit" list is conservative: it must never be
mistaken for a certified migration. Android UI/FFI adapters are excluded.
"""
import json
import os
from pathlib import Path
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[1]
MANIFEST = ROOT / "docs/UNIFIED_CAD_ENGINE_INVENTORY.json"


def violations(changed, inventory):
    changed = set(changed)
    indexed = {
        row["path"] for row in inventory["java_modules"]
        if row["classification"] == "needs_core_audit"
    }
    engine = sorted(changed & indexed)
    # Genuine platform-neutral implementation and direct regression proof.
    common_impl = sorted(p for p in changed
                         if p.startswith("core/native/src/") and p.endswith(".c")
                         or p.startswith("core/native/include/") and p.endswith(".h"))
    common_test = sorted(p for p in changed
                         if p.startswith("core/native/tests/") and p.endswith(".c"))
    failures = []
    if engine and not common_impl:
        failures.append(
            "Java CAD engine changed without a shared-core C implementation: "
            + ", ".join(engine)
        )
    if engine and not common_test:
        failures.append(
            "Java CAD engine changed without a shared native C regression fixture: "
            + ", ".join(engine)
        )
    return failures


def main():
    if os.environ.get("GITHUB_EVENT_NAME") != "pull_request":
        print("PR-only migration diff gate skipped for manual run.")
        return 0
    base = os.environ.get("GITHUB_BASE_REF", "")
    if base != "main":
        print("Unexpected PR base; refuse unreviewed shared-core migration:", base)
        return 1
    # Actions pull_request checkout is a synthetic merge ref. Diffing directly
    # against the checked-out base is more reliable than a shallow three-dot merge base.
    subprocess.run(
        ["git", "fetch", "--no-tags", "--depth=1", "origin",
         "+refs/heads/main:refs/remotes/origin/main"], cwd=ROOT, check=True
    )
    changed = subprocess.check_output(
        ["git", "diff", "--name-only", "--diff-filter=ACMR",
         "refs/remotes/origin/main", "HEAD"], cwd=ROOT, text=True
    ).splitlines()
    inventory = json.loads(MANIFEST.read_text(encoding="utf-8"))
    errors = violations(changed, inventory)
    if errors:
        for error in errors:
            print("BLOCK:", error, file=sys.stderr)
        print("Port affected CAD semantics to common C ABI, add native fixture, "
              "then rerun complete PR gates.", file=sys.stderr)
        return 1
    print("PASS: No Java-only CAD engine changes bypassing shared C source and tests.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
