/* SPDX-License-Identifier: MIT
 * Strict predecoded source-evidence admission for three priority DWG readers.
 * NOT a decoder, scene renderer, cryptographic proof, or production opener.
 */
#ifndef MUSA_DWG_HANDOFF_H
#define MUSA_DWG_HANDOFF_H
#include "musa_dwg_route.h"
#include <stddef.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
typedef enum MusaDwgHandoffKind {
    MUSA_DWG_HANDOFF_LINE = 1,
    MUSA_DWG_HANDOFF_CIRCLE = 2,
    MUSA_DWG_HANDOFF_INSERT = 3
} MusaDwgHandoffKind;
typedef enum MusaDwgHandoffColor {
    MUSA_DWG_HANDOFF_COLOR_EXPLICIT = 1,
    MUSA_DWG_HANDOFF_COLOR_BYLAYER = 2,
    MUSA_DWG_HANDOFF_COLOR_BYBLOCK = 3
} MusaDwgHandoffColor;
/* All provenance flags are caller attestations: only a real independent
 * version decoder + source oracle can establish their truth.
 *
 * LINE: geometry[0..3] are x0,y0,x1,y1 in a declared 2D WCS slice.
 * CIRCLE: geometry[0..2] are center x,center y,radius (positive).
 * INSERT: geometry[0..5] is a finite, nonsingular 2D affine transform,
 *         not a complete DWG BLOCK_HEADER/attribute/OCS certificate.
 * Source layer handle zero is allowed (DWG layer 0); owner zero is allowed
 * only if a caller has independently verified root ownership.
 */
typedef struct MusaDwgHandoffEntity {
    uint64_t source_handle;
    uint64_t source_owner_handle;
    uint64_t source_layer_handle;
    uint32_t entity_kind;
    uint32_t color_mode;
    double geometry[6];
    uint8_t record_verified;
    uint8_t ownership_verified;
    uint8_t layer_verified;
    uint8_t color_verified;
    uint8_t geometry_verified;
} MusaDwgHandoffEntity;

typedef struct MusaDwgHandoffSummary {
    MusaDwgRouteFormat source_format;
    size_t total;
    size_t line_count;
    size_t circle_count;
    size_t insert_count;
} MusaDwgHandoffSummary;

/* Fail-closed on wrong adapter, incomplete source evidence, unknown entities,
 * duplicates, missing records, nonfinite 2D geometry or malformed header.
 * All admitted rows must be strictly sorted by original unique handle.
 * 'declared_drawable_count' is an externally authenticated count of *all*
 * drawable source records, not a count of records the reader happened to
 * decode. Caller MUST reject unknown drawable object families before asking
 * for full-scene admission; this gate cannot discover them from raw bytes.
 * 0 rows allowed only with externally authenticated empty model.
 * Returns 1 only for an internally consistent, caller-attested predecoded
 * packet. Output remains unchanged on every failure.
 */
int musa_dwg_handoff_validate(
    const uint8_t *dwg_data, size_t dwg_size,
    MusaDwgRouteAdapter decoder_adapter,
    const MusaDwgHandoffEntity *rows, size_t count,
    size_t declared_drawable_count,
    int inventory_authenticated,
    int empty_model_authenticated,
    MusaDwgHandoffSummary *out);

#ifdef __cplusplus
}
#endif
#endif
