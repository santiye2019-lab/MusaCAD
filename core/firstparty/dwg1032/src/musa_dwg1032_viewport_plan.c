/* SPDX-License-Identifier: MIT
 * First-party C11 post-decode viewport plan; no external CAD code.
 */
#include "musa_dwg1032_viewport_plan.h"
#include <stdint.h>
#include <stddef.h>

static int overlaps(const void *a, size_t an, const void *b, size_t bn) {
    if (!an || !bn) return 0;
    const uintptr_t x=(uintptr_t)a, y=(uintptr_t)b;
    return x<=y ? y-x<an : x-y<bn;
}
static size_t find_layer(const MusaDwg1032LayerState *layers,
                         size_t count, uint64_t handle) {
    size_t lo=0u,hi=count;
    while (lo<hi) {
        const size_t mid=lo+(hi-lo)/2u;
        if (layers[mid].handle<handle) lo=mid+1u;
        else hi=mid;
    }
    return lo;
}
static size_t find_override(const MusaDwg1032ViewportFreeze *overrides,
                            size_t count, uint64_t handle) {
    size_t lo=0u,hi=count;
    while (lo<hi) {
        const size_t mid=lo+(hi-lo)/2u;
        if (overrides[mid].layer_handle<handle) lo=mid+1u;
        else hi=mid;
    }
    return lo;
}
int musa_dwg1032_viewport_plan_init(
    MusaDwg1032ViewportPlan *plan,
    const MusaDwg1032LayerState *layers, size_t layer_count,
    const MusaDwg1032ViewportFreeze *overrides, size_t override_count) {
    if (!plan || !layers || !layer_count ||
        (override_count && !overrides) ||
        layer_count>PTRDIFF_MAX/sizeof(*layers) ||
        override_count>PTRDIFF_MAX/sizeof(*overrides) ||
        overlaps(plan,sizeof(*plan),layers,layer_count*sizeof(*layers)) ||
        overlaps(plan,sizeof(*plan),overrides,
                 override_count*sizeof(*overrides))) return 0;
    uint64_t previous=0u;
    for (size_t i=0u;i<layer_count;++i) {
        const MusaDwg1032LayerState layer=layers[i];
        if (!layer.handle || layer.handle<=previous ||
            layer.on>1u || layer.thawed>1u) return 0;
        previous=layer.handle;
    }
    previous=0u;
    for (size_t i=0u;i<override_count;++i) {
        const MusaDwg1032ViewportFreeze freeze=overrides[i];
        if (!freeze.layer_handle || freeze.layer_handle<=previous ||
            freeze.frozen>1u) return 0;
        const size_t at=find_layer(layers,layer_count,freeze.layer_handle);
        if (at==layer_count || layers[at].handle!=freeze.layer_handle)
            return 0;
        previous=freeze.layer_handle;
    }
    const MusaDwg1032ViewportPlan next={
        layers,layer_count,overrides,override_count
    };
    *plan=next;
    return 1;
}
int musa_dwg1032_viewport_plan_page(
    const MusaDwg1032ViewportPlan *plan,
    const uint64_t *entity_layer_handles,
    const uint8_t *entity_visible, size_t total_count,
    size_t first, size_t page_count, uint8_t *page_visible) {
    if (!plan || !plan->layers || !plan->layer_count ||
        (plan->override_count && !plan->overrides) ||
        !entity_layer_handles || !entity_visible || !page_visible ||
        !page_count || first>total_count || page_count>total_count-first ||
        total_count>PTRDIFF_MAX/sizeof(*entity_layer_handles) ||
        total_count>PTRDIFF_MAX/sizeof(*entity_visible) ||
        page_count>PTRDIFF_MAX ||
        plan->layer_count>PTRDIFF_MAX/sizeof(*plan->layers) ||
        plan->override_count>PTRDIFF_MAX/sizeof(*plan->overrides))
        return 0;
    if (overlaps(page_visible,page_count,plan,sizeof(*plan)) ||
        overlaps(page_visible,page_count,entity_layer_handles,
                 total_count*sizeof(*entity_layer_handles)) ||
        overlaps(page_visible,page_count,entity_visible,total_count) ||
        overlaps(page_visible,page_count,plan->layers,
                 plan->layer_count*sizeof(*plan->layers)) ||
        overlaps(page_visible,page_count,plan->overrides,
                 plan->override_count*sizeof(*plan->overrides)))
        return 0;
    /* Validate the entire requested page before publishing even its first
     * byte. The plan's metadata is immutable after successful init. */
    for (size_t i=0u;i<page_count;++i) {
        const size_t at=first+i;
        const uint64_t handle=entity_layer_handles[at];
        if (!handle || entity_visible[at]>1u) return 0;
        const size_t layer=find_layer(plan->layers,plan->layer_count,handle);
        if (layer==plan->layer_count ||
            plan->layers[layer].handle!=handle) return 0;
    }
    for (size_t i=0u;i<page_count;++i) {
        const size_t at=first+i;
        const uint64_t handle=entity_layer_handles[at];
        const size_t layer=find_layer(plan->layers,plan->layer_count,handle);
        const size_t ov=find_override(plan->overrides,
                                      plan->override_count,handle);
        const int frozen=ov<plan->override_count &&
            plan->overrides[ov].layer_handle==handle &&
            plan->overrides[ov].frozen;
        page_visible[i]=(uint8_t)(entity_visible[at] &&
            plan->layers[layer].on && plan->layers[layer].thawed && !frozen);
    }
    return 1;
}
