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


## Bounded page adapter (2026-11-03)

The independently authored `musa_dwg1032_insert_visibility_page_scratch`
accepts a caller-authenticated full child array but processes one bounded
page at a time. It rejects output or scratch aliasing with the **entire**
source arrays (including unprocessed pages), then delegates atomic
publication to the existing scratch implementation. The synthetic 20,000
child differential test uses a 257-byte scratch buffer, exercises an
invalid late child and checks that earlier published pages stay intact.

This is post-decode evidence only. It does not identify or decode AC1032
objects, discover BLOCK/INSERT ownership, or prove exact DWG fidelity.
The change neither imports third-party code nor modifies the 3D core.
