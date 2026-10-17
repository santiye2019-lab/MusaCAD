/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "musacad_3d_admission.h"
#include <assert.h>
#include <stdio.h>

static void check(size_t xyz,size_t faces,size_t edges,int proxy,
                  MusaCore3dAdmission expected) {
    assert(musa_core_3d_only_admission(xyz,faces,edges,proxy)==expected);
}
int main(void) {
    check(9,3,0,0,MUSA_CORE_3D_ACCEPT);     /* 3DFACE-only DXF */
    check(6,0,2,0,MUSA_CORE_3D_ACCEPT);     /* LINE-only 3D mesh */
    check(9,3,0,1,MUSA_CORE_3D_PROXY_ONLY); /* Fake ACIS bounds never accepted */
    check(0,0,0,0,MUSA_CORE_3D_MISSING_VERTICES);
    check(9,0,0,0,MUSA_CORE_3D_MISSING_FACES_AND_EDGES);
    check(8,3,0,0,MUSA_CORE_3D_MALFORMED_ARRAYS);
    check(9,4,0,0,MUSA_CORE_3D_MALFORMED_ARRAYS);
    check(9,0,3,0,MUSA_CORE_3D_MALFORMED_ARRAYS);
    check(0,0,0,1,MUSA_CORE_3D_MISSING_VERTICES);
    puts("PASS: canonical cross-platform 3D-only file admission");
    return 0;
}
