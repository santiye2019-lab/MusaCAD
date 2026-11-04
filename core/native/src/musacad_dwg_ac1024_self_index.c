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

/* An immutable cursor after the first canonical absolute H. We verify
 * frame CRC, raw OT and the MC-bounded DATA/handle partition again for
 * each call. This does NOT establish the rest of the common entity header. */
int musa_dwg_ac1024_record_self_prefix(
    const uint8_t *objects, size_t object_bytes, uint64_t indexed_offset,
    MusaDwgAc1024SelfPrefix *out) {
    if (!objects || !out ||
        overlaps(objects, object_bytes, out, sizeof(*out)))
        return 0;
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
    if (!handle || bits.bit_pos > bounds.handle_stream_start_bit)
        return 0;
    const MusaDwgAc1024SelfPrefix next = {
        indexed_offset, handle, bounds.payload_start_byte, bits.bit_pos,
        bounds.handle_stream_start_bit, bounds.handle_stream_end_bit,
        bounds.raw_object_type, 1u, 1u
    };
    *out = next;
    return 1;
}

/* Second strict common-data field: the zero-length EED case only.
 * The caller must never reinterpret a nonzero EED as zero, or decode
 * graphics/visibility/owner/layer from this partial prefix. */
int musa_dwg_ac1024_record_zero_eed_prefix(
    const uint8_t *objects, size_t object_bytes, uint64_t indexed_offset,
    MusaDwgAc1024ZeroEedPrefix *out) {
    if (!objects || !out ||
        overlaps(objects, object_bytes, out, sizeof(*out)))
        return 0;
    MusaDwgAc1024SelfPrefix self = {0};
    if (!musa_dwg_ac1024_record_self_prefix(
            objects, object_bytes, indexed_offset, &self))
        return 0;
    MusaDwgAc1015Bits bits = {0};
    if (!musa_dwg_ac1015_bits_init(
            &bits, objects + self.payload_start_byte,
            self.handle_stream_end_bit / 8u,
            self.data_after_self_bit, self.handle_stream_start_bit))
        return 0;
    uint16_t eed_bytes = UINT16_MAX;
    if (!musa_dwg_ac1015_bits_read_bs(&bits, &eed_bytes) ||
        eed_bytes != 0u || bits.bit_pos > self.handle_stream_start_bit)
        return 0;
    const MusaDwgAc1024ZeroEedPrefix next = {
        self.indexed_object_offset, self.embedded_absolute_handle,
        self.payload_start_byte, self.data_after_self_bit, bits.bit_pos,
        self.handle_stream_start_bit, self.handle_stream_end_bit,
        self.raw_ot, self.frame_crc16_verified, 1u
    };
    *out = next;
    return 1;
}

/* Third bounded common DATA field: graphic-present bit, admitted only
 * as 0 after a verified zero-length EED. No payload skipping or scene. */
int musa_dwg_ac1024_record_no_proxy_prefix(
    const uint8_t *objects, size_t object_bytes, uint64_t indexed_offset,
    MusaDwgAc1024NoProxyPrefix *out) {
    if (!objects || !out ||
        overlaps(objects, object_bytes, out, sizeof(*out)))
        return 0;
    MusaDwgAc1024ZeroEedPrefix zero = {0};
    if (!musa_dwg_ac1024_record_zero_eed_prefix(
            objects, object_bytes, indexed_offset, &zero))
        return 0;
    MusaDwgAc1015Bits bits = {0};
    if (!musa_dwg_ac1015_bits_init(
            &bits, objects + zero.payload_start_byte,
            zero.handle_stream_end_bit / 8u,
            zero.data_after_zero_eed_bit,
            zero.handle_stream_start_bit))
        return 0;
    uint32_t graphic_flag = 1u;
    if (!musa_dwg_ac1015_bits_read_u(&bits, 1u, &graphic_flag) ||
        graphic_flag != 0u || bits.bit_pos > zero.handle_stream_start_bit)
        return 0;
    const MusaDwgAc1024NoProxyPrefix next = {
        zero.indexed_object_offset, zero.embedded_absolute_handle,
        zero.payload_start_byte, bits.bit_pos,
        zero.handle_stream_start_bit, zero.handle_stream_end_bit,
        zero.raw_ot, zero.record_crc16_verified,
        zero.zero_eed_verified, 1u
    };
    *out = next;
    return 1;
}

