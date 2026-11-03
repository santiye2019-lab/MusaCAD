# INSERT visibility page: independent code provenance

- Authoring: original first-party C11 implementation for MusaCAD on 2026-11-03.
- Scope: post-decode, caller-authenticated layer-zero visibility for one concrete INSERT instance and one bounded page. No DWG record parsing, decompression, object authentication, rendering, or new runtime integration.
- Inputs: immutable full definition arrays, owner-only visibility, layer on/thawed visibility, and verified enclosing INSERT visibility. Output is a separate caller-owned page buffer.
- Safety: reject empty/overflowing ranges, nonrepresentable C object spans, and any output aliasing with **all** source pages. Existing full-context function preflights every element of the selected page before publishing.
- Limits: other pages are not revalidated by this helper; the caller must validate/authenticate the entire definition and owner graph before paging. Nested INSERT contexts remain distinct. No claim of real AC1032 DWG support or original-file fidelity.
- Origin: written without copying, adapting, linking, or referencing GPL/AGPL/LGPL/LibreDWG or uncertain-origin CAD source code. MIT SPDX headers remain on implementation and interface.
