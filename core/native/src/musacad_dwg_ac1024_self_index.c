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

/* Original R2010 EED framing only. Reject malformed lengths, non-canonical
 * application H and unterminated sequences. This reader consumes *only*
 * EED metadata; it intentionally does not reinterpret raw application
 * contents, enable no-proxy geometry, or authorize rendering.
 * Limits prevent CPU/resource abuse on untrusted CRC-correct records. */
int musa_dwg_ac1024_record_eed_chain_probe(
    const uint8_t *objects, size_t object_bytes, uint64_t indexed_offset,
    MusaDwgAc1024EedChainEvidence *out) {
    if (!objects || !object_bytes || !out ||
        overlaps(objects, object_bytes, out, sizeof(*out)))
        return 0;

    MusaDwgAc1024SelfPrefix self = {0};
    if (!musa_dwg_ac1024_record_self_prefix(
            objects, object_bytes, indexed_offset, &self) ||
        self.payload_start_byte >= object_bytes ||
        self.handle_stream_end_bit / 8u >
            object_bytes - self.payload_start_byte)
        return 0;
    MusaDwgAc1015Bits bits = {0};
    if (!musa_dwg_ac1015_bits_init(
            &bits, objects + self.payload_start_byte,
            self.handle_stream_end_bit / 8u,
            self.data_after_self_bit, self.handle_stream_start_bit))
        return 0;
    size_t applications = 0u, bytes_read = 0u;
    for (;;) {
        uint16_t data_bytes = 0u;
        if (!musa_dwg_ac1015_bits_read_bs(&bits, &data_bytes))
            return 0;
        if (!data_bytes) break;
        if (applications >= 64u ||
            data_bytes > 1048576u - bytes_read)
            return 0;
        uint32_t code = 0u, count = 0u, octet = 0u;
        if (!musa_dwg_ac1015_bits_read_u(&bits, 4u, &code) ||
            !musa_dwg_ac1015_bits_read_u(&bits, 4u, &count) ||
            code != 5u || count == 0u || count > 8u)
            return 0;
        uint64_t application_handle = 0u;
        for (uint32_t k = 0u; k < count; ++k) {
            if (!musa_dwg_ac1015_bits_read_u(&bits, 8u, &octet) ||
                (k == 0u && octet == 0u))
                return 0;
            application_handle =
                (application_handle << 8u) | (uint64_t)octet;
        }
        if (!application_handle ||
            bits.bit_pos > bits.bit_end ||
            data_bytes > (bits.bit_end - bits.bit_pos) / 8u)
            return 0;
        bits.bit_pos += (size_t)data_bytes * 8u;
        bytes_read += (size_t)data_bytes;
        ++applications;
    }
    const MusaDwgAc1024EedChainEvidence next = {
        self.indexed_object_offset, self.embedded_absolute_handle,
        self.payload_start_byte, bits.bit_pos,
        self.handle_stream_start_bit, self.handle_stream_end_bit,
        applications, bytes_read, self.raw_ot,
        self.frame_crc16_verified, 1u
    };
    *out = next;
    return 1;
}

/* Validate each nonempty, CRC-bounded original EED application H5
 * against the independent source-indexed APPID OT67 record. This is
 * intentionally stricter than merely checking EED frame boundaries.
 * No imported GPL library, no EED semantic evaluation, no mutation. */
static int musa_original_eed_appid_impl(
    const uint8_t *objects, size_t object_bytes,
    const MusaDwgAc1024SelfIdentity *recovered_index, size_t record_count,
    uint64_t canonical_line_handle, MusaDwgAc1024EedAppidEvidence *out,
    uint16_t required_ot) {
    if (!objects || !object_bytes || !recovered_index || !out ||
        !canonical_line_handle || !record_count ||
        record_count > MUSA_AC1024_SELF_INDEX_MAX ||
        record_count > SIZE_MAX / sizeof(*recovered_index) ||
        overlaps(objects, object_bytes, recovered_index,
                 record_count * sizeof(*recovered_index)) ||
        overlaps(objects, object_bytes, out, sizeof(*out)) ||
        overlaps(recovered_index, record_count * sizeof(*recovered_index),
                 out, sizeof(*out)))
        return 0;

    MusaDwgAc1024SelfIdentity entity = {0};
    MusaDwgAc1024EedChainEvidence chain = {0};
    MusaDwgAc1024SelfPrefix original = {0};
    if (!musa_dwg_ac1024_self_index_find(
            recovered_index, record_count, canonical_line_handle, &entity) ||
        entity.raw_ot != required_ot || !entity.frame_crc16_verified ||
        !entity.embedded_handle_code0_verified ||
        !musa_dwg_ac1024_record_eed_chain_probe(
            objects, object_bytes, entity.record_offset, &chain) ||
        chain.raw_ot != required_ot ||
        chain.embedded_absolute_handle != canonical_line_handle ||
        !chain.application_count || chain.application_count > 64u ||
        !chain.original_crc16_verified || !chain.eed_boundaries_verified ||
        !musa_dwg_ac1024_record_self_prefix(
            objects, object_bytes, entity.record_offset, &original) ||
        original.raw_ot != required_ot ||
        original.embedded_absolute_handle != canonical_line_handle ||
        !original.frame_crc16_verified ||
        original.payload_start_byte >= object_bytes ||
        original.handle_stream_end_bit / 8u >
            object_bytes - original.payload_start_byte ||
        original.data_after_self_bit >= chain.data_after_eed_bit)
        return 0;

    MusaDwgAc1015Bits bits = {0};
    if (!musa_dwg_ac1015_bits_init(
            &bits, objects + original.payload_start_byte,
            original.handle_stream_end_bit / 8u,
            original.data_after_self_bit, original.handle_stream_start_bit))
        return 0;

    uint64_t seen[64] = {0};
    size_t refs = 0u, unique = 0u, sum_bytes = 0u;
    uint64_t first_handle = 0u, first_offset = 0u;
    for (;;) {
        uint16_t data_bytes = 0u;
        if (!musa_dwg_ac1015_bits_read_bs(&bits, &data_bytes))
            return 0;
        if (!data_bytes) break;
        if (refs >= chain.application_count || refs >= 64u ||
            data_bytes > 1048576u - sum_bytes)
            return 0;

        uint32_t code = 0u, size = 0u, byte = 0u;
        if (!musa_dwg_ac1015_bits_read_u(&bits, 4u, &code) ||
            !musa_dwg_ac1015_bits_read_u(&bits, 4u, &size) ||
            code != 5u || size == 0u || size > 8u)
            return 0;
        uint64_t appid_h = 0u;
        for (uint32_t k = 0u; k < size; ++k) {
            if (!musa_dwg_ac1015_bits_read_u(&bits, 8u, &byte) ||
                (k == 0u && byte == 0u))
                return 0;
            appid_h = (appid_h << 8u) | (uint64_t)byte;
        }
        if (!appid_h || appid_h == canonical_line_handle)
            return 0;

        MusaDwgAc1024SelfIdentity index_appid = {0};
        MusaDwgAc1024SelfPrefix original_appid = {0};
        if (!musa_dwg_ac1024_self_index_find(
                recovered_index, record_count, appid_h, &index_appid) ||
            index_appid.raw_ot != 67u ||
            !index_appid.frame_crc16_verified ||
            !index_appid.embedded_handle_code0_verified ||
            !musa_dwg_ac1024_record_self_prefix(
                objects, object_bytes,
                index_appid.record_offset, &original_appid) ||
            original_appid.raw_ot != 67u ||
            original_appid.embedded_absolute_handle != appid_h ||
            !original_appid.frame_crc16_verified ||
            !original_appid.embedded_handle_code0_verified ||
            bits.bit_pos > bits.bit_end ||
            data_bytes > (bits.bit_end - bits.bit_pos) / 8u)
            return 0;

        if (!refs) {
            first_handle = appid_h;
            first_offset = index_appid.record_offset;
        }
        size_t k = 0u;
        for (; k < unique; ++k) {
            if (seen[k] == appid_h) break;
        }
        if (k == unique) seen[unique++] = appid_h;
        bits.bit_pos += (size_t)data_bytes * 8u;
        sum_bytes += (size_t)data_bytes;
        ++refs;
    }
    if (refs != chain.application_count ||
        sum_bytes != chain.application_data_bytes ||
        bits.bit_pos != chain.data_after_eed_bit)
        return 0;

    MusaDwgAc1024EedAppidEvidence result = {0};
    result.chain = chain;
    result.appid_reference_count = refs;
    result.distinct_appid_count = unique;
    result.first_appid_handle = first_handle;
    result.first_appid_record_offset = first_offset;
    result.original_appid_records_crc_verified = 1u;
    *out = result;
    return 1;
}

