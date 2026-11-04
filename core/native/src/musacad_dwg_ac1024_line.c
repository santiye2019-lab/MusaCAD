/* SPDX-License-Identifier: MIT
 * Original AC1024/R2010 bounded LINE common-data and coordinate adapter.
 * Reuses existing MusaCAD bit numeric APIs, not another drawing engine.
 */
#include "musacad_dwg_ac1024_line.h"
#include "musacad_dwg_ac1024_self_index.h"
#include "musacad_dwg_ac1015_values.h"
#include <math.h>
#include <stddef.h>
#include <stdint.h>

static int overlap(const void *a, size_t an, const void *b, size_t bn) {
    if (!a || !b || !an || !bn) return 0;
    const uintptr_t x = (uintptr_t)a, y = (uintptr_t)b;
    return x <= y ? y - x < an : x - y < bn;
}

int musa_dwg_ac1024_line_common_probe(
    const uint8_t *objects, size_t object_bytes, uint64_t indexed_offset,
    MusaDwgAc1024LineCommon *out) {
    if (!objects || !out || !object_bytes ||
        overlap(objects, object_bytes, out, sizeof(*out))) return 0;

    MusaDwgAc1024EntityMetaPrefix meta = {0};
    if (!musa_dwg_ac1024_record_entity_meta_prefix(
            objects, object_bytes, indexed_offset, &meta) ||
        meta.raw_ot != 19u || !meta.embedded_absolute_handle ||
        !meta.original_crc16_verified ||
        meta.entity_mode != 0u || meta.reactor_count != 0u ||
        meta.xdictionary_missing != 1u ||
        meta.payload_start_byte >= object_bytes ||
        meta.handle_stream_end_bit / 8u >
            object_bytes - meta.payload_start_byte ||
        meta.data_after_xdic_bit >= meta.handle_stream_start_bit)
        return 0;

    MusaDwgAc1015Bits b = {0};
    if (!musa_dwg_ac1015_bits_init(
            &b, objects + meta.payload_start_byte,
            meta.handle_stream_end_bit / 8u,
            meta.data_after_xdic_bit, meta.handle_stream_start_bit))
        return 0;
    MusaDwgAc1024LineCommon next = {0};
    next.indexed_handle = meta.embedded_absolute_handle;
    next.data_end_bit = meta.handle_stream_start_bit;
    if (!musa_dwg_ac1015_bits_read_bs(&b, &next.encoded_color))
        return 0;
    const uint16_t flags = (uint16_t)(next.encoded_color & 0xe000u);
    next.color_index = (uint16_t)(next.encoded_color & 0x1fffu);
    if ((flags & 0x4000u) && !(flags & 0x8000u)) return 0;
    if (flags & 0x8000u) {
        if (!musa_dwg_ac1015_bits_read_bl(&b, &next.rgb) ||
            next.rgb > 0x00ffffffu) return 0;
        next.has_rgb = 1u;
        next.has_color_reference = (uint8_t)((flags & 0x4000u) != 0u);
    } else if (next.color_index > 256u) return 0;
    if (flags & 0x2000u) {
        if (!musa_dwg_ac1015_bits_read_bl(&b, &next.transparency))
            return 0;
        const uint32_t mode = (next.transparency >> 24u) & 0xffu;
        if (mode != 0u && mode != 1u && mode != 3u) return 0;
        next.has_transparency = 1u;
    }
    uint32_t ltype = 0u, plot = 0u, material = 0u;
    uint32_t shadow = 0u, full = 0u, face = 0u, edge = 0u;
    uint32_t weight = 0u;
    if (!musa_dwg_ac1015_bits_read_bd(&b, &next.linetype_scale) ||
        !isfinite(next.linetype_scale) || next.linetype_scale <= 0.0 ||
        !musa_dwg_ac1015_bits_read_u(&b, 2u, &ltype) ||
        !musa_dwg_ac1015_bits_read_u(&b, 2u, &plot) ||
        !musa_dwg_ac1015_bits_read_u(&b, 2u, &material) ||
        !musa_dwg_ac1015_bits_read_u(&b, 8u, &shadow) ||
        !musa_dwg_ac1015_bits_read_u(&b, 1u, &full) ||
        !musa_dwg_ac1015_bits_read_u(&b, 1u, &face) ||
        !musa_dwg_ac1015_bits_read_u(&b, 1u, &edge) ||
        !musa_dwg_ac1015_bits_read_bs(&b, &next.invisibility) ||
        !musa_dwg_ac1015_bits_read_u(&b, 8u, &weight) ||
        b.bit_pos >= b.bit_end) return 0;
    next.linetype_flags = (uint8_t)ltype;
    next.plotstyle_flags = (uint8_t)plot;
    next.material_flags = (uint8_t)material;
    next.shadow_flags = (uint8_t)shadow;
    next.visualstyle_flags =
        (uint8_t)(full | (face << 1u) | (edge << 2u));
    next.lineweight = (uint8_t)weight;
    next.geometry_start_bit = b.bit_pos;
    next.record_crc_verified = 1u;
    *out = next;
    return 1;
}

