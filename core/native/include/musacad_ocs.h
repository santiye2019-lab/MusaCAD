/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef MUSACAD_OCS_H
#define MUSACAD_OCS_H
#ifdef __cplusplus
extern "C" {
#endif
/* Portable DWG Object Coordinate System basis computed with the published
 * arbitrary-axis algorithm: right-handed unit vectors, without treating
 * entity OCS as WCS or applying a viewer UCS in its place.
 * Extrusion must be finite and nonzero; normalize before use.
 * Both functions are deterministic, allocation-free, atomic on failure,
 * and never modify the caller's data. */
typedef struct MusaCoreOcsBasis {
    double x[3];
    double y[3];
    double z[3]; /* unit extrusion */
} MusaCoreOcsBasis;
int musa_core_ocs_basis(const double extrusion[3], MusaCoreOcsBasis *out);
/* Maps OCS XY + elevation (OCS Z) into WCS XYZ. Validate the supplied
 * basis before exposing geometry (including right-handed orientation). */
int musa_core_ocs_to_world(const MusaCoreOcsBasis *basis,
                           const double xy[2], double elevation,
                           double out_xyz[3]);
#ifdef __cplusplus
}
#endif
#endif
