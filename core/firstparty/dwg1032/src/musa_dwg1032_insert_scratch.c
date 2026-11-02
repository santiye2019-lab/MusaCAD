/* SPDX-License-Identifier: MIT
 * Independently authored bounded INSERT visibility calculation.
 */
#include "musa_dwg1032_insert_scratch.h"
#include <stdint.h>
#include <stddef.h>
#include <string.h>

static int spans_overlap(const void *a, size_t an, const void *b, size_t bn) {
    if (!an || !bn) return 0;
    const uintptr_t x=(uintptr_t)a, y=(uintptr_t)b;
    return x<=y ? y-x<an : x-y<bn;
}
static size_t layer_pos(const MusaDwg1032LayerState *a,
                        size_t n, uint64_t handle) {
    size_t lo=0,hi=n;
    while (lo<hi) {
        const size_t mid=lo+(hi-lo)/2;
        if (a[mid].handle<handle) lo=mid+1;
        else hi=mid;
    }
    return lo;
}
static size_t freeze_pos(const MusaDwg1032ViewportFreeze *a,
                         size_t n, uint64_t handle) {
    size_t lo=0,hi=n;
    while (lo<hi) {
        const size_t mid=lo+(hi-lo)/2;
        if (a[mid].layer_handle<handle) lo=mid+1;
        else hi=mid;
    }
    return lo;
}
int musa_dwg1032_insert_visibility_scratch(
    const uint64_t *child_layers, const uint8_t *owner_visible,
    size_t count, uint64_t zero_layer, uint8_t insert_visible,
    const MusaDwg1032LayerState *layers, size_t layer_count,
    const MusaDwg1032ViewportFreeze *overrides, size_t override_count,
    uint8_t *result, uint8_t *scratch, size_t scratch_capacity) {
    if (!zero_layer || insert_visible>1 || !layers || !layer_count ||
        (override_count && !overrides) ||
        (count && (!child_layers || !owner_visible || !result || !scratch)) ||
        scratch_capacity<count ||
        count>SIZE_MAX/sizeof(*child_layers) ||
        layer_count>SIZE_MAX/sizeof(*layers) ||
        override_count>SIZE_MAX/sizeof(*overrides)) return 0;

    /* Reject writes into any input, including unused scratch capacity.
     * Validation must not modify the public result or its source arrays. */
    const void *inputs[]={child_layers,owner_visible,layers,overrides};
    const size_t bytes[]={count*sizeof(*child_layers),count,
        layer_count*sizeof(*layers),override_count*sizeof(*overrides)};
    if (spans_overlap(result,count,scratch,scratch_capacity)) return 0;
    for (size_t i=0;i<4;++i)
        if (spans_overlap(result,count,inputs[i],bytes[i]) ||
            spans_overlap(scratch,scratch_capacity,inputs[i],bytes[i]))
            return 0;

    uint64_t previous=0;
    for (size_t i=0;i<layer_count;++i) {
        if (!layers[i].handle || layers[i].handle<=previous ||
            layers[i].on>1 || layers[i].thawed>1) return 0;
        previous=layers[i].handle;
    }
    const size_t z=layer_pos(layers,layer_count,zero_layer);
    if (z==layer_count || layers[z].handle!=zero_layer) return 0;
    previous=0;
    for (size_t i=0;i<override_count;++i) {
        const uint64_t h=overrides[i].layer_handle;
        if (!h || h<=previous || overrides[i].frozen>1) return 0;
        const size_t at=layer_pos(layers,layer_count,h);
        if (at==layer_count || layers[at].handle!=h) return 0;
        previous=h;
    }
    /* A single child pass writes disposable scratch only. A late invalid
     * handle or flag cannot expose a partially computed public mask. */
    for (size_t i=0;i<count;++i) {
        const uint64_t h=child_layers[i];
        if (!h || owner_visible[i]>1) return 0;
        const size_t at=layer_pos(layers,layer_count,h);
        if (at==layer_count || layers[at].handle!=h) return 0;
        uint8_t visible=(uint8_t)(insert_visible && owner_visible[i]);
        if (h!=zero_layer) {
            const size_t f=freeze_pos(overrides,override_count,h);
            const int frozen=f<override_count &&
                overrides[f].layer_handle==h && overrides[f].frozen;
            visible=(uint8_t)(visible && layers[at].on &&
                              layers[at].thawed && !frozen);
        }
        scratch[i]=visible;
    }
    if (count) memcpy(result,scratch,count);
    return 1;
}
