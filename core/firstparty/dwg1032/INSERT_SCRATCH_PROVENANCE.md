# INSERT scratch visibility provenance (2026-11-02)

This is independently authored MusaCAD C11 code, added to the existing
first-party AC1032 post-decode evidence layer. No LibreDWG, ODA, GPL, AGPL,
LGPL or unknown-origin source was copied, adapted or linked.

The function consumes **already decoded and authenticated** child layer
handles, owner visibility and viewport layer state. It cannot read compressed
DWG bytes, resolve block ownership, verify actual DWG records, or grant render
permission. It preserves the existing first-party 3D implementation.

The scratch-backed path performs one child computation pass, then publishes
the mask only after every child validates. It is an optional performance
primitive for large blocks, not proof of complete AC1032 support.
