/* SPDX-License-Identifier: MIT
 * Original bounded UTF-16LE display text bridge over verified RAW pages.
 */
#ifndef MUSA_DWG1032_OBJECT_TEXT_WINDOW_H
#define MUSA_DWG1032_OBJECT_TEXT_WINDOW_H
#include "musa_dwg1032_object_window.h"
#include <stddef.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
/* Convert a caller-authenticated UTF-16LE field inside an already located
 * MS-prefixed object candidate, without assembling the whole section.
 * The caller supplies the exact payload-relative field offset and byte
 * length; no entity type, string layout, or DWG authenticity is inferred.
 * Pages and object index MUST have passed their complete validation gates
 * and remain immutable. All writable buffers, metadata and source storage
 * must be disjoint, including the entire scratch and UTF-8 capacities.
 * On failure utf8 and *written remain unchanged; scratch is disposable.
 * Empty fields are valid. O(field bytes + touched pages), caller-owned
 * O(field bytes) scratch, no allocation and no third-party CAD code. */
int musa_dwg1032_object_ms_utf16le_text(
    const uint8_t *file_bytes, size_t file_size,
    const MusaDwg1032FilePage *validated_pages, size_t page_count,
    const MusaDwg1032ObjectIndex *validated_index, uint64_t handle,
    uint64_t field_payload_offset, size_t field_byte_count,
    size_t max_payload_bytes, uint8_t *scratch, size_t scratch_capacity,
    char *utf8, size_t utf8_capacity, size_t *written);
/* Ordinal counterpart for a previously located, immutable object index.
 * Avoids a second O(log n) handle lookup when rendering a viewport page.
 * The position is a verified index ordinal, NOT a DWG byte offset.
 * Same transactional output, buffer disjointness and caller-authenticated
 * UTF-16LE field requirements as the handle-based entry point. */
int musa_dwg1032_object_ms_utf16le_text_at_position(
    const uint8_t *file_bytes, size_t file_size,
    const MusaDwg1032FilePage *validated_pages, size_t page_count,
    const MusaDwg1032ObjectIndex *validated_index, size_t position,
    uint64_t field_payload_offset, size_t field_byte_count,
    size_t max_payload_bytes, uint8_t *scratch, size_t scratch_capacity,
    char *utf8, size_t utf8_capacity, size_t *written);
#ifdef __cplusplus
}
#endif
#endif
