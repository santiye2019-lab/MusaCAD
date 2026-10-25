/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef MUSACAD_DWG_AC1032_MTEXT_H
#define MUSACAD_DWG_AC1032_MTEXT_H
#include "musacad_dwg_ac1032_object_streams.h"
#include <stddef.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
/* Only one authenticated original R2018 OT44 MTEXT TU source string.
 * MTEXT formatting, paragraphs, font metrics, WCS placement, visibility,
 * owner/model-space, glyphs and rendered text remain UNRESOLVED.
 * Never reinterpret an OT1 TEXT as MTEXT or claim full MTEXT parsing. */
typedef struct MusaDwgAc1032MTextSource {
    uint64_t indexed_handle;
    size_t original_string_start_bit;
    uint32_t utf16_code_units;
    uint32_t utf8_bytes;
    uint8_t original_record_crc_verified;
    uint8_t geometry_style_and_glyphs_accepted; /* ALWAYS 0 */
    char utf8[2048];
} MusaDwgAc1032MTextSource;
/* Authenticated layout, original CRC-backed payload, strict entity common
 * boundary. Returns no output for unsupported multi-TU/extended frames. */
int musa_dwg_ac1032_mtext_single_tu_at(
    const uint8_t *payload,size_t payload_bytes,
    const MusaDwgAc1032ObjectStreams *layout,
    size_t geometry_start_bit,MusaDwgAc1032MTextSource *out);
#ifdef __cplusplus
}
#endif
#endif
