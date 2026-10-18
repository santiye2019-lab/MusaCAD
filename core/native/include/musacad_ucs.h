/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef MUSACAD_UCS_H
#define MUSACAD_UCS_H
/* One portable right-handed UCS engine for Android, Windows and iOS.
 * Input: origin XYZ, X-axis XYZ, Y-axis XYZ (nine doubles).
 * Output unchanged on failure. No platform APIs, Android algorithms or CAD SDK.
 */
#ifdef __cplusplus
extern "C" {
#endif
typedef struct MusaCoreUcsFrame {
    double origin[3], axis[3][3]; /* X,Y,Z unit axes in WCS */
} MusaCoreUcsFrame;
int musa_core_ucs_create(const double origin_x_y_z_xaxis_yaxis[9],
                         MusaCoreUcsFrame *out);
int musa_core_ucs_to_world(const MusaCoreUcsFrame *ucs,
                           const double local[3],double world[3]);
int musa_core_ucs_to_local(const MusaCoreUcsFrame *ucs,
                           const double world[3],double local[3]);
int musa_core_ucs_move(const MusaCoreUcsFrame *ucs,int axis,
                       double distance,double out_displacement[3]);
#ifdef __cplusplus
}
#endif
#endif
