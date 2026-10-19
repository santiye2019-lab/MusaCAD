/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "musacad_dwg_ac1018_curve.h"
#include "musacad_dwg_ac1015_values.h"
#include "musacad_crc16.h"
#include <math.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

/* Portable, bounds-checked R2004 MS, RS CRC and compressed DWG bitcodes.
 * No LibreDWG/ODA source copied; implemented from public format rules. */
static int read_ms(const uint8_t *p, size_t n, size_t *bytes, size_t *size) {
    if (!p || !bytes || !size || n < 2u) return 0;
    const uint16_t first = (uint16_t)p[0] | ((uint16_t)p[1] << 8u);
    if ((first & 0x8000u) == 0u) {
        if (first < 8u) return 0;
        *size = first; *bytes = 2u; return 1;
    }
    if (n < 4u) return 0;
    const uint16_t second = (uint16_t)p[2] | ((uint16_t)p[3] << 8u);
    if (second == 0u || (second & 0x8000u) != 0u) return 0;
    *size = ((size_t)(first & 0x7fffu)) | ((size_t)second << 15u);
    *bytes = 4u;
    return *size >= 8u;
}
static int read_self_handle(MusaDwgAc1015Bits *b, uint64_t *handle) {
    uint32_t head = 0u;
    if (!musa_dwg_ac1015_bits_read_u(b, 8u, &head) || (head >> 4u) != 0u ||
        (head & 15u) == 0u || (head & 15u) > 8u) return 0;
    uint64_t v = 0u;
    for (unsigned i = 0u; i < (head & 15u); i++) {
        uint32_t c = 0u;
        if (!musa_dwg_ac1015_bits_read_u(b, 8u, &c)) return 0;
        v = (v << 8u) | (uint64_t)c;
    }
    *handle = v;
    return 1;
}
int musa_dwg_ac1018_read_curve_record(const uint8_t *object_section,
    size_t section_bytes, size_t object_offset, uint64_t indexed_handle,
    MusaDwgAc1018Curve *out) {
    if (!out || !object_section || !indexed_handle ||
        object_offset >= section_bytes) return 0;
    const uint8_t *p = object_section + object_offset;
    const size_t remaining = section_bytes - object_offset;
    size_t size_bytes = 0u, data_bytes = 0u;
    if (!read_ms(p, remaining, &size_bytes, &data_bytes) ||
        remaining < size_bytes + 2u ||
        data_bytes > remaining - size_bytes - 2u) return 0;
    uint16_t crc = 0u;
    if (!musa_core_crc16_arc(0xc0c1u, p, size_bytes + data_bytes, &crc))
        return 0;
    const size_t end = size_bytes + data_bytes;
    if (crc != ((uint16_t)p[end] | ((uint16_t)p[end + 1u] << 8u))) return 0;
    if (data_bytes > SIZE_MAX / 8u) return 0;
    MusaDwgAc1015Bits b = {0};
    if (!musa_dwg_ac1015_bits_init(&b, p + size_bytes, data_bytes,
                                  0u, data_bytes * 8u)) return 0;
    MusaDwgAc1018Curve value = {0};
    uint32_t pre_handle_bits = 0u, flag = 0u, reactors = 0u;
    uint16_t eed = 0u;
    if (!musa_dwg_ac1015_bits_read_bs(&b, &value.type_code) ||
        (value.type_code != 17u && value.type_code != 18u) ||
        !musa_dwg_ac1015_bits_read_rl(&b, &pre_handle_bits) ||
        (size_t)pre_handle_bits > data_bytes * 8u ||
        !read_self_handle(&b, &value.handle) ||
        value.handle != indexed_handle ||
        !musa_dwg_ac1015_bits_read_bs(&b, &eed) || eed != 0u ||
        !musa_dwg_ac1015_bits_read_u(&b, 1u, &flag) || flag != 0u ||
        !musa_dwg_ac1015_bits_read_u(&b, 2u, &flag) || flag == 3u ||
        !musa_dwg_ac1015_bits_read_bl(&b, &reactors) || reactors != 0u)
        return 0;
    value.entity_mode = (uint8_t)flag;
    if (!musa_dwg_ac1015_bits_read_u(&b, 1u, &flag)) return 0;
    value.xdictionary_missing = (uint8_t)flag;
    /* True-color and extended colorbook are deliberately not admitted yet. */
    if (!musa_dwg_ac1015_bits_read_bs(&b, &value.color) ||
        (value.color & 0xc000u) != 0u) return 0;
    if ((value.color & 0x2000u) != 0u) {
        uint32_t transparency = 0u;
        if (!musa_dwg_ac1015_bits_read_bl(&b, &transparency)) return 0;
    }
    if (!musa_dwg_ac1015_bits_read_bd(&b, &value.linetype_scale) ||
        !musa_dwg_ac1015_bits_read_u(&b, 2u, &flag)) return 0;
    value.linetype_flags = (uint8_t)flag;
    if (!musa_dwg_ac1015_bits_read_u(&b, 2u, &flag)) return 0;
    value.plotstyle_flags = (uint8_t)flag;
    if (!musa_dwg_ac1015_bits_read_bs(&b, &value.invisibility) ||
        !musa_dwg_ac1015_bits_read_u(&b, 8u, &flag)) return 0;
    value.lineweight = (uint8_t)flag;
    for (unsigned i = 0u; i < 3u; i++) {
        if (!musa_dwg_ac1015_bits_read_bd(&b, &value.center[i]) ||
            !isfinite(value.center[i])) return 0;
    }
    if (!musa_dwg_ac1015_bits_read_bd(&b, &value.radius) ||
        !(value.radius > 0.0) || !isfinite(value.radius) ||
        !musa_dwg_ac1015_bits_read_bt(&b, &value.thickness) ||
        !musa_dwg_ac1015_bits_read_be(&b, value.extrusion)) return 0;
    if (value.type_code == 17u) {
        if (!musa_dwg_ac1015_bits_read_bd(&b, &value.start_angle) ||
            !musa_dwg_ac1015_bits_read_bd(&b, &value.end_angle)) return 0;
    }
    if (b.bit_pos != (size_t)pre_handle_bits ||
        !isfinite(value.start_angle) || !isfinite(value.end_angle) ||
        !isfinite(value.thickness) || !isfinite(value.linetype_scale))
        return 0;
    for (unsigned i = 0u; i < 3u; ++i)
        if (!isfinite(value.extrusion[i])) return 0;
    *out = value;
    return 1;
}
