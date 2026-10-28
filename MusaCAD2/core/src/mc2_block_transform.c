#include "mc2_block_transform.h"
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
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

typedef struct { uint64_t handle; size_t index; } Mc2BlockIndex;
static int mc2_block_index_compare(const void *a,const void *b) {
    const uint64_t x=((const Mc2BlockIndex *)a)->handle;
    const uint64_t y=((const Mc2BlockIndex *)b)->handle;
    return (x>y)-(x<y);
}
static int mc2_block_find(const Mc2BlockIndex *sorted,size_t count,
                          uint64_t handle,size_t *out_index) {
    const Mc2BlockIndex key={handle,0};
    const Mc2BlockIndex *found=(const Mc2BlockIndex *)
        bsearch(&key,sorted,count,sizeof(*sorted),mc2_block_index_compare);
    if (!found) return 0;
    *out_index=found->index;
    return 1;
}
int mc2_block_world_transforms_all(const Mc2BlockTransformNode *nodes,
                                   size_t count,Mc2Affine2 *out) {
    if (!count) return 1;
    if (!nodes || !out || count>SIZE_MAX/sizeof(Mc2BlockIndex) ||
        count>SIZE_MAX/sizeof(Mc2Affine2) ||
        count>SIZE_MAX/sizeof(size_t)) return 0;
    Mc2BlockIndex *sorted=(Mc2BlockIndex *)malloc(count*sizeof(*sorted));
    Mc2Affine2 *world=(Mc2Affine2 *)malloc(count*sizeof(*world));
    size_t *path=(size_t *)malloc(count*sizeof(*path));
    uint8_t *state=(uint8_t *)calloc(count,sizeof(*state));
    int ok=0;
    if (!sorted || !world || !path || !state) goto cleanup;
    for (size_t i=0;i<count;++i) {
        if (!nodes[i].ownership.handle) goto cleanup;
        sorted[i]=(Mc2BlockIndex){nodes[i].ownership.handle,i};
    }
    qsort(sorted,count,sizeof(*sorted),mc2_block_index_compare);
    for (size_t i=1;i<count;++i)
        if (sorted[i-1].handle==sorted[i].handle) goto cleanup;
    const Mc2Affine2 identity=mc2_transform_identity();
    for (size_t i=0;i<count;++i) {
        if (state[i]==2) continue;
        size_t current=i,depth=0;
        for (;;) {
            if (state[current]==2) break;
            if (state[current]==1) goto cleanup;
            state[current]=1;
            path[depth++]=current;
            const uint64_t parent=nodes[current].ownership.parent_handle;
            if (!parent) break;
            if (!mc2_block_find(sorted,count,parent,&current)) goto cleanup;
        }
        while (depth) {
            const size_t k=path[--depth];
            const uint64_t parent=nodes[k].ownership.parent_handle;
            const Mc2Affine2 *base=&identity;
            size_t parent_index=0;
            if (parent) {
                if (!mc2_block_find(sorted,count,parent,&parent_index) ||
                    state[parent_index]!=2) goto cleanup;
                base=&world[parent_index];
            }
            if (!mc2_transform_multiply(base,&nodes[k].local_transform,
                                        &world[k])) goto cleanup;
            state[k]=2;
        }
    }
    memcpy(out,world,count*sizeof(*out));
    ok=1;
cleanup:
    free(state);
    free(path);
    free(world);
    free(sorted);
    return ok;
}