/* Fixed built-in R2010 entities only; class-indexed and other
 * OT values cannot be inferred from a generic common-data header. */
static int is_fixed_entity_ot(uint16_t type) {
    return type == 7u || type == 17u || type == 18u || type == 19u;
}

int musa_dwg_ac1024_record_entity_meta_prefix(
    const uint8_t *objects, size_t object_bytes, uint64_t indexed_offset,
    MusaDwgAc1024EntityMetaPrefix *out) {
    if (!objects || !out ||
        overlaps(objects, object_bytes, out, sizeof(*out)))
        return 0;
    MusaDwgAc1024NoProxyPrefix no_proxy = {0};
    if (!musa_dwg_ac1024_record_no_proxy_prefix(
            objects, object_bytes, indexed_offset, &no_proxy) ||
        !is_fixed_entity_ot(no_proxy.raw_ot) ||
        !no_proxy.frame_crc16_verified || !no_proxy.zero_eed_verified ||
        !no_proxy.no_proxy_graphic_verified)
        return 0;

    MusaDwgAc1015Bits bits = {0};
    if (!musa_dwg_ac1015_bits_init(
            &bits, objects + no_proxy.payload_start_byte,
            no_proxy.handle_stream_end_bit / 8u,
            no_proxy.data_after_no_proxy_flag_bit,
            no_proxy.handle_stream_start_bit))
        return 0;
    uint32_t mode = 3u, reactors = UINT32_MAX, xdic_missing = 2u;
    if (!musa_dwg_ac1015_bits_read_u(&bits, 2u, &mode) || mode == 3u)
        return 0;
    const size_t after_mode = bits.bit_pos;
    if (!musa_dwg_ac1015_bits_read_bl(&bits, &reactors) ||
        reactors > 1024u)
        return 0;
    const size_t after_reactors = bits.bit_pos;
    if (!musa_dwg_ac1015_bits_read_u(&bits, 1u, &xdic_missing) ||
        bits.bit_pos > no_proxy.handle_stream_start_bit)
        return 0;
    const MusaDwgAc1024EntityMetaPrefix next = {
        indexed_offset, no_proxy.embedded_absolute_handle,
        no_proxy.payload_start_byte, after_mode, after_reactors,
        bits.bit_pos, no_proxy.handle_stream_start_bit,
        no_proxy.handle_stream_end_bit, reactors, no_proxy.raw_ot,
        (uint8_t)mode, (uint8_t)xdic_missing, 1u
    };
    *out = next;
    return 1;
}

/* Original R2010 H reference grammar, 4-bit code + 4-bit byte count.
 * Absolute owner/layer references retain the explicit role-specific 4/5
 * code gate. The published relative soft-pointer forms 6/8/A/C resolve
 * against the authenticated embedded self-H and still must resolve to an
 * independently CRC-authenticated OT49 owner or OT51 layer record.
 * Codes 6/8 have ZERO payload bytes; A/C carry a canonical positive
 * offset. No global handle decoder, GPL import or scene acceptance. */
