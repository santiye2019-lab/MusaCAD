# Bounded 2BD/3BD coordinate batches — independent implementation record

Date: 2026-11-03. Scope: `include/musa_dwg1032_point_batch.h`,
`src/musa_dwg1032_point_batch.c`, and
`tests/dwg1032_point_batch_test.c`.

- Authorship: new MusaCAD C11 code written for this branch, based on the
  existing first-party `musa_dwg1032_point2bd` and `point3bd` public
  interfaces and the ordinary C11 library. No GPL, AGPL, LGPL, LibreDWG,
  ODA SDK, external CAD implementation, or unknown-origin source was copied,
  adapted, included, or linked for this increment.
- Contract: the caller must already have authenticated the source section,
  entity schema and number of 2BD/3BD tuples. The routine consumes only
  bounded caller-supplied bytes. It neither discovers a DWG entity nor
  establishes object handles, owners, layers, colors, visibility, text,
  dynamic block behavior or AC1032 document completeness.
- Atomicity: output and cursor are unchanged if any tuple fails, including
  a malformed last tuple. Caller-owned scratch may be partially overwritten.
  Source bytes must be immutable; full source/destination/scratch overlap is
  rejected, and size arithmetic is checked before any write.
- Test evidence: synthetic 20,000-tuple 2BD comparison against individual
  point reads, unaligned bit offsets, 3BD raw/constant mixtures, truncated
  and reserved opcodes, rollback and alias/overflow rejection. These are
  synthetic regressions, **not** a real-file AC1032 conformance claim.
- Platform gate: `firstparty_dwg1032_atomic_coordinate_batches_20k`
  belongs to the standalone CTest target on Linux, Windows and macOS.
  GitHub Actions conclusions must be inspected for the final commit;
  this document does not itself assert any unobserved CI result.

The existing 3D engine, Android normal open path, `main` branch and
`MusaCAD2/` remain outside this increment. No release or APK is produced.
