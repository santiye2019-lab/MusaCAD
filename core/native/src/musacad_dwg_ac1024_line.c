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
        /* R2010 mode 0 has an explicit owner H; modes 1 and 2 are
         * implicit paper/model space and DO NOT carry an owner reference.
         * This is DATA-only geometry evidence: owner/layer resolution
         * remains a distinct verified graph gate before scene admission. */
        meta.entity_mode > 2u || meta.reactor_count != 0u ||
        /* An XDictionary-present flag adds an H-stream reference, not
         * a DATA-stream offset. A CRC-authenticated LINE may expose
         * provisional DATA-only geometry here; no owner/layer/scene
         * graph may use that geometry until the XDictionary H is
         * separately validated. See ODA R2004+ common entity fields. */
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

/* Never elevate a successfully decoded implicit-space LINE into a scene
 * through a guessed owner. Explicit-owner graph evidence is separate
 * from provisional DATA-only geometry and comes from the exact same
 * authenticated original record. */
int musa_dwg_ac1024_line_graph_probe(
    const uint8_t *objects, size_t object_bytes,
    const MusaDwgAc1024SelfIdentity *recovered_index, size_t record_count,
    uint64_t canonical_line_handle, MusaDwgAc1024LineGraphEvidence *out) {
    if (!objects || !object_bytes || !recovered_index || !record_count ||
        !canonical_line_handle || !out ||
        record_count > SIZE_MAX / sizeof(*recovered_index) ||
        overlap(objects, object_bytes, out, sizeof(*out)) ||
        overlap(recovered_index, record_count * sizeof(*recovered_index),
                out, sizeof(*out)))
        return 0;

    MusaDwgAc1024SelfIdentity identity = {0};
    if (!musa_dwg_ac1024_self_index_find(
            recovered_index, record_count, canonical_line_handle,
            &identity) ||
        identity.raw_ot != 19u ||
        !identity.frame_crc16_verified ||
        !identity.embedded_handle_code0_verified)
        return 0;

    MusaDwgAc1024EntityMetaPrefix meta = {0};
    MusaDwgAc1024LineCommon common = {0};
    MusaDwgAc1024LineGraphEvidence next = {0};
    if (!musa_dwg_ac1024_record_entity_meta_prefix(
            objects, object_bytes, identity.record_offset, &meta) ||
        meta.raw_ot != 19u || meta.entity_mode != 0u ||
        meta.embedded_absolute_handle != canonical_line_handle ||
        !meta.original_crc16_verified ||
        !musa_dwg_ac1024_line_common_probe(
            objects, object_bytes, identity.record_offset, &common) ||
        !musa_dwg_ac1024_line_fields_probe(
            objects, object_bytes, identity.record_offset, &next.geometry) ||
        !musa_dwg_ac1024_entity_owner_layer_probe(
            objects, object_bytes, recovered_index, record_count,
            canonical_line_handle, &next.owner_layer) ||
        !common.record_crc_verified ||
        common.indexed_handle != canonical_line_handle ||
        next.geometry.indexed_handle != canonical_line_handle ||
        !next.geometry.coordinates_decoded ||
        next.geometry.geometry_start_bit != common.geometry_start_bit ||
        next.geometry.geometry_end_bit <= common.geometry_start_bit ||
        next.geometry.geometry_end_bit > common.data_end_bit ||
        next.owner_layer.entity_handle != canonical_line_handle ||
        next.owner_layer.entity_record_offset != identity.record_offset ||
        next.owner_layer.entity_raw_ot != 19u ||
        !next.owner_layer.source_crc_verified ||
        !next.owner_layer.target_records_verified)
        return 0;
    *out = next;
    return 1;
}

/* R2010 mode 1/2 lack an explicit owner H. Verify the original
 * model/paper-space LINE geometry and independently authenticated OT51
 * layer from the exact same recovered self-H index. Do NOT accept any
 * implied owner/layout and never emit scene geometry here. */
int musa_dwg_ac1024_line_implicit_layer_probe(
    const uint8_t *objects, size_t object_bytes,
    const MusaDwgAc1024SelfIdentity *recovered_index, size_t record_count,
    uint64_t canonical_line_handle,
    MusaDwgAc1024ImplicitLineLayerEvidence *out) {
    if (!objects || !object_bytes || !recovered_index || !record_count ||
        !canonical_line_handle || !out ||
        record_count > SIZE_MAX / sizeof(*recovered_index) ||
        overlap(objects, object_bytes, out, sizeof(*out)) ||
        overlap(recovered_index, record_count * sizeof(*recovered_index),
                out, sizeof(*out)))
        return 0;

    MusaDwgAc1024SelfIdentity identity = {0};
    if (!musa_dwg_ac1024_self_index_find(
            recovered_index, record_count, canonical_line_handle,
            &identity) ||
        identity.raw_ot != 19u ||
        !identity.frame_crc16_verified ||
        !identity.embedded_handle_code0_verified)
        return 0;
    MusaDwgAc1024EntityMetaPrefix meta = {0};
    MusaDwgAc1024LineCommon common = {0};
    MusaDwgAc1024ImplicitLineLayerEvidence next = {0};
    if (!musa_dwg_ac1024_record_entity_meta_prefix(
            objects, object_bytes, identity.record_offset, &meta) ||
        meta.raw_ot != 19u ||
        (meta.entity_mode != 1u && meta.entity_mode != 2u) ||
        meta.embedded_absolute_handle != canonical_line_handle ||
        !meta.original_crc16_verified ||
        !musa_dwg_ac1024_line_common_probe(
            objects, object_bytes, identity.record_offset, &common) ||
        !musa_dwg_ac1024_line_fields_probe(
            objects, object_bytes, identity.record_offset, &next.geometry) ||
        !musa_dwg_ac1024_implicit_layer_probe(
            objects, object_bytes, recovered_index, record_count,
            canonical_line_handle, &next.implicit_layer) ||
        !common.record_crc_verified ||
        common.indexed_handle != canonical_line_handle ||
        !next.geometry.coordinates_decoded ||
        next.geometry.indexed_handle != canonical_line_handle ||
        next.geometry.geometry_start_bit != common.geometry_start_bit ||
        next.geometry.geometry_end_bit <= common.geometry_start_bit ||
        next.geometry.geometry_end_bit > common.data_end_bit ||
        next.implicit_layer.entity_handle != canonical_line_handle ||
        next.implicit_layer.entity_record_offset != identity.record_offset ||
        next.implicit_layer.entity_raw_ot != 19u ||
        next.implicit_layer.entity_mode != meta.entity_mode ||
        !next.implicit_layer.source_crc_verified ||
        !next.implicit_layer.layer_crc_verified)
        return 0;
    *out = next;
    return 1;
}
