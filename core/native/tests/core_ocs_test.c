/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "musacad_ocs.h"
#include "musacad_dwg_ac1018_lwpolyline.h"
#include <assert.h>
#include <float.h>
#include <math.h>
#include <string.h>
/* All geometry vectors synthetic. No private DWG source data. */
static int near(double a,double b){return fabs(a-b)<=1e-12;}
static void vector(const double normal[3],const double xy[2],double elev,
                   const double expected[3]) {
    MusaCoreOcsBasis basis={0};
    double world[3]={7,8,9};
    assert(musa_core_ocs_basis(normal,&basis));
    assert(musa_core_ocs_to_world(&basis,xy,elev,world));
    for(int k=0;k<3;++k) assert(near(world[k],expected[k]));
}
int main(void) {
    const double point[2]={2.0,3.0};
    const double front[3]={0,0,1},front_xyz[3]={2,3,4};
    const double side[3]={0,1,0},side_xyz[3]={-2,4,3};
    const double underside[3]={0,0,-1},under_xyz[3]={-2,3,-4};
    vector(front,point,4,front_xyz);
    vector(side,point,4,side_xyz);
    vector(underside,point,4,under_xyz);
    const double scaled[3]={0,3e150,0};
    vector(scaled,point,4,side_xyz);
    const double threshold[3]={0.0,0.01,1.0};
    MusaCoreOcsBasis basis={0};
    assert(musa_core_ocs_basis(threshold,&basis));
    assert(basis.x[0]>0.999 && near(basis.x[1],0.0));
    const double other[3]={0,0.02,1.0};
    assert(musa_core_ocs_basis(other,&basis));
    assert(basis.x[0]<-0.999 && near(basis.x[1],0.0));
    /* The actual R2004 LWPOLYLINE adapter must agree with its core basis. */
    MusaDwgAc1018LWPolyline poly={0};
    poly.extrusion[1]=1.0;
    poly.elevation=4.0;
    MusaDwgAc1018LWVertex vertex={0};
    vertex.x=2.0; vertex.y=3.0;
    double lw_wcs[3]={0};
    assert(musa_dwg_ac1018_lwpolyline_vertex_to_wcs(&poly,&vertex,lw_wcs));
    for(int i=0;i<3;++i) assert(near(lw_wcs[i],side_xyz[i]));
    double preserved[3]={6,7,8};
    vertex.x=NAN;
    assert(!musa_dwg_ac1018_lwpolyline_vertex_to_wcs(&poly,&vertex,preserved));
    assert(preserved[0]==6 && preserved[1]==7 && preserved[2]==8);
    vertex.x=2.0;
    const double zero[3]={0,0,0},bad[3]={NAN,1,0};
    MusaCoreOcsBasis unchanged=basis;
    assert(!musa_core_ocs_basis(zero,&basis));
    assert(!musa_core_ocs_basis(bad,&basis));
    assert(!memcmp(&basis,&unchanged,sizeof(basis)));
    assert(!musa_core_ocs_to_world(&basis,point,NAN,(double[3]){0}));
    double out[3]={5,6,7};
    MusaCoreOcsBasis bogus=basis;
    bogus.x[0]=300.0;
    assert(!musa_core_ocs_to_world(&bogus,point,4,out));
    assert(out[0]==5 && out[1]==6 && out[2]==7);
    const double huge[2]={DBL_MAX,DBL_MAX};
    assert(!musa_core_ocs_to_world(&basis,huge,DBL_MAX,out));
    assert(out[0]==5 && out[1]==6 && out[2]==7);
    return 0;
}
