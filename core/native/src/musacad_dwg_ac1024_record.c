/* SPDX-License-Identifier: MIT
 * Original MusaCAD R2010 record-frame parser from published DWG bit codes.
 * Strictly narrower than a full AC1024 decoder, geometry or writer.
 */
#include "musacad_dwg_ac1024_record.h"
#include "musacad_dwg_ac1015_values.h"
#include "musacad_crc16.h"
#include <stdint.h>
#include <stddef.h>

static int overlaps(const void *a, size_t n, const void *b, size_t m) {
    if (!a || !b || !n || !m) return 0;
    const uintptr_t x = (uintptr_t)a, y = (uintptr_t)b;
    return x <= y ? y - x < n : x - y < m;
}

int musa_dwg_ac1024_record_probe(
    const uint8_t *objects, size_t object_bytes, uint64_t indexed_offset,
    MusaDwgAc1024RecordFrame *out) {
    if (!objects || !out || indexed_offset >= (uint64_t)object_bytes ||
        overlaps(objects, object_bytes, out, sizeof(*out)))
        return 0;
    const size_t offset = (size_t)indexed_offset;
    const size_t available = object_bytes - offset;
    if (available < 7u) return 0;
    const uint8_t *const p = objects + offset;
    const uint16_t first = (uint16_t)p[0] | ((uint16_t)p[1] << 8u);
    size_t ms_len = 2u;
    size_t body_len = first;
    if (first & 0x8000u) {
        if (available < 9u) return 0;
        const uint16_t second = (uint16_t)p[2] |
                                ((uint16_t)p[3] << 8u);
        if (!second || (second & 0x8000u)) return 0;
        ms_len = 4u;
        body_len = (size_t)(first & 0x7fffu) |
                   ((size_t)second << 15u);
    }
    if (body_len < 3u || ms_len > available ||
        body_len > available - ms_len - 2u) return 0;

    size_t mc_len = 0u;
    uint64_t handle_bits = 0u;
    int terminated = 0;
    while (mc_len < 10u) {
        if (mc_len >= available - ms_len) return 0;
        const uint8_t encoded = p[ms_len + mc_len];
        const uint64_t data = (uint64_t)(encoded & 0x7fu);
        const unsigned shift = (unsigned)(mc_len * 7u);
        if (shift >= 64u || data > (UINT64_MAX >> shift))
            return 0;
        handle_bits |= data << shift;
        ++mc_len;
        if (!(encoded & 0x80u)) {
            terminated = 1;
            break;
        }
    }
    if (!terminated || !mc_len || mc_len > UINT8_MAX ||
        mc_len > available - ms_len ||
        body_len > available - ms_len - mc_len - 2u)
        return 0;
    const size_t crc_at = ms_len + mc_len + body_len;
    uint16_t crc = 0u;
    if (!musa_core_crc16_arc(0xc0c1u, p, crc_at, &crc))
        return 0;
    const uint16_t stored = (uint16_t)p[crc_at] |
                            ((uint16_t)p[crc_at + 1u] << 8u);
    if (crc != stored) return 0;

    MusaDwgAc1015Bits bits = {0};
    if (body_len > SIZE_MAX / 8u ||
        !musa_dwg_ac1015_bits_init(
            &bits, p + ms_len + mc_len, body_len, 0u, body_len * 8u))
        return 0;
    uint32_t selector = 0u, lo = 0u, hi = 0u;
    if (!musa_dwg_ac1015_bits_read_u(&bits, 2u, &selector) ||
        !musa_dwg_ac1015_bits_read_u(&bits, 8u, &lo))
        return 0;
    uint32_t type = lo;
    if (selector == 1u) type += 0x1f0u;
    else if (selector >= 2u) {
        if (!musa_dwg_ac1015_bits_read_u(&bits, 8u, &hi))
            return 0;
        type |= hi << 8u;
    }
    if (type > UINT16_MAX) return 0;

    const MusaDwgAc1024RecordFrame candidate = {
        indexed_offset, crc_at + 2u, body_len,
        handle_bits, (uint16_t)type,
        (uint8_t)ms_len, (uint8_t)mc_len, 1u
    };
    *out = candidate;
    return 1;
}
