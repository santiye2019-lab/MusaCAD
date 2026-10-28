#ifndef MC2_BLOCK_TRANSFORM_H
#define MC2_BLOCK_TRANSFORM_H
#include "mc2_ownership.h"
#include "mc2_transform.h"
typedef struct {
    Mc2OwnerNode ownership;
    Mc2Affine2 local_transform;
} Mc2BlockTransformNode;
/* Resolve an entire ownership forest in O(n log n), including nested
 * affine transforms. Rejects duplicate/missing handles and cycles.
 * Does not modify output on failure; out[i] matches nodes[i]. */
int mc2_block_world_transforms_all(const Mc2BlockTransformNode *nodes,
                                   size_t count,Mc2Affine2 *out);
/* Parent-first composition, rejecting missing owners and cycles. */
int mc2_block_world_transform(const Mc2BlockTransformNode *nodes,size_t count,
                              uint64_t handle,Mc2Affine2 *out);
#endif