/* Preserve historical OT19-only entry point and add an OT51-only
 * entry point without weakening either original target-identity gate. */
int musa_dwg_ac1024_line_eed_appid_probe(
    const uint8_t *objects, size_t object_bytes,
    const MusaDwgAc1024SelfIdentity *recovered_index, size_t record_count,
    uint64_t canonical_line_handle, MusaDwgAc1024EedAppidEvidence *out) {
    return musa_original_eed_appid_impl(
        objects, object_bytes, recovered_index, record_count,
        canonical_line_handle, out, 19u);
}
int musa_dwg_ac1024_layer_eed_appid_probe(
    const uint8_t *objects, size_t object_bytes,
    const MusaDwgAc1024SelfIdentity *recovered_index, size_t record_count,
    uint64_t canonical_layer_handle, MusaDwgAc1024EedAppidEvidence *out) {
    return musa_original_eed_appid_impl(
        objects, object_bytes, recovered_index, record_count,
        canonical_layer_handle, out, 51u);
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

/* Strictly DATA-only: an original nonzero R2010 EED chain can be
 * authenticated without assigning any meaning to its application bytes.
 * Skip only validated BS/H5/opaque-data frames and require the next common
 * entity header to be bounded. No owner, LAYER or scene witness is issued. */
int musa_dwg_ac1024_record_line_eed_meta_prefix(
    const uint8_t *objects, size_t object_bytes, uint64_t indexed_offset,
    MusaDwgAc1024EntityMetaPrefix *out) {
    if (!objects || !object_bytes || !out ||
        overlaps(objects, object_bytes, out, sizeof(*out)))
        return 0;

    MusaDwgAc1024EedChainEvidence chain = {0};
    if (!musa_dwg_ac1024_record_eed_chain_probe(
            objects, object_bytes, indexed_offset, &chain) ||
        chain.raw_ot != 19u || !chain.application_count ||
        !chain.original_crc16_verified ||
        !chain.eed_boundaries_verified ||
        chain.payload_start_byte >= object_bytes ||
        chain.handle_stream_end_bit / 8u >
            object_bytes - chain.payload_start_byte ||
        chain.data_after_eed_bit >= chain.handle_stream_start_bit)
        return 0;
    MusaDwgAc1015Bits bits = {0};
    if (!musa_dwg_ac1015_bits_init(
            &bits, objects + chain.payload_start_byte,
            chain.handle_stream_end_bit / 8u,
            chain.data_after_eed_bit, chain.handle_stream_start_bit))
        return 0;

    uint32_t graphic_flag = 1u, mode = 3u;
    uint32_t reactors = UINT32_MAX, xdic_missing = 2u;
    if (!musa_dwg_ac1015_bits_read_u(&bits, 1u, &graphic_flag) ||
        graphic_flag != 0u ||
        !musa_dwg_ac1015_bits_read_u(&bits, 2u, &mode) ||
        mode == 3u)
        return 0;
    const size_t after_mode = bits.bit_pos;
    if (!musa_dwg_ac1015_bits_read_bl(&bits, &reactors) ||
        reactors > 1024u)
        return 0;
    const size_t after_reactors = bits.bit_pos;
    if (!musa_dwg_ac1015_bits_read_u(&bits, 1u, &xdic_missing) ||
        bits.bit_pos >= chain.handle_stream_start_bit)
        return 0;

    const MusaDwgAc1024EntityMetaPrefix result = {
        indexed_offset, chain.embedded_absolute_handle,
        chain.payload_start_byte, after_mode, after_reactors,
        bits.bit_pos, chain.handle_stream_start_bit,
        chain.handle_stream_end_bit, reactors, 19u,
        (uint8_t)mode, (uint8_t)xdic_missing, 1u
    };
    *out = result;
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
                         uint32_t absolute_code, uint64_t *target,
                         uint8_t *code_out) {
    MusaDwgAc1015Bits cur = *bits;
    uint32_t code = 0u, count = 0u, byte = 0u;
    if (!musa_dwg_ac1015_bits_read_u(&cur, 4u, &code) ||
        !musa_dwg_ac1015_bits_read_u(&cur, 4u, &count))
        return 0;
    const int implicit = code == 6u || code == 8u;
    const int relative = implicit || code == 10u || code == 12u;
    if (!(relative || code == absolute_code) ||
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
    if (!r2010_link_h(&tail, entity_handle, 4u, &owner_h, &owner_code) ||
        !r2010_link_h(&tail, entity_handle, 5u, &layer_h, &layer_code) ||
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


/* Implicit paper/model-space entities omit the owner H. Their first
 * applicable handle-stream reference is a layer H: check the original
 * CRC-backed layer object instead of fabricating a BLOCK_HEADER owner.
 * Only fixed OT19 and empty EED/reactor/XDictionary subsets are admitted.
 * All outputs remain unchanged if any lookup or bit boundary fails. */
int musa_dwg_ac1024_implicit_layer_probe(
    const uint8_t *objects, size_t object_bytes,
    const MusaDwgAc1024SelfIdentity *recovered_index, size_t record_count,
    uint64_t entity_handle, MusaDwgAc1024ImplicitLayerEvidence *out) {
    if (!objects || !object_bytes || !recovered_index || !out ||
        !entity_handle || !record_count ||
        record_count > MUSA_AC1024_SELF_INDEX_MAX ||
        record_count > SIZE_MAX / sizeof(*recovered_index) ||
        overlaps(objects, object_bytes, recovered_index,
                 record_count * sizeof(*recovered_index)) ||
        overlaps(objects, object_bytes, out, sizeof(*out)) ||
        overlaps(recovered_index, record_count * sizeof(*recovered_index),
                 out, sizeof(*out)))
        return 0;
    MusaDwgAc1024SelfIdentity entity = {0}, layer = {0};
    MusaDwgAc1024EntityMetaPrefix meta = {0};
    if (!musa_dwg_ac1024_self_index_find(
            recovered_index, record_count, entity_handle, &entity) ||
        entity.raw_ot != 19u || !entity.frame_crc16_verified ||
        !entity.embedded_handle_code0_verified ||
        !musa_dwg_ac1024_record_entity_meta_prefix(
            objects, object_bytes, entity.record_offset, &meta) ||
        meta.raw_ot != 19u ||
        meta.embedded_absolute_handle != entity_handle ||
        !meta.original_crc16_verified ||
        (meta.entity_mode != 1u && meta.entity_mode != 2u) ||
        meta.reactor_count != 0u || meta.xdictionary_missing != 1u ||
        meta.payload_start_byte >= object_bytes ||
        meta.handle_stream_end_bit / 8u >
            object_bytes - meta.payload_start_byte)
        return 0;

    MusaDwgAc1015Bits tail = {0};
    if (!musa_dwg_ac1015_bits_init(
            &tail, objects + meta.payload_start_byte,
            meta.handle_stream_end_bit / 8u,
            meta.handle_stream_start_bit, meta.handle_stream_end_bit))
        return 0;
    uint64_t layer_handle = 0u;
    uint8_t code = 0u;
    if (!r2010_link_h(&tail, entity_handle, 5u, &layer_handle, &code) ||
        !musa_dwg_ac1024_self_index_find(
            recovered_index, record_count, layer_handle, &layer) ||
        layer.raw_ot != 51u ||
        !layer.frame_crc16_verified ||
        !layer.embedded_handle_code0_verified)
        return 0;

    MusaDwgAc1024SelfPrefix original_layer = {0};
    if (!musa_dwg_ac1024_record_self_prefix(
            objects, object_bytes, layer.record_offset, &original_layer) ||
        original_layer.raw_ot != 51u ||
        original_layer.embedded_absolute_handle != layer_handle ||
        !original_layer.frame_crc16_verified ||
        !original_layer.embedded_handle_code0_verified ||
        tail.bit_pos > tail.bit_end)
        return 0;

    const MusaDwgAc1024ImplicitLayerEvidence result = {
        entity_handle, entity.record_offset,
        layer_handle, layer.record_offset,
        tail.bit_end - tail.bit_pos,
        19u, meta.entity_mode, code, 1u, 1u
    };
    *out = result;
    return 1;
}


/* A CRC-preserving, source-internal XDictionary graph witness.
 * R2010 common H order: [explicit owner H in mode 0],
 * [reactor H repeated count times], [xdicobjhandle hard owner H],
 * LAYER hard-pointer H. This narrow reader admits zero reactors and
 * original built-in OT19, and verifies original CRC/OT/self-H for
 * each nonzero referenced target. No style/scene or dictionary
 * contents are decoded. */
static int musa_line_original_hgraph_impl(
    const uint8_t *objects, size_t object_bytes,
    const MusaDwgAc1024SelfIdentity *recovered_index, size_t record_count,
    uint64_t entity_handle, MusaDwgAc1024LineXDictionaryGraph *out,
    int has_eed, int require_xdictionary) {
    if (!objects || !object_bytes || !recovered_index || !out ||
        !entity_handle || !record_count ||
        record_count > MUSA_AC1024_SELF_INDEX_MAX ||
        record_count > SIZE_MAX / sizeof(*recovered_index) ||
        overlaps(objects, object_bytes, recovered_index,
                 record_count * sizeof(*recovered_index)) ||
        overlaps(objects, object_bytes, out, sizeof(*out)) ||
        overlaps(recovered_index, record_count * sizeof(*recovered_index),
                 out, sizeof(*out)))
        return 0;

    MusaDwgAc1024SelfIdentity entity = {0};
    MusaDwgAc1024EntityMetaPrefix meta = {0};
    if (!musa_dwg_ac1024_self_index_find(
            recovered_index, record_count, entity_handle, &entity) ||
        entity.raw_ot != 19u || !entity.frame_crc16_verified ||
        !entity.embedded_handle_code0_verified ||
        !(has_eed ?
          musa_dwg_ac1024_record_line_eed_meta_prefix(
              objects, object_bytes, entity.record_offset, &meta) :
          musa_dwg_ac1024_record_entity_meta_prefix(
              objects, object_bytes, entity.record_offset, &meta)) ||
        meta.raw_ot != 19u ||
        meta.embedded_absolute_handle != entity_handle ||
        !meta.original_crc16_verified ||
        meta.entity_mode > 2u ||
        meta.reactor_count != 0u ||
        (require_xdictionary && meta.xdictionary_missing != 0u) ||
        meta.payload_start_byte >= object_bytes ||
        meta.handle_stream_end_bit / 8u >
            object_bytes - meta.payload_start_byte)
        return 0;

    MusaDwgAc1015Bits tail = {0};
    if (!musa_dwg_ac1015_bits_init(
            &tail, objects + meta.payload_start_byte,
            meta.handle_stream_end_bit / 8u,
            meta.handle_stream_start_bit, meta.handle_stream_end_bit))
        return 0;

    uint64_t owner_h = 0u, dictionary_h = 0u, layer_h = 0u;
    uint8_t owner_code = 0u, dictionary_code = 0u, layer_code = 0u;
    if (meta.entity_mode == 0u &&
        !r2010_link_h(&tail, entity_handle, 4u, &owner_h, &owner_code))
        return 0;
    if ((meta.xdictionary_missing == 0u &&
         !r2010_link_h(&tail, entity_handle, 3u,
                       &dictionary_h, &dictionary_code)) ||
        !r2010_link_h(&tail, entity_handle, 5u,
                      &layer_h, &layer_code) ||
        (dictionary_h && dictionary_h == layer_h) ||
        (owner_h && (owner_h == dictionary_h || owner_h == layer_h)) ||
        tail.bit_pos > tail.bit_end)
        return 0;

    MusaDwgAc1024SelfIdentity dictionary = {0}, layer = {0}, owner = {0};
    if ((dictionary_h &&
         (!musa_dwg_ac1024_self_index_find(
              recovered_index, record_count, dictionary_h, &dictionary) ||
          dictionary.raw_ot != 42u ||
          !dictionary.frame_crc16_verified ||
          !dictionary.embedded_handle_code0_verified)) ||
        !musa_dwg_ac1024_self_index_find(
            recovered_index, record_count, layer_h, &layer) ||
        layer.raw_ot != 51u ||
        !layer.frame_crc16_verified ||
        !layer.embedded_handle_code0_verified)
        return 0;
    if (owner_h &&
        (!musa_dwg_ac1024_self_index_find(
             recovered_index, record_count, owner_h, &owner) ||
         owner.raw_ot != 49u ||
         !owner.frame_crc16_verified ||
         !owner.embedded_handle_code0_verified))
        return 0;

    MusaDwgAc1024SelfPrefix dict_record = {0}, layer_record = {0};
    MusaDwgAc1024SelfPrefix owner_record = {0};
    if ((dictionary_h &&
         (!musa_dwg_ac1024_record_self_prefix(
              objects, object_bytes, dictionary.record_offset, &dict_record) ||
          dict_record.raw_ot != 42u ||
          dict_record.embedded_absolute_handle != dictionary_h ||
          !dict_record.frame_crc16_verified)) ||
        !musa_dwg_ac1024_record_self_prefix(
            objects, object_bytes, layer.record_offset, &layer_record) ||
        layer_record.raw_ot != 51u ||
        layer_record.embedded_absolute_handle != layer_h ||
        !layer_record.frame_crc16_verified ||
        (owner_h &&
         (!musa_dwg_ac1024_record_self_prefix(
              objects, object_bytes, owner.record_offset, &owner_record) ||
          owner_record.raw_ot != 49u ||
          owner_record.embedded_absolute_handle != owner_h ||
          !owner_record.frame_crc16_verified)))
        return 0;

    MusaDwgAc1024LineXDictionaryGraph next = {0};
    next.entity_handle = entity_handle;
    next.entity_record_offset = entity.record_offset;
    next.owner_handle = owner_h;
    next.owner_record_offset = owner_h ? owner.record_offset : 0u;
    next.xdictionary_handle = dictionary_h;
    next.xdictionary_record_offset =
        dictionary_h ? dictionary.record_offset : 0u;
    next.layer_handle = layer_h;
    next.layer_record_offset = layer.record_offset;
    next.handle_tail_unparsed_bits = tail.bit_end - tail.bit_pos;
    next.entity_raw_ot = 19u;
    next.entity_mode = meta.entity_mode;
    next.owner_raw_h_code = owner_code;
    next.xdictionary_raw_h_code = dictionary_code;
    next.layer_raw_h_code = layer_code;
    next.original_records_crc_verified = 1u;
    *out = next;
    return 1;
}

/* Preserve original zero-EED/XDictionary-present API and its strict gate.
 * EED-bearing LINEs must go through the explicitly separate API below. */
int musa_dwg_ac1024_line_xdictionary_graph_probe(
    const uint8_t *objects, size_t object_bytes,
    const MusaDwgAc1024SelfIdentity *recovered_index, size_t record_count,
    uint64_t entity_handle, MusaDwgAc1024LineXDictionaryGraph *out) {
    return musa_line_original_hgraph_impl(
        objects, object_bytes, recovered_index, record_count,
        entity_handle, out, 0, 1);
}

/* EED application data is independently bounded. Owner and layer refer
 * only to independently CRC/OT/self-H verified original target records.
 * An absent XDictionary is serialized as no dictionary H (zero). */
int musa_dwg_ac1024_line_eed_handle_graph_probe(
    const uint8_t *objects, size_t object_bytes,
    const MusaDwgAc1024SelfIdentity *recovered_index, size_t record_count,
    uint64_t entity_handle, MusaDwgAc1024LineEedHandleGraph *out) {
    if (!objects || !object_bytes || !recovered_index || !record_count ||
        !entity_handle || !out ||
        record_count > MUSA_AC1024_SELF_INDEX_MAX ||
        record_count > SIZE_MAX / sizeof(*recovered_index) ||
        overlaps(objects, object_bytes, recovered_index,
                 record_count * sizeof(*recovered_index)) ||
        overlaps(objects, object_bytes, out, sizeof(*out)) ||
        overlaps(recovered_index, record_count * sizeof(*recovered_index),
                 out, sizeof(*out)))
        return 0;
    MusaDwgAc1024SelfIdentity identity = {0};
    MusaDwgAc1024LineEedHandleGraph next = {0};
    if (!musa_dwg_ac1024_self_index_find(
            recovered_index, record_count, entity_handle, &identity) ||
        identity.raw_ot != 19u || !identity.frame_crc16_verified ||
        !identity.embedded_handle_code0_verified ||
        !musa_dwg_ac1024_record_eed_chain_probe(
            objects, object_bytes, identity.record_offset, &next.eed) ||
        !next.eed.application_count ||
        next.eed.embedded_absolute_handle != entity_handle ||
        !next.eed.original_crc16_verified ||
        !next.eed.eed_boundaries_verified ||
        !musa_line_original_hgraph_impl(
            objects, object_bytes, recovered_index, record_count,
            entity_handle, &next.handles, 1, 0) ||
        next.handles.entity_handle != entity_handle ||
        next.handles.entity_record_offset != identity.record_offset ||
        next.handles.entity_raw_ot != 19u ||
        !next.handles.original_records_crc_verified)
        return 0;
    *out = next;
    return 1;
}



/* First-party, bounded original R2010 LAYER OT51 flag preflight.
 * The output is the on-disk numeric evidence, not an interpreted layer
 * visibility, CMC color, viewport state, H ownership or drawing gate. */
static int musa_layer_raw_flags_impl(
    const uint8_t *objects, size_t object_bytes,
    const MusaDwgAc1024SelfIdentity *recovered_index, size_t record_count,
    uint64_t layer_handle, MusaDwgAc1024LayerRawFlagsEvidence *out,
    uint8_t *stage, int nonzero_eed) {
    if (stage) *stage = MUSA_DWG_LAYER_FLAGS_ARGUMENT;
    if (!objects || !object_bytes || !recovered_index || !record_count ||
        !layer_handle || !out ||
        record_count > MUSA_AC1024_SELF_INDEX_MAX ||
        record_count > SIZE_MAX / sizeof(*recovered_index) ||
        overlaps(objects, object_bytes, recovered_index,
                 record_count * sizeof(*recovered_index)) ||
        overlaps(objects, object_bytes, out, sizeof(*out)) ||
        overlaps(recovered_index, record_count * sizeof(*recovered_index),
                 out, sizeof(*out)))
        return 0;

    MusaDwgAc1024SelfIdentity identity = {0};
    MusaDwgAc1024ZeroEedPrefix zero = {0};
    MusaDwgAc1024EedChainEvidence chain = {0};
    MusaDwgAc1024SelfPrefix original = {0};
    if (stage) *stage = MUSA_DWG_LAYER_FLAGS_INDEX;
    if (!musa_dwg_ac1024_self_index_find(
            recovered_index, record_count, layer_handle, &identity) ||
        identity.raw_ot != 51u || !identity.frame_crc16_verified ||
        !identity.embedded_handle_code0_verified)
        return 0;
    if (stage) *stage = MUSA_DWG_LAYER_FLAGS_ZERO_EED;
    if (nonzero_eed) {
        if (!musa_dwg_ac1024_record_eed_chain_probe(
                objects, object_bytes, identity.record_offset, &chain) ||
            chain.application_count == 0u ||
            !chain.original_crc16_verified ||
            !chain.eed_boundaries_verified)
            return 0;
    } else if (!musa_dwg_ac1024_record_zero_eed_prefix(
                   objects, object_bytes, identity.record_offset, &zero))
        return 0;

    const size_t payload_start_byte = nonzero_eed ?
        chain.payload_start_byte : zero.payload_start_byte;
    const size_t data_after_eed_bit = nonzero_eed ?
        chain.data_after_eed_bit : zero.data_after_zero_eed_bit;
    const size_t handle_stream_start_bit = nonzero_eed ?
        chain.handle_stream_start_bit : zero.handle_stream_start_bit;
    const size_t handle_stream_end_bit = nonzero_eed ?
        chain.handle_stream_end_bit : zero.handle_stream_end_bit;

    if (stage) *stage = MUSA_DWG_LAYER_FLAGS_RECORD_BOUNDS;
    if (!musa_dwg_ac1024_record_self_prefix(
            objects, object_bytes, identity.record_offset, &original) ||
        original.raw_ot != 51u ||
        original.indexed_object_offset != identity.record_offset ||
        original.embedded_absolute_handle != layer_handle ||
        !original.frame_crc16_verified ||
        !original.embedded_handle_code0_verified ||
        payload_start_byte != original.payload_start_byte ||
        handle_stream_start_bit != original.handle_stream_start_bit ||
        handle_stream_end_bit != original.handle_stream_end_bit ||
        payload_start_byte >= object_bytes ||
        handle_stream_end_bit / 8u >
            object_bytes - payload_start_byte ||
        data_after_eed_bit >= handle_stream_start_bit ||
        (nonzero_eed ?
         (chain.raw_ot != 51u ||
          chain.indexed_object_offset != identity.record_offset ||
          chain.embedded_absolute_handle != layer_handle) :
         (zero.raw_ot != 51u ||
          zero.indexed_object_offset != identity.record_offset ||
          zero.embedded_absolute_handle != layer_handle ||
          !zero.record_crc16_verified || !zero.zero_eed_verified)))
        return 0;

    if (stage) *stage = MUSA_DWG_LAYER_FLAGS_REACTORS;
    MusaDwgAc1015Bits bits = {0};
    uint32_t reactors = UINT32_MAX, xdic = 2u;
    if (!musa_dwg_ac1015_bits_init(
            &bits, objects + payload_start_byte,
            handle_stream_end_bit / 8u,
            data_after_eed_bit, handle_stream_start_bit) ||
        !musa_dwg_ac1015_bits_read_bl(&bits, &reactors) ||
        reactors != 0u)
        return 0;
    if (stage) *stage = MUSA_DWG_LAYER_FLAGS_XDICTIONARY_BIT;
    if (!musa_dwg_ac1015_bits_read_u(&bits, 1u, &xdic) ||
        bits.bit_pos >= handle_stream_start_bit)
        return 0;

    if (stage) *stage = MUSA_DWG_LAYER_FLAGS_NUMERIC;
    const size_t start = bits.bit_pos;
    uint32_t flag64 = 0u, dependent = 0u;
    uint16_t xref_plus_one = 0u, raw = 0u;
    if (!musa_dwg_ac1015_bits_read_u(&bits, 1u, &flag64) ||
        !musa_dwg_ac1015_bits_read_bs(&bits, &xref_plus_one) ||
        !musa_dwg_ac1015_bits_read_u(&bits, 1u, &dependent) ||
        !musa_dwg_ac1015_bits_read_bs(&bits, &raw) ||
        bits.bit_pos >= handle_stream_start_bit)
        return 0;

    MusaDwgAc1024LayerRawFlagsEvidence next = {0};
    next.layer_handle = layer_handle;
    next.layer_record_offset = identity.record_offset;
    next.raw_flags_start_bit = start;
    next.raw_flags_end_bit = bits.bit_pos;
    next.xref_index_plus_one = xref_plus_one;
    next.raw_values_70_290_370 = raw;
    next.unmapped_bits = (uint16_t)(raw & (uint16_t)~UINT16_C(0x03ff));
    next.entry_64_flag = (uint8_t)flag64;
    next.xref_dependent = (uint8_t)dependent;
    next.xdictionary_missing = (uint8_t)xdic;
    next.frozen_bit = (uint8_t)((raw >> 0u) & 1u);
    next.on_bit = (uint8_t)((raw >> 1u) & 1u);
    next.frozen_new_viewports_bit = (uint8_t)((raw >> 2u) & 1u);
    next.locked_bit = (uint8_t)((raw >> 3u) & 1u);
    next.plotting_bit = (uint8_t)((raw >> 4u) & 1u);
    next.lineweight_field = (uint8_t)((raw >> 5u) & 31u);
    next.original_record_crc_verified = 1u;
    next.display_accepted = 0u;
    *out = next;
    if (stage) *stage = MUSA_DWG_LAYER_FLAGS_OK;
    return 1;
}

int musa_dwg_ac1024_layer_raw_flags_probe(
    const uint8_t *objects, size_t object_bytes,
    const MusaDwgAc1024SelfIdentity *recovered_index, size_t record_count,
    uint64_t layer_handle, MusaDwgAc1024LayerRawFlagsEvidence *out) {
    return musa_layer_raw_flags_impl(
        objects, object_bytes, recovered_index, record_count,
        layer_handle, out, NULL, 0);
}

int musa_dwg_ac1024_layer_eed_raw_flags_probe(
    const uint8_t *objects, size_t object_bytes,
    const MusaDwgAc1024SelfIdentity *recovered_index, size_t record_count,
    uint64_t layer_handle, MusaDwgAc1024LayerRawFlagsEvidence *out) {
    return musa_layer_raw_flags_impl(
        objects, object_bytes, recovered_index, record_count,
        layer_handle, out, NULL, 1);
}
uint8_t musa_dwg_ac1024_layer_raw_flags_failure_stage(
    const uint8_t *objects, size_t object_bytes,
    const MusaDwgAc1024SelfIdentity *recovered_index, size_t record_count,
    uint64_t layer_handle) {
    MusaDwgAc1024LayerRawFlagsEvidence ignored = {0};
    uint8_t stage = MUSA_DWG_LAYER_FLAGS_ARGUMENT;
    (void)musa_layer_raw_flags_impl(
        objects, object_bytes, recovered_index, record_count,
        layer_handle, &ignored, &stage, 0);
    return stage;
}



/* Only first two documented original OT51 reference roles are admitted.
 * LAYER_CONTROL is a soft-pointer H4 to OT50, then XDictionary is a
 * hard-owner H3 to OT42. Later references remain deliberately unparsed. */
/* This is NOT a second H decoder: it reads only the first eight
 * unmodified H-header bits after independently proven original EED,
 * OT67 APPID and OT51 raw-flags source boundaries. A raw H prefix is
 * never a substitute for authenticated OT50/OT42 target records. */
int musa_dwg_ac1024_layer_eed_first_h_header_probe(
    const uint8_t *objects, size_t object_bytes,
    const MusaDwgAc1024SelfIdentity *recovered_index, size_t record_count,
    uint64_t canonical_layer_handle,
    MusaDwgAc1024LayerEedRawHandleHeader *out) {
    if (!objects || !object_bytes || !recovered_index || !record_count ||
        !canonical_layer_handle || !out ||
        record_count > MUSA_AC1024_SELF_INDEX_MAX ||
        record_count > SIZE_MAX / sizeof(*recovered_index) ||
        overlaps(objects, object_bytes, recovered_index,
                 record_count * sizeof(*recovered_index)) ||
        overlaps(objects, object_bytes, out, sizeof(*out)) ||
        overlaps(recovered_index, record_count * sizeof(*recovered_index),
                 out, sizeof(*out)))
        return 0;
    MusaDwgAc1024SelfIdentity layer = {0};
    MusaDwgAc1024EedChainEvidence chain = {0};
    MusaDwgAc1024LayerRawFlagsEvidence flags = {0};
    MusaDwgAc1024EedAppidEvidence appid = {0};
    if (!musa_dwg_ac1024_self_index_find(
            recovered_index, record_count, canonical_layer_handle, &layer) ||
        layer.raw_ot != 51u || !layer.frame_crc16_verified ||
        !layer.embedded_handle_code0_verified ||
        !musa_dwg_ac1024_record_eed_chain_probe(
            objects, object_bytes, layer.record_offset, &chain) ||
        chain.raw_ot != 51u ||
        chain.embedded_absolute_handle != canonical_layer_handle ||
        chain.indexed_object_offset != layer.record_offset ||
        !chain.original_crc16_verified || !chain.eed_boundaries_verified ||
        !chain.application_count ||
        !musa_dwg_ac1024_layer_eed_raw_flags_probe(
            objects, object_bytes, recovered_index, record_count,
            canonical_layer_handle, &flags) ||
        flags.layer_record_offset != layer.record_offset ||
        flags.layer_handle != canonical_layer_handle ||
        flags.xdictionary_missing != 0u ||
        !flags.original_record_crc_verified || flags.display_accepted ||
        !musa_dwg_ac1024_layer_eed_appid_probe(
            objects, object_bytes, recovered_index, record_count,
            canonical_layer_handle, &appid) ||
        appid.appid_reference_count != chain.application_count ||
        !appid.original_appid_records_crc_verified ||
        chain.payload_start_byte >= object_bytes ||
        chain.handle_stream_end_bit / 8u >
            object_bytes - chain.payload_start_byte ||
        chain.handle_stream_start_bit >= chain.handle_stream_end_bit)
        return 0;
    MusaDwgAc1015Bits tail = {0};
    uint32_t code = 0u, size = 0u;
    if (!musa_dwg_ac1015_bits_init(
            &tail, objects + chain.payload_start_byte,
            chain.handle_stream_end_bit / 8u,
            chain.handle_stream_start_bit, chain.handle_stream_end_bit) ||
        !musa_dwg_ac1015_bits_read_u(&tail, 4u, &code) ||
        !musa_dwg_ac1015_bits_read_u(&tail, 4u, &size) ||
        code > 15u || size > 15u)
        return 0;
    MusaDwgAc1024LayerEedRawHandleHeader next = {0};
    next.layer_handle = canonical_layer_handle;
    next.layer_record_offset = layer.record_offset;
    next.original_handle_start_bit = chain.handle_stream_start_bit;
    next.handle_tail_bits = chain.handle_stream_end_bit -
                            chain.handle_stream_start_bit;
    next.first_h_code = (uint8_t)code;
    next.first_h_size = (uint8_t)size;
    next.record_crc_verified = 1u;
    next.display_accepted = 0u;
    *out = next;
    return 1;
}

static int musa_layer_eed_xdictionary_impl(
    const uint8_t *objects, size_t object_bytes,
    const MusaDwgAc1024SelfIdentity *recovered_index, size_t record_count,
    uint64_t canonical_layer_handle,
    MusaDwgAc1024LayerEedXDictionaryGraph *out, uint8_t *stage) {
    if (stage) *stage = MUSA_DWG_LAYER_XDIC_GRAPH_ARGUMENT;
    if (!objects || !object_bytes || !recovered_index || !record_count ||
        !canonical_layer_handle || !out ||
        record_count > MUSA_AC1024_SELF_INDEX_MAX ||
        record_count > SIZE_MAX / sizeof(*recovered_index) ||
        overlaps(objects, object_bytes, recovered_index,
                 record_count * sizeof(*recovered_index)) ||
        overlaps(objects, object_bytes, out, sizeof(*out)) ||
        overlaps(recovered_index, record_count * sizeof(*recovered_index),
                 out, sizeof(*out)))
        return 0;

    MusaDwgAc1024SelfIdentity layer = {0}, control = {0}, xdic = {0};
    MusaDwgAc1024SelfPrefix original_control = {0}, original_xdic = {0};
    MusaDwgAc1024EedChainEvidence eed = {0};
    MusaDwgAc1024LayerRawFlagsEvidence flags = {0};
    MusaDwgAc1024EedAppidEvidence appids = {0};
    if (stage) *stage = MUSA_DWG_LAYER_XDIC_GRAPH_IDENTITY;
    if (!musa_dwg_ac1024_self_index_find(
            recovered_index, record_count, canonical_layer_handle, &layer) ||
        layer.raw_ot != 51u || !layer.frame_crc16_verified ||
        !layer.embedded_handle_code0_verified)
        return 0;
    if (stage) *stage = MUSA_DWG_LAYER_XDIC_GRAPH_EED;
    if (!musa_dwg_ac1024_record_eed_chain_probe(
            objects, object_bytes, layer.record_offset, &eed) ||
        eed.raw_ot != 51u ||
        eed.indexed_object_offset != layer.record_offset ||
        eed.embedded_absolute_handle != canonical_layer_handle ||
        !eed.original_crc16_verified || !eed.eed_boundaries_verified ||
        !eed.application_count)
        return 0;
    if (stage) *stage = MUSA_DWG_LAYER_XDIC_GRAPH_FLAGS;
    if (!musa_dwg_ac1024_layer_eed_raw_flags_probe(
            objects, object_bytes, recovered_index, record_count,
            canonical_layer_handle, &flags) ||
        flags.layer_handle != canonical_layer_handle ||
        flags.layer_record_offset != layer.record_offset ||
        flags.xdictionary_missing != 0u ||
        !flags.original_record_crc_verified || flags.display_accepted)
        return 0;
    if (stage) *stage = MUSA_DWG_LAYER_XDIC_GRAPH_APPID;
    if (!musa_dwg_ac1024_layer_eed_appid_probe(
            objects, object_bytes, recovered_index, record_count,
            canonical_layer_handle, &appids) ||
        appids.chain.indexed_object_offset != layer.record_offset ||
        appids.chain.embedded_absolute_handle != canonical_layer_handle ||
        appids.chain.application_count != eed.application_count ||
        !appids.original_appid_records_crc_verified ||
        appids.appid_reference_count != eed.application_count)
        return 0;
    if (stage) *stage = MUSA_DWG_LAYER_XDIC_GRAPH_TAIL;
    if (eed.payload_start_byte >= object_bytes ||
        eed.handle_stream_end_bit / 8u >
            object_bytes - eed.payload_start_byte ||
        eed.handle_stream_start_bit >= eed.handle_stream_end_bit)
        return 0;
    MusaDwgAc1015Bits tail = {0};
    if (!musa_dwg_ac1015_bits_init(
            &tail, objects + eed.payload_start_byte,
            eed.handle_stream_end_bit / 8u,
            eed.handle_stream_start_bit, eed.handle_stream_end_bit))
        return 0;

    uint64_t control_h = 0u, xdic_h = 0u;
    uint8_t control_code = 0u, xdic_code = 0u;
    if (stage) *stage = MUSA_DWG_LAYER_XDIC_GRAPH_CONTROL_H;
    if (!r2010_link_h(&tail, canonical_layer_handle, 4u,
                      &control_h, &control_code))
        return 0;
    if (stage) *stage = MUSA_DWG_LAYER_XDIC_GRAPH_DICTIONARY_H;
    if (!r2010_link_h(&tail, canonical_layer_handle, 3u,
                      &xdic_h, &xdic_code) ||
        control_h == xdic_h)
        return 0;
    if (stage) *stage = MUSA_DWG_LAYER_XDIC_GRAPH_TARGET_INDEX;
    if (!musa_dwg_ac1024_self_index_find(
            recovered_index, record_count, control_h, &control) ||
        !musa_dwg_ac1024_self_index_find(
            recovered_index, record_count, xdic_h, &xdic))
        return 0;
    if (stage) *stage = MUSA_DWG_LAYER_XDIC_GRAPH_TARGET_CLASS;
    if (control.raw_ot != 50u || xdic.raw_ot != 42u ||
        !control.frame_crc16_verified || !xdic.frame_crc16_verified ||
        !control.embedded_handle_code0_verified ||
        !xdic.embedded_handle_code0_verified)
        return 0;
    if (stage) *stage = MUSA_DWG_LAYER_XDIC_GRAPH_TARGET_CRC;
    if (!musa_dwg_ac1024_record_self_prefix(
            objects, object_bytes, control.record_offset,
            &original_control) ||
        !musa_dwg_ac1024_record_self_prefix(
            objects, object_bytes, xdic.record_offset, &original_xdic) ||
        original_control.raw_ot != 50u || original_xdic.raw_ot != 42u ||
        original_control.embedded_absolute_handle != control_h ||
        original_xdic.embedded_absolute_handle != xdic_h ||
        !original_control.frame_crc16_verified ||
        !original_xdic.frame_crc16_verified ||
        !original_control.embedded_handle_code0_verified ||
        !original_xdic.embedded_handle_code0_verified ||
        tail.bit_pos > tail.bit_end)
        return 0;

    MusaDwgAc1024LayerEedXDictionaryGraph next = {0};
    next.layer_handle = canonical_layer_handle;
    next.layer_record_offset = layer.record_offset;
    next.layer_control_handle = control_h;
    next.layer_control_record_offset = control.record_offset;
    next.xdictionary_handle = xdic_h;
    next.xdictionary_record_offset = xdic.record_offset;
    next.remaining_handle_tail_bits = tail.bit_end - tail.bit_pos;
    next.eed_appid_reference_count = appids.appid_reference_count;
    next.layer_control_raw_h_code = control_code;
    next.xdictionary_raw_h_code = xdic_code;
    next.original_records_crc_verified = 1u;
    next.display_accepted = 0u;
    *out = next;
    if (stage) *stage = MUSA_DWG_LAYER_XDIC_GRAPH_OK;
    return 1;
}

int musa_dwg_ac1024_layer_eed_xdictionary_graph_probe(
    const uint8_t *objects, size_t object_bytes,
    const MusaDwgAc1024SelfIdentity *recovered_index, size_t record_count,
    uint64_t canonical_layer_handle,
    MusaDwgAc1024LayerEedXDictionaryGraph *out) {
    return musa_layer_eed_xdictionary_impl(
        objects, object_bytes, recovered_index, record_count,
        canonical_layer_handle, out, NULL);
}

uint8_t musa_dwg_ac1024_layer_eed_xdictionary_graph_failure_stage(
    const uint8_t *objects, size_t object_bytes,
    const MusaDwgAc1024SelfIdentity *recovered_index, size_t record_count,
    uint64_t canonical_layer_handle) {
    MusaDwgAc1024LayerEedXDictionaryGraph ignored = {0};
    uint8_t stage = MUSA_DWG_LAYER_XDIC_GRAPH_ARGUMENT;
    (void)musa_layer_eed_xdictionary_impl(
        objects, object_bytes, recovered_index, record_count,
        canonical_layer_handle, &ignored, &stage);
    return stage;
}



/* Continue ONLY the proven OT50/H4 -> OT42/H3 source prefix.
 * Inspect the next eight raw H-header bits without interpreting the
 * subsequent target or assigning linetype/material/plotstyle roles.
 * The canonical original CRC-backed graph is a required prerequisite. */
int musa_dwg_ac1024_layer_eed_tail_raw_header_probe(
    const uint8_t *objects, size_t object_bytes,
    const MusaDwgAc1024SelfIdentity *recovered_index, size_t record_count,
    uint64_t canonical_layer_handle,
    MusaDwgAc1024LayerEedTailRawHeader *out) {
    if (!objects || !object_bytes || !recovered_index || !record_count ||
        !canonical_layer_handle || !out ||
        record_count > MUSA_AC1024_SELF_INDEX_MAX ||
        record_count > SIZE_MAX / sizeof(*recovered_index) ||
        overlaps(objects, object_bytes, recovered_index,
                 record_count * sizeof(*recovered_index)) ||
        overlaps(objects, object_bytes, out, sizeof(*out)) ||
        overlaps(recovered_index, record_count * sizeof(*recovered_index),
                 out, sizeof(*out)))
        return 0;

    MusaDwgAc1024LayerEedXDictionaryGraph graph = {0};
    MusaDwgAc1024EedChainEvidence chain = {0};
    if (!musa_dwg_ac1024_layer_eed_xdictionary_graph_probe(
            objects, object_bytes, recovered_index, record_count,
            canonical_layer_handle, &graph) ||
        !graph.original_records_crc_verified || graph.display_accepted ||
        !musa_dwg_ac1024_record_eed_chain_probe(
            objects, object_bytes, graph.layer_record_offset, &chain) ||
        chain.raw_ot != 51u ||
        chain.embedded_absolute_handle != canonical_layer_handle ||
        chain.payload_start_byte >= object_bytes ||
        chain.handle_stream_end_bit / 8u >
            object_bytes - chain.payload_start_byte)
        return 0;

    MusaDwgAc1015Bits tail = {0};
    if (!musa_dwg_ac1015_bits_init(
            &tail, objects + chain.payload_start_byte,
            chain.handle_stream_end_bit / 8u,
            chain.handle_stream_start_bit, chain.handle_stream_end_bit))
        return 0;
    uint64_t control_h = 0u, dictionary_h = 0u;
    uint8_t control_code = 0u, dictionary_code = 0u;
    if (!r2010_link_h(&tail, canonical_layer_handle, 4u,
                      &control_h, &control_code) ||
        !r2010_link_h(&tail, canonical_layer_handle, 3u,
                      &dictionary_h, &dictionary_code) ||
        control_h != graph.layer_control_handle ||
        dictionary_h != graph.xdictionary_handle ||
        control_code != graph.layer_control_raw_h_code ||
        dictionary_code != graph.xdictionary_raw_h_code ||
        tail.bit_pos > tail.bit_end ||
        tail.bit_end - tail.bit_pos != graph.remaining_handle_tail_bits ||
        graph.remaining_handle_tail_bits < 8u)
        return 0;

    const size_t start = tail.bit_pos;
    uint32_t raw_code = 0u, raw_size = 0u;
    if (!musa_dwg_ac1015_bits_read_u(&tail, 4u, &raw_code) ||
        !musa_dwg_ac1015_bits_read_u(&tail, 4u, &raw_size) ||
        raw_code > 15u || raw_size > 8u ||
        raw_size > (tail.bit_end - tail.bit_pos) / 8u)
        return 0;

    MusaDwgAc1024LayerEedTailRawHeader next = {0};
    next.layer_handle = canonical_layer_handle;
    next.layer_record_offset = graph.layer_record_offset;
    next.original_tail_start_bit = start;
    next.remaining_handle_tail_bits = graph.remaining_handle_tail_bits;
    next.raw_h_code = (uint8_t)raw_code;
    next.raw_h_size = (uint8_t)raw_size;
    next.original_records_crc_verified = 1u;
    next.display_accepted = 0u;
    *out = next;
    return 1;
}


/* Source-preserving H-token SHAPE census after strictly authenticated
 * OT50/H4 + OT42/H3 original targets. No target identity, semantic
 * handle role or appearance is inferred from this stream. */
static int musa_layer_eed_tail_shape_impl(
    const uint8_t *objects, size_t object_bytes,
    const MusaDwgAc1024SelfIdentity *recovered_index, size_t record_count,
    uint64_t canonical_layer_handle, MusaDwgAc1024LayerEedTailShape *out,
    uint8_t *stage, MusaDwgAc1024LayerEedTerminalFragment *fragment) {
    if (stage) *stage = MUSA_DWG_LAYER_TAIL_SHAPE_ARGUMENT;
    if (!objects || !object_bytes || !recovered_index || !record_count ||
        !canonical_layer_handle || !out ||
        record_count > MUSA_AC1024_SELF_INDEX_MAX ||
        record_count > SIZE_MAX / sizeof(*recovered_index) ||
        overlaps(objects, object_bytes, recovered_index,
                 record_count * sizeof(*recovered_index)) ||
        overlaps(objects, object_bytes, out, sizeof(*out)) ||
        overlaps(recovered_index, record_count * sizeof(*recovered_index),
                 out, sizeof(*out)))
        return 0;

    if (stage) *stage = MUSA_DWG_LAYER_TAIL_SHAPE_PRECONDITION;
    MusaDwgAc1024LayerEedXDictionaryGraph graph = {0};
    MusaDwgAc1024LayerEedTailRawHeader first = {0};
    MusaDwgAc1024EedChainEvidence chain = {0};
    if (!musa_dwg_ac1024_layer_eed_xdictionary_graph_probe(
            objects, object_bytes, recovered_index, record_count,
            canonical_layer_handle, &graph) ||
        !graph.original_records_crc_verified || graph.display_accepted ||
        !musa_dwg_ac1024_layer_eed_tail_raw_header_probe(
            objects, object_bytes, recovered_index, record_count,
            canonical_layer_handle, &first) ||
        !first.original_records_crc_verified || first.display_accepted ||
        first.layer_record_offset != graph.layer_record_offset ||
        first.remaining_handle_tail_bits != graph.remaining_handle_tail_bits ||
        !musa_dwg_ac1024_record_eed_chain_probe(
            objects, object_bytes, graph.layer_record_offset, &chain) ||
        chain.raw_ot != 51u ||
        chain.embedded_absolute_handle != canonical_layer_handle ||
        chain.payload_start_byte >= object_bytes ||
        chain.handle_stream_end_bit / 8u >
            object_bytes - chain.payload_start_byte)
        return 0;

    if (stage) *stage = MUSA_DWG_LAYER_TAIL_SHAPE_STREAM;
    MusaDwgAc1015Bits bits = {0};
    if (!musa_dwg_ac1015_bits_init(
            &bits, objects + chain.payload_start_byte,
            chain.handle_stream_end_bit / 8u,
            chain.handle_stream_start_bit, chain.handle_stream_end_bit))
        return 0;
    if (stage) *stage = MUSA_DWG_LAYER_TAIL_SHAPE_PREFIX;
    uint64_t control_h = 0u, dictionary_h = 0u;
    uint8_t control_code = 0u, dictionary_code = 0u;
    if (!r2010_link_h(&bits, canonical_layer_handle, 4u,
                      &control_h, &control_code) ||
        !r2010_link_h(&bits, canonical_layer_handle, 3u,
                      &dictionary_h, &dictionary_code) ||
        control_h != graph.layer_control_handle ||
        dictionary_h != graph.xdictionary_handle ||
        control_code != graph.layer_control_raw_h_code ||
        dictionary_code != graph.xdictionary_raw_h_code ||
        bits.bit_pos != first.original_tail_start_bit ||
        graph.remaining_handle_tail_bits != bits.bit_end - bits.bit_pos)
        return 0;

    const size_t start = bits.bit_pos;
    size_t count = 0u, h5_zero = 0u, other = 0u, octets = 0u;
    size_t h5_payload = 0u, other_zero = 0u, other_payload = 0u;
    uint8_t prefix_codes[4] = {0};
    uint8_t prefix_sizes[4] = {0};
    while (bits.bit_pos < bits.bit_end) {
        uint32_t code = 0u, length = 0u, value = 0u;
        if (stage) *stage = MUSA_DWG_LAYER_TAIL_SHAPE_TOKEN_LIMIT;
        if (count >= 64u) return 0;
        if (stage) *stage = MUSA_DWG_LAYER_TAIL_SHAPE_SHORT_HEADER;
        if (bits.bit_end - bits.bit_pos < 8u) {
            const size_t rem = bits.bit_end - bits.bit_pos;
            if (fragment && rem > 0u && rem <= 7u && count > 0u) {
                MusaDwgAc1015Bits view = bits;
                uint32_t raw = 0u;
                if (musa_dwg_ac1015_bits_read_u(
                        &view, (uint32_t)rem, &raw) &&
                    view.bit_pos == view.bit_end) {
                    MusaDwgAc1024LayerEedTerminalFragment next = {0};
                    next.layer_handle = canonical_layer_handle;
                    next.layer_record_offset = graph.layer_record_offset;
                    next.complete_h_token_count = count;
                    next.complete_h5_zero_token_count = h5_zero;
                    next.terminal_fragment_bit_count = rem;
                    next.prefix_h_token_count = (uint8_t)(count < 4u ? count : 4u);
                    memcpy(next.prefix_h_code, prefix_codes, sizeof(prefix_codes));
                    memcpy(next.prefix_h_size, prefix_sizes, sizeof(prefix_sizes));
                    next.terminal_fragment_all_zero = (uint8_t)(raw == 0u);
                    next.original_records_crc_verified = 1u;
                    next.display_accepted = 0u;
                    *fragment = next;
                }
            }
            return 0;
        }
        if (stage) *stage = MUSA_DWG_LAYER_TAIL_SHAPE_HEADER_READ;
        if (!musa_dwg_ac1015_bits_read_u(&bits, 4u, &code) ||
            !musa_dwg_ac1015_bits_read_u(&bits, 4u, &length))
            return 0;
        if (stage) *stage = MUSA_DWG_LAYER_TAIL_SHAPE_LENGTH;
        if (length > 8u) return 0;
        if (stage) *stage = MUSA_DWG_LAYER_TAIL_SHAPE_LENGTH_OVERRUN;
        if (length > (bits.bit_end - bits.bit_pos) / 8u) return 0;
        if (stage) *stage = MUSA_DWG_LAYER_TAIL_SHAPE_PAYLOAD;
        for (uint32_t i = 0u; i < length; ++i) {
            if (!musa_dwg_ac1015_bits_read_u(&bits, 8u, &value))
                return 0;
        }
        if (count < 4u) {
            prefix_codes[count] = (uint8_t)code;
            prefix_sizes[count] = (uint8_t)length;
        }
        ++count;
        octets += length;
        if (code == 5u && length == 0u)
            ++h5_zero;
        else {
            ++other;
            if (code == 5u)
                ++h5_payload;
            else if (length == 0u)
                ++other_zero;
            else
                ++other_payload;
        }
    }
    if (stage) *stage = MUSA_DWG_LAYER_TAIL_SHAPE_FINAL;
    if (!count || bits.bit_pos != bits.bit_end ||
        start != first.original_tail_start_bit ||
        bits.bit_pos - start != graph.remaining_handle_tail_bits ||
        count != h5_zero + other ||
        other != h5_payload + other_zero + other_payload ||
        octets > SIZE_MAX - count ||
        count + octets > SIZE_MAX / 8u ||
        (count + octets) * 8u != bits.bit_pos - start)
        return 0;

    MusaDwgAc1024LayerEedTailShape next = {0};
    next.layer_handle = canonical_layer_handle;
    next.layer_record_offset = graph.layer_record_offset;
    next.tail_start_bit = start;
    next.tail_bit_count = bits.bit_pos - start;
    next.h_token_count = count;
    next.raw_h5_zero_length_count = h5_zero;
    next.other_raw_h_count = other;
    next.raw_payload_octets = octets;
    next.raw_h5_nonzero_length_count = h5_payload;
    next.raw_non_h5_zero_length_count = other_zero;
    next.raw_non_h5_nonzero_length_count = other_payload;
    next.original_records_crc_verified = 1u;
    next.display_accepted = 0u;
    *out = next;
    if (stage) *stage = MUSA_DWG_LAYER_TAIL_SHAPE_OK;
    return 1;
}

int musa_dwg_ac1024_layer_eed_tail_shape_probe(
    const uint8_t *objects, size_t object_bytes,
    const MusaDwgAc1024SelfIdentity *recovered_index, size_t record_count,
    uint64_t canonical_layer_handle, MusaDwgAc1024LayerEedTailShape *out) {
    return musa_layer_eed_tail_shape_impl(
        objects, object_bytes, recovered_index, record_count,
        canonical_layer_handle, out, NULL, NULL);
}

uint8_t musa_dwg_ac1024_layer_eed_tail_shape_failure_stage(
    const uint8_t *objects, size_t object_bytes,
    const MusaDwgAc1024SelfIdentity *recovered_index, size_t record_count,
    uint64_t canonical_layer_handle) {
    MusaDwgAc1024LayerEedTailShape ignored = {0};
    uint8_t stage = MUSA_DWG_LAYER_TAIL_SHAPE_ARGUMENT;
    (void)musa_layer_eed_tail_shape_impl(
        objects, object_bytes, recovered_index, record_count,
        canonical_layer_handle, &ignored, &stage, NULL);
    return stage;
}

int musa_dwg_ac1024_layer_eed_terminal_fragment_probe(
    const uint8_t *objects, size_t object_bytes,
    const MusaDwgAc1024SelfIdentity *recovered_index, size_t record_count,
    uint64_t canonical_layer_handle,
    MusaDwgAc1024LayerEedTerminalFragment *out) {
    if (!objects || !object_bytes || !recovered_index || !record_count ||
        !canonical_layer_handle || !out ||
        record_count > MUSA_AC1024_SELF_INDEX_MAX ||
        record_count > SIZE_MAX / sizeof(*recovered_index) ||
        overlaps(objects, object_bytes, out, sizeof(*out)) ||
        overlaps(recovered_index, record_count * sizeof(*recovered_index),
                 out, sizeof(*out)))
        return 0;
    MusaDwgAc1024LayerEedTailShape ignored = {0};
    MusaDwgAc1024LayerEedTerminalFragment candidate = {0};
    uint8_t stage = MUSA_DWG_LAYER_TAIL_SHAPE_ARGUMENT;
    (void)musa_layer_eed_tail_shape_impl(
        objects, object_bytes, recovered_index, record_count,
        canonical_layer_handle, &ignored, &stage, &candidate);
    if (stage != MUSA_DWG_LAYER_TAIL_SHAPE_SHORT_HEADER ||
        !candidate.original_records_crc_verified ||
        !candidate.complete_h_token_count ||
        candidate.terminal_fragment_bit_count < 1u ||
        candidate.terminal_fragment_bit_count > 7u ||
        candidate.display_accepted)
        return 0;
    *out = candidate;
    return 1;
}


/* Only evaluate RAW absolute-H numeric candidates after a completely
 * authenticated, source-preserving OT51/OT50/OT42/EED prefix and the
 * independently verified short-header terminal witness. This function
 * cannot determine what a handle MEANS for CAD drawing semantics. */
int musa_dwg_ac1024_layer_eed_tail_candidate_census_probe(
    const uint8_t *objects, size_t object_bytes,
    const MusaDwgAc1024SelfIdentity *recovered_index, size_t record_count,
    uint64_t canonical_layer_handle,
    MusaDwgAc1024LayerTailCandidateCensus *out) {
    if (!objects || !object_bytes || !recovered_index || !record_count ||
        !canonical_layer_handle || !out ||
        record_count > MUSA_AC1024_SELF_INDEX_MAX ||
        record_count > SIZE_MAX / sizeof(*recovered_index) ||
        overlaps(objects, object_bytes, recovered_index,
                 record_count * sizeof(*recovered_index)) ||
        overlaps(objects, object_bytes, out, sizeof(*out)) ||
        overlaps(recovered_index, record_count * sizeof(*recovered_index),
                 out, sizeof(*out)))
        return 0;

    MusaDwgAc1024LayerEedTerminalFragment fragment = {0};
    MusaDwgAc1024LayerEedXDictionaryGraph graph = {0};
    MusaDwgAc1024EedChainEvidence chain = {0};
    if (!musa_dwg_ac1024_layer_eed_terminal_fragment_probe(
            objects, object_bytes, recovered_index, record_count,
            canonical_layer_handle, &fragment) ||
        !musa_dwg_ac1024_layer_eed_xdictionary_graph_probe(
            objects, object_bytes, recovered_index, record_count,
            canonical_layer_handle, &graph) ||
        !musa_dwg_ac1024_record_eed_chain_probe(
            objects, object_bytes, graph.layer_record_offset, &chain) ||
        !fragment.original_records_crc_verified || fragment.display_accepted ||
        !graph.original_records_crc_verified || graph.display_accepted ||
        !chain.original_crc16_verified ||
        fragment.layer_record_offset != graph.layer_record_offset ||
        chain.embedded_absolute_handle != canonical_layer_handle ||
        chain.raw_ot != 51u ||
        fragment.prefix_h_token_count == 0u ||
        fragment.prefix_h_token_count > 4u ||
        fragment.prefix_h_token_count > fragment.complete_h_token_count ||
        chain.payload_start_byte >= object_bytes ||
        chain.handle_stream_end_bit / 8u >
            object_bytes - chain.payload_start_byte)
        return 0;

    MusaDwgAc1015Bits bits = {0};
    if (!musa_dwg_ac1015_bits_init(
            &bits, objects + chain.payload_start_byte,
            chain.handle_stream_end_bit / 8u,
            chain.handle_stream_start_bit, chain.handle_stream_end_bit))
        return 0;
    uint64_t control_h = 0u, dictionary_h = 0u;
    uint8_t control_code = 0u, dictionary_code = 0u;
    if (!r2010_link_h(&bits, canonical_layer_handle, 4u,
                      &control_h, &control_code) ||
        !r2010_link_h(&bits, canonical_layer_handle, 3u,
                      &dictionary_h, &dictionary_code) ||
        control_h != graph.layer_control_handle ||
        dictionary_h != graph.xdictionary_handle ||
        control_code != graph.layer_control_raw_h_code ||
        dictionary_code != graph.xdictionary_raw_h_code ||
        bits.bit_end < bits.bit_pos ||
        bits.bit_end - bits.bit_pos != graph.remaining_handle_tail_bits)
        return 0;

    MusaDwgAc1024LayerTailCandidateCensus next = {0};
    for (size_t n = 0u; n < fragment.prefix_h_token_count; ++n) {
        uint32_t code = 0u, length = 0u, octet = 0u;
        if (bits.bit_pos > bits.bit_end ||
            bits.bit_end - bits.bit_pos < 8u ||
            !musa_dwg_ac1015_bits_read_u(&bits, 4u, &code) ||
            !musa_dwg_ac1015_bits_read_u(&bits, 4u, &length) ||
            code != fragment.prefix_h_code[n] ||
            length != fragment.prefix_h_size[n] ||
            length > 8u ||
            length > (bits.bit_end - bits.bit_pos) / 8u)
            return 0;
        const int absolute = code >= 2u && code <= 5u && length > 0u;
        uint64_t value = 0u;
        for (uint32_t k = 0u; k < length; ++k) {
            if (!musa_dwg_ac1015_bits_read_u(&bits, 8u, &octet))
                return 0;
            if (absolute && k == 0u && octet == 0u)
                return 0; /* noncanonical absolute value */
            value = (value << 8u) | (uint64_t)octet;
        }
        if (!absolute) {
            ++next.nonabsolute_or_empty_tokens;
            continue;
        }
        if (!value) return 0;
        ++next.absolute_candidates;
        MusaDwgAc1024SelfIdentity candidate = {0};
        MusaDwgAc1024SelfPrefix original = {0};
        if (musa_dwg_ac1024_self_index_find(
                recovered_index, record_count, value, &candidate) &&
            candidate.frame_crc16_verified &&
            candidate.embedded_handle_code0_verified &&
            musa_dwg_ac1024_record_self_prefix(
                objects, object_bytes, candidate.record_offset,
                &original) &&
            original.frame_crc16_verified &&
            original.embedded_absolute_handle == value &&
            original.raw_ot == candidate.raw_ot) {
            ++next.verified_index_matches;
            if (original.raw_ot == 49u) ++next.verified_ot49;
            else if (original.raw_ot == 51u) ++next.verified_ot51;
            else ++next.verified_other_ot;
        } else {
            ++next.unmatched_or_unverified;
        }
    }
    next.complete_prefix_h_tokens = fragment.prefix_h_token_count;
    if (next.absolute_candidates + next.nonabsolute_or_empty_tokens !=
            next.complete_prefix_h_tokens ||
        next.verified_index_matches + next.unmatched_or_unverified !=
            next.absolute_candidates ||
        next.verified_ot49 + next.verified_ot51 + next.verified_other_ot !=
            next.verified_index_matches)
        return 0;
    next.original_records_crc_verified = 1u;
    next.display_accepted = 0u;
    *out = next;
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

