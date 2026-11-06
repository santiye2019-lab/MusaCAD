/* SPDX-License-Identifier: MIT
 * This module validates only decoded/caller-attested evidence, not DWG bytes.
 */
#include "musa_dwg_handoff.h"
#include <math.h>
#include <stdint.h>

static int geometry_ok(const MusaDwgHandoffEntity *e) {
    for (size_t j = 0; j < 6u; ++j)
        if (!isfinite(e->geometry[j])) return 0;
    if (e->entity_kind == MUSA_DWG_HANDOFF_CIRCLE)
        return e->geometry[2] > 0.0;
    if (e->entity_kind == MUSA_DWG_HANDOFF_INSERT) {
        const double a = e->geometry[0], b = e->geometry[1];
        const double c = e->geometry[2], d = e->geometry[3];
        const double determinant = a*d - b*c;
        return isfinite(determinant) && determinant != 0.0;
    }
    return e->entity_kind == MUSA_DWG_HANDOFF_LINE;
}
int musa_dwg_handoff_validate(
    const uint8_t *dwg_data, size_t dwg_size,
    MusaDwgRouteAdapter decoder_adapter,
    const MusaDwgHandoffEntity *rows, size_t count,
    size_t declared_drawable_count,
    int inventory_authenticated,
    int empty_model_authenticated,
    MusaDwgHandoffSummary *out) {
    if (!out || inventory_authenticated != 1 ||
        count != declared_drawable_count)
        return 0;
    const MusaDwgRoute route = musa_dwg_route_probe(dwg_data,dwg_size);
    if (route.status != MUSA_DWG_ROUTE_ADAPTER_CANDIDATE ||
        route.adapter != decoder_adapter)
        return 0;
    MusaDwgHandoffSummary staged = {route.format, count, 0u, 0u, 0u};
    if (!count) {
        if (empty_model_authenticated != 1) return 0;
        *out = staged;
        return 1;
    }
    if (!rows || empty_model_authenticated != 0) return 0;
    uint64_t previous = 0;
    for (size_t i = 0; i < count; ++i) {
        const MusaDwgHandoffEntity *e = &rows[i];
        if (!e->source_handle || e->source_handle <= previous ||
            e->source_owner_handle == e->source_handle ||
            e->record_verified != 1u || e->ownership_verified != 1u ||
            e->layer_verified != 1u || e->color_verified != 1u ||
            e->geometry_verified != 1u ||
            e->color_mode < MUSA_DWG_HANDOFF_COLOR_EXPLICIT ||
            e->color_mode > MUSA_DWG_HANDOFF_COLOR_BYBLOCK ||
            !geometry_ok(e))
            return 0;
        previous = e->source_handle;
        switch (e->entity_kind) {
            case MUSA_DWG_HANDOFF_LINE: ++staged.line_count; break;
            case MUSA_DWG_HANDOFF_CIRCLE: ++staged.circle_count; break;
            case MUSA_DWG_HANDOFF_INSERT: ++staged.insert_count; break;
            default: return 0;
        }
    }
    *out = staged;
    return 1;
}
