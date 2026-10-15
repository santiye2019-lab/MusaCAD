/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "musacad_core.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <limits.h>
static void near(double actual,double expected,double tol){
    assert(isfinite(actual));
    assert(fabs(actual-expected)<=tol);
}
int main(void){
    const float tetra[]={0,0,0,1,0,0,0,1,0,0,0,1};
    const int outward[]={0,2,1,0,1,3,0,3,2,1,2,3};
    const int inward[]={0,1,2,0,3,1,0,2,3,1,3,2};
    MusaCoreMeshMass r={42,43,44,45,46};
    assert(musa_core_mesh_mass_f32(tetra,4,outward,4,1,&r));
    near(r.signed_volume,1./6.,1e-12);
    near(r.centroid_x,.25,1e-12);
    near(r.centroid_y,.25,1e-12);
    near(r.centroid_z,.25,1e-12);
    assert(r.triangle_count==4);
    assert(musa_core_mesh_mass_f32(tetra,4,inward,4,1,&r));
    near(r.signed_volume,-1./6.,1e-12);
    near(r.centroid_x,.25,1e-12);
    /* WCS CAD drawings can be millions of drawing units from origin. */
    float world[12];
    for(int i=0;i<12;i++)world[i]=tetra[i]+1000000.f;
    assert(musa_core_mesh_mass_f32(world,4,outward,4,1,&r));
    near(r.signed_volume,1./6.,1e-12);
    near(r.centroid_x,1000000.25,1e-9);
    near(r.centroid_y,1000000.25,1e-9);
    near(r.centroid_z,1000000.25,1e-9);
    const float cube[]={0,0,0,1,0,0,1,1,0,0,1,0,
                         0,0,1,1,0,1,1,1,1,0,1,1};
    const int box[]={0,1,2,0,2,3,4,6,5,4,7,6,
                     0,4,5,0,5,1,1,5,6,1,6,2,
                     2,6,7,2,7,3,3,7,4,3,4,0};
    assert(musa_core_mesh_mass_f32(cube,8,box,12,1,&r));
    near(fabs(r.signed_volume),1.,1e-12);
    near(r.centroid_x,.5,1e-12);
    near(r.centroid_y,.5,1e-12);
    near(r.centroid_z,.5,1e-12);
    /* Fail closed: no topology preflight, open surface, bad indices,
       non-finite values and oversized uninspected geometry. */
    const int open[]={0,2,1};
    const int bad[]={0,2,9,0,1,3,0,3,2,1,2,3};
    float nanmesh[12]={0,0,0,1,0,0,0,NAN,0,0,0,1};
    r.signed_volume=12345.;r.centroid_x=6789.;r.triangle_count=5;
    assert(!musa_core_mesh_mass_f32(tetra,4,outward,4,0,&r));
    assert(!musa_core_mesh_mass_f32(tetra,4,open,1,1,&r));
    assert(!musa_core_mesh_mass_f32(tetra,4,bad,4,1,&r));
    assert(!musa_core_mesh_mass_f32(nanmesh,4,outward,4,1,&r));
    assert(!musa_core_mesh_mass_f32(tetra,4,outward,4000001u,1,&r));
    assert(!musa_core_mesh_mass_f32(tetra,4,outward,4,1,NULL));
    assert(!musa_core_mesh_mass_f32(NULL,4,outward,4,1,&r));
    near(r.signed_volume,12345.,0);
    near(r.centroid_x,6789.,0);
    assert(r.triangle_count==5);
    puts("MusaCAD portable indexed 3D mesh mass, signed volume, centroid PASS");
    return 0;
}
