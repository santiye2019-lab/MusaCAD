# MusaCAD — R2013+ ACIS / SAB integrity

## Purpose

Keep the production DWG viewer stable. Report the difference between:
1. ACIS object records counted in the original DWG.
2. SAT/SAB body payload retained in converted DXF.
3. Faces actually tessellated from verified B-Rep topology.
4. Source faces and bodies that are missing or only partly rendered.

Never infer missing solids from visible geometry; never invent edges or faces to make counts appear complete. Editable or measurable geometric accuracy cannot be inferred from partial meshes.

## Verified public regression

Pinned upstream drawing: `LibreDWG/libredwg/test/test-data/example_2013.dwg`.

- Original DWG: 3 ACIS bodies, all flagged for external AcDs payload.
- Converted DXF: 3 ACIS records with data.
- SAT reader: 10 face records, 1 tessellated face (2 triangles), 9 unsupported faces.
- Source/reference check: 18 unresolved SAT node references.
- Exact partial token `strai ght-curve` is repaired only when both tokens match. Other broken links remain unresolved.
- `AcisSatMesh.Result.complete` is false if *any* SAT pointer is unresolved; nonempty triangle arrays alone do not establish full fidelity.
- Java 3D viewer distinguishes visible, partial and absent source solids, and reports unresolved references.
- The older public Cone.dwg test still yields a partial 34-triangle mesh.
- An isolated `-DCAN_ACIS_HISTORY` build produced the **same** 3 bodies / 10 faces / 1 tessellated face / 18 unresolved references. This experiment did not help and was removed from routine CI.

See the project workflow: `.github/workflows/cad-host-regression.yml`.

## Upstream limitation

LibreDWG issue [#1411](https://github.com/LibreDWG/libredwg/issues/1411) documents cases where R2013+ AcDs/ACDSDATA SAB geometry cannot be read or round-tripped. A successful DXF export does not prove 3D fidelity. The pinned source has partial salvage, but this does not guarantee other DWG files have any usable SAB content.

## Next engineering work

- Compare raw public SAB record count, decoded SAT record count and parsed node count before attempting any index re-assignment.
- If SAB content is absent, investigate a dedicated AcDs importer or licensed, reliable alternate conversion path; never silently replace B-Rep geometry with the header extents.
- Add independent gold-standard face/edge checks and large-drawing phone rendering benchmarks before APK promotion.
- Keep Android 2D CAD rendering, original colors, zoom/pan and currently working AI behavior unchanged until regression tests and real-phone smoke tests pass.

No production deployment or APK promotion is authorized by these diagnostics.
