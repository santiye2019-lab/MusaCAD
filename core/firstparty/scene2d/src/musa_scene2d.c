/* SPDX-License-Identifier: MIT
 * Original, self-contained C11 scene graph evaluator. No third-party code.
 */
#include "musa_scene2d.h"
#include <math.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
typedef struct HandleSlot { uint64_t handle; size_t index; } HandleSlot;
static int compare_slot(const void *a, const void *b) {
    const uint64_t x = ((const HandleSlot *)a)->handle;
    const uint64_t y = ((const HandleSlot *)b)->handle;
    return (x > y) - (x < y);
}
static int locate(const HandleSlot *slots, size_t count, uint64_t handle,
                  size_t *index) {
    size_t lo = 0, hi = count;
    while (lo < hi) {
        const size_t mid = lo + (hi - lo) / 2;
        if (slots[mid].handle < handle) lo = mid + 1;
        else hi = mid;
    }
    if (lo == count || slots[lo].handle != handle) return 0;
    *index = slots[lo].index;
    return 1;
}
static int overlap(const void *a, size_t na, const void *b, size_t nb) {
    if (!na || !nb) return 0;
    const uintptr_t x = (uintptr_t)a, y = (uintptr_t)b;
    return x <= y ? y - x < na : x - y < nb;
}
/* FMA prevents overflow in its multiplied operand, but the OTHER
 * product may still overflow before FMA can cancel it. Try both operand
 * orders when the first evaluation is nonfinite. Ordinary drawing
 * coordinates take only the fast path. */
