#include "mc2_block_transform.h"
#include <stddef.h>
int mc2_block_world_transform(const Mc2BlockTransformNode *nodes,size_t count,
                              uint64_t handle,Mc2Affine2 *out) {
    if (!nodes || !out || !handle) return 0;
    size_t found=count;
    for (size_t i=0;i<count;++i) {
        if (nodes[i].ownership.handle==handle) {
            if (found!=count) return 0;
            found=i;
        }
    }
    if (found==count) return 0;
    /* Reject duplicate handles anywhere in the graph, even off this path. */
    for (size_t i=0;i<count;++i) {
        if (!nodes[i].ownership.handle) return 0;
        for (size_t j=0;j<i;++j)
            if (nodes[i].ownership.handle==nodes[j].ownership.handle) return 0;
    }
    Mc2Affine2 world=mc2_transform_identity();
    /* Walk child to root, pre-multiplying each ancestor transform. */
    uint64_t current=handle;
    for (size_t step=0;step<count;++step) {
        size_t index=count;
        for (size_t i=0;i<count;++i) {
            if (nodes[i].ownership.handle==current) {
                if (index!=count) return 0;
                index=i;
            }
        }
        if (index==count) return 0;
        Mc2Affine2 next;
        if (!mc2_transform_multiply(&nodes[index].local_transform,&world,&next)) return 0;
        world=next;
        current=nodes[index].ownership.parent_handle;
        if (!current) { *out=world; return 1; }
    }
    return 0;
}
