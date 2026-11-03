/* SPDX-License-Identifier: MIT */
#include "musa_dwg1032_owner_graph.h"
#include <stdint.h>
#include <stddef.h>
#include <string.h>

/* Reject nonrepresentable C object spans before pointer indexing, even if
 * the byte multiplication would fit in size_t. */
static int overlaps(const void *a, size_t an, const void *b, size_t bn) {
    if (!an || !bn) return 0;
    const uintptr_t x=(uintptr_t)a, y=(uintptr_t)b;
    return x<=y ? y-x<an : x-y<bn;
}

int musa_dwg1032_owner_graph_resolve(const MusaDwg1032ObjectIndex *index,
                                     const uint64_t *owner_handles,
                                     MusaDwg1032OwnerWork *work,
                                     size_t work_count,
                                     uint64_t *roots,
                                     size_t *depths) {
    if (!index || !owner_handles || !work || !roots || !depths ||
        !index->entries || !index->count || work_count<index->count ||
        index->count>PTRDIFF_MAX/sizeof(*index->entries) ||
        index->count>PTRDIFF_MAX/sizeof(*owner_handles) ||
        index->count>PTRDIFF_MAX/sizeof(*work) ||
        work_count>PTRDIFF_MAX/sizeof(*work) ||
        index->count>PTRDIFF_MAX/sizeof(*roots) ||
        index->count>PTRDIFF_MAX/sizeof(*depths)) return 0;
    const size_t n=index->count;
    const void *ptrs[]={index,index->entries,owner_handles,work,roots,depths};
    const size_t sizes[]={sizeof(*index),n*sizeof(*index->entries),
        n*sizeof(*owner_handles),work_count*sizeof(*work),n*sizeof(*roots),
        n*sizeof(*depths)};
    for (size_t a=0;a<6;++a)
        for (size_t b=a+1;b<6;++b)
            if (overlaps(ptrs[a],sizes[a],ptrs[b],sizes[b])) return 0;
    /* Index storage is caller-owned and may have changed since init.
     * Validate it before touching scratch or publishing any roots: the
     * binary-search owner lookup requires strictly increasing handles. */
    uint64_t previous_handle=0;
    for (size_t i=0;i<n;++i) {
        const MusaDwg1032ObjectLocation e=index->entries[i];
        if (!e.handle || e.handle<=previous_handle || !e.length ||
            e.offset>index->section_bytes ||
            e.length>index->section_bytes-e.offset)
            return 0;
        previous_handle=e.handle;
    }
    memset(work,0,n*sizeof(*work));
    for (size_t i=0;i<n;++i) {
        if (work[i].state==2u) continue;
        size_t at=i, top=SIZE_MAX;
        uint64_t root=0;
        size_t depth=0;
        for (;;) {
            if (work[at].state==2u) {
                root=work[at].root;
                depth=work[at].depth;
                break;
            }
            if (work[at].state==1u) return 0; /* Ownership cycle. */
            work[at].state=1u;
            work[at].previous=top;
            top=at;
            const uint64_t parent=owner_handles[at];
            if (!parent) {
                root=index->entries[at].handle;
                break;
            }
            if (!musa_dwg1032_object_index_find(index,parent,&at)) return 0;
        }
        while (top!=SIZE_MAX) {
            const size_t current=top;
            top=work[current].previous;
            if (owner_handles[current]) {
                if (depth==SIZE_MAX) return 0;
                ++depth;
            } else depth=0;
            work[current].root=root;
            work[current].depth=depth;
            work[current].state=2u;
        }
    }
    for (size_t i=0;i<n;++i) {
        roots[i]=work[i].root;
        depths[i]=work[i].depth;
    }
    return 1;
}

/* Visibility flags must be decoded by a separate, verified reader. */
int musa_dwg1032_owner_graph_visibility(
    const MusaDwg1032ObjectIndex *index,
    const uint64_t *owner_handles, const uint8_t *local_visible,
    MusaDwg1032OwnerWork *work, size_t work_count,
    uint8_t *effective_visible) {
    if (!index || !index->entries || !index->count || !owner_handles ||
        !local_visible || !work || !effective_visible ||
        work_count<index->count ||
        index->count>PTRDIFF_MAX/sizeof(*index->entries) ||
        index->count>PTRDIFF_MAX/sizeof(*owner_handles) ||
        work_count>PTRDIFF_MAX/sizeof(*work) ||
        index->count>PTRDIFF_MAX/sizeof(*effective_visible)) return 0;
    const size_t n=index->count;
    const void *ptrs[]={index,index->entries,owner_handles,local_visible,
                        work,effective_visible};
    const size_t sizes[]={sizeof(*index),n*sizeof(*index->entries),
                          n*sizeof(*owner_handles),n*sizeof(*local_visible),
                          work_count*sizeof(*work),n*sizeof(*effective_visible)};
    for (size_t a=0;a<6u;++a)
        for (size_t b=a+1u;b<6u;++b)
            if (overlaps(ptrs[a],sizes[a],ptrs[b],sizes[b])) return 0;
    uint64_t previous_handle=0;
    for (size_t i=0;i<n;++i) {
        const MusaDwg1032ObjectLocation e=index->entries[i];
        if (!e.handle || e.handle<=previous_handle || !e.length ||
            e.offset>index->section_bytes ||
            e.length>index->section_bytes-e.offset ||
            local_visible[i]>1u) return 0;
        previous_handle=e.handle;
    }
    memset(work,0,n*sizeof(*work));
    for (size_t i=0;i<n;++i) {
        if (work[i].state==2u) continue;
        size_t at=i,top=SIZE_MAX;
        uint8_t inherited=1u;
        for (;;) {
            if (work[at].state==2u) {
                inherited=(uint8_t)work[at].root;
                break;
            }
            if (work[at].state==1u) return 0;
            work[at].state=1u;
            work[at].previous=top;
            top=at;
            const uint64_t parent=owner_handles[at];
            if (!parent) break;
            if (!musa_dwg1032_object_index_find(index,parent,&at)) return 0;
        }
        while (top!=SIZE_MAX) {
            const size_t current=top;
            top=work[current].previous;
            inherited=(uint8_t)(inherited && local_visible[current]);
            work[current].root=(uint64_t)inherited;
            work[current].state=2u;
        }
    }
    for (size_t i=0;i<n;++i)
        effective_visible[i]=(uint8_t)work[i].root;
    return 1;
}

