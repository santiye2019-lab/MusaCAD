/* SPDX-License-Identifier: MIT
 * Independent source-evidence gate between a verified DWG decoder and 2D scene.
 * This does NOT decode AC1032 bytes or enable production DWG opening.
 */
#ifndef MUSA_DWG1032_SCENE_HANDOFF_H
#define MUSA_DWG1032_SCENE_HANDOFF_H
#include <stddef.h>
#include <stdint.h>

typedef struct MusaDwg1032SceneEvidence {
    uint64_t source_handle;
    uint64_t owner_handle; /* 0 only for a verified root-owned entity */
    uint64_t layer_handle;
    uint32_t source_color;
    uint32_t entity_kind;
    unsigned char owner_verified;
    unsigned char layer_verified;
    unsigned char color_verified;
    unsigned char geometry_verified;
} MusaDwg1032SceneEvidence;

/* Requires strictly increasing, nonzero source handles and complete source
 * evidence for EVERY decoded entity. Zero entities is valid only when the
 * caller has independently authenticated an empty model.
 * Rejects unknown entity types (kind=0), unresolved owner/layer/color and
 * geometry; does not guess defaults or silently skip unsupported objects.
 * O(count), no allocation and no output side effects. */
static inline int musa_dwg1032_scene_handoff_validate(
    const MusaDwg1032SceneEvidence *entities, size_t count,
    int authenticated_empty_model) {
    if (!count) return authenticated_empty_model == 1;
    if (!entities || authenticated_empty_model) return 0;
    uint64_t previous = 0;
    for (size_t i = 0; i < count; ++i) {
        const MusaDwg1032SceneEvidence *e = &entities[i];
        if (!e->source_handle || e->source_handle <= previous ||
            !e->layer_handle || !e->entity_kind ||
            e->owner_verified != 1u || e->layer_verified != 1u ||
            e->color_verified != 1u || e->geometry_verified != 1u ||
            e->owner_handle == e->source_handle)
            return 0;
        previous = e->source_handle;
    }
    return 1;
}
#endif
