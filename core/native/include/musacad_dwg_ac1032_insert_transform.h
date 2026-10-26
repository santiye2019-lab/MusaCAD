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

/* Compose a child INSERT's local placement through a verified parent
 * placement. Only world-Z, finite and nonzero scales are supported.
 * This is arithmetic, NOT proof of DWG child membership or BLOCK_HEADER
 * base points. All output fields are committed atomically. */
static inline int musa_dwg_ac1032_insert_compose_world_z(
    const MusaDwgAc1032InsertPlacement *parent,
    const double parent_block_base[3],
    const MusaDwgAc1032InsertPlacement *child,
    MusaDwgAc1032InsertPlacement *out){
    if(!parent||!parent_block_base||!child||!out||
       !child->numeric_fields_decoded||
       !child->original_record_crc_verified||
       !isfinite(child->rotation)||
       fabs(child->extrusion[0])>1e-12||
       fabs(child->extrusion[1])>1e-12||
       fabs(child->extrusion[2]-1.0)>1e-12)return 0;
    MusaDwgAc1032InsertPlacement v=*child;
    if(!musa_dwg_ac1032_insert_xy_world_z(
           parent,parent_block_base,child->insertion,v.insertion))return 0;
    for(unsigned k=0;k<3;k++){
        if(!isfinite(child->scale[k])||child->scale[k]==0.0||
           !isfinite(parent->scale[k]))return 0;
        v.scale[k]=parent->scale[k]*child->scale[k];
        if(!isfinite(v.scale[k])||v.scale[k]==0.0)return 0;
    }
    /* Nonuniform XY parent scale and rotated child introduce shear.
     * Such transforms cannot be represented by this placement struct. */
    if(fabs(parent->scale[0]-parent->scale[1])>1e-12&&
       fabs(sin(child->rotation))>1e-12)return 0;
    v.rotation=parent->rotation+child->rotation;
    if(!isfinite(v.rotation))return 0;
    *out=v;
    return 1;
}

#endif
