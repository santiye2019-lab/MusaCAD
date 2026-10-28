# MC1032 controlled source migration

This directory is a **quarantine boundary** for the next MC1032 core. It does not currently contain migrated source code and makes no claim that the old code is GPL-free.

## Admission rules

1. Inspect the original source file, authorship, introducing commits, external dependencies, and license grants.
2. Record an evidence-backed decision in `PROVENANCE_LEDGER.csv` before importing code.
3. `OWNED_APPROVED`: copy only when rights to relicense are confirmed for **all** relevant contributors.
4. `THIRD_PARTY_GPL`: do not copy or mechanically paraphrase. Implement independently against public format descriptions and independently prepared tests.
5. `UNKNOWN`: quarantine; do not copy until evidence resolves ownership, or replace independently.
6. Preserve required attribution for any other permitted third-party code. A GPL header must never be removed merely to bypass obligations.
7. Build migrated modules as a distinct portable C11 target, without linking the legacy library. Require real DWG regression tests before cutover.

## Sequence

- File-format signature, bounded byte readers, checksums
- AC1032 sections and handle/object graph
- 2D geometry and BLOCK/INSERT
- Layer/color/visibility and text
- Scene generation, DWG writing, later 3D

## Release criteria

Every included source and linked dependency has verified redistribution rights, the target passes cross-platform tests, and an approved licensing decision is recorded. No release is authorized while any imported component remains `UNKNOWN`.
