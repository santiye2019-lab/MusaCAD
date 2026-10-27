/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef MUSACAD_DWG_AC1018_HANDLES_H
#define MUSACAD_DWG_AC1018_HANDLES_H
#include <stddef.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
/* Independently decompressed and CRC-checked AcDb:Handles section.
 * The offsets are RELATIVE TO the uncompressed AcDb:AcDbObjects section
 * and are NOT DWG file offsets. A valid index does not prove geometry.
 */
typedef struct MusaDwgAc1018HandleEntry {
    uint64_t handle;
    uint64_t object_offset;
} MusaDwgAc1018HandleEntry;
/* First-party R2004 handle-map pages:
 * big-endian page length including 2-byte header, CRC-16/ARC seed 0xc0c1
 * stored big endian after each page, signed/unsigned modular deltas.
 * Handle increments persist across pages; object offsets start from ZERO
 * in EVERY data page. Terminal empty CRC-backed page is mandatory.
 *
 * Pass entries=NULL, capacity=0 to obtain validated count only.
 * With entries, capacity must hold every entry. Atomic on any error:
 * no partial entries or count are exposed; caller keeps buffers immutable
 * during both validation passes. Source bytes, output entries and out_count
 * must not overlap; aliased buffers or overflowing capacity are rejected
 * without writes. No platform-specific or CAD SDK dependency.
 */
int musa_dwg_ac1018_handles_index(const uint8_t *handle_section,
    size_t handle_bytes, size_t object_section_bytes,
    MusaDwgAc1018HandleEntry *entries, size_t capacity,
    size_t *out_count);
/* R2018 handle-map pages: both handle and logical object-location
 * deltas persist across CRC-checked pages. Preserve legacy R2004 API. */
int musa_dwg_ac1018_handles_index_continuous_offsets(
    const uint8_t *handle_section,size_t handle_bytes,
    size_t object_section_bytes,MusaDwgAc1018HandleEntry *entries,
    size_t capacity,size_t *out_count);
/* Alternative R2018 candidate: absolute first handle on each page,
 * monotonic handles, page-local object offsets. CRC and bounds checked;
 * callers must independently verify embedded original self handles. */
int musa_dwg_ac1018_handles_index_page_absolute_handles(
    const uint8_t *handle_section,size_t handle_bytes,
    size_t object_section_bytes,MusaDwgAc1018HandleEntry *entries,
    size_t capacity,size_t *out_count);
#ifdef __cplusplus
}
#endif
#endif
