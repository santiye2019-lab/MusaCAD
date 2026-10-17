/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef MUSACAD_3D_ADMISSION_H
#define MUSACAD_3D_ADMISSION_H
#include <stddef.h>
#ifdef __cplusplus
extern "C" {
#endif

/* Canonical source-safe 3D-only drawing admission, shared by ALL platforms.
 * A parsed mesh with only fake ACIS extents is NEVER a real 3D drawing.
 * The input cardinalities represent decoded source geometry, not a bounding
 * box or a claim that ACIS/SAB B-Rep has been decoded.
 * This only validates admission to a 3D mesh viewer, NOT topology/solidity.
 */
typedef enum MusaCore3dAdmission {
    MUSA_CORE_3D_ACCEPT = 0,
    MUSA_CORE_3D_MISSING_VERTICES = 1,
    MUSA_CORE_3D_PROXY_ONLY = 2,
    MUSA_CORE_3D_MISSING_FACES_AND_EDGES = 3,
    MUSA_CORE_3D_MALFORMED_ARRAYS = 4
} MusaCore3dAdmission;

/* Count components (float XYZ values, triangle indices, edge indices).
 * Reject malformed tuples, missing real geometry and fake ACIS proxies.
 * No allocation, mutation, JNI, Android/Win32 or Apple SDK dependency.
 */
MusaCore3dAdmission musa_core_3d_only_admission(
    size_t xyz_float_count,size_t triangle_index_count,
    size_t edge_index_count,int synthetic_solid_proxy);
#ifdef __cplusplus
}
#endif
#endif
