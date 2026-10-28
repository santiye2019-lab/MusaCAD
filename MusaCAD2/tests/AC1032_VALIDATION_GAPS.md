# AC1032 validation gaps

The current bit-coded scalar, handle and object-type unit tests use independently constructed byte fixtures. They do not demonstrate parsing a complete AC1032 DWG file.

Before declaring AC1032 reading supported, add legally sourced, redistributable real-world fixtures and verify: file header and section-directory integrity; page decompression and checksums; object-map addressing; object record boundaries; handle ownership and references; entity geometry; layer visibility and true-color inheritance; nested block transformations; malformed/truncated input handling. Keep original file provenance and fixture licensing recorded.

Do not import, link, or adapt GPL/AGPL/LGPL or uncertain-provenance implementation code. Keep all changes within MusaCAD2 and require CI evidence for each claim.
