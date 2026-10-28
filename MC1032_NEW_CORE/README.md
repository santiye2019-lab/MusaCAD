# MC1032 — NEW CORE

This top-level workspace is the **only destination** for staged, provenance-reviewed transfers from the existing `main` source tree. The old code stays in place as a read-only source; no new development or merges to `main` as part of this migration.

## Transfer gate

1. Identify a source file in the existing `main` tree and record its Git history, authors, rights and dependencies.
2. If independently owned and relicensable, copy the reviewed code here.
3. If third-party GPL, **do not copy it**: independently implement the behavior and test it.
4. If provenance is unresolved, block the transfer until reviewed.
5. Keep the new C11 core build isolated from legacy libraries and GPL dependencies.
6. Track each transfer in `TRANSFER_LEDGER.csv` and validate functionality against real DWG files.

**No source files are approved for transfer yet. No claim of GPL-free status.**
