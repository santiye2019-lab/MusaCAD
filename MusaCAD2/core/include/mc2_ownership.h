#ifndef MC2_OWNERSHIP_H
#define MC2_OWNERSHIP_H
#include <stddef.h>
#include <stdint.h>
typedef struct {
    uint64_t handle;
    uint64_t parent_handle; /* zero means model space */
    int visible;
} Mc2OwnerNode;
/* Requires unique nonzero handles, existing parents, and an acyclic hierarchy. */
int mc2_ownership_validate(const Mc2OwnerNode *nodes, size_t count);
/* Fails closed if any ancestor is missing, hidden, duplicated, or cyclic. */
int mc2_ownership_visibility(const Mc2OwnerNode *nodes, size_t count,
                             uint64_t handle, int *visible);
#endif
