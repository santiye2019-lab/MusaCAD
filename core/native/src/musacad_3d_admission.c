/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "musacad_3d_admission.h"

MusaCore3dAdmission musa_core_3d_only_admission(
    size_t xyz_float_count,size_t triangle_index_count,
    size_t edge_index_count,int synthetic_solid_proxy) {
    if(xyz_float_count==0)
        return MUSA_CORE_3D_MISSING_VERTICES;
    /* Unsupported ACIS or SAB may emit approximate $EXTMIN/$EXTMAX vertices.
     * Such geometry is NOT accepted, even if fake faces/edges are present.
     */
    if(synthetic_solid_proxy)
        return MUSA_CORE_3D_PROXY_ONLY;
    if(xyz_float_count%3!=0 ||
       triangle_index_count%3!=0 ||
       edge_index_count%2!=0)
        return MUSA_CORE_3D_MALFORMED_ARRAYS;
    if(triangle_index_count==0 && edge_index_count==0)
        return MUSA_CORE_3D_MISSING_FACES_AND_EDGES;
    return MUSA_CORE_3D_ACCEPT;
}
