/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef MUSACAD_DWG_AC1032_STYLE_EVIDENCE_H
#define MUSACAD_DWG_AC1032_STYLE_EVIDENCE_H
#include "musacad_dwg_ac1032_color_provenance.h"
#ifdef __cplusplus
extern "C" {
#endif
/* One ORIGINAL R2018 object -> CRC-verified LAYER evidence bundle.
 * Source-color provenance, literal status BS fields and Unicode layer name
 * are authenticated separately and joined by the SAME indexed layer handle.
 * These are inputs to a future renderer, NOT permission to render:
 * color display, frozen/off/locked, XREF, per-viewport and BYBLOCK
 * inheritance are explicitly NOT decided by this function.
 * Supports the exact built-in planar subset admitted by
 * musa_dwg_ac1032_scene_planar_color_provenance. Fail-atomic output. */
typedef struct MusaAc1032PlanarStyleEvidence {
    MusaAc1032ColorProvenance color;
    MusaDwgAc1032LayerFlagsEvidence raw_layer_status;
    MusaDwgAc1032LayerName original_layer_name;
    uint8_t source_layer_join_verified;
    uint8_t visibility_resolved; /* ALWAYS zero until semantic acceptance */
    uint8_t display_style_resolved; /* ALWAYS zero until semantic acceptance */
} MusaAc1032PlanarStyleEvidence;
int musa_dwg_ac1032_scene_planar_style_evidence(
    const MusaDwgAc1032SceneIndex *scene,
    uint64_t entity_handle,MusaAc1032PlanarStyleEvidence *out);
#ifdef __cplusplus
}
#endif
#endif
