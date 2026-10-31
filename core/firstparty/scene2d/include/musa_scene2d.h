/* SPDX-License-Identifier: MIT
 * Independently authored MusaCAD 2D scene resolution interface (2026-10-28).
 */
#ifndef MUSA_SCENE2D_H
#define MUSA_SCENE2D_H
#include <stddef.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
typedef enum MusaScene2dKind {
    MUSA_SCENE2D_ENTITY = 1,
    MUSA_SCENE2D_INSERT = 2
} MusaScene2dKind;
typedef enum MusaScene2dColorMode {
    MUSA_SCENE2D_EXPLICIT = 1,
    MUSA_SCENE2D_BYLAYER = 2,
    MUSA_SCENE2D_BYBLOCK = 3
} MusaScene2dColorMode;
typedef struct MusaScene2dLayer {
    uint64_t handle; /* Layer zero is a valid handle. */
    uint32_t rgba;
    uint8_t visible; /* 0 or 1; caller must incorporate frozen/off state. */
} MusaScene2dLayer;
typedef struct MusaScene2dNode {
    uint64_t handle;        /* Nonzero, unique. */
    uint64_t parent_insert; /* Zero for model-space roots. */
    uint64_t layer;         /* Layer 0 inherits enclosing INSERT's layer. */
    uint32_t rgba;          /* Used for MUSA_SCENE2D_EXPLICIT only. */
    uint8_t kind;
    uint8_t visible;        /* 0 or 1; caller must incorporate entity flags. */
    uint8_t color_mode;
    /* x' = a*x + c*y + tx; y' = b*x + d*y + ty. */
    double affine[6];       /* a,b,c,d,tx,ty */
} MusaScene2dNode;
typedef struct MusaScene2dResolved {
    uint64_t handle;
    uint64_t effective_layer;
    uint32_t rgba;
    uint8_t visible;
    double world[6];
} MusaScene2dResolved;
/* Resolves an already decoded scene; NOT a DWG parser or a renderability
 * certificate. Rejects missing/duplicate handles, non-INSERT parents,
 * ownership cycles, unresolved layers, BYBLOCK roots, nonfinite transforms.
 * Runs O((nodes+layers) log(nodes+layers)) with iterative ancestry (no
 * recursion). Preserves outputs and *written on ALL failures. Does not
 * depend on, copy, or link any legacy CAD implementation. */
int musa_scene2d_resolve(const MusaScene2dNode *nodes, size_t node_count,
                         const MusaScene2dLayer *layers, size_t layer_count,
                         MusaScene2dResolved *out, size_t capacity,
                         size_t *written);
#ifdef __cplusplus
}
#endif
#endif
