/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef MUSACAD_DWG_AC1032_SCENE_INDEX_H
#define MUSACAD_DWG_AC1032_SCENE_INDEX_H
#include "musacad_dwg_ac1032_handles.h"
#include "musacad_dwg_ac1032_line_file.h"
#include "musacad_dwg_ac1032_curve.h"
#include "musacad_dwg_ac1032_point.h"
#include "musacad_dwg_ac1032_lwpolyline.h"
#include "musacad_dwg_ac1032_polyline3d.h"
#include <stddef.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
/* Opaque immutable authenticated original R2018 scene INDEX, NOT an
 * editable/lossless DWG document. Original physical page verification,
 * handles and object-section extraction are performed at OPEN time, not
 * repeatedly for every entity. Owns its extracted Objects and H index;
 * the input DWG byte array may be released after open succeeds.
 * Supports one shared pure C11 ABI on Windows, iOS and Android.
 */
typedef struct MusaDwgAc1032SceneIndex MusaDwgAc1032SceneIndex;
int musa_dwg_ac1032_scene_index_open(
    const uint8_t *file,size_t bytes,MusaDwgAc1032SceneIndex **out);
void musa_dwg_ac1032_scene_index_close(MusaDwgAc1032SceneIndex *index);
/* Both information queries are O(1) on already CRC-validated H index. */
size_t musa_dwg_ac1032_scene_index_count(
    const MusaDwgAc1032SceneIndex *index);
int musa_dwg_ac1032_scene_index_entry(
    const MusaDwgAc1032SceneIndex *index,size_t at,
    MusaDwgAc1032HandleEntry *out);
/* Original record CRC and OT checked on each individual binary-search
 * access; O(log N) index lookup, independent of original file size.
 * Does not claim all indexed objects have decoded geometry.
 */
int musa_dwg_ac1032_scene_index_object_type(
    const MusaDwgAc1032SceneIndex *index,uint64_t indexed_handle,
    uint16_t *out_type);
int musa_dwg_ac1032_scene_index_read_line(
    const MusaDwgAc1032SceneIndex *index,uint64_t indexed_handle,
    MusaDwgAc1032NativeLine *out);
int musa_dwg_ac1032_scene_index_read_curve(
    const MusaDwgAc1032SceneIndex *index,uint64_t indexed_handle,
    MusaDwgAc1032NativeCurve *out);
int musa_dwg_ac1032_scene_index_read_point(
    const MusaDwgAc1032SceneIndex *index,uint64_t indexed_handle,
    MusaDwgAc1032NativePoint *out);
int musa_dwg_ac1032_scene_index_read_lwpolyline(
    const MusaDwgAc1032SceneIndex *index,uint64_t indexed_handle,
    MusaDwgAc1032LWVertex *vertices,size_t capacity,
    MusaDwgAc1032NativeLWPolyline *out);
int musa_dwg_ac1032_scene_index_read_vertex3d(
    const MusaDwgAc1032SceneIndex *index,uint64_t indexed_handle,
    MusaDwgAc1032Vertex3D *out);
#ifdef __cplusplus
}
#endif
#endif
