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
/* Safe public API always independently verifies the exact mesh connectivity,
 * winding, duplicate and degenerate faces before the mass integration.
 * Never treats an adapter's prior topology decision as binding. */
int musa_core_mesh_mass_verified_f32(const float *xyz,size_t point_count,
                                     const int *triangles,size_t face_count,
                                     MusaCoreMeshMass *out);
/*
 * Triangle-mesh SURFACE metrics (not signed enclosed volume).
 * Reads float XYZ vertices + indexed triangles; returns total area and
 * area-weighted centroid in WCS drawing units, plus degenerate face count.
 * Open shells are supported. An empty/zero-area set has NaN centroids.
 * Invalid indices, nonfinite vertices or overflow return 0 and leave *out
 * intact. No conclusion is made about solids, watertight B-Rep or metraj.
 */
typedef struct MusaCoreMeshSurface {
    double area;
    double centroid_x,centroid_y,centroid_z;
    size_t triangle_count,degenerate_count;
} MusaCoreMeshSurface;
int musa_core_mesh_surface_f32(const float *xyz,size_t point_count,
                              const int *triangles,size_t face_count,
                              MusaCoreMeshSurface *out);
/* Portable result-integrity contract. Zero-area mesh has unavailable NaN
 * centroids; positive-area mesh has finite WCS centroids. This is NOT
 * geometry/B-Rep validity certification. Never silently publish invalid data. */
int musa_core_mesh_surface_result_valid(const MusaCoreMeshSurface *result,
                                        size_t expected_faces);
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
/*
 * Read-only nearest triangle hit test for CAD 3D mesh selection.
 * Ray direction need not be normalized; distance is always measured in
 * drawing WCS units. front_facing means the ray meets outward winding.
 *  1: nearest hit and *out is written;
 *  0: no hit, *out unchanged;
 * -1: invalid/nonfinite input, resource guard or arithmetic overflow.
 *
 * This does not establish B-Rep, hidden-line picking, occlusion by other
 * scene objects or certified 3D object selection. Triangle vertices must be
 * the decoded mesh, and the caller retains ownership of all arrays.
 */
typedef struct MusaCoreMeshRayHit {
    size_t triangle_index;
    double distance;
    double barycentric_u,barycentric_v;
    double wcs_x,wcs_y,wcs_z;
    int front_facing;
} MusaCoreMeshRayHit;
int musa_core_mesh_ray_pick_f32(const float *xyz,size_t point_count,
                               const int *triangles,size_t face_count,
                               const double origin[3],
                               const double direction[3],
                               double max_distance,
                               MusaCoreMeshRayHit *out);
/*
 * Reusable read-only 3D triangle BVH for large CAD drawing mesh selection.
 * Copies vertex and face data at construction; caller may release input
 * immediately. Subsequent picks are thread-safe when the BVH is immutable.
 * This does not construct/edit ACIS B-Rep or apply scene-layer visibility.
 *
 * Build returns 1 and writes a new owned handle, or 0 on malformed input,
 * resource limit or allocation failure without modifying *out.
 * Query returns 1 for a real closest face, 0 for no hit, -1 for invalid
 * ray input. Outputs are never modified on a miss/failure.
 */
typedef struct MusaCoreMeshBVH MusaCoreMeshBVH;
int musa_core_mesh_bvh_build_f32(const float *xyz,size_t point_count,
                                const int *triangles,size_t face_count,
                                MusaCoreMeshBVH **out);
typedef struct MusaCoreMeshBVHStats {
    size_t tested_faces;
    size_t visited_nodes;
    size_t rejected_nodes;
} MusaCoreMeshBVHStats;
int musa_core_mesh_bvh_pick_stats(const MusaCoreMeshBVH *tree,
                                 const double ray_origin[3],
                                 const double ray_direction[3],
                                 double max_distance,MusaCoreMeshRayHit *out,
                                 MusaCoreMeshBVHStats *stats);
int musa_core_mesh_bvh_pick(const MusaCoreMeshBVH *tree,
                           const double ray_origin[3],
                           const double ray_direction[3],
                           double max_distance,MusaCoreMeshRayHit *out);
/*
 * Object-local BVH root AABB. Returns 1 for a nonempty indexed mesh,
 * 0 for an empty BVH and -1 for null pointers. Outputs are unchanged
 * on empty/invalid. Immutable BVH supports concurrent read queries.
 */
int musa_core_mesh_bvh_bounds(const MusaCoreMeshBVH *tree,
                             double out_minimum[3],double out_maximum[3]);
void musa_core_mesh_bvh_destroy(MusaCoreMeshBVH *tree);
#ifdef __cplusplus
}
#endif
#endif
