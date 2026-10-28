#ifndef MC2_BLOCK_TRANSFORM_H
#define MC2_BLOCK_TRANSFORM_H
#include "mc2_ownership.h"
#include "mc2_transform.h"
typedef struct {
    Mc2OwnerNode ownership;
    Mc2Affine2 local_transform;
} Mc2BlockTransformNode;
/* Parent-first composition, rejecting missing owners and cycles. */
int mc2_block_world_transform(const Mc2BlockTransformNode *nodes,size_t count,
                              uint64_t handle,Mc2Affine2 *out);
#endif
