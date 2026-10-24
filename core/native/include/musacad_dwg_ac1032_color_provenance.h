/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef MUSACAD_DWG_AC1032_COLOR_PROVENANCE_H
#define MUSACAD_DWG_AC1032_COLOR_PROVENANCE_H
#include "musacad_dwg_ac1032_scene_index.h"
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
/* Original real AC1032 built-in LINE/POINT color evidence, not a renderer
 * color resolver. Every geometry entity and LAYER lookup uses source
 * CRC/H and independent first-party geometry/common parsers. This records
 * BOTH independently sourced fields, including opaque/unknown cases.
 *
 * IMPORTANT: Layer0 CMC C3 ACI7 is a narrow source evidence profile only.
 * Frozen/off, viewport overrides, transparency, BYBLOCK ancestry and
 * full TrueColor are NOT authenticated, so rendering_accepted is always 0.
 * Never silently promote 'source_layer_aci' to a final visible color. */
typedef struct MusaAc1032ColorProvenance {
    uint64_t entity_handle;
    uint64_t layer_handle;
    uint16_t raw_entity_type;
    uint16_t entity_aci;
    uint8_t entity_has_direct_aci;
    uint8_t entity_by_layer;
    uint8_t entity_by_block;
    uint8_t entity_complex_color_or_transparency;
    uint8_t source_layer_aci_supported;
    uint8_t source_layer_aci;
    uint8_t entity_and_layer_crc_verified;
    uint8_t rendering_accepted; /* must remain 0 until style gates pass */
} MusaAc1032ColorProvenance;
/* Source-authenticated original AC1032 planar entity color evidence for
 * ARC, CIRCLE, LINE, POINT and LWPOLYLINE (OT77). The latter requires
 * strict first-party full source geometry/vertex decoding; malformed,
 * unbounded or unsupported vertices must not yield invented color.
 * This API NEVER resolves display-ready layer visibility, inheritance,
 * BYBLOCK context, transparency or TrueColor (rendering_accepted=0).
 * LINE/POINT-only entry point below retains its earlier narrow contract. */
int musa_dwg_ac1032_scene_planar_color_provenance(
    const MusaDwgAc1032SceneIndex *scene,
    uint64_t entity_handle,MusaAc1032ColorProvenance *out);
int musa_dwg_ac1032_scene_line_point_color_provenance(
    const MusaDwgAc1032SceneIndex *scene,
    uint64_t entity_handle,MusaAc1032ColorProvenance *out);
#ifdef __cplusplus
}
#endif
#endif
