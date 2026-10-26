/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef MUSACAD_DWG_AC1032_DISPLAY_READINESS_H
#define MUSACAD_DWG_AC1032_DISPLAY_READINESS_H
#include "musacad_dwg_ac1032_style_evidence.h"
#include <stddef.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
/* Original AC1032 indexed record accounting for a diagnostic 2D scene.
 * This is source-H/CRC-backed classification, NOT visible-entity count:
 * non-entity table, block and dependent records count as other_records.
 * Every known planar object is source-decoded and joined to the real layer
 * before it is marked as independently style-evidenced.
 * Includes original ELLIPSE OT35 strict source evidence where verified.
 * No complete model-space/INSERT/text/viewport/layer-off interpretation is
 * established by these fields. display_accepted MUST remain zero. */
typedef struct MusaAc1032DisplayReadiness {
    size_t indexed_records;
    size_t known_planar_records;
    size_t source_style_joined;
    size_t source_direct_aci;
    size_t source_bylayer;
    size_t source_byblock;
    size_t source_complex_or_invisible;
    size_t known_style_rejected;
    size_t other_indexed_records;
    size_t narrow_layer_aci_available;
    /* Exact direct RGB24 source subset OF source_complex_or_invisible.
     * Remaining complex colors need more resolution/viewport semantics. */
    size_t source_direct_rgb24;
    /* Subset of source_complex_or_invisible. Indicates that the ENTITY's
     * own invisibility bits were source-authenticated, not that a whole
     * drawing or layer state can be certified. */
    size_t source_entity_invisible;
    /* Source-authenticated genuine OT1 TEXT; explicitly NOT glyph paint. */
    size_t original_ot1_text_records;
    size_t original_ot1_text_joined;
    size_t original_ot1_text_rejected;
    uint8_t display_accepted;
} MusaAc1032DisplayReadiness;
/* Bounded 1M record diagnostic. Caller output is fail-atomic.
 * The original indexed source is immutable; no fallback drawing is made. */
int musa_dwg_ac1032_scene_display_readiness(
    const MusaDwgAc1032SceneIndex *scene,MusaAc1032DisplayReadiness *out);
#ifdef __cplusplus
}
#endif
#endif
