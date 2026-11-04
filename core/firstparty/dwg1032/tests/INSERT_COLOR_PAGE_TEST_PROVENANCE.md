# INSERT color page regression provenance (2026-11-04)

This test-only note covers `dwg1032_insert_color_page_test.c`, including the
single-record final-page and adjacent-output sentinel checks added in
`1efd366f82b5ee80dbd555657e77808868d75dc7`.

- Origin: independently authored MusaCAD first-party synthetic C11 test inputs.
  No third-party CAD implementation, GPL/AGPL/LGPL source, or undocumented
  decoder code was copied, adapted, or linked.
- Contract: the layer-color function receives already decoded and authenticated
  block definition, source color, and layer arrays. It does **not** read a DWG.
- Positive case: the final object of a 20,000-object definition resolves through
  a one-element page without changing adjacent output sentinels.
- Negative case: a two-object page starting at the final object must be rejected
  without changing any output sentinel.
- Limitations: synthetic color resolution is not evidence of authentic AC1032
  section/object decoding, Android rendering, original DWG fidelity, or 3D support.

Do not promote this regression to a real-file acceptance claim. Run the
`musacad-firstparty-dwg1032.yml` C11 matrix and inspect its actual result
for the exact commit before describing the regression as passing.

## Cross-INSERT failure isolation (30f981a, 2026-11-04)

A late malformed synthetic source color must reject the entire 127-record page
without writing output. After restoring that source record, a **different**
INSERT context must resolve the same page exactly as the full-array reference;
neighboring output sentinels remain unchanged. This checks retry isolation of
post-decode color resolution, not independent DWG parsing or real-file fidelity.
No external CAD code or fixtures were introduced. CI results must be checked
against the exact commit; adding this test alone does not establish a pass.
