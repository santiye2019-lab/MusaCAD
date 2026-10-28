/* SPDX-License-Identifier: MIT
 * Independent viewport layer-mask adapter; no legacy CAD dependencies.
 */
#include "musa_scene2d_viewport.h"
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

static int ranges_overlap(const void *a, size_t na,
                          const void *b, size_t nb) {
    if (!na || !nb) return 0;
    const uintptr_t x = (uintptr_t)a, y = (uintptr_t)b;
    return x <= y ? y - x < na : x - y < nb;
}

int musa_scene2d_resolve_viewport(
    const MusaScene2dNode *nodes, size_t node_count,
    const MusaScene2dLayer *layers, size_t layer_count,
    const uint8_t *viewport_layer_visible,
    MusaScene2dResolved *out, size_t capacity, size_t *written) {
    if (!written || !layers || !viewport_layer_visible || !layer_count ||
        (node_count && (!nodes || !out)) || capacity < node_count ||
        node_count > SIZE_MAX / sizeof(*out) ||
        layer_count > SIZE_MAX / sizeof(*layers))
        return 0;

    /* A viewport mask is input data, including when the caller attempts to
     * reuse it as an output buffer. Keep all inputs immutable on failure. */
    if (ranges_overlap(out, node_count * sizeof(*out),
                       viewport_layer_visible, layer_count) ||
        ranges_overlap(written, sizeof(*written),
                       viewport_layer_visible, layer_count) ||
        ranges_overlap(out, node_count * sizeof(*out),
                       layers, layer_count * sizeof(*layers)) ||
        ranges_overlap(written, sizeof(*written),
                       layers, layer_count * sizeof(*layers)))
        return 0;

    for (size_t i = 0; i < layer_count; ++i)
        if (viewport_layer_visible[i] > 1 || layers[i].visible > 1)
            return 0;

    MusaScene2dLayer *effective = malloc(layer_count * sizeof(*effective));
    if (!effective) return 0;
    memcpy(effective, layers, layer_count * sizeof(*effective));
    for (size_t i = 0; i < layer_count; ++i)
        effective[i].visible =
            (uint8_t)(effective[i].visible && viewport_layer_visible[i]);

    const int ok = musa_scene2d_resolve(nodes, node_count,
                                       effective, layer_count,
                                       out, capacity, written);
    free(effective);
    return ok;
}
