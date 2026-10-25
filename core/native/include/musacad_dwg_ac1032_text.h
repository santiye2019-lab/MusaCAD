/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef MUSACAD_DWG_AC1032_TEXT_H
#define MUSACAD_DWG_AC1032_TEXT_H
#include "musacad_dwg_ac1032_object_streams.h"
#include <stddef.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
/* One independently H/CRC-verified genuine R2018 TEXT (raw OT1)
 * Unicode TU string, WITHOUT guessing text placement, style, font,
 * glyph metrics, color inheritance, or final display acceptance.
 * No LibreDWG dependency; common native C11 for all operating systems.
 * Fail-atomic: output remains untouched for corrupted/bad payloads. */
typedef struct MusaDwgAc1032NativeTextContent {
    uint64_t indexed_handle;
    size_t original_string_start_bit;
    uint32_t utf16_code_units;
    uint32_t utf8_bytes;
    uint8_t original_record_crc_verified;
    uint8_t geometry_and_style_accepted; /* ALWAYS ZERO */
    char utf8[2048];
} MusaDwgAc1032NativeTextContent;
/* Original object payload bytes + source-authenticated R2018 layout.
 * The separate TU string stream must contain exactly ONE independently
 * framed UTF-16 text value. No source geometry or font is inferred. */
int musa_dwg_ac1032_text_content_at(
    const uint8_t *payload,size_t payload_bytes,
    const MusaDwgAc1032ObjectStreams *layout,
    size_t geometry_start_bit,MusaDwgAc1032NativeTextContent *out);
#ifdef __cplusplus
}
#endif
#endif
