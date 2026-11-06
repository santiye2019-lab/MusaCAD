/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef MUSACAD_DWG_AC1024_SECTION_DIRECTORY_H
#define MUSACAD_DWG_AC1024_SECTION_DIRECTORY_H
#include <stddef.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
/* Bounded *candidate* record layout of a separately decoded R2010 section
 * map. It cannot authenticate the original DWG, section data or geometry
 * by itself. Full-byte coverage and structural consistency are required. */
typedef struct MusaDwgAc1024DirectoryShape {
    uint32_t sections;
    uint32_t referenced_pages;
    uint32_t objects_name_candidates;
    uint32_t handles_name_candidates;
    uint8_t duplicate_section_names;
} MusaDwgAc1024DirectoryShape;
/* Fail atomic. No names, source bytes, handles or geometry leave this
 * function. Caller must independently verify the original source map CRC. */
int musa_dwg_ac1024_directory_shape(
    const uint8_t *decoded,size_t bytes,MusaDwgAc1024DirectoryShape *out);
#ifdef __cplusplus
}
#endif
#endif