int musa_dwg_ac1024_line_fields_probe(
    const uint8_t *objects, size_t object_bytes, uint64_t indexed_offset,
    MusaDwgAc1024LineFields *out) {
    if (!objects || !out || !object_bytes ||
        overlap(objects, object_bytes, out, sizeof(*out))) return 0;

    MusaDwgAc1024EntityMetaPrefix meta = {0};
    MusaDwgAc1024LineCommon common = {0};
    if (!musa_dwg_ac1024_record_entity_meta_prefix(
            objects, object_bytes, indexed_offset, &meta) ||
        !musa_dwg_ac1024_line_common_probe(
            objects, object_bytes, indexed_offset, &common) ||
        common.indexed_handle != meta.embedded_absolute_handle ||
        !common.record_crc_verified ||
        common.geometry_start_bit >= common.data_end_bit ||
        meta.handle_stream_end_bit / 8u >
            object_bytes - meta.payload_start_byte) return 0;

    MusaDwgAc1015Bits b = {0};
    if (!musa_dwg_ac1015_bits_init(
            &b, objects + meta.payload_start_byte,
            meta.handle_stream_end_bit / 8u,
            common.geometry_start_bit, common.data_end_bit)) return 0;
    MusaDwgAc1024LineFields next = {0};
    next.indexed_handle = common.indexed_handle;
    next.geometry_start_bit = common.geometry_start_bit;
    uint32_t zero_z = 0u;
    if (!musa_dwg_ac1015_bits_read_u(&b, 1u, &zero_z) ||
        !musa_dwg_ac1015_bits_read_rd(&b, &next.start[0]) ||
        !musa_dwg_ac1015_bits_read_dd(&b, next.start[0], &next.end[0]) ||
        !musa_dwg_ac1015_bits_read_rd(&b, &next.start[1]) ||
        !musa_dwg_ac1015_bits_read_dd(&b, next.start[1], &next.end[1]))
        return 0;
    if (!zero_z &&
        (!musa_dwg_ac1015_bits_read_rd(&b, &next.start[2]) ||
         !musa_dwg_ac1015_bits_read_dd(&b, next.start[2], &next.end[2])))
        return 0;
    if (!musa_dwg_ac1015_bits_read_bt(&b, &next.thickness) ||
        !musa_dwg_ac1015_bits_read_be(&b, next.extrusion))
        return 0;
    for (unsigned i = 0u; i < 3u; ++i) {
        if (!isfinite(next.start[i]) || !isfinite(next.end[i]) ||
            !isfinite(next.extrusion[i])) return 0;
    }
    if (!isfinite(next.thickness) ||
        (next.extrusion[0] == 0.0 && next.extrusion[1] == 0.0 &&
         next.extrusion[2] == 0.0)) return 0;
    next.geometry_end_bit = b.bit_pos;
    next.coordinates_decoded = 1u;
    *out = next;
    return 1;
}
