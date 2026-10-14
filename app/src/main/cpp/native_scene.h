/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef MUSACAD_NATIVE_SCENE_H
#define MUSACAD_NATIVE_SCENE_H
#include <stddef.h>
#include "dwg.h"

typedef struct {
    float *values;
    size_t count;
    size_t capacity;
    int primitives;
    double min_x,min_y,max_x,max_y;
    int has_bounds;
    int truncated;
    /* Per-page cap (floats); zero retains the legacy 9 Mi-float budget. */
    size_t max_words;
    /* Internal opt-in nested paging. Never part of the v6 wire header. */
    size_t nested_skip, nested_visited;
    int nested_paging;
} MusaNativeScene;

/*
 * Compact float stream, v6:
 * header = [6, primitiveCount, minX, minY, maxX, maxY, truncated]
 * ACI metadata      = [type*256 + ACI]
 * TrueColor metadata= [-(type*256+1), RGB24]
 * LINE   = [metadata..., x1, y1, x2, y2]
 * POLY   = [metadata..., signedPointCount, x1, y1, ...] (negative count = closed)
 * POINT  = [metadata..., x, y]
 * CURVE  = [metadata..., cx, cy, ux, uy, vx, vy, startRadians, sweepRadians]
 *          point(t) = center + U*cos(t) + V*sin(t)
 * MASK    = [metadata(type=5)..., -pointCount, x1, y1, ...] (filled canvas-background wipeout)
 * TEXT    = [metadata(type=6)..., x,y, ux,uy, vx,vy, hAlign,vAlign, byteCount, packed UTF-8/3 bytes per float]
 * Java keeps backward parsing for v1/v2/v3/v4/v5 scenes.
 */
int musa_scene_build(Dwg_Data *dwg,MusaNativeScene *scene);
/*
 * Incremental scene generator for the host acceptance path.
 * first_index and next_index are *top-level model entity* cursors, not
 * primitive offsets. Each page is an independent v6 scene. Returns:
 * 0 = complete, 1 = more pages, -3 = no drawable model,
 * -4 = single top-level entity (possibly an expanded block) exceeds page.
 * Never silently returns a partial top-level entity.
 * Android's existing first-paint path is unaffected until explicitly wired.
 */
int musa_scene_build_page(Dwg_Data *dwg,MusaNativeScene *scene,
                          size_t first_index,size_t *next_index,
                          size_t max_words);
/*
 * Resumable host-only block paging: a two-part cursor (top-level model index,
 * leaf index inside the recursively expanded top-level entity).
 * Can split nested INSERT/MINSERT/DIMENSION expansion only at whole leaf
 * entity boundaries. A single oversized leaf still fails closed with -4.
 * This experimental replay based path is not yet connected to Android UI.
 */
int musa_scene_build_page_nested(Dwg_Data *dwg,MusaNativeScene *scene,
                                 size_t first_index,size_t first_leaf,
                                 size_t *next_index,size_t *next_leaf,
                                 size_t max_words);
/*
 * Equivalent bounded page builder with a pre-indexed MODEL-SPACE entity list.
 * The index contains non-owning Dwg_Object pointers in the precise same
 * order as get_first/get_next_owned_entity. Its lifetime MUST be shorter
 * than its Dwg_Data owner. The (top,leaf) cursor and float output are identical
 * to the non-indexed function; the index skips O(top) prefix walks per page.
 * Deep INSERT leaf replay is NOT accelerated by this top-level index.
 */
int musa_scene_build_page_nested_indexed(Dwg_Data *dwg,MusaNativeScene *scene,
                                         size_t first_index,size_t first_leaf,
                                         size_t *next_index,size_t *next_leaf,
                                         size_t max_words,
                                         Dwg_Object *const *model_entities,
                                         size_t model_count);
void musa_scene_free(MusaNativeScene *scene);

#endif
