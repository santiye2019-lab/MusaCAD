/* SPDX-License-Identifier: MIT
 * First-party C11 post-decode viewport layer-state composition.
 */
#include "musa_dwg1032_viewport_visibility.h"
#include <stdint.h>
#include <stddef.h>

static int overlaps(const void *a, size_t an, const void *b, size_t bn) {
    if (!an || !bn) return 0;
    const uintptr_t x=(uintptr_t)a, y=(uintptr_t)b;
    return x<=y ? y-x<an : x-y<bn;
}
static size_t layer_at(const MusaDwg1032LayerState *layers,
                       size_t count, uint64_t handle) {
    size_t lo=0u, hi=count;
    while (lo<hi) {
        const size_t mid=lo+(hi-lo)/2u;
        if (layers[mid].handle<handle) lo=mid+1u;
        else hi=mid;
    }
    return lo;
}
static size_t override_at(const MusaDwg1032ViewportFreeze *overrides,
                          size_t count, uint64_t handle) {
    size_t lo=0u, hi=count;
    while (lo<hi) {
        const size_t mid=lo+(hi-lo)/2u;
        if (overrides[mid].layer_handle<handle) lo=mid+1u;
        else hi=mid;
    }
    return lo;
}
int musa_dwg1032_viewport_local_visibility(
    const uint64_t *entity_layer_handles,
    const uint8_t *entity_visible, size_t entity_count,
    const MusaDwg1032LayerState *layers, size_t layer_count,
    const MusaDwg1032ViewportFreeze *overrides, size_t override_count,
    uint8_t *local_visible) {
    if (!entity_count || !entity_layer_handles || !entity_visible ||
        !layers || !layer_count || !local_visible ||
        (override_count && !overrides) ||
        entity_count>SIZE_MAX/sizeof(*entity_layer_handles) ||
        layer_count>SIZE_MAX/sizeof(*layers) ||
        override_count>SIZE_MAX/sizeof(*overrides))
        return 0;
    if (overlaps(local_visible,entity_count,entity_layer_handles,
                 entity_count*sizeof(*entity_layer_handles)) ||
        overlaps(local_visible,entity_count,entity_visible,entity_count) ||
        overlaps(local_visible,entity_count,layers,
                 layer_count*sizeof(*layers)) ||
        overlaps(local_visible,entity_count,overrides,
                 override_count*sizeof(*overrides)))
        return 0;
    uint64_t previous=0u;
    for (size_t i=0u;i<layer_count;++i) {
        if (!layers[i].handle || layers[i].handle<=previous ||
            layers[i].on>1u || layers[i].thawed>1u) return 0;
        previous=layers[i].handle;
    }
    previous=0u;
    for (size_t i=0u;i<override_count;++i) {
        const uint64_t h=overrides[i].layer_handle;
        if (!h || h<=previous || overrides[i].frozen>1u) return 0;
        const size_t at=layer_at(layers,layer_count,h);
        if (at==layer_count || layers[at].handle!=h) return 0;
        previous=h;
    }
    /* Validate ALL entity handles and flags before publishing a prefix. */
    for (size_t i=0u;i<entity_count;++i) {
        const uint64_t h=entity_layer_handles[i];
        if (!h || entity_visible[i]>1u) return 0;
        const size_t at=layer_at(layers,layer_count,h);
        if (at==layer_count || layers[at].handle!=h) return 0;
    }
    for (size_t i=0u;i<entity_count;++i) {
        const uint64_t h=entity_layer_handles[i];
        const size_t at=layer_at(layers,layer_count,h);
        const size_t ov=override_at(overrides,override_count,h);
        const int viewport_frozen=ov<override_count &&
            overrides[ov].layer_handle==h && overrides[ov].frozen;
        local_visible[i]=(uint8_t)(entity_visible[i] &&
            layers[at].on && layers[at].thawed && !viewport_frozen);
    }
    return 1;
}

/* Two-stage, fail-closed composition. The owner resolver never publishes
 * output until the complete forest has passed validation. */
int musa_dwg1032_viewport_owner_visibility(
    const MusaDwg1032ObjectIndex *index,
    const uint64_t *owner_handles,
    const uint64_t *entity_layer_handles,
    const uint8_t *entity_visible,
    const MusaDwg1032LayerState *layers, size_t layer_count,
    const MusaDwg1032ViewportFreeze *overrides, size_t override_count,
    MusaDwg1032OwnerWork *work, size_t work_count,
    uint8_t *local_work, uint8_t *effective_visible) {
    if (!index || !index->entries || !index->count || !owner_handles ||
        !entity_layer_handles || !entity_visible || !layers || !layer_count ||
        (override_count && !overrides) || !work || !local_work ||
        !effective_visible || work_count<index->count ||
        index->count>SIZE_MAX/sizeof(*index->entries) ||
        index->count>SIZE_MAX/sizeof(*owner_handles) ||
        index->count>SIZE_MAX/sizeof(*entity_layer_handles) ||
        layer_count>SIZE_MAX/sizeof(*layers) ||
        override_count>SIZE_MAX/sizeof(*overrides) ||
        work_count>SIZE_MAX/sizeof(*work))
        return 0;
    const size_t n=index->count;
    const void *ptrs[]={index,index->entries,owner_handles,
        entity_layer_handles,entity_visible,layers,overrides,
        work,local_work,effective_visible};
    const size_t sizes[]={sizeof(*index),n*sizeof(*index->entries),
        n*sizeof(*owner_handles),n*sizeof(*entity_layer_handles),
        n*sizeof(*entity_visible),layer_count*sizeof(*layers),
        override_count*sizeof(*overrides),work_count*sizeof(*work),
        n*sizeof(*local_work),n*sizeof(*effective_visible)};
    for (size_t i=0u;i<10u;++i)
        for (size_t j=i+1u;j<10u;++j)
            if (overlaps(ptrs[i],sizes[i],ptrs[j],sizes[j])) return 0;
    if (!musa_dwg1032_viewport_local_visibility(
            entity_layer_handles,entity_visible,n,layers,layer_count,
            overrides,override_count,local_work)) return 0;
    return musa_dwg1032_owner_graph_visibility(
        index,owner_handles,local_work,work,work_count,effective_visible);
}
