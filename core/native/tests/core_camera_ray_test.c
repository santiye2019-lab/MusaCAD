/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "musacad_camera_ray.h"
#include "musacad_bvh_scene.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
static void near(double a,double b,double tol){
    assert(isfinite(a)&&fabs(a-b)<=tol);
}
static void xyz(const double p[3],double x,double y,double z,double t){
    near(p[0],x,t);near(p[1],y,t);near(p[2],z,t);
}
int main(void){
    const double eye4[16]={1,0,0,0, 0,1,0,0,
                            0,0,1,0, 0,0,0,1};
    double o[3]={55,66,77},d[3]={88,99,111};
    /* Orthographic center + top-left with OpenGL -1..1 near plane. */
    assert(musa_core_camera_screen_ray(eye4,200,100,100,50,0,o,d));
    xyz(o,0,0,-1,0.);xyz(d,0,0,1,0.);
    assert(musa_core_camera_screen_ray(eye4,200,100,0,0,0,o,d));
    xyz(o,-1,1,-1,0.);xyz(d,0,0,1,0.);
    /* D3D/Metal 0..1 near depth; pixel offsets and view range respected. */
    assert(musa_core_camera_screen_ray(eye4,200,100,100,50,1,o,d));
    xyz(o,0,0,0,0.);xyz(d,0,0,1,0.);
    /* Infinite-screen margins are rejected, not extrapolated. */
    o[0]=55.;d[1]=99.;
    assert(!musa_core_camera_screen_ray(eye4,200,100,201,50,0,o,d));
    assert(!musa_core_camera_screen_ray(eye4,200,100,100,-1,0,o,d));
    assert(!musa_core_camera_screen_ray(eye4,0,100,100,50,0,o,d));
    assert(!musa_core_camera_screen_ray(eye4,200,100,100,50,2,o,d));
    assert(o[0]==55.&&d[1]==99.);

    /* True GL perspective, fovy 90, near 1, far 10, aspect 1.
     * Upper-left column-major representation, camera looks down -Z.
     */
    const double perspective[16]={
        1,0,0,0, 0,1,0,0,
        0,0,-11./9.,-1.,
        0,0,-20./9.,0
    };
    assert(musa_core_camera_screen_ray(perspective,200,200,
                                       100,100,0,o,d));
    xyz(o,0,0,-1,1e-12);xyz(d,0,0,-1,1e-12);
    assert(musa_core_camera_screen_ray(perspective,200,200,
                                       150,100,0,o,d));
    xyz(o,.5,0,-1,1e-12);
    near(d[0],1./sqrt(5.),1e-12);
    near(d[1],0,1e-12);
    near(d[2],-2./sqrt(5.),1e-12);

    /* Real WCS offset in matrix: reject float-style origin truncation. */
    const double translated[16]={
        1,0,0,0, 0,1,0,0, 0,0,1,0,
        -1000000.,2000000.,-3000000.,1
    };
    assert(musa_core_camera_screen_ray(translated,200,100,
                                       100,50,0,o,d));
    xyz(o,1000000,-2000000,2999999,1e-7);
    xyz(d,0,0,1,1e-12);

    /* Integration: screen center ray directly selects a 3D scene mesh.
     * This checks geometric consumption; it is not a phone touch test.
     */
    const float vertices[]={-.5f,-.5f,0, .5f,-.5f,0, 0,.5f,0};
    const int faces[]={0,1,2};
    MusaCoreMeshBVH *mesh=NULL;
    assert(musa_core_mesh_bvh_build_f32(vertices,3,faces,1,&mesh));
    MusaCoreBVHInstance inst={mesh,musa_core_affine3_identity(),42,1};
    MusaCoreBVHSceneHit hit={0};
    assert(musa_core_camera_screen_ray(eye4,200,100,100,50,0,o,d));
    assert(musa_core_bvh_scene_pick_wcs(&inst,1,o,d,10.,&hit,NULL)==1);
    assert(hit.object_id==42&&hit.instance_index==0);
    near(hit.face.distance,1.,1e-12);
    near(hit.face.wcs_z,0.,1e-12);
    musa_core_mesh_bvh_destroy(mesh);

    const double singular[16]={0};
    const double bad[16]={
        NAN,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1
    };
    o[0]=123.;d[2]=456.;
    assert(!musa_core_camera_screen_ray(singular,200,100,100,50,0,o,d));
    assert(!musa_core_camera_screen_ray(bad,200,100,100,50,0,o,d));
    assert(!musa_core_camera_screen_ray(NULL,200,100,100,50,0,o,d));
    assert(!musa_core_camera_screen_ray(eye4,200,100,100,50,0,NULL,d));
    assert(!musa_core_camera_screen_ray(eye4,200,100,100,50,0,o,NULL));
    assert(o[0]==123.&&d[2]==456.);
    puts("MusaCAD cross-platform screen -> WCS ray + 3D BVH scene selection PASS");
    return 0;
}
