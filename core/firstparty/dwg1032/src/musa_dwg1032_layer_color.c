/* SPDX-License-Identifier: MIT
 * Independently authored C11 post-decode layer color selection.
 */
#include "musa_dwg1032_layer_color.h"
#include <stdint.h>
#include <stddef.h>

static int overlaps(const void *a, size_t an, const void *b, size_t bn) {
    if (!an || !bn) return 0;
    const uintptr_t x=(uintptr_t)a, y=(uintptr_t)b;
    return x<=y ? y-x<an : x-y<bn;
}

static int valid_color(const MusaDwg1032CadColor *c) {
    return c->aci<=256u && c->has_truecolor<=1u &&
           (!c->has_truecolor || c->truecolor_rgb<=0xffffffu);
}

static size_t find_layer(const MusaDwg1032LayerColor *layers,
                         size_t count, uint64_t handle) {
    size_t lo=0,hi=count;
    while (lo<hi) {
        const size_t mid=lo+(hi-lo)/2u;
        if (layers[mid].handle<handle) lo=mid+1u;
        else hi=mid;
    }
    return lo;
}

int musa_dwg1032_layer_color_select(
    const MusaDwg1032ObjectIndex *index,
    const uint64_t *entity_layer_handles,
    const MusaDwg1032CadColor *entity_colors,
    const MusaDwg1032LayerColor *layers, size_t layer_count,
    MusaDwg1032ResolvedColor *resolved) {
    if (!index || !index->entries || !index->count ||
        !entity_layer_handles || !entity_colors || !layers ||
        !layer_count || !resolved ||
        index->count>SIZE_MAX/sizeof(*index->entries) ||
        index->count>SIZE_MAX/sizeof(*entity_layer_handles) ||
        index->count>SIZE_MAX/sizeof(*entity_colors) ||
        layer_count>SIZE_MAX/sizeof(*layers) ||
        index->count>SIZE_MAX/sizeof(*resolved)) return 0;
    const size_t n=index->count;
    const size_t out_bytes=n*sizeof(*resolved);
    if (overlaps(resolved,out_bytes,index,sizeof(*index)) ||
        overlaps(resolved,out_bytes,index->entries,n*sizeof(*index->entries)) ||
        overlaps(resolved,out_bytes,entity_layer_handles,n*sizeof(*entity_layer_handles)) ||
        overlaps(resolved,out_bytes,entity_colors,n*sizeof(*entity_colors)) ||
        overlaps(resolved,out_bytes,layers,layer_count*sizeof(*layers))) return 0;

    /* Complete preflight before publishing a single output. */
    uint64_t previous=0;
    for (size_t i=0;i<n;++i) {
        const MusaDwg1032ObjectLocation e=index->entries[i];
        if (!e.handle || e.handle<=previous || !e.length ||
            e.offset>index->section_bytes ||
            e.length>index->section_bytes-e.offset ||
            !entity_layer_handles[i] || !valid_color(&entity_colors[i]))
            return 0;
        previous=e.handle;
    }
    previous=0;
    for (size_t i=0;i<layer_count;++i) {
        const MusaDwg1032CadColor *c=&layers[i].color;
        if (!layers[i].handle || layers[i].handle<=previous ||
            !valid_color(c) ||
            (!c->has_truecolor && (c->aci==0u || c->aci==256u)))
            return 0;
        previous=layers[i].handle;
    }
    for (size_t i=0;i<n;++i) {
        const size_t at=find_layer(layers,layer_count,entity_layer_handles[i]);
        if (at==layer_count || layers[at].handle!=entity_layer_handles[i])
            return 0;
    }
    for (size_t i=0;i<n;++i) {
        MusaDwg1032ResolvedColor result={{0,0,0},0};
        const MusaDwg1032CadColor c=entity_colors[i];
        if (c.has_truecolor || (c.aci!=0u && c.aci!=256u)) {
            result.color=c;
        } else if (c.aci==0u) {
            result.byblock=1u;
        } else {
            const size_t at=find_layer(layers,layer_count,entity_layer_handles[i]);
            result.color=layers[at].color;
        }
        resolved[i]=result;
    }
    return 1;
}

/* A canonical unresolved ByBlock value carries no definition-level RGB/ACI.
 * Explicit colors must have an ACI or an actual TrueColor override. */
static int valid_resolved(const MusaDwg1032ResolvedColor *c) {
    if (c->byblock>1u) return 0;
    if (c->byblock)
        return c->color.aci==0u && c->color.has_truecolor==0u &&
               c->color.truecolor_rgb==0u;
    return valid_color(&c->color) &&
           (c->color.has_truecolor ||
            (c->color.aci>=1u && c->color.aci<=255u)) &&
           (c->color.has_truecolor || c->color.truecolor_rgb==0u);
}

int musa_dwg1032_layer_color_apply_insert(
    const MusaDwg1032ResolvedColor *definition_colors, size_t count,
    const MusaDwg1032ResolvedColor *insert_color,
    MusaDwg1032ResolvedColor *resolved) {
    if (!definition_colors || !insert_color || !resolved || !count ||
        count>SIZE_MAX/sizeof(*definition_colors)) return 0;
    const size_t bytes=count*sizeof(*definition_colors);
    if (overlaps(resolved,bytes,definition_colors,bytes) ||
        overlaps(resolved,bytes,insert_color,sizeof(*insert_color)) ||
        !valid_resolved(insert_color)) return 0;
    for (size_t i=0;i<count;++i)
        if (!valid_resolved(&definition_colors[i])) return 0;
    for (size_t i=0;i<count;++i)
        resolved[i]=definition_colors[i].byblock
            ? *insert_color : definition_colors[i];
    return 1;
}
