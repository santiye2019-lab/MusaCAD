/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef MUSACAD_DWG_AC1032_INSERT_TRANSFORM_H
#define MUSACAD_DWG_AC1032_INSERT_TRANSFORM_H
#include "musacad_dwg_ac1032_insert.h"
#include <math.h>
/* Verified numeric-only INSERT local XY -> WCS for world-Z extrusion.
 * Reject tilted OCS, non-finite coordinates and missing block base points;
 * no BLOCK membership, layer inheritance, nested recursion or paint claim. */
static inline int musa_dwg_ac1032_insert_xy_world_z(
    const MusaDwgAc1032InsertPlacement *p,
    const double block_base[3],const double local[3],double world[3]){
    if(!p||!block_base||!local||!world||!p->numeric_fields_decoded||
       !p->original_record_crc_verified)return 0;
    for(unsigned k=0;k<3;k++)
        if(!isfinite(block_base[k])||!isfinite(local[k])||
           !isfinite(p->insertion[k])||!isfinite(p->scale[k])||
           p->scale[k]==0.0||!isfinite(p->extrusion[k]))return 0;
    if(!isfinite(p->rotation)||fabs(p->extrusion[0])>1e-12||
       fabs(p->extrusion[1])>1e-12||
       fabs(p->extrusion[2]-1.0)>1e-12)return 0;
    const double x=(local[0]-block_base[0])*p->scale[0];
    const double y=(local[1]-block_base[1])*p->scale[1];
    const double z=(local[2]-block_base[2])*p->scale[2];
    const double c=cos(p->rotation),s=sin(p->rotation);
    const double a[3]={p->insertion[0]+c*x-s*y,
                       p->insertion[1]+s*x+c*y,
                       p->insertion[2]+z};
    for(unsigned k=0;k<3;k++)if(!isfinite(a[k]))return 0;
    for(unsigned k=0;k<3;k++)world[k]=a[k];
    return 1;
}
#endif
