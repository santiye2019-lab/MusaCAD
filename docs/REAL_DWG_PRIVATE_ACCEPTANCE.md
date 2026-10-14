# MusaCAD — confidential real-DWG read-only acceptance

This is a **separate reader test**, not the R2000/AC1015 writer pilot.
Never upload private drawings to GitHub, Cloudflare, public issues or Actions.
The originals stay unchanged.

## What is measured

The script tests DWG version signatures and SHA-256 before and after reading.
When a native host converter executable is supplied, it also measures DWG
decode and DXF export, collects model entity types/counts, layers, block
definitions, insert references and XREF flags from the resulting ASCII DXF.
It imposes a 180-second process timeout and optional Linux virtual-memory
limit (default 1536 MiB). Temporary converted drawings are deleted.

Run locally outside the repository, with pinned LibreDWG source available:

    cmake -S app/src/main/cpp -B build-host -DCMAKE_BUILD_TYPE=Release
    cmake --build build-host --parallel 2
    python3 tests/private_dwg_read_audit.py \
      --converter build-host/musacad_dwg_convert \
      --report /path/outside/repo/private-audit.json \
      /private/drawings/project1.dwg /private/drawings/project2.dwg

If the native converter is not available, run with --manifest-only and an
explicit --report path instead. The result will say
metadata_only_not_geometry_checked. Such a result must NEVER be described
as successful geometry, layer, block or render verification.

A decoded_dxf_inventory result confirms a conversion and inventory only:
it does not prove lossless geometry, true color, layer visibility, blocks,
XREF resolution, layouts, dimensions, measurements, exact solids or printing.

For a complete acceptance, export independent baselines from AutoCAD or a
second trusted CAD reader. Compare drawing entity counts, layer/state/color,
blocks/attributes, layout geometry, annotations, coordinates and XREF files.
Physical Android tests must also demonstrate correct initial colors, smooth
zoom and pan, no freezes and acceptable opening times for large drawings.

## GitHub checks

CI runs on upstream redistributable LibreDWG example_2013.dwg and
2000/TS1.dwg fixtures only. Actual customer or municipal files are never
included, uploaded or stored as artifacts.

The user's private DWGs have been verified only for header version, size
and unchanged SHA-256 in the working environment. Native geometry read for
them remains NOT VERIFIED while a runnable host decoder is unavailable.
Production APK, main and DWG Save As stay untouched.
