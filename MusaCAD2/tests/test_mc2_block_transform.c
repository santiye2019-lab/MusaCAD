#include "mc2_block_transform.h"
#include <assert.h>
int main(void) {
    Mc2BlockTransformNode nodes[3]={
        {{1,0,1},{1,0,0,1,10,0}},
        {{2,1,1},{2,0,0,2,0,0}},
        {{3,2,1},{1,0,0,1,1,0}}
    };
    Mc2Affine2 world;
    Mc2Point2 point;
    assert(mc2_block_world_transform(nodes,3,3,&world));
    assert(mc2_transform_point(&world,(Mc2Point2){1,1},&point));
    assert(point.x==14 && point.y==2);
    nodes[0].ownership.parent_handle=3;
    assert(!mc2_block_world_transform(nodes,3,3,&world));
    nodes[0].ownership.parent_handle=0;
    nodes[2].ownership.handle=1;
    assert(!mc2_block_world_transform(nodes,3,2,&world));
    nodes[2].ownership.handle=3;
    nodes[1].ownership.parent_handle=99;
    assert(!mc2_block_world_transform(nodes,3,3,&world));
    return 0;
}
