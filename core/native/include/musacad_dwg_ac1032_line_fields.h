/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef MUSACAD_DWG_AC1032_LINE_FIELDS_H
#define MUSACAD_DWG_AC1032_LINE_FIELDS_H
#include "musacad_dwg_ac1032_object_streams.h"
#include <stddef.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
/* Original R2018 LINE (fixed numeric OT=19) geometry field sequence.
 * This is a FIELD reader only: geometry_start_bit is supplied by a
 * separately authenticated common ENTITY-prefix decoder. The common
 * prefix is NOT yet decoded by this API and the caller MUST NOT pass
 * an offset discovered through scanning/guessing. No real-DWG LINE
 * geometry claim follows merely from successful synthetic field tests.
 */
typedef struct MusaDwgAc1032LineFields {
    uint64_t indexed_handle;
    uint64_t geometry_start_bit;
    uint64_t geometry_end_bit;
    double start[3];
    double end[3];
    double thickness;
    double extrusion[3];
    uint8_t coordinates_decoded;
} MusaDwgAc1032LineFields;

/* Decoder for R2000+ documented LINE fields, reused bit primitives:
 * Z-zero flag, start RD and end DD for X/Y and optionally Z,
 * BT thickness, BE extrusion. Enforces verified DATA boundary,
 * numeric validity, object type 19 and failure-atomic output.
 * Operates on an immutable original payload copy, does NOT locate
 * an entity's common-prefix endpoint or accept a real file itself.
 */
int musa_dwg_ac1032_line_fields_at(
    const uint8_t *payload,size_t payload_bytes,
    const MusaDwgAc1032ObjectStreams *layout,size_t geometry_start_bit,
    MusaDwgAc1032LineFields *out);
#ifdef __cplusplus
}
#endif
#endif
