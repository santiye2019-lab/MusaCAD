/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef MUSACAD_DWG_AC1032_INDEXED_AUDIT_H
#define MUSACAD_DWG_AC1032_INDEXED_AUDIT_H
#include <stddef.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif

/* Original R2018 AcDbObjects object-map inventory. Types are RAW numeric OT
 * hints only: candidate does NOT mean decoded geometry, visibility, model
 * space ownership, non-entity/AcDb class identity or full drawing parity.
 * Unknown types are RETAINED and counted as opaque, never silently lost. */
#define MUSA_AC1032_INDEXED_OPAQUE 0u
#define MUSA_AC1032_INDEXED_GEOMETRY_CANDIDATE 1u
#define MUSA_AC1032_INDEXED_DEPENDENT 2u
typedef struct MusaDwgAc1032IndexedRecord {
    uint64_t indexed_handle;
    uint64_t object_section_offset;
    uint32_t framed_record_bytes;
    uint16_t raw_object_type;
    uint8_t category;
    uint8_t record_crc_verified;
} MusaDwgAc1032IndexedRecord;
typedef struct MusaDwgAc1032IndexedAudit {
    size_t indexed_records;
    size_t geometry_candidates;
    size_t dependent_records;
    size_t opaque_records;
    uint8_t all_indexed_record_crcs_verified;
    uint8_t no_indexed_record_overlap;
} MusaDwgAc1032IndexedAudit;

/* Authenticates EVERY referenced original indexed record and its exact
 * length/CRC and verifies disjoint record byte ranges in uncompressed
 * AcDbObjects, not merely the index or recognized geometry. Reads protected
 * sections once into bounded first-party C11 buffers (no external CAD SDK).
 *
 * rows=NULL,capacity=0 gives a full audit/count only. Otherwise the array
 * must have room for ALL indexed records. Output is ascending indexed H.
 * Any corrupt/unsupported record framing, non-disjoint indexed source
 * ranges, alias, insufficient capacity or physical CRC error is FATAL:
 * all caller outputs and the immutable DWG remain unchanged.
 * This is NOT a scene loader, geometry emitter, owner verifier, renderer,
 * or DWG writer. A success may include opaque/unclassified objects. */
int musa_dwg_ac1032_audit_indexed_objects_file(
    const uint8_t *file,size_t file_bytes,
    MusaDwgAc1032IndexedRecord *rows,size_t capacity,
    MusaDwgAc1032IndexedAudit *out);
#ifdef __cplusplus
}
#endif
#endif
