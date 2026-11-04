/* SPDX-License-Identifier: MIT
 * First-party R2010/AC1024 LINE field bridge. No external CAD code.
 */
#ifndef MUSACAD_DWG_AC1024_LINE_H
#define MUSACAD_DWG_AC1024_LINE_H
#include <stddef.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif

/* Evidence-only common DATA fields for an original CRC-verified AC1024
 * fixed OT19 LINE: owner-mode 0, zero EED/reactors, no proxy/XDictionary.
 * The R2013+ DS bit MUST NOT be consumed from an R2010 record.
 * Bit offsets refer to the record payload, never to the source file.
 * This is not layer/color inheritance, Android paint, or normal-open proof.
 */
typedef struct MusaDwgAc1024LineCommon {
    uint64_t indexed_handle;
    size_t geometry_start_bit;
    size_t data_end_bit;
    uint16_t encoded_color;
    uint16_t color_index;
    uint16_t invisibility;
    uint32_t rgb;
    uint32_t transparency;
    double linetype_scale;
    uint8_t linetype_flags;
    uint8_t plotstyle_flags;
    uint8_t material_flags;
    uint8_t shadow_flags;
    uint8_t visualstyle_flags;
    uint8_t lineweight;
    uint8_t has_rgb;
    uint8_t has_color_reference;
    uint8_t has_transparency;
    uint8_t record_crc_verified;
} MusaDwgAc1024LineCommon;

/* Both APIs preserve caller output on failure and never alter source data.
 * Their authentication scope is ONE indexed record in a separately verified
 * AcDbObjects section, not a complete source DWG or entity owner graph.
 */
int musa_dwg_ac1024_line_common_probe(
    const uint8_t *objects, size_t object_bytes, uint64_t indexed_offset,
    MusaDwgAc1024LineCommon *out);

typedef struct MusaDwgAc1024LineFields {
    uint64_t indexed_handle;
    size_t geometry_start_bit;
    size_t geometry_end_bit;
    double start[3];
    double end[3];
    double thickness;
    double extrusion[3];
    uint8_t coordinates_decoded;
} MusaDwgAc1024LineFields;

/* Reuses shared bounded RD/DD/BT/BE decoders, but ONLY after the AC1024
 * common-data boundary is authenticated from the same CRC-checked record.
 * It does not prove display, measurement, owner/layer semantics or parity
 * with the source drawing; those require independent real-DWG tests.
 */
int musa_dwg_ac1024_line_fields_probe(
    const uint8_t *objects, size_t object_bytes, uint64_t indexed_offset,
    MusaDwgAc1024LineFields *out);

#ifdef __cplusplus
}
#endif
#endif
