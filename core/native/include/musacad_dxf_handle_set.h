/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef MUSACAD_DXF_HANDLE_SET_H
#define MUSACAD_DXF_HANDLE_SET_H
#include <stddef.h>
#ifdef __cplusplus
extern "C" {
#endif
/*
 * Bounded, portable source-identity set for staged DXF save verification.
 * All 5/105 IDs in HEADER/TABLES/BLOCKS/ENTITIES/OBJECTS are interpreted
 * numerically, so 000a == A == a. Zero and non-hex IDs are forbidden.
 * This API is not a full DXF/DWG document parser or a DWG handle resolver.
 */
typedef struct MusaDxfHandleSet MusaDxfHandleSet;
enum MusaDxfHandleInsert {
    MUSA_DXF_HANDLE_ADDED=1,
    MUSA_DXF_HANDLE_DUPLICATE=0,
    MUSA_DXF_HANDLE_INVALID=-1,
    MUSA_DXF_HANDLE_LIMIT=-2,
    MUSA_DXF_HANDLE_NO_MEMORY=-3
};
/* Maximum 1,500,000 distinct IDs; allocation grows as needed (bounded). */
MusaDxfHandleSet *musa_core_dxf_handle_set_create(size_t maximum);
int musa_core_dxf_handle_set_insert(MusaDxfHandleSet *set,
                                    const char *text,size_t length);
size_t musa_core_dxf_handle_set_count(const MusaDxfHandleSet *set);
void musa_core_dxf_handle_set_destroy(MusaDxfHandleSet *set);
#ifdef __cplusplus
}
#endif
#endif
