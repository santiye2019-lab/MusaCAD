/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef MUSACAD_DWG_AC1032_SECTIONS_H
#define MUSACAD_DWG_AC1032_SECTIONS_H
#include "musacad_dwg_ac1018_sections.h"
#ifdef __cplusplus
extern "C" {
#endif
/* First-party AC1032/R2018 PHYSICAL section page and payload reader.
 * The documented R2018 physical section-map/page format uses the R2004
 * physical envelope. Reuse a single native C11 envelope validator for
 * both versions but NEVER interpret R2018 object payloads as R2004 AcDb
 * records: actual R2018 CLASS/OBJECT/HANDLES grammar is NOT covered here.
 *
 * Section queries validate the complete protected file/page maps and
 * per-section page data/header Adler checksums. A successful extraction
 * contains opaque, originally decompressed section bytes ONLY.
 *
 * These are physical organization and checksum APIs, NOT AutoCAD 2027
 * semantic correctness, read/write parity, or a DWG save operation.
 * Fail closed, no source writes, outputs unchanged on failure. */
typedef MusaDwgAc1018SectionKind MusaDwgAc1032SectionKind;
typedef MusaDwgAc1018SectionDescriptor MusaDwgAc1032SectionDescriptor;
typedef MusaDwgAc1018SectionPage MusaDwgAc1032SectionPage;

#define MUSA_DWG_AC1032_OBJECTS MUSA_DWG_AC1018_OBJECTS
#define MUSA_DWG_AC1032_HANDLES MUSA_DWG_AC1018_HANDLES

int musa_dwg_ac1032_section_pages(
    const uint8_t *file,size_t file_bytes,MusaDwgAc1032SectionKind kind,
    MusaDwgAc1032SectionPage *pages,size_t capacity,
    MusaDwgAc1032SectionDescriptor *desc,size_t *out_page_count);

int musa_dwg_ac1032_extract_section(
    const uint8_t *file,size_t file_bytes,MusaDwgAc1032SectionKind kind,
    uint8_t *output,size_t capacity,size_t *out_bytes);

#ifdef __cplusplus
}
#endif
#endif
