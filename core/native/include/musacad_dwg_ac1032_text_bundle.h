/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef MUSACAD_DWG_AC1032_TEXT_BUNDLE_H
#define MUSACAD_DWG_AC1032_TEXT_BUNDLE_H
#include "musacad_dwg_ac1032_color_provenance.h"
#ifdef __cplusplus
extern "C" {
#endif
/* Single source-authenticated OT1 TEXT evidence join for future rendering.
 * Does NOT equate a valid source record with a visible model-space glyph.
 * Font metrics, justification, owner ancestry, layer/viewport state and
 * BYLAYER/BYBLOCK/alpha are deliberately unresolved. Pure portable C11. */
typedef struct MusaDwgAc1032TextBundle {
    MusaDwgAc1032TextSource unicode;
    MusaDwgAc1032TextWorldPlacement world;
    MusaDwgAc1032TextGraph graph;
    MusaDwgAc1032LayerName layer;
    /* Raw source OT51 status, NOT viewport-resolved visibility. */
    MusaDwgAc1032LayerFlagsEvidence layer_flags;
    MusaDwgAc1032LayerBaseBitPreflight layer_bits;
    MusaAc1032ColorProvenance color;
    uint8_t source_join_verified;
    uint8_t font_glyph_layout_accepted; /* ALWAYS 0 */
    uint8_t model_viewport_visibility_accepted; /* ALWAYS 0 */
    uint8_t final_display_color_accepted; /* ALWAYS 0 */
} MusaDwgAc1032TextBundle;
/* On ANY failure out is byte-for-byte unchanged. Indexed handle must
 * identify a genuine CRC/H-authenticated OT1, never OT44 MTEXT. */
int musa_dwg_ac1032_scene_text_bundle(
    const MusaDwgAc1032SceneIndex *scene,
    uint64_t indexed_handle,MusaDwgAc1032TextBundle *out);
#ifdef __cplusplus
}
#endif
#endif
