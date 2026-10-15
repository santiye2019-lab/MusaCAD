/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef MUSACAD_CORE_H
#define MUSACAD_CORE_H
/*
 * Stable C-facing API for reusable MusaCAD geometry. No JNI, Android SDK,
 * Win32 or Apple Foundation headers may appear in the core ABI.
 * ABI 1: append functions rather than changing existing struct layouts.
 */
#include <stddef.h>
#ifdef __cplusplus
extern "C" {
#endif
#define MUSA_CORE_ABI_VERSION 1u
#define MUSA_CORE_NURBS_MAX_DEGREE 16
#define MUSA_CORE_NURBS_MAX_CONTROL 4096
#define MUSA_CORE_NURBS_SAMPLES_PER_KNOT_SPAN 12
typedef struct MusaCoreNurbsControl {
    double x, y, w;
} MusaCoreNurbsControl;

/* Returns the ABI contract version so platform adapters can fail closed. */
unsigned int musa_core_abi_version(void);

/* LibreDWG may serialize implicit weights as zero: they mean 1, not 0. */
double musa_core_effective_weight(double stored_weight,int has_explicit_weights);

/* Return 1 on success; 0 on invalid, disconnected or out-of-bounds geometry. */
int musa_core_nurbs_validate(const MusaCoreNurbsControl *controls,size_t count,
                            const double *knots,size_t knot_count,int degree);
int musa_core_nurbs_evaluate(const MusaCoreNurbsControl *controls,size_t count,
                            const double *knots,size_t knot_count,int degree,double parameter,
                            double *out_x,double *out_y);
/* High-throughput evaluation for an array ALREADY validated using
 * musa_core_nurbs_validate(). The source buffer MUST stay immutable
 * during the subsequent calls. This avoids quadratic revalidation cost in
 * DWG first-paint loops; untrusted callers should use the checked method.
 */
int musa_core_nurbs_evaluate_validated(
                            const MusaCoreNurbsControl *controls,size_t count,
                            const double *knots,int degree,double parameter,
                            double *out_x,double *out_y);
int musa_core_nurbs_preview_points(const double *knots,size_t control_count,
                                  size_t knot_count,
                                  int degree);
/*
 * Signed volume/centroid of a PRE-VALIDATED closed oriented indexed triangle
 * shell in native drawing units. Mirrors Android Cad3dSignedVolume's robust,
 * origin-shifted compensated tetrahedron integrals.
 * This function does NOT validate mesh topology or prove a CAD 3DSOLID:
 * the caller must have separately checked watertight incidence, winding,
 * degenerate and duplicate faces before setting topology_prevalidated = 1.
 * Return 1 for nonzero volume, 0 on invalid/unsupported inputs without
 * changing *out; no invented centroid for an open/zero-volume shell.
 */
typedef struct MusaCoreMeshMass {
    double signed_volume;
    double centroid_x,centroid_y,centroid_z;
    size_t triangle_count;
} MusaCoreMeshMass;
int musa_core_mesh_mass_f32(const float *xyz,size_t point_count,
                            const int *triangles,size_t face_count,
                            int topology_prevalidated,
                            MusaCoreMeshMass *out);
/*
 * Independently audit indexed 3D triangle connectivity before signed-mass
 * calculations. Untrusted meshes are NEVER certified solely by edge count.
 * Mirrors Android Cad3dMeshTopology: index-shared edges, winding, duplicate
 * faces and zero-area facets. This is not an ACIS/B-Rep certification:
 * self-intersection and coincident disconnected surfaces require further
 * geometric tests. Geometry exceeding 50,000 triangles returns checked=0.
 *
 * Return 1 with an audit result; 0 for invalid arguments/indices/NaN or
 * allocation failure. *out remains unchanged on failure.
 */
typedef struct MusaCoreMeshTopology {
    int checked,closed_oriented_by_index;
    size_t triangles,boundary_edges,orientation_conflicts;
    size_t non_manifold_edges,degenerate_triangles,duplicate_faces;
} MusaCoreMeshTopology;
int musa_core_mesh_topology_f32(const float *xyz,size_t point_count,
                               const int *triangles,size_t face_count,
                               MusaCoreMeshTopology *out);
#ifdef __cplusplus
}
#endif
#endif