static double sum_products(double a, double b, double c, double d) {
    const double result = fma(a, b, c * d);
    return isfinite(result) ? result : fma(c, d, a * b);
}
static double sum_products_offset(double a, double b, double c, double d,
                                  double offset) {
    const double result = fma(a, b, fma(c, d, offset));
    return isfinite(result) ? result : fma(c, d, fma(a, b, offset));
}
static int compose(const double parent[6], const double child[6], double dst[6]) {
    /* Evaluate both orders before rejecting a finite cancellation.
     * This is still bounded double arithmetic, not arbitrary precision. */
    dst[0] = sum_products(parent[0],child[0],parent[2],child[1]);
    dst[1] = sum_products(parent[1],child[0],parent[3],child[1]);
    dst[2] = sum_products(parent[0],child[2],parent[2],child[3]);
    dst[3] = sum_products(parent[1],child[2],parent[3],child[3]);
    dst[4] = sum_products_offset(parent[0],child[4],
                                 parent[2],child[5],parent[4]);
    dst[5] = sum_products_offset(parent[1],child[4],
                                 parent[3],child[5],parent[5]);
    for (size_t j = 0; j < 6; ++j) if (!isfinite(dst[j])) return 0;
    return 1;
}
int musa_scene2d_resolve(const MusaScene2dNode *nodes, size_t node_count,
                         const MusaScene2dLayer *layers, size_t layer_count,
                         MusaScene2dResolved *out, size_t capacity,
                         size_t *written) {
    HandleSlot *node_slots = NULL, *layer_slots = NULL;
    MusaScene2dResolved *pending = NULL;
    uint8_t *state = NULL;
    size_t *path = NULL;
    int ok = 0;
    if (!written || (!nodes && node_count) || (!out && node_count) ||
        !layers || !layer_count || capacity < node_count ||
        node_count > SIZE_MAX / sizeof(HandleSlot) ||
        layer_count > SIZE_MAX / sizeof(HandleSlot) ||
        node_count > SIZE_MAX / sizeof(MusaScene2dResolved) ||
        node_count > SIZE_MAX / sizeof(MusaScene2dNode) ||
        layer_count > SIZE_MAX / sizeof(MusaScene2dLayer) ||
        node_count > SIZE_MAX / sizeof(size_t)) return 0;
    const size_t out_bytes = node_count * sizeof(*out);
    const size_t node_bytes = node_count * sizeof(*nodes);
    const size_t layer_bytes = layer_count * sizeof(*layers);
    if (overlap(out,out_bytes,nodes,node_bytes) ||
        overlap(out,out_bytes,layers,layer_bytes) ||
        overlap(out,out_bytes,written,sizeof(*written)) ||
        overlap(written,sizeof(*written),nodes,node_bytes) ||
        overlap(written,sizeof(*written),layers,layer_bytes)) return 0;
    node_slots = malloc((node_count ? node_count : 1) * sizeof(*node_slots));
    layer_slots = malloc(layer_count * sizeof(*layer_slots));
    pending = malloc((node_count ? node_count : 1) * sizeof(*pending));
    state = calloc(node_count ? node_count : 1, sizeof(*state));
    path = malloc((node_count ? node_count : 1) * sizeof(*path));
    if (!node_slots || !layer_slots || !pending || !state || !path) goto done;
    for (size_t i = 0; i < layer_count; ++i) {
        if (layers[i].visible > 1) goto done;
        layer_slots[i] = (HandleSlot){layers[i].handle, i};
    }
    qsort(layer_slots, layer_count, sizeof(*layer_slots), compare_slot);
    for (size_t i = 1; i < layer_count; ++i)
        if (layer_slots[i-1].handle == layer_slots[i].handle) goto done;
    for (size_t i = 0; i < node_count; ++i) {
        const MusaScene2dNode *n = &nodes[i];
        if (!n->handle || n->visible > 1 ||
            (n->kind != MUSA_SCENE2D_ENTITY && n->kind != MUSA_SCENE2D_INSERT) ||
            (n->color_mode != MUSA_SCENE2D_EXPLICIT &&
             n->color_mode != MUSA_SCENE2D_BYLAYER &&
             n->color_mode != MUSA_SCENE2D_BYBLOCK)) goto done;
        for (size_t j = 0; j < 6; ++j)
            if (!isfinite(n->affine[j])) goto done;
        node_slots[i] = (HandleSlot){n->handle, i};
    }
    qsort(node_slots, node_count, sizeof(*node_slots), compare_slot);
    for (size_t i = 1; i < node_count; ++i)
        if (node_slots[i-1].handle == node_slots[i].handle) goto done;
    for (size_t i = 0; i < node_count; ++i) {
        if (state[i] == 2) continue;
        size_t depth = 0, current = i;
        for (;;) {
            if (state[current] == 2) break;
            if (state[current] == 1) goto done; /* Ownership cycle. */
            state[current] = 1;
            path[depth++] = current;
            const uint64_t parent = nodes[current].parent_insert;
            if (!parent) break;
            if (!locate(node_slots, node_count, parent, &current) ||
                nodes[current].kind != MUSA_SCENE2D_INSERT) goto done;
        }
        while (depth) {
            const size_t idx = path[--depth];
            const MusaScene2dNode *n = &nodes[idx];
            size_t parent_idx = 0, layer_idx = 0;
            const int has_parent = n->parent_insert != 0;
            if (has_parent && !locate(node_slots, node_count,
                                      n->parent_insert, &parent_idx)) goto done;
            if (has_parent && state[parent_idx] != 2) goto done;
            const MusaScene2dResolved *parent = has_parent ? &pending[parent_idx] : NULL;
            const uint64_t layer = (parent && n->layer == 0) ?
                                   parent->effective_layer : n->layer;
            if (!locate(layer_slots, layer_count, layer, &layer_idx)) goto done;
            MusaScene2dResolved *dst = &pending[idx];
            dst->handle = n->handle;
            dst->effective_layer = layer;
            if (n->color_mode == MUSA_SCENE2D_BYBLOCK) {
                if (!parent) goto done; /* No enclosing INSERT color. */
                dst->rgba = parent->rgba;
            } else if (n->color_mode == MUSA_SCENE2D_BYLAYER) {
                dst->rgba = layers[layer_idx].rgba;
            } else dst->rgba = n->rgba;
            dst->visible = (uint8_t)(n->visible && layers[layer_idx].visible &&
                                     (!parent || parent->visible));
            if (parent) {
                if (!compose(parent->world, n->affine, dst->world)) goto done;
            } else memcpy(dst->world, n->affine, sizeof(dst->world));
            state[idx] = 2;
        }
    }
    if (node_count) memcpy(out, pending, node_count * sizeof(*out));
    *written = node_count;
    ok = 1;
done:
    free(path);
    free(state);
    free(pending);
    free(layer_slots);
    free(node_slots);
    return ok;
}
