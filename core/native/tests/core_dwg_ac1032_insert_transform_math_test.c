/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "musacad_dwg_ac1032_insert_transform.h"
#include <assert.h>
#include <math.h>
#include <string.h>
static int eq(double a,double b){return fabs(a-b)<1e-9;}
int main(void){
    MusaDwgAc1032InsertPlacement p={0};
    p.numeric_fields_decoded=1;
    p.original_record_crc_verified=1;
    p.insertion[0]=10;p.insertion[1]=20;p.insertion[2]=30;
    p.scale[0]=2;p.scale[1]=3;p.scale[2]=4;
    p.extrusion[2]=1;
    const double base[3]={1,2,3},local[3]={3,4,5};
    double out[3]={-7,-7,-7};
    assert(musa_dwg_ac1032_insert_xy_world_z(&p,base,local,out));
    assert(eq(out[0],14)&&eq(out[1],26)&&eq(out[2],38));
    p.rotation=acos(-1.0)/2.0;
    assert(musa_dwg_ac1032_insert_xy_world_z(&p,base,local,out));
    assert(eq(out[0],4)&&eq(out[1],24)&&eq(out[2],38));
    const double saved[3]={out[0],out[1],out[2]};
    p.extrusion[0]=0.5;
    assert(!musa_dwg_ac1032_insert_xy_world_z(&p,base,local,out));
    assert(!memcmp(out,saved,sizeof(saved)));
    p.extrusion[0]=0;
    p.extrusion[2]=-1;
    assert(!musa_dwg_ac1032_insert_xy_world_z(&p,base,local,out));
    assert(!memcmp(out,saved,sizeof(saved)));
    p.extrusion[2]=1;
    p.rotation=INFINITY;
    assert(!musa_dwg_ac1032_insert_xy_world_z(&p,base,local,out));
    assert(!memcmp(out,saved,sizeof(saved)));
    p.rotation=0;
    p.scale[0]=NAN;
    assert(!musa_dwg_ac1032_insert_xy_world_z(&p,base,local,out));
    assert(!memcmp(out,saved,sizeof(saved)));
    p.scale[0]=2;
    /* Overflow is rejected without modifying the output. */
    p.insertion[0]=1e308;
    p.insertion[1]=1e308;
    p.scale[0]=1e308;
    assert(!musa_dwg_ac1032_insert_xy_world_z(&p,base,local,out));
    assert(!memcmp(out,saved,sizeof(saved)));
    p.insertion[0]=10;p.insertion[1]=20;p.scale[0]=2;
    p.numeric_fields_decoded=0;
    assert(!musa_dwg_ac1032_insert_xy_world_z(&p,base,local,out));
    assert(!memcmp(out,saved,sizeof(saved)));
    return 0;
}