/* A strict post-decode layer lookup. Never infer layer metadata from DWG
 * object bytes here: the caller must authenticate and decode it separately. */
int musa_dwg1032_owner_graph_layer_visibility(
    const MusaDwg1032ObjectIndex *index,
    const uint64_t *owner_handles,
    const uint64_t *entity_layer_handles,
    const uint8_t *entity_visible,
    const MusaDwg1032LayerState *layers, size_t layer_count,
    MusaDwg1032OwnerWork *work, size_t work_count,
    uint8_t *local_work, uint8_t *effective_visible) {
    if (!index || !index->entries || !index->count || !owner_handles ||
        !entity_layer_handles || !entity_visible || !layers || !layer_count ||
        !work || !local_work || !effective_visible ||
        work_count < index->count ||
        index->count > PTRDIFF_MAX / sizeof(*index->entries) ||
        index->count > PTRDIFF_MAX / sizeof(*owner_handles) ||
        index->count > PTRDIFF_MAX / sizeof(*entity_layer_handles) ||
        index->count > PTRDIFF_MAX / sizeof(*entity_visible) ||
        layer_count > PTRDIFF_MAX / sizeof(*layers) ||
        work_count > PTRDIFF_MAX / sizeof(*work) ||
        index->count > PTRDIFF_MAX / sizeof(*local_work) ||
        index->count > PTRDIFF_MAX / sizeof(*effective_visible)) return 0;

    const size_t n=index->count;
    const void *ptrs[]={index,index->entries,owner_handles,
        entity_layer_handles,entity_visible,layers,work,local_work,
        effective_visible};
    const size_t sizes[]={sizeof(*index),n*sizeof(*index->entries),
        n*sizeof(*owner_handles),n*sizeof(*entity_layer_handles),
        n*sizeof(*entity_visible),layer_count*sizeof(*layers),
        work_count*sizeof(*work),n*sizeof(*local_work),
        n*sizeof(*effective_visible)};
    for (size_t a=0;a<9u;++a)
        for (size_t c=a+1u;c<9u;++c)
            if (overlaps(ptrs[a],sizes[a],ptrs[c],sizes[c])) return 0;

    uint64_t last=0;
    for (size_t i=0;i<layer_count;++i) {
        if (!layers[i].handle || layers[i].handle<=last ||
            layers[i].on>1u || layers[i].thawed>1u) return 0;
        last=layers[i].handle;
    }
    for (size_t i=0;i<n;++i) {
        if (entity_visible[i]>1u || !entity_layer_handles[i]) return 0;
        size_t low=0,high=layer_count;
        while (low<high) {
            const size_t mid=low+(high-low)/2u;
            if (layers[mid].handle<entity_layer_handles[i])
                low=mid+1u;
            else high=mid;
        }
        if (low==layer_count ||
            layers[low].handle!=entity_layer_handles[i]) return 0;
        local_work[i]=(uint8_t)(entity_visible[i] &&
            layers[low].on && layers[low].thawed);
    }
    /* The existing owner resolver publishes output only after checking
     * the complete forest, so a late dangling owner or cycle rolls back. */
    return musa_dwg1032_owner_graph_visibility(
        index,owner_handles,local_work,work,work_count,effective_visible);
}

/* Per-instance layer-zero inheritance, without changing block definitions.
 * The caller supplies owner-only flags to avoid incorrectly retaining a
 * definition-time layer-0 suppression after insertion on another layer. */
int musa_dwg1032_owner_graph_apply_insert_context(
    const uint64_t *source_layer_handles,
    const uint8_t *definition_owner_visible,
    const uint8_t *definition_layer_visible,
    size_t count, uint64_t layer_zero_handle, uint8_t insert_visible,
    uint8_t *effective_visible) {
    if (!layer_zero_handle || insert_visible>1u ||
        count>PTRDIFF_MAX/sizeof(*source_layer_handles) ||
        (count && (!source_layer_handles || !definition_owner_visible ||
                   !definition_layer_visible || !effective_visible)))
        return 0;
    if (overlaps(effective_visible,count,source_layer_handles,
                 count*sizeof(*source_layer_handles)) ||
        overlaps(effective_visible,count,definition_owner_visible,count) ||
        overlaps(effective_visible,count,definition_layer_visible,count))
        return 0;
    /* Validate the entire instance before publishing a single flag. */
    for (size_t i=0;i<count;++i)
        if (!source_layer_handles[i] || definition_owner_visible[i]>1u ||
            definition_layer_visible[i]>1u) return 0;
    for (size_t i=0;i<count;++i)
        effective_visible[i]=(uint8_t)(
            insert_visible && definition_owner_visible[i] &&
            (source_layer_handles[i]==layer_zero_handle ||
             definition_layer_visible[i]));
    return 1;
}
