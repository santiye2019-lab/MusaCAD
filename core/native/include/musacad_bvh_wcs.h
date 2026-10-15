/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef MUSACAD_CORE_BVH_WCS_H
#define MUSACAD_CORE_BVH_WCS_H

#include "musacad_core.h"
#include "musacad_affine3.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Pick an immutable object-local BVH as an instance transformed into WCS.
 * Reuses ONE indexed mesh for many nested DWG BLOCK/INSERT transforms:
 * world = object_to_wcs * local.
 *
 * WCS ray direction may be any nonzero finite vector. max_world_distance
 * and output distance use WCS drawing units regardless of nonuniform scale,
 * mirror/reflection, rotation or shear. Result hit XYZ is WCS; face index
 * and barycentric coordinates remain object-local mesh identifiers. An odd
 * reflection flips front_facing.
 *
 * Return 1=hit, 0=miss, -1=invalid/noninvertible transform, overflow or
 * invalid ray. *out (and *stats when supplied) stay untouched on -1/miss
 * except *stats records valid miss traversal counts.
 * A return of 1 does NOT certify ACIS/B-Rep solids or DWG edit parity.
 */
int musa_core_mesh_bvh_pick_wcs(
    const MusaCoreMeshBVH *mesh,
    const MusaCoreAffine3 *object_to_wcs,
    const double origin_wcs[3],
    const double direction_wcs[3],
    double max_world_distance,
    MusaCoreMeshRayHit *out,
    MusaCoreMeshBVHStats *stats);

#ifdef __cplusplus
}
#endif
#endif
