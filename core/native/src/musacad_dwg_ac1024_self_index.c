/* SPDX-License-Identifier: MIT
 * Original first-party R2010 bounded self-H/record-offset reconciliation.
 * This does not read a LINE or INSERT and cannot authorize drawing output.
 */
#include "musacad_dwg_ac1024_self_index.h"
#include "musacad_dwg_ac1024_record.h"
#include "musacad_dwg_ac1015_values.h"
#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#define MUSA_AC1024_SELF_INDEX_MAX 400000u

static int overlaps(const void *a, size_t n, const void *b, size_t m) {
    if (!a || !b || !n || !m) return 0;
    const uintptr_t x = (uintptr_t)a, y = (uintptr_t)b;
    return x <= y ? y - x < n : x - y < m;
}

/* The R2010 common object DATA stream contains absolute H after OT.
 * Read ONLY canonical CODE=0 and length=1..8, in DWG high-byte first
 * order. Payload bounds already require original frame CRC and MC. */
static int extract_self_h(
    const uint8_t *objects, size_t object_bytes,
    uint64_t indexed_offset, uint64_t *out_handle,
    uint16_t *out_ot) {
    if (!objects || !out_handle || !out_ot) return 0;
    MusaDwgAc1024StreamBounds bounds = {0};
    if (!musa_dwg_ac1024_record_stream_bounds(
            objects, object_bytes, indexed_offset, &bounds) ||
        bounds.handle_stream_start_bit <= bounds.data_after_ot_bit ||
        bounds.payload_start_byte >= object_bytes)
        return 0;
    MusaDwgAc1015Bits bits = {0};
    if (!musa_dwg_ac1015_bits_init(
            &bits, objects + bounds.payload_start_byte,
            bounds.handle_stream_end_bit / 8u,
            bounds.data_after_ot_bit, bounds.handle_stream_start_bit))
        return 0;
    uint32_t code = 0u, count = 0u, byte = 0u;
    if (!musa_dwg_ac1015_bits_read_u(&bits, 4u, &code) ||
        !musa_dwg_ac1015_bits_read_u(&bits, 4u, &count) ||
        code != 0u || count < 1u || count > 8u)
        return 0;
    uint64_t handle = 0u;
    for (uint32_t j = 0u; j < count; j++) {
        if (!musa_dwg_ac1015_bits_read_u(&bits, 8u, &byte) ||
            (j == 0u && byte == 0u))
            return 0;
        handle = (handle << 8u) | (uint64_t)byte;
    }
    if (!handle) return 0;
    *out_handle = handle;
    *out_ot = bounds.raw_object_type;
    return 1;
}

int musa_dwg_ac1024_recover_self_index(
    const uint8_t *objects, size_t object_bytes,
    const uint64_t *offset_in_handle_order, size_t record_count,
    MusaDwgAc1024SelfIdentity *out, size_t capacity) {
    if (!objects || !object_bytes || !offset_in_handle_order || !out ||
        !record_count || record_count > MUSA_AC1024_SELF_INDEX_MAX ||
        record_count > capacity ||
        record_count > SIZE_MAX / sizeof(uint64_t) ||
        capacity > SIZE_MAX / sizeof(*out) ||
        record_count > SIZE_MAX / sizeof(*out))
        return 0;
    const size_t input_bytes = record_count * sizeof(uint64_t);
    const size_t output_bytes = capacity * sizeof(*out);
    if (overlaps(objects, object_bytes, offset_in_handle_order, input_bytes) ||
        overlaps(objects, object_bytes, out, output_bytes) ||
        overlaps(offset_in_handle_order, input_bytes, out, output_bytes))
        return 0;
    MusaDwgAc1024SelfIdentity *staged =
        (MusaDwgAc1024SelfIdentity *)calloc(
            record_count, sizeof(*staged));
    if (!staged) return 0;
    int valid = 1;
    uint64_t previous = 0u;
    for (size_t i = 0u; i < record_count; i++) {
        const uint64_t offset = offset_in_handle_order[i];
        uint64_t embedded = 0u;
        uint16_t raw_ot = 0u;
        if (offset >= (uint64_t)object_bytes ||
            !extract_self_h(
                objects, object_bytes, offset, &embedded, &raw_ot) ||
            embedded <= previous) {
            valid = 0;
            break;
        }
        previous = embedded;
        staged[i].embedded_absolute_handle = embedded;
        staged[i].record_offset = offset;
        staged[i].raw_ot = raw_ot;
        staged[i].frame_crc16_verified = 1u;
        staged[i].embedded_handle_code0_verified = 1u;
    }
    if (valid)
        memcpy(out, staged, record_count * sizeof(*out));
    free(staged);
    return valid;
}
