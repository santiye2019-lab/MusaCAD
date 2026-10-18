# DWG candidate file-region validation (P0)

First-party portable C module: `musacad_dwg_regions.h/.c`.

A future independently implemented DWG version decoder can pass its candidate
sections/pages to `musa_dwg_regions_validate()`, then obtain bounded,
**read-only** source views with `musa_dwg_region_readonly_span()`. The module
rejects integer overflow, crossing or duplicate ranges, empty regions,
truncated offsets, excessive entries and allocation-free immutable source
spans. Tests run on Linux/Windows/macOS and Android NDK/iPhoneOS builds through
the same `musacad_core` C target.

This is **not** a real DWG section directory parser. No AC1032 page layout,
checksum, decompression, file structure, entity semantics, handles, B-Rep
or real DWG output is inferred. Region offsets must be extracted later from
authentic DWG format-specific records and validated independently. LibreDWG
may be used as a host-only behavioral reference, never linked into MusaCAD.

The `MC1032` proprietary MusaCAD `.mcad` file and Autodesk `AC1032` DWG
are distinct formats. No percent of AutoCAD equivalence changes here.
