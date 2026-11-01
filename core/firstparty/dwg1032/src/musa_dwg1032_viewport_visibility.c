/* SPDX-License-Identifier: MIT
 * First-party C11 post-decode viewport layer-state composition.
 */
#include "musa_dwg1032_viewport_visibility.h"
#include <stdint.h>
#include <stddef.h>
#include <string.h>

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

/* A block definition's layer-zero state is not the INSERT instance's layer
 * state. Resolve each child in the selected viewport; never reuse the
 * definition's layer-zero visibility for a different INSERT instance. */
int musa_dwg1032_viewport_insert_visibility(
    const uint64_t *source_layer_handles,
    const uint8_t *definition_owner_visible, size_t count,
    uint64_t layer_zero_handle, uint8_t insert_visible,
    const MusaDwg1032LayerState *layers, size_t layer_count,
    const MusaDwg1032ViewportFreeze *overrides, size_t override_count,
    uint8_t *effective_visible) {
    if (!layer_zero_handle || insert_visible>1u ||
        !layers || !layer_count ||
        (override_count && !overrides) ||
        (count && (!source_layer_handles || !definition_owner_visible ||
                   !effective_visible)) ||
        count>SIZE_MAX/sizeof(*source_layer_handles) ||
        layer_count>SIZE_MAX/sizeof(*layers) ||
        override_count>SIZE_MAX/sizeof(*overrides))
        return 0;
    if (overlaps(effective_visible,count,source_layer_handles,
                 count*sizeof(*source_layer_handles)) ||
        overlaps(effective_visible,count,definition_owner_visible,count) ||
        overlaps(effective_visible,count,layers,
                 layer_count*sizeof(*layers)) ||
        overlaps(effective_visible,count,overrides,
                 override_count*sizeof(*overrides)))
        return 0;

    uint64_t previous=0u;
    for (size_t i=0u;i<layer_count;++i) {
        if (!layers[i].handle || layers[i].handle<=previous ||
            layers[i].on>1u || layers[i].thawed>1u) return 0;
        previous=layers[i].handle;
    }
    const size_t zero_at=layer_at(layers,layer_count,layer_zero_handle);
    if (zero_at==layer_count ||
        layers[zero_at].handle!=layer_zero_handle) return 0;
    previous=0u;
    for (size_t i=0u;i<override_count;++i) {
        const uint64_t h=overrides[i].layer_handle;
        if (!h || h<=previous || overrides[i].frozen>1u) return 0;
        const size_t at=layer_at(layers,layer_count,h);
        if (at==layer_count || layers[at].handle!=h) return 0;
        previous=h;
    }
    /* Do not publish an early child if a later layer is unproven. */
    for (size_t i=0u;i<count;++i) {
        const uint64_t h=source_layer_handles[i];
        if (!h || definition_owner_visible[i]>1u) return 0;
        const size_t at=layer_at(layers,layer_count,h);
        if (at==layer_count || layers[at].handle!=h) return 0;
    }
    for (size_t i=0u;i<count;++i) {
        const uint64_t h=source_layer_handles[i];
        uint8_t visible=(uint8_t)(
            insert_visible && definition_owner_visible[i]);
        if (h!=layer_zero_handle) {
            const size_t at=layer_at(layers,layer_count,h);
            const size_t ov=override_at(overrides,override_count,h);
            const int frozen=ov<override_count &&
                overrides[ov].layer_handle==h && overrides[ov].frozen;
            visible=(uint8_t)(visible && layers[at].on &&
                              layers[at].thawed && !frozen);
        }
        effective_visible[i]=visible;
    }
    return 1;
}

/* Single layer lookup per entity. The caller's scratch buffer absorbs
 * partial work; no public mask is published until all inputs validate.
 * This is post-decode evidence, not a DWG parser or render permission. */
int musa_dwg1032_viewport_local_visibility_scratch(
    const uint64_t *entity_layer_handles,
    const uint8_t *entity_visible, size_t entity_count,
    const MusaDwg1032LayerState *layers, size_t layer_count,
    const MusaDwg1032ViewportFreeze *overrides, size_t override_count,
    uint8_t *local_visible, uint8_t *scratch, size_t scratch_capacity) {
    if (!entity_count || !entity_layer_handles || !entity_visible ||
        !layers || !layer_count || !local_visible || !scratch ||
        scratch_capacity<entity_count || (override_count && !overrides) ||
        entity_count>SIZE_MAX/sizeof(*entity_layer_handles) ||
        layer_count>SIZE_MAX/sizeof(*layers) ||
        override_count>SIZE_MAX/sizeof(*overrides))
        return 0;
    const void *inputs[]={entity_layer_handles,entity_visible,layers,overrides};
    const size_t input_bytes[]={
        entity_count*sizeof(*entity_layer_handles),entity_count,
        layer_count*sizeof(*layers),override_count*sizeof(*overrides)
    };
    if (overlaps(local_visible,entity_count,scratch,scratch_capacity))
        return 0;
    for (size_t i=0u;i<4u;++i)
        if (overlaps(local_visible,entity_count,inputs[i],input_bytes[i]) ||
            overlaps(scratch,scratch_capacity,inputs[i],input_bytes[i]))
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
    for (size_t i=0u;i<entity_count;++i) {
        const uint64_t h=entity_layer_handles[i];
        if (!h || entity_visible[i]>1u) return 0;
        const size_t at=layer_at(layers,layer_count,h);
        if (at==layer_count || layers[at].handle!=h) return 0;
        const size_t ov=override_at(overrides,override_count,h);
        const int frozen=ov<override_count &&
            overrides[ov].layer_handle==h && overrides[ov].frozen;
        scratch[i]=(uint8_t)(entity_visible[i] && layers[at].on &&
                             layers[at].thawed && !frozen);
    }
    memcpy(local_visible,scratch,entity_count);
    return 1;
}
