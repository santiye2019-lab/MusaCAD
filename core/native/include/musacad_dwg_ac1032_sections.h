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


/* Complete physical AC1032 section DIRECTORY, including unrecognized
 * sections needed to retain independent class, annotation, XREF and ACIS
 * data in a future lossless database. Raw name bytes are copied exactly.
 * Does NOT interpret any version-specific class or object payload.
 *
 * The entire section directory and each referenced physical data page are
 * validated, not just selected Objects/Handles. Duplicate section names,
 * section IDs or shared data-page IDs fail closed. Unsupported/empty or
 * encrypted sections fail closed rather than silently dropping records.
 * Caller output and original source never change on failure.
 *
 * entries=NULL/capacity=0: verified count query. A non-NULL entries
 * requires capacity for ALL directory records; no partial output.
 */
typedef struct MusaDwgAc1032SectionCatalogEntry {
    uint8_t name[64];             /* original fixed 64-byte name, NUL within */
    uint64_t logical_bytes;
    uint32_t section_id;
    uint32_t page_count;
    uint32_t compression;
    uint32_t encryption;
    uint8_t all_physical_page_checksums_verified;
} MusaDwgAc1032SectionCatalogEntry;
int musa_dwg_ac1032_section_catalog(
    const uint8_t *file,size_t file_bytes,
    MusaDwgAc1032SectionCatalogEntry *entries,size_t capacity,
    size_t *out_count);

#ifdef __cplusplus
}
#endif
#endif
