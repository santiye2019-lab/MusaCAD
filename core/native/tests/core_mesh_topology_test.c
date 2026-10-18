/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "musacad_core.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>

int main(void){
    float tetra[]={0,0,0, 1,0,0, 0,1,0, 0,0,1};
    int outward[]={0,2,1,0,1,3,0,3,2,1,2,3};
    MusaCoreMeshTopology r={0};
    assert(musa_core_mesh_topology_f32(tetra,4,outward,4,&r));
    assert(r.checked&&r.closed_oriented_by_index&&r.triangles==4);
    assert(r.boundary_edges==0&&r.orientation_conflicts==0&&
           r.non_manifold_edges==0&&r.degenerate_triangles==0&&
           r.duplicate_faces==0);

    int open[]={0,2,1,0,1,3,0,3,2};
    assert(musa_core_mesh_topology_f32(tetra,4,open,3,&r));
    assert(r.checked&&!r.closed_oriented_by_index&&r.boundary_edges==3);

    int reversed[]={0,1,2,0,1,3,0,3,2,1,2,3};
    assert(musa_core_mesh_topology_f32(tetra,4,reversed,4,&r));
    assert(!r.closed_oriented_by_index&&r.orientation_conflicts==3);

    int extra[]={0,2,1,0,1,3,0,3,2,1,2,3,0,2,1};
    assert(musa_core_mesh_topology_f32(tetra,4,extra,5,&r));
    assert(!r.closed_oriented_by_index&&r.non_manifold_edges==3&&
           r.duplicate_faces==1);

    int doubled[]={0,1,2,0,2,1};
    assert(musa_core_mesh_topology_f32(tetra,4,doubled,2,&r));
    assert(r.boundary_edges==0&&r.orientation_conflicts==0);
    assert(r.duplicate_faces==1&&!r.closed_oriented_by_index);

    float collinear[]={0,0,0,1,1,1,2,2,2};
    int facet[]={0,1,2};
    assert(musa_core_mesh_topology_f32(collinear,3,facet,1,&r));
    assert(r.degenerate_triangles==1&&!r.closed_oriented_by_index);

    int identical[]={0,0,2};
    assert(musa_core_mesh_topology_f32(tetra,4,identical,1,&r));
    assert(r.degenerate_triangles==1&&!r.closed_oriented_by_index);

    float disconnected[]={0,0,0,1,0,0,0,1,0,0,0,0,1,0,0,0,0,1};
    int split[]={0,1,2,3,4,5};
    assert(musa_core_mesh_topology_f32(disconnected,6,split,2,&r));
    assert(!r.closed_oriented_by_index&&r.boundary_edges==6);

    int corrupt[]={0,1,99};
    float bad_xyz[]={0,0,0,1,0,0,0,NAN,0};
    r.triangles=123;r.checked=7;
    assert(!musa_core_mesh_topology_f32(tetra,4,corrupt,1,&r));
    assert(!musa_core_mesh_topology_f32(bad_xyz,3,facet,1,&r));
    assert(!musa_core_mesh_topology_f32(tetra,4,facet,1,NULL));
    assert(!musa_core_mesh_topology_f32(NULL,3,facet,1,&r));
    assert(r.checked==7&&r.triangles==123);

    assert(musa_core_mesh_topology_f32(tetra,4,outward,50001u,&r));
    assert(!r.checked&&!r.closed_oriented_by_index&&r.triangles==50001u);
    /* Oversized files must not force allocation/read of huge face payloads.
     * The shared C core itself returns unchecked without dereferencing. */
    assert(musa_core_mesh_topology_f32(NULL,4,NULL,50001u,&r));
    assert(!r.checked&&!r.closed_oriented_by_index&&r.triangles==50001u);
    assert(musa_core_mesh_topology_f32(NULL,0,NULL,0,&r));
    assert(r.checked&&!r.closed_oriented_by_index&&r.triangles==0);

    /* Topology preflight + signed mesh mass are independently callable. */
    assert(musa_core_mesh_topology_f32(tetra,4,outward,4,&r));
    MusaCoreMeshMass m={0};
    assert(musa_core_mesh_mass_f32(tetra,4,outward,4,
                                  r.checked&&r.closed_oriented_by_index,&m));
    assert(fabs(m.signed_volume-1./6.)<1e-12);
    assert(musa_core_mesh_topology_f32(tetra,4,open,3,&r));
    assert(!musa_core_mesh_mass_f32(tetra,4,open,3,
                                   r.checked&&r.closed_oriented_by_index,&m));
    puts("MusaCAD portable 3D indexed topology preflight PASS");
    return 0;
}
