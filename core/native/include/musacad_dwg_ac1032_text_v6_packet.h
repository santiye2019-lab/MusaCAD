/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef MUSACAD_DWG_AC1032_TEXT_V6_PACKET_H
#define MUSACAD_DWG_AC1032_TEXT_V6_PACKET_H
#include "musacad_dwg_ac1032_text_bundle.h"
#include "musacad_dwg_ac1032_preview_v6.h"
#ifdef __cplusplus
extern "C" {
#endif
/* ONE independently authenticated original R2018 OT1 TEXT to a standalone
 * native v6 type-6 diagnostic packet (NO v6 scene header). Never a full
 * glyph/font/layout/owner/viewport or effective layer-color assertion.
 * Source-invisible, oblique, mirrored, unsupported alignment, nonplanar
 * and malformed text are rejected. Source BYLAYER/BYBLOCK remains ACI7.
 * For non-left alignment the source WCS alignment anchor is used; Canvas
 * generic font can shift the real glyph baseline, so it is PROVISIONAL.
 * Read-only C11 shared by Android/iOS/Windows, no external DWG runtime. */
typedef struct MusaAc1032TextV6PacketStats {
    uint64_t source_handle,source_layer_handle;
    size_t float_words;
    uint32_t utf8_bytes;
    uint8_t source_crc_and_graph_verified;
    uint8_t source_direct_style_exact;
    uint8_t provisional_font_and_visibility; /* ALWAYS 1 */
    uint8_t final_display_accepted; /* ALWAYS 0 */
} MusaAc1032TextV6PacketStats;
/* words=NULL,capacity=0 computes length only. Invalid/unsupported source,
 * insufficient capacity or overlapping outputs fail without ANY writes. */
int musa_dwg_ac1032_text_v6_packet(
    const MusaDwgAc1032SceneIndex *scene,uint64_t handle,
    float *words,size_t capacity,MusaAc1032TextV6PacketStats *out);
#ifdef __cplusplus
}
#endif
#endif
