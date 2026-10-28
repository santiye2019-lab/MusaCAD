#include "mc2_ownership.h"
#include <stddef.h>
static int locate(const Mc2OwnerNode *nodes, size_t count, uint64_t handle, size_t *index) {
    int found = 0;
    for (size_t i = 0; i < count; ++i) {
        if (nodes[i].handle == handle) {
            if (found) return 0;
            *index = i;
            found = 1;
        }
    }
    return found;
}
int mc2_ownership_visibility(const Mc2OwnerNode *nodes, size_t count,
                             uint64_t handle, int *visible) {
    if (!visible || !handle || (count && !nodes)) return 0;
    size_t index = 0;
    if (!locate(nodes,count,handle,&index)) return 0;
    int result = 1;
    uint64_t current = handle;
    /* A valid chain cannot traverse more than count nodes. */
    for (size_t steps = 0; steps < count; ++steps) {
        if (!locate(nodes,count,current,&index)) return 0;
        if (!nodes[index].visible) result = 0;
        current = nodes[index].parent_handle;
        if (!current) { *visible = result; return 1; }
    }
    return 0;
}
int mc2_ownership_validate(const Mc2OwnerNode *nodes, size_t count) {
    if (count && !nodes) return 0;
    for (size_t i = 0; i < count; ++i) {
        size_t index = 0;
        int visible = 0;
        if (!nodes[i].handle || !locate(nodes,count,nodes[i].handle,&index) ||
            !mc2_ownership_visibility(nodes,count,nodes[i].handle,&visible)) return 0;
    }
    return 1;
}
