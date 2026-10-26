/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef MUSACAD_DWG_AC1032_TEXT_H
#define MUSACAD_DWG_AC1032_TEXT_H
#include "musacad_dwg_ac1032_object_streams.h"
#include <stddef.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
/* Genuine R2018 built-in OT1 AcDbText source STRING TU, distinct from
 * OT44 MTEXT. Exact Unicode source only: NOT insertion/rotation/height,
 * font, alignment, layer, owner, visibility or rendered glyphs. */
typedef struct MusaDwgAc1032TextSource {
    uint64_t indexed_handle;
    size_t original_string_start_bit;
    uint32_t utf16_code_units,utf8_bytes;
    uint8_t original_record_crc_verified;
    uint8_t geometry_font_and_display_accepted; /* ALWAYS 0 */
    char utf8[2048];
} MusaDwgAc1032TextSource;
/* Original OT1 type/H/CRC/layout must already be authenticated.
 * Bounded single-TU source grammar; fail atomic on malformed frames. */
int musa_dwg_ac1032_text_single_tu_at(
    const uint8_t *payload,size_t payload_bytes,
    const MusaDwgAc1032ObjectStreams *layout,
    size_t geometry_start_bit,MusaDwgAc1032TextSource *out);
#ifdef __cplusplus
}
#endif
#endif
