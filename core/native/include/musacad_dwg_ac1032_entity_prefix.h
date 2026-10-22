/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef MUSACAD_DWG_AC1032_ENTITY_PREFIX_H
#define MUSACAD_DWG_AC1032_ENTITY_PREFIX_H
#include "musacad_dwg_ac1032_object_streams.h"
#include <stddef.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
/* ORIGINAL R2018 LINE entity prefix, authenticated DATA payload precondition.
 * Parses ONLY OT followed by H self, EED size, preview flag, entity
 * mode, reactor BL, XDic, DS bit and no-link bit. R2010+ separate MC
 * handle-stream bits REPLACE the obsolete in-payload R2000 RL length
 * assumption, demonstrated by real R2018 2018/Line.dwg.
 *
 * The returned next_color_bit is NOT a geometry offset; R2018 ENC color,
 * linetype/material/visual-style, etc MUST be decoded before LINE XYZ.
 */
typedef struct MusaDwgAc1032LinePrefix {
    uint64_t indexed_handle;
    uint16_t raw_object_type;
    uint32_t reactor_count;
    size_t next_color_bit;
    uint8_t entity_mode;
    uint8_t xdictionary_missing;
    uint8_t has_binary_data_store;
    uint8_t no_prev_next_links;
    uint8_t prefix_decoded;
} MusaDwgAc1032LinePrefix;

/* Strict, deliberately LIMITED no-EED/no-graphics/no-reactors LINE
 * supported prefix. Fail-atomic, no source mutation, rejects aliases.
 * Layout must first be authenticated against original AC1032 source. */
int musa_dwg_ac1032_line_prefix_bytes(
    const uint8_t *payload,size_t payload_bytes,
    const MusaDwgAc1032ObjectStreams *layout,
    MusaDwgAc1032LinePrefix *out);
#ifdef __cplusplus
}
#endif
#endif