static int r2010_link_h(MusaDwgAc1015Bits *bits, uint64_t self,
                         int owner, uint64_t *target, uint8_t *code_out) {
    MusaDwgAc1015Bits cur = *bits;
    uint32_t code = 0u, count = 0u, byte = 0u;
    if (!musa_dwg_ac1015_bits_read_u(&cur, 4u, &code) ||
        !musa_dwg_ac1015_bits_read_u(&cur, 4u, &count))
        return 0;
    const int implicit = code == 6u || code == 8u;
    const int relative = implicit || code == 10u || code == 12u;
    if (!(relative || (owner ? code == 4u : code == 5u)) ||
        (implicit ? count != 0u : (count == 0u || count > 8u)))
        return 0;
    uint64_t value = 0u;
    for (uint32_t i = 0u; i < count; ++i) {
        if (!musa_dwg_ac1015_bits_read_u(&cur, 8u, &byte) ||
            (i == 0u && byte == 0u)) return 0;
        value = (value << 8u) | (uint64_t)byte;
    }
    uint64_t resolved = 0u;
    if (code == 6u) {
        if (self == UINT64_MAX) return 0;
        resolved = self + 1u;
    } else if (code == 8u) {
        if (self <= 1u) return 0;
        resolved = self - 1u;
    } else if (code == 10u) {
        if (!value || value > UINT64_MAX - self) return 0;
        resolved = self + value;
    } else if (code == 12u) {
        if (!value || value >= self) return 0;
        resolved = self - value;
    } else {
        resolved = value;
    }
    if (!resolved || resolved == self) return 0;
    *bits = cur; *target = resolved; *code_out = (uint8_t)code;
    return 1;
}

/* Only an explicit-owner, no-reactor, no-XDictionary built-in entity.
 * Authenticates both referenced ORIGINAL target records by CRC/OT/self-H.
 * Further tail handles, geometry and final paint remain unexamined. */
int musa_dwg_ac1024_entity_owner_layer_probe(
    const uint8_t *objects, size_t object_bytes,
    const MusaDwgAc1024SelfIdentity *recovered_index, size_t record_count,
    uint64_t entity_handle, MusaDwgAc1024OwnerLayerEvidence *out) {
    if (!objects || !object_bytes || !recovered_index || !out ||
        !entity_handle || !record_count ||
        record_count > MUSA_AC1024_SELF_INDEX_MAX ||
        record_count > SIZE_MAX / sizeof(*recovered_index) ||
        overlaps(objects, object_bytes, recovered_index,
                 record_count * sizeof(*recovered_index)) ||
        overlaps(objects, object_bytes, out, sizeof(*out)) ||
        overlaps(recovered_index, record_count * sizeof(*recovered_index),
                 out, sizeof(*out))) return 0;

    MusaDwgAc1024SelfIdentity entity = {0}, owner = {0}, layer = {0};
    MusaDwgAc1024EntityMetaPrefix meta = {0};
    if (!musa_dwg_ac1024_self_index_find(
            recovered_index, record_count, entity_handle, &entity) ||
        !musa_dwg_ac1024_record_entity_meta_prefix(
            objects, object_bytes, entity.record_offset, &meta) ||
        meta.embedded_absolute_handle != entity_handle ||
        meta.raw_ot != entity.raw_ot || !meta.original_crc16_verified ||
        meta.entity_mode != 0u || meta.reactor_count != 0u ||
        meta.xdictionary_missing != 1u ||
        meta.payload_start_byte >= object_bytes ||
        meta.handle_stream_end_bit / 8u >
            object_bytes - meta.payload_start_byte) return 0;

    MusaDwgAc1015Bits tail = {0};
    if (!musa_dwg_ac1015_bits_init(
            &tail, objects + meta.payload_start_byte,
            meta.handle_stream_end_bit / 8u,
            meta.handle_stream_start_bit, meta.handle_stream_end_bit))
        return 0;
    uint64_t owner_h = 0u, layer_h = 0u;
    uint8_t owner_code = 0u, layer_code = 0u;
    if (!r2010_link_h(&tail, entity_handle, 1, &owner_h, &owner_code) ||
        !r2010_link_h(&tail, entity_handle, 0, &layer_h, &layer_code) ||
        owner_h == layer_h ||
        !musa_dwg_ac1024_self_index_find(
            recovered_index, record_count, owner_h, &owner) ||
        !musa_dwg_ac1024_self_index_find(
            recovered_index, record_count, layer_h, &layer) ||
        owner.raw_ot != 49u || layer.raw_ot != 51u) return 0;

    MusaDwgAc1024SelfPrefix owner_record = {0}, layer_record = {0};
    if (!musa_dwg_ac1024_record_self_prefix(
            objects, object_bytes, owner.record_offset, &owner_record) ||
        !musa_dwg_ac1024_record_self_prefix(
            objects, object_bytes, layer.record_offset, &layer_record) ||
        owner_record.embedded_absolute_handle != owner_h ||
        layer_record.embedded_absolute_handle != layer_h ||
        owner_record.raw_ot != 49u || layer_record.raw_ot != 51u ||
        !owner_record.frame_crc16_verified ||
        !layer_record.frame_crc16_verified ||
        tail.bit_pos > tail.bit_end) return 0;

    const MusaDwgAc1024OwnerLayerEvidence result = {
        entity_handle, entity.record_offset,
        owner_h, owner.record_offset, layer_h, layer.record_offset,
        tail.bit_end - tail.bit_pos, meta.raw_ot,
        owner_code, layer_code, 1u, 1u
    };
    *out = result;
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
        MusaDwgAc1024SelfPrefix prefix = {0};
        if (offset >= (uint64_t)object_bytes ||
            !musa_dwg_ac1024_record_self_prefix(
                objects, object_bytes, offset, &prefix) ||
            prefix.embedded_absolute_handle <= previous) {
            valid = 0;
            break;
        }
        previous = prefix.embedded_absolute_handle;
        staged[i].embedded_absolute_handle = prefix.embedded_absolute_handle;
        staged[i].record_offset = offset;
        staged[i].raw_ot = prefix.raw_ot;
        staged[i].frame_crc16_verified = 1u;
        staged[i].embedded_handle_code0_verified = 1u;
    }
    if (valid)
        memcpy(out, staged, record_count * sizeof(*out));
    free(staged);
    return valid;
}

