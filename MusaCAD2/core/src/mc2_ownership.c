#include "mc2_ownership.h"
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
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
/* Independent indexed graph validation: O(n log n) sorting and lookups.
 * States: 0 unvisited, 1 on current ancestor path, 2 resolved. */
typedef struct { uint64_t handle; size_t index; } Mc2OwnerIndex;
static int mc2_owner_compare(const void *a,const void *b) {
    uint64_t x=((const Mc2OwnerIndex *)a)->handle;
    uint64_t y=((const Mc2OwnerIndex *)b)->handle;
    return (x>y)-(x<y);
}
static int mc2_owner_find(const Mc2OwnerIndex *sorted,size_t count,
                          uint64_t handle,size_t *index) {
    const Mc2OwnerIndex key={handle,0};
    const Mc2OwnerIndex *entry=(const Mc2OwnerIndex *)
        bsearch(&key,sorted,count,sizeof(*sorted),mc2_owner_compare);
    if (!entry) return 0;
    *index=entry->index;
    return 1;
}
int mc2_ownership_validate(const Mc2OwnerNode *nodes, size_t count) {
    if (!count) return 1;
    if (!nodes || count>SIZE_MAX/sizeof(Mc2OwnerIndex) ||
        count>SIZE_MAX/sizeof(size_t)) return 0;
    Mc2OwnerIndex *sorted=(Mc2OwnerIndex *)malloc(count*sizeof(*sorted));
    size_t *path=(size_t *)malloc(count*sizeof(*path));
    uint8_t *state=(uint8_t *)calloc(count,sizeof(*state));
    int ok=0;
    if (!sorted || !path || !state) goto done;
    for (size_t i=0;i<count;++i) {
        if (!nodes[i].handle) goto done;
        sorted[i]=(Mc2OwnerIndex){nodes[i].handle,i};
    }
    qsort(sorted,count,sizeof(*sorted),mc2_owner_compare);
    for (size_t i=1;i<count;++i)
        if (sorted[i-1].handle==sorted[i].handle) goto done;
    for (size_t i=0;i<count;++i) {
        if (state[i]==2) continue;
        size_t current=i,depth=0;
        for (;;) {
            if (state[current]==2) break;
            if (state[current]==1) goto done;
            state[current]=1;
            path[depth++]=current;
            uint64_t parent=nodes[current].parent_handle;
            if (!parent) break;
            if (!mc2_owner_find(sorted,count,parent,&current)) goto done;
        }
        while (depth) state[path[--depth]]=2;
    }
    ok=1;
done:
    free(state);
    free(path);
    free(sorted);
    return ok;
}
