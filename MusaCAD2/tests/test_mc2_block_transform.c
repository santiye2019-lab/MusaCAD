#include "mc2_block_transform.h"
#include <assert.h>
#include <stdlib.h>
#include <math.h>
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
    nodes[1].ownership.parent_handle=1;
    Mc2Affine2 batch[3];
    assert(mc2_block_world_transforms_all(nodes,3,batch));
    assert(mc2_transform_point(&batch[2],(Mc2Point2){1,1},&point));
    assert(point.x==14 && point.y==2);
    nodes[0].ownership.parent_handle=3;
    batch[2].tx=999;
    assert(!mc2_block_world_transforms_all(nodes,3,batch));
    assert(batch[2].tx==999);
    nodes[0].ownership.parent_handle=0;
    nodes[2].ownership.handle=1;
    assert(!mc2_block_world_transforms_all(nodes,3,batch));
    nodes[2].ownership.handle=3;
    nodes[1].ownership.parent_handle=99;
    assert(!mc2_block_world_transforms_all(nodes,3,batch));
    nodes[1].ownership.parent_handle=1;
    nodes[1].local_transform.tx=NAN;
    assert(!mc2_block_world_transforms_all(nodes,3,batch));
    nodes[1].local_transform.tx=0;
    assert(mc2_block_world_transforms_all(NULL,0,NULL));
    /* Deep hierarchy must be processed iteratively without recursion. */
    const size_t n=2048;
    Mc2BlockTransformNode *chain=calloc(n,sizeof(*chain));
    Mc2Affine2 *all=calloc(n,sizeof(*all));
    assert(chain && all);
    for (size_t i=0;i<n;++i) {
        chain[i].ownership.handle=(uint64_t)(i+1);
        chain[i].ownership.parent_handle=i?(uint64_t)i:0;
        chain[i].local_transform=mc2_transform_identity();
        chain[i].local_transform.tx=1;
    }
    assert(mc2_block_world_transforms_all(chain,n,all));
    assert(all[n-1].tx==(double)n);
    chain[n-1].ownership.parent_handle=(uint64_t)n;
    all[n-1].tx=12345;
    assert(!mc2_block_world_transforms_all(chain,n,all));
    assert(all[n-1].tx==12345);
    free(all);
    free(chain);
    return 0;
}
