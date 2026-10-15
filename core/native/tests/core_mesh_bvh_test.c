/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "musacad_core.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
static void same(double a,double b){assert(isfinite(a)&&fabs(a-b)<1e-9);}
int main(void){
    float vertices[]={0,0,0,2,0,0,0,2,0,
                      0,0,-2,2,0,-2,0,2,-2,
                      10,0,1,12,0,1,10,2,1};
    int tris[]={0,1,2,3,4,5,6,7,8};
    MusaCoreMeshBVH *tree=NULL;
    assert(musa_core_mesh_bvh_build_f32(vertices,9,tris,3,&tree)&&tree);
    const double origin[]={.5,.5,5},ray[]={0,0,-7};
    MusaCoreMeshRayHit raw={0},accelerated={0};
    assert(musa_core_mesh_ray_pick_f32(vertices,9,tris,3,
                                      origin,ray,10.,&raw)==1);
    assert(musa_core_mesh_bvh_pick(tree,origin,ray,10.,&accelerated)==1);
    assert(raw.triangle_index==accelerated.triangle_index);
    same(raw.distance,accelerated.distance);
    same(raw.wcs_z,accelerated.wcs_z);
    same(raw.barycentric_u,accelerated.barycentric_u);
    same(raw.barycentric_v,accelerated.barycentric_v);
    /* The index owns all vertices/faces; changing source cannot change it. */
    vertices[0]=1000;tris[0]=9999;
    assert(musa_core_mesh_bvh_pick(tree,origin,ray,10.,&accelerated)==1);
    assert(accelerated.triangle_index==0);
    const double miss[]={50,50,5};
    accelerated.triangle_index=1234;
    assert(musa_core_mesh_bvh_pick(tree,miss,ray,10.,&accelerated)==0);
    assert(accelerated.triangle_index==1234);
    assert(musa_core_mesh_bvh_pick(tree,origin,ray,4.9,&accelerated)==0);
    assert(musa_core_mesh_bvh_pick(tree,origin,ray,5.,&accelerated)==1);
    musa_core_mesh_bvh_destroy(tree);tree=NULL;

    /* More than one BVH leaf, many triangles on an extended site grid.
     * Compare identical brute-force output for deterministic WCS picks.
     */
    float mesh[96*9];int ix[96*3];
    for(int i=0;i<96;i++) {
        for(int a=0;a<3;a++){
            mesh[i*9+a*3]=(float)(i*10+a==i*10+1?i*10+1:i*10);
            mesh[i*9+a*3+1]=(float)(a==2?1.:0.);
            mesh[i*9+a*3+2]=0;
            ix[i*3+a]=i*3+a;
        }
    }
    /* Correct triangle mesh shape with nonzero area. */
    for(int i=0;i<96;i++){
        mesh[i*9+0]=i*10;mesh[i*9+1]=0;
        mesh[i*9+3]=i*10+2;mesh[i*9+4]=0;
        mesh[i*9+6]=i*10;mesh[i*9+7]=2;
    }
    assert(musa_core_mesh_bvh_build_f32(mesh,288,ix,96,&tree));
    const double d[]={0,0,-1};
    for(int i=0;i<96;i+=7){
        double start[]={i*10+.5,.5,8};
        MusaCoreMeshRayHit a={0},b={0};
        assert(musa_core_mesh_ray_pick_f32(mesh,288,ix,96,start,d,10,&a)==1);
        assert(musa_core_mesh_bvh_pick(tree,start,d,10,&b)==1);
        assert(a.triangle_index==(size_t)i&&b.triangle_index==a.triangle_index);
        same(a.distance,b.distance);
    }
    musa_core_mesh_bvh_destroy(tree);tree=NULL;
    const float world[]={1000000,2000000,3000000,
                         1000002,2000000,3000000,
                         1000000,2000002,3000000};
    const int tri[]={0,1,2};
    assert(musa_core_mesh_bvh_build_f32(world,3,tri,1,&tree));
    const double wo[]={1000000.5,2000000.5,3000010};
    assert(musa_core_mesh_bvh_pick(tree,wo,d,10,&accelerated)==1);
    same(accelerated.wcs_z,3000000);
    musa_core_mesh_bvh_destroy(tree);tree=NULL;
    /* Reject invalid geometry before building; never publish index. */
    const int bad[]={0,1,7};
    const float nanmesh[]={0,0,0,1,NAN,0,0,1,0};
    assert(!musa_core_mesh_bvh_build_f32(world,3,bad,1,&tree));
    assert(!musa_core_mesh_bvh_build_f32(nanmesh,3,tri,1,&tree));
    assert(!musa_core_mesh_bvh_build_f32(NULL,3,tri,1,&tree));
    assert(!musa_core_mesh_bvh_build_f32(world,3,tri,250001,&tree));
    assert(tree==NULL);
    assert(musa_core_mesh_bvh_build_f32(NULL,0,NULL,0,&tree));
    assert(musa_core_mesh_bvh_pick(tree,origin,d,10,&accelerated)==0);
    const double zero[]={0,0,0};
    assert(musa_core_mesh_bvh_pick(tree,origin,zero,10,&accelerated)==-1);
    assert(musa_core_mesh_bvh_pick(tree,origin,d,-1,&accelerated)==-1);
    musa_core_mesh_bvh_destroy(tree);
    puts("MusaCAD shared portable BVH indexed 3D ray selection PASS");
    return 0;
}
