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
- Without the R2013 source-history fix or planar-hole support: 10 face records, 1 tessellated face (2 triangles), 9 unsupported faces. With both development fixes enabled: 10 triangulated planar faces (28 triangles), but 26 unresolved SAT pointers persist.
- Source/reference check: 18 unresolved SAT node references.
- A public-fixture SAT record parity audit confirmed record counts equal parsed nodes for all three bodies (27/27, 27/27, 126/126). No SAT records were silently dropped by the Java parser. Missing referenced node ids predate its node map.
- Exact partial token `strai ght-curve` is repaired only when both tokens match. Other broken links remain unresolved.
- `AcisSatMesh.Result.complete` is false if *any* SAT pointer is unresolved; nonempty triangle arrays alone do not establish full fidelity.
- Java 3D viewer distinguishes visible, partial and absent source solids, and reports unresolved references.
- The older public Cone.dwg test still yields a partial 34-triangle mesh.
- An isolated `-DCAN_ACIS_HISTORY` build produced the **same** 3 bodies / 10 faces / 1 tessellated face / 18 unresolved references. This experiment did not help and was removed from routine CI.

See the project workflow: `.github/workflows/cad-host-regression.yml`.

## Original SAB versus converted SAT: confirmed provenance

The host-only analyzer `tests/native_acis_sab_audit.c` reads the original *public*
SAB byte stream before calling the DXF writer. It never logs any user drawing
contents or writes back to source DWGs.

| Original ACIS body | Original SAB record terminators | Default SAT nodes | Experimentally retained SAT nodes |
|---|---:|---:|---:|
| 1 | 27 | 27 | 27 |
| 2 | 32 | 27 | 32 |
| 3 | 134 | 126 | 134 |

The original binary has **193** records, while default DXF export keeps **180**.
The SAT Java parser preserves **all** records actually written. This identifies
missing conversion records upstream of the Java mesh parser.

An isolated R2013 host-only experiment changed the LibreDWG branch that clears
the SAB `has_history` flag when the DXF writer downconverts a R2013+ DWG.
The public example initially yielded **3/3 mesh-bearing solids and 8/10
triangulated faces (16 triangles)** instead of **1/3 and 1/10
(2 triangles)**. Further inspection identified the other two surfaces as
nested triangular planar rings with real central openings. A verified
non-overlapping, hole-preserving triangulator brings the same pinned R2013
fixture to **10/10 tessellated source faces, 28 triangles and 26 original
boundary edges**. Regression tests also forbid filling the two openings, or
connecting disjoint or noncoplanar contours.

**26 SAT references are still unresolved after history preservation**. In
particular, the two affected R2013 bodies continue to report incomplete;
visible surfaces do NOT certify fully intact B-Rep topology, precise
solid-volume measurements or DWG round-trip fidelity. The new tessellation
is NOT a production-approved complete solid converter.

After an isolated host A/B test, the exact version-gated workaround
was added to the **separate MusaCAD development branch** as a CMake switch
(`MUSACAD_PRESERVE_R2013_SAB_HISTORY`). It is enabled for development
native compilation and now passes host CAD regression; no production merge,
APK release or real Android-device ACIS fidelity test has occurred.

Cross-version risk: `example_2018.dwg` has 3 original external-AcDs ACIS
entity records, but the pinned LibreDWG reader exposes **no binary SAB
payload** for any of them. History preservation cannot repair missing raw data.
The older public R2000 `Cone.dwg` remains partially tessellated (34 triangles)
in the A/B test.

## Upstream limitation

LibreDWG issue [#1411](https://github.com/LibreDWG/libredwg/issues/1411) documents cases where R2013+ AcDs/ACDSDATA SAB geometry cannot be read or round-tripped. A successful DXF export does not prove 3D fidelity. The pinned source has partial salvage, but this does not guarantee other DWG files have any usable SAB content.

## Next engineering work

- Compare source SAB record count versus converted SAT record count and missing node IDs; converted SAT record count versus Java node count is already verified equal. Do not reassign indices by guessing.
- If SAB content is absent, investigate a dedicated AcDs importer or licensed, reliable alternate conversion path; never silently replace B-Rep geometry with the header extents.
- Add independent gold-standard face/edge checks and large-drawing phone rendering benchmarks before APK promotion.
- Keep Android 2D CAD rendering, original colors, zoom/pan and currently working AI behavior unchanged until regression tests and real-phone smoke tests pass.

The native history workaround is applied at configure time to the pinned
third-party LibreDWG source checkout. For an unpatched build, use a **fresh**
LibreDWG checkout with `-DMUSACAD_PRESERVE_R2013_SAB_HISTORY=OFF`; toggling
OFF in a previously patched checkout does not by itself revert that checkout.

No production deployment or APK promotion is authorized by these diagnostics.
