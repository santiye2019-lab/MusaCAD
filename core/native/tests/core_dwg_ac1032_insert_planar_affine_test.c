/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Portable numeric affine regression. Synthetic coordinates, NOT a genuine
 * R2018 DWG fixture or evidence of complete block rendering. */
#include "musacad_dwg_ac1032_insert.h"
#include <math.h>
#include <string.h>
int main(void){
    MusaDwgAc1032InsertPlacement p={0};
    const double base[3]={2.0,3.0,0.0};
    double m[6]={123.0,123.0,123.0,123.0,123.0,123.0};
    p.original_record_crc_verified=1u;
    p.numeric_fields_decoded=1u;
    p.extrusion[2]=1.0;p.scale[0]=2.0;p.scale[1]=3.0;p.scale[2]=1.0;
    p.insertion[0]=10.0;p.insertion[1]=20.0;
    if(!musa_dwg_ac1032_insert_planar_affine(&p,base,m))return 1;
    if(fabs(m[0]-2.0)>1e-12||fabs(m[1])>1e-12||
       fabs(m[2]-6.0)>1e-12||fabs(m[3])>1e-12||
       fabs(m[4]-3.0)>1e-12||fabs(m[5]-11.0)>1e-12)return 2;
    const double pi=3.14159265358979323846;
    p.rotation=pi/2.0;
    if(!musa_dwg_ac1032_insert_planar_affine(&p,base,m))return 3;
    if(fabs(m[0])>1e-12||fabs(m[1]+3.0)>1e-12||
       fabs(m[2]-19.0)>1e-12||fabs(m[3]-2.0)>1e-12||
       fabs(m[4])>1e-12||fabs(m[5]-16.0)>1e-12)return 4;
    double saved[6];memcpy(saved,m,sizeof(m));
    p.extrusion[0]=0.1;
    if(musa_dwg_ac1032_insert_planar_affine(&p,base,m)||
       memcmp(saved,m,sizeof(m)))return 5;
    p.extrusion[0]=0.0;p.scale[0]=0.0;
    if(musa_dwg_ac1032_insert_planar_affine(&p,base,m)||
       memcmp(saved,m,sizeof(m)))return 6;
    return 0;
}