/* Fast bounded lookup in a previously authenticated recovered index.
 * A caller-provided array is NOT authenticated by this function; checking
 * all 145k records every lookup would defeat the purpose. The recovered
 * index must be retained unchanged for the lifetime of all lookups. */
int musa_dwg_ac1024_self_index_find(
    const MusaDwgAc1024SelfIdentity *verified_index, size_t record_count,
    uint64_t canonical_absolute_handle, MusaDwgAc1024SelfIdentity *out) {
    if (!verified_index || !out || !canonical_absolute_handle ||
        !record_count || record_count > MUSA_AC1024_SELF_INDEX_MAX ||
        record_count > SIZE_MAX / sizeof(*verified_index) ||
        overlaps(verified_index, record_count * sizeof(*verified_index),
                 out, sizeof(*out)))
        return 0;

    size_t low = 0u, high = record_count; /* exclusive upper bound */
    while (low < high) {
        const size_t mid = low + (high - low) / 2u;
        if (verified_index[mid].embedded_absolute_handle <
            canonical_absolute_handle)
            low = mid + 1u;
        else
            high = mid;
    }
    if (low == record_count ||
        verified_index[low].embedded_absolute_handle !=
            canonical_absolute_handle)
        return 0;
    const MusaDwgAc1024SelfIdentity *found = &verified_index[low];
    if (found->frame_crc16_verified != 1u ||
        found->embedded_handle_code0_verified != 1u ||
        (low > 0u &&
         (verified_index[low - 1u].embedded_absolute_handle >=
              canonical_absolute_handle ||
          verified_index[low - 1u].frame_crc16_verified != 1u ||
          verified_index[low - 1u].embedded_handle_code0_verified != 1u)) ||
        (low + 1u < record_count &&
         (verified_index[low + 1u].embedded_absolute_handle <=
              canonical_absolute_handle ||
          verified_index[low + 1u].frame_crc16_verified != 1u ||
          verified_index[low + 1u].embedded_handle_code0_verified != 1u)))
        return 0;
    *out = *found;
    return 1;
}

