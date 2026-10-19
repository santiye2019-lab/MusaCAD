/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "musacad_dwg_ac1018_line.h"
#include "musacad_dwg_ac1015_values.h"
#include "musacad_crc16.h"
#include <math.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

/* R2004 original object size: one or two little-endian 15-bit shorts. */
static int read_ms(const uint8_t *p, size_t n, size_t *bytes, size_t *size) {
    if (!p || !bytes || !size || n < 2u) return 0;
    const uint16_t first = (uint16_t)p[0] | ((uint16_t)p[1] << 8);
    if (!(first & 0x8000u)) {
        if (first < 8u) return 0;
        *size = first; *bytes = 2u; return 1;
    }
    if (n < 4u) return 0;
    const uint16_t second = (uint16_t)p[2] | ((uint16_t)p[3] << 8);
    if (!second || (second & 0x8000u)) return 0;
    *size = ((size_t)(first & 0x7fffu)) | ((size_t)second << 15);
    *bytes = 4u;
    return *size >= 8u;
}
static int read_h_self(MusaDwgAc1015Bits *b, uint64_t *handle) {
    uint32_t header = 0u;
    if (!musa_dwg_ac1015_bits_read_u(b, 8u, &header) ||
        (header >> 4) != 0u || !(header & 15u) || (header & 15u) > 8u)
        return 0;
    uint64_t value = 0u;
    for (unsigned i = 0u; i < (header & 15u); ++i) {
        uint32_t byte = 0u;
        if (!musa_dwg_ac1015_bits_read_u(b, 8u, &byte)) return 0;
        value = (value << 8u) | (uint64_t)byte;
    }
    *handle = value;
    return 1;
}
static int read_double_pair(MusaDwgAc1015Bits *b, double *first, double *last) {
    return musa_dwg_ac1015_bits_read_rd(b, first) &&
           musa_dwg_ac1015_bits_read_dd(b, *first, last);
}
int musa_dwg_ac1018_read_line_record(const uint8_t *object_section,
    size_t section_bytes, size_t object_offset, uint64_t indexed_handle,
    MusaDwgAc1018Line *out) {
    if (!object_section || !out || !indexed_handle ||
        object_offset >= section_bytes) return 0;
    const size_t remaining = section_bytes - object_offset;
    const uint8_t *p = object_section + object_offset;
    size_t size_bytes = 0u, data_bytes = 0u;
    if (!read_ms(p, remaining, &size_bytes, &data_bytes) ||
        remaining < size_bytes + 2u ||
        data_bytes > remaining - size_bytes - 2u) return 0;
    uint16_t crc = 0u;
    if (!musa_core_crc16_arc(0xc0c1u, p, size_bytes + data_bytes, &crc))
        return 0;
    const size_t crc_at = size_bytes + data_bytes;
    if (crc != ((uint16_t)p[crc_at] | ((uint16_t)p[crc_at + 1u] << 8)))
        return 0;
    MusaDwgAc1015Bits b = {0};
    if (!musa_dwg_ac1015_bits_init(&b, p + size_bytes,
                                  data_bytes, 0u, data_bytes * 8u)) return 0;
    uint16_t type = 0u, eed_size = 0u;
    uint32_t pre_handle_bits = 0u, flag = 0u, reactors = 0u;
    uint64_t self = 0u;
    if (!musa_dwg_ac1015_bits_read_bs(&b, &type) || type != 19u ||
        !musa_dwg_ac1015_bits_read_rl(&b, &pre_handle_bits) ||
        pre_handle_bits > data_bytes * 8u ||
        !read_h_self(&b, &self) || self != indexed_handle ||
        !musa_dwg_ac1015_bits_read_bs(&b, &eed_size) || eed_size != 0u ||
        !musa_dwg_ac1015_bits_read_u(&b, 1u, &flag) || flag != 0u)
        return 0;
    MusaDwgAc1018Line result = {0};
    result.handle = indexed_handle;
    if (!musa_dwg_ac1015_bits_read_u(&b, 2u, &flag) || flag == 3u ||
        !musa_dwg_ac1015_bits_read_bl(&b, &reactors) || reactors != 0u)
        return 0;
    result.entity_mode = (uint8_t)flag;
    if (!musa_dwg_ac1015_bits_read_u(&b, 1u, &flag)) return 0;
    result.xdictionary_missing = (uint8_t)flag;
    /* AC1018: no stored nolinks bit. Parsing one would shift every coordinate
     * by one bit and produce nonfinite/misframed values. */
    if (!musa_dwg_ac1015_bits_read_bs(&b, &result.color) ||
        (result.color & 0xc000u)) return 0; /* defer truecolor / colorbook */
    if (result.color & 0x2000u) {
        uint32_t transparency = 0u;
        if (!musa_dwg_ac1015_bits_read_bl(&b, &transparency)) return 0;
    }
    if (!musa_dwg_ac1015_bits_read_bd(&b, &result.linetype_scale) ||
        !musa_dwg_ac1015_bits_read_u(&b, 2u, &flag)) return 0;
    result.linetype_flags = (uint8_t)flag;
    if (!musa_dwg_ac1015_bits_read_u(&b, 2u, &flag)) return 0;
    result.plotstyle_flags = (uint8_t)flag;
    if (!musa_dwg_ac1015_bits_read_bs(&b, &result.invisibility) ||
        !musa_dwg_ac1015_bits_read_u(&b, 8u, &flag)) return 0;
    result.lineweight = (uint8_t)flag;
    uint32_t zero_z = 0u;
    if (!musa_dwg_ac1015_bits_read_u(&b, 1u, &zero_z) ||
        !read_double_pair(&b, &result.start[0], &result.end[0]) ||
        !read_double_pair(&b, &result.start[1], &result.end[1])) return 0;
    if (!zero_z && !read_double_pair(&b, &result.start[2], &result.end[2]))
        return 0;
    if (!musa_dwg_ac1015_bits_read_bt(&b, &result.thickness) ||
        !musa_dwg_ac1015_bits_read_be(&b, result.extrusion) ||
        b.bit_pos != (size_t)pre_handle_bits) return 0;
    for (unsigned i = 0u; i < 3u; ++i) {
        if (!isfinite(result.start[i]) || !isfinite(result.end[i]) ||
            !isfinite(result.extrusion[i])) return 0;
    }
    if (!isfinite(result.thickness) || !isfinite(result.linetype_scale))
        return 0;
    *out = result;
    return 1;
}
