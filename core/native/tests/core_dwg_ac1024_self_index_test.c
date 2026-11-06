/* SPDX-License-Identifier: MIT
 * Synthetic read-only AC1024 embedded self-H identity test.
 * This never turns recovered identities into scene/geometry acceptance.
 */
#include "musacad_dwg_ac1024_self_index.h"
#include "musacad_crc16.h"
#include <stdint.h>
#include <stdio.h>
#include <string.h>

static void set_bits(uint8_t *body, size_t first,
                     unsigned width, uint32_t value) {
    for (unsigned i = 0u; i < width; i++) {
        const size_t at = first + i;
        const unsigned shift = width - i - 1u;
        const uint8_t mask = (uint8_t)(1u << (7u - at % 8u));
        if ((value >> shift) & 1u)
            body[at / 8u] |= mask;
        else
            body[at / 8u] &= (uint8_t)~mask;
    }
}
static int make_record(uint8_t *out, uint32_t self_handle) {
    if (!out || !self_handle || self_handle > 255u)
        return 0;
    memset(out, 0, 13u);
    out[0] = 8u; /* MS body length */
    out[2] = 16u; /* MC length of subsequent handle stream in bits */
    uint8_t *const body = out + 3u;
    set_bits(body, 0u, 2u, 0u); /* OT selector 0 */
    set_bits(body, 2u, 8u, 19u); /* LINE numeric type */
    set_bits(body, 10u, 4u, 0u); /* absolute code-0 self H */
    set_bits(body, 14u, 4u, 1u); /* 1-byte canonical H */
    set_bits(body, 18u, 8u, self_handle);
    uint16_t crc = 0u;
    if (!musa_core_crc16_arc(0xc0c1u, out, 11u, &crc))
        return 0;
    out[11] = (uint8_t)crc;
    out[12] = (uint8_t)(crc >> 8u);
    return 1;
}
static int test_self_prefix_cursor(void) {
    _Alignas(MusaDwgAc1024SelfPrefix) uint8_t record[64] = {0};
    if (!make_record(record, 44u)) return 0;
    uint8_t original[13];
    memcpy(original, record, sizeof(original));
    MusaDwgAc1024SelfPrefix prefix = {0};
    if (!musa_dwg_ac1024_record_self_prefix(
            record, 13u, 0u, &prefix) ||
        prefix.indexed_object_offset != 0u ||
        prefix.embedded_absolute_handle != 44u ||
        prefix.payload_start_byte != 3u ||
        prefix.data_after_self_bit != 26u ||
        prefix.handle_stream_start_bit != 48u ||
        prefix.handle_stream_end_bit != 64u ||
        prefix.raw_ot != 19u ||
        !prefix.frame_crc16_verified ||
        !prefix.embedded_handle_code0_verified ||
        memcmp(record, original, sizeof(original)))
        return 0;
    const MusaDwgAc1024SelfPrefix saved = prefix;

    /* Late CRC corruption, truncation, bad absolute H code and canonical
     * leading-zero errors all preserve the caller's original output. */
    record[12] ^= 0x40u;
    if (musa_dwg_ac1024_record_self_prefix(
            record, 13u, 0u, &prefix) ||
        memcmp(&prefix, &saved, sizeof(saved)))
        return 0;
    if (!make_record(record, 44u)) return 0;
    set_bits(record + 3u, 10u, 4u, 1u);
    uint16_t crc = 0u;
    if (!musa_core_crc16_arc(0xc0c1u, record, 11u, &crc))
        return 0;
    record[11] = (uint8_t)crc;
    record[12] = (uint8_t)(crc >> 8u);
    if (musa_dwg_ac1024_record_self_prefix(
            record, 13u, 0u, &prefix) ||
        memcmp(&prefix, &saved, sizeof(saved)))
        return 0;
    if (!make_record(record, 44u)) return 0;
    set_bits(record + 3u, 18u, 8u, 0u);
    if (!musa_core_crc16_arc(0xc0c1u, record, 11u, &crc))
        return 0;
    record[11] = (uint8_t)crc;
    record[12] = (uint8_t)(crc >> 8u);
    if (musa_dwg_ac1024_record_self_prefix(
            record, 13u, 0u, &prefix) ||
        memcmp(&prefix, &saved, sizeof(saved)))
        return 0;
    if (!make_record(record, 44u)) return 0;
    if (musa_dwg_ac1024_record_self_prefix(
            record, 12u, 0u, &prefix) ||
        musa_dwg_ac1024_record_self_prefix(
            record, 13u, 13u, &prefix) ||
        musa_dwg_ac1024_record_self_prefix(
            record, 13u, 0u, (MusaDwgAc1024SelfPrefix *)(void *)record) ||
        musa_dwg_ac1024_record_self_prefix(
            NULL, 0u, 0u, &prefix) ||
        memcmp(&prefix, &saved, sizeof(saved)) ||
        memcmp(record, original, sizeof(original)))
        return 0;
    return 1;
}

/* Exact bounded R2010 EED framing: BS byte count + canonical H5
 * application handle + opaque raw bytes, terminated by BS zero. The
 * underlying source/CRC are original fixture bytes; there is no
 * inference of app semantics, layer, owner or geometry. */
static int test_eed_chain_bounded(void) {
    _Alignas(MusaDwgAc1024EedChainEvidence) uint8_t record[64] = {0};
    record[0] = 16u; /* 16 payload bytes; 16 trailing H bits */
    record[2] = 16u;
    uint8_t *const p = record + 3u;
    set_bits(p, 0u, 2u, 0u);  /* OT short selector */
    set_bits(p, 2u, 8u, 19u); /* LINE */
    set_bits(p, 10u, 4u, 0u);
    set_bits(p, 14u, 4u, 1u);
    set_bits(p, 18u, 8u, 44u); /* self-H */
    set_bits(p, 26u, 2u, 1u); /* first EED application length BS(3) */
    set_bits(p, 28u, 8u, 3u);
    set_bits(p, 36u, 4u, 5u); /* app handle H5 */
    set_bits(p, 40u, 4u, 1u);
    set_bits(p, 44u, 8u, 42u);
    set_bits(p, 52u, 8u, 0x23u); /* opaque 3-byte payload */
    set_bits(p, 60u, 8u, 0x55u);
    set_bits(p, 68u, 8u, 0xaau);
    set_bits(p, 76u, 2u, 2u); /* zero BS EED terminator */
    uint16_t crc = 0u;
    if (!musa_core_crc16_arc(0xc0c1u, record, 19u, &crc))
        return 0;
    record[19] = (uint8_t)crc;
    record[20] = (uint8_t)(crc >> 8u);
    uint8_t original[21];
    memcpy(original, record, sizeof(original));
    MusaDwgAc1024EedChainEvidence out = {0};
    if (!musa_dwg_ac1024_record_eed_chain_probe(
            record, sizeof(original), 0u, &out) ||
        out.indexed_object_offset != 0u ||
        out.embedded_absolute_handle != 44u ||
        out.payload_start_byte != 3u ||
        out.raw_ot != 19u ||
        out.data_after_eed_bit != 78u ||
        out.handle_stream_start_bit != 112u ||
        out.handle_stream_end_bit != 128u ||
        out.application_count != 1u ||
        out.application_data_bytes != 3u ||
        !out.original_crc16_verified ||
        !out.eed_boundaries_verified ||
        memcmp(original, record, sizeof(original)))
        return 0;
    const MusaDwgAc1024EedChainEvidence saved = out;

    /* A second bounded EED application ends exactly at DATA bit 112. */
    set_bits(p, 76u, 2u, 1u); /* BS length 1 */
    set_bits(p, 78u, 8u, 1u);
    set_bits(p, 86u, 4u, 5u); /* H5 */
    set_bits(p, 90u, 4u, 1u);
    set_bits(p, 94u, 8u, 43u);
    set_bits(p, 102u, 8u, 0x70u);
    set_bits(p, 110u, 2u, 2u); /* BS zero terminator */
    if (!musa_core_crc16_arc(0xc0c1u, record, 19u, &crc))
        return 0;
    record[19] = (uint8_t)crc;
    record[20] = (uint8_t)(crc >> 8u);
    if (!musa_dwg_ac1024_record_eed_chain_probe(
            record, sizeof(original), 0u, &out) ||
        out.application_count != 2u ||
        out.application_data_bytes != 4u ||
        out.data_after_eed_bit != 112u)
        return 0;
    memcpy(record, original, sizeof(original));
    out = saved;

    /* Correct frame CRC alone never authorizes a malformed application
     * handle, overlong data, nonterminated chain or noncanonical H. */
    set_bits(p, 36u, 4u, 4u); /* wrong H code */
    if (!musa_core_crc16_arc(0xc0c1u, record, 19u, &crc))
        return 0;
    record[19] = (uint8_t)crc;
    record[20] = (uint8_t)(crc >> 8u);
    if (musa_dwg_ac1024_record_eed_chain_probe(
            record, sizeof(original), 0u, &out) ||
        memcmp(&out, &saved, sizeof(saved))) return 0;
    memcpy(record, original, sizeof(original));

    set_bits(p, 28u, 8u, 12u); /* too many bytes for bounded DATA */
    if (!musa_core_crc16_arc(0xc0c1u, record, 19u, &crc))
        return 0;
    record[19] = (uint8_t)crc;
    record[20] = (uint8_t)(crc >> 8u);
    if (musa_dwg_ac1024_record_eed_chain_probe(
            record, sizeof(original), 0u, &out) ||
        memcmp(&out, &saved, sizeof(saved))) return 0;
    memcpy(record, original, sizeof(original));

    set_bits(p, 40u, 4u, 0u); /* no app handle ID bytes */
    if (!musa_core_crc16_arc(0xc0c1u, record, 19u, &crc))
        return 0;
    record[19] = (uint8_t)crc;
    record[20] = (uint8_t)(crc >> 8u);
    if (musa_dwg_ac1024_record_eed_chain_probe(
            record, sizeof(original), 0u, &out) ||
        memcmp(&out, &saved, sizeof(saved))) return 0;
    memcpy(record, original, sizeof(original));

    record[20] ^= 1u; /* corrupted source CRC */
    if (musa_dwg_ac1024_record_eed_chain_probe(
            record, sizeof(original), 0u, &out) ||
        memcmp(&out, &saved, sizeof(saved))) return 0;
    memcpy(record, original, sizeof(original));

    if (musa_dwg_ac1024_record_eed_chain_probe(
            record, sizeof(original) - 1u, 0u, &out) ||
        musa_dwg_ac1024_record_eed_chain_probe(
            record, sizeof(original), 0u, NULL) ||
        musa_dwg_ac1024_record_eed_chain_probe(
            record, sizeof(original), 0u,
            (MusaDwgAc1024EedChainEvidence *)(void *)record) ||
        memcmp(&out, &saved, sizeof(saved)) ||
        memcmp(record, original, sizeof(original)))
        return 0;

    /* No EED is still valid bounded framing but must NOT promote
     * the older zero-EED-only entry points to arbitrary EED. */
    uint8_t zero[13] = {0};
    if (!make_record(zero, 44u)) return 0;
    set_bits(zero + 3u, 26u, 2u, 2u);
    if (!musa_core_crc16_arc(0xc0c1u, zero, 11u, &crc))
        return 0;
    zero[11] = (uint8_t)crc;
    zero[12] = (uint8_t)(crc >> 8u);
    if (!musa_dwg_ac1024_record_eed_chain_probe(
            zero, sizeof(zero), 0u, &out) ||
        out.application_count != 0u ||
        out.application_data_bytes != 0u ||
        out.data_after_eed_bit != 28u)
        return 0;
    return 1;
}

static int test_zero_eed_subset(void) {
    _Alignas(MusaDwgAc1024ZeroEedPrefix) uint8_t record[64] = {0};
    if (!make_record(record, 44u)) return 0;

    /* Native compressed BS selector 2 means zero, using exactly 2 bits
     * after the 26-bit (OT + canonical H) prefix. */
    set_bits(record + 3u, 26u, 2u, 2u);
    uint16_t crc = 0u;
    if (!musa_core_crc16_arc(0xc0c1u, record, 11u, &crc))
        return 0;
    record[11] = (uint8_t)crc;
    record[12] = (uint8_t)(crc >> 8u);
    uint8_t untouched[13];
    memcpy(untouched, record, sizeof(untouched));
    MusaDwgAc1024ZeroEedPrefix out = {0};
    if (!musa_dwg_ac1024_record_zero_eed_prefix(
            record, 13u, 0u, &out) ||
        out.embedded_absolute_handle != 44u ||
        out.raw_ot != 19u ||
        out.payload_start_byte != 3u ||
        out.data_after_self_bit != 26u ||
        out.data_after_zero_eed_bit != 28u ||
        out.handle_stream_start_bit != 48u ||
        out.handle_stream_end_bit != 64u ||
        !out.zero_eed_verified || !out.record_crc16_verified ||
        memcmp(record, untouched, sizeof(untouched)))
        return 0;
    const MusaDwgAc1024ZeroEedPrefix saved = out;

    /* Nonzero EED has additional application data: refuse this subset,
       even with an otherwise valid original record CRC. */
    set_bits(record + 3u, 26u, 2u, 1u);
    set_bits(record + 3u, 28u, 8u, 1u);
    if (!musa_core_crc16_arc(0xc0c1u, record, 11u, &crc))
        return 0;
    record[11] = (uint8_t)crc;
    record[12] = (uint8_t)(crc >> 8u);
    if (musa_dwg_ac1024_record_zero_eed_prefix(
            record, 13u, 0u, &out) ||
        memcmp(&out, &saved, sizeof(saved)))
        return 0;

    /* 1 bit remaining for BS cannot contain a complete 2-bit selector.
       Self H is still complete, but the zero-EED gate MUST reject. */
    memcpy(record, untouched, sizeof(untouched));
    record[2] = 37u;  /* handle stream starts at bit 27 */
    if (!musa_core_crc16_arc(0xc0c1u, record, 11u, &crc))
        return 0;
    record[11] = (uint8_t)crc;
    record[12] = (uint8_t)(crc >> 8u);
    if (musa_dwg_ac1024_record_zero_eed_prefix(
            record, 13u, 0u, &out) ||
        memcmp(&out, &saved, sizeof(saved)))
        return 0;
    memcpy(record, untouched, sizeof(untouched));
    record[12] ^= 0x01u;
    if (musa_dwg_ac1024_record_zero_eed_prefix(
            record, 13u, 0u, &out) ||
        memcmp(&out, &saved, sizeof(saved)))
        return 0;
    memcpy(record, untouched, sizeof(untouched));
    if (musa_dwg_ac1024_record_zero_eed_prefix(
            record, 12u, 0u, &out) ||
        musa_dwg_ac1024_record_zero_eed_prefix(
            record, 13u, 13u, &out) ||
        musa_dwg_ac1024_record_zero_eed_prefix(
            record, 13u, 0u,
            (MusaDwgAc1024ZeroEedPrefix *)(void *)record) ||
        musa_dwg_ac1024_record_zero_eed_prefix(
            NULL, 0u, 0u, &out) ||
        memcmp(&out, &saved, sizeof(saved)) ||
        memcmp(record, untouched, sizeof(untouched)))
        return 0;
    return 1;
}

static int test_no_proxy_subset(void) {
    _Alignas(MusaDwgAc1024NoProxyPrefix) uint8_t record[64] = {0};
    if (!make_record(record, 44u)) return 0;
    set_bits(record + 3u, 26u, 2u, 2u); /* EED size zero */
    set_bits(record + 3u, 28u, 1u, 0u); /* No proxy graphic */
    uint16_t crc = 0u;
    if (!musa_core_crc16_arc(0xc0c1u, record, 11u, &crc))
        return 0;
    record[11] = (uint8_t)crc;
    record[12] = (uint8_t)(crc >> 8u);
    uint8_t original[13];
    memcpy(original, record, sizeof(original));
    MusaDwgAc1024NoProxyPrefix out = {0};
    if (!musa_dwg_ac1024_record_no_proxy_prefix(
            record, 13u, 0u, &out) ||
        out.embedded_absolute_handle != 44u ||
        out.raw_ot != 19u || out.payload_start_byte != 3u ||
        out.data_after_no_proxy_flag_bit != 29u ||
        out.handle_stream_start_bit != 48u ||
        !out.frame_crc16_verified || !out.zero_eed_verified ||
        !out.no_proxy_graphic_verified ||
        memcmp(original, record, sizeof(original)))
        return 0;
    const MusaDwgAc1024NoProxyPrefix saved = out;

    /* Flag=1 introduces graphic payload: unsupported and atomic. */
    set_bits(record + 3u, 28u, 1u, 1u);
    if (!musa_core_crc16_arc(0xc0c1u, record, 11u, &crc))
        return 0;
    record[11] = (uint8_t)crc;
    record[12] = (uint8_t)(crc >> 8u);
    if (musa_dwg_ac1024_record_no_proxy_prefix(
            record, 13u, 0u, &out) ||
        memcmp(&out, &saved, sizeof(saved)))
        return 0;

    /* The zero-EED selector can fit exactly but the next 1-bit flag
     * must never borrow bits from the separate handle stream. */
    memcpy(record, original, sizeof(original));
    record[2] = 36u; /* stream starts at bit 28 */
    if (!musa_core_crc16_arc(0xc0c1u, record, 11u, &crc))
        return 0;
    record[11] = (uint8_t)crc;
    record[12] = (uint8_t)(crc >> 8u);
    if (musa_dwg_ac1024_record_no_proxy_prefix(
            record, 13u, 0u, &out) ||
        memcmp(&out, &saved, sizeof(saved)))
        return 0;

    memcpy(record, original, sizeof(original));
    record[12] ^= 0x40u;
    if (musa_dwg_ac1024_record_no_proxy_prefix(
            record, 13u, 0u, &out) ||
        memcmp(&out, &saved, sizeof(saved)))
        return 0;
    memcpy(record, original, sizeof(original));
    if (musa_dwg_ac1024_record_no_proxy_prefix(
            record, 12u, 0u, &out) ||
        musa_dwg_ac1024_record_no_proxy_prefix(
            record, 13u, 13u, &out) ||
        musa_dwg_ac1024_record_no_proxy_prefix(
            record, 13u, 0u,
            (MusaDwgAc1024NoProxyPrefix *)(void *)record) ||
        musa_dwg_ac1024_record_no_proxy_prefix(
            NULL, 0u, 0u, &out) ||
        memcmp(&out, &saved, sizeof(saved)) ||
        memcmp(record, original, sizeof(original)))
        return 0;
    return 1;
}

static int test_entity_meta_subset(void) {
    _Alignas(MusaDwgAc1024EntityMetaPrefix) uint8_t record[64] = {0};
    if (!make_record(record, 44u)) return 0;
    set_bits(record + 3u, 26u, 2u, 2u); /* zero EED BS */
    set_bits(record + 3u, 28u, 1u, 0u); /* no proxy */
    set_bits(record + 3u, 29u, 2u, 0u); /* owner-reference mode */
    set_bits(record + 3u, 31u, 2u, 2u); /* BL zero reactors */
    set_bits(record + 3u, 33u, 1u, 1u); /* no XDictionary */
    uint16_t crc = 0u;
    if (!musa_core_crc16_arc(0xc0c1u, record, 11u, &crc)) return 0;
    record[11] = (uint8_t)crc;
    record[12] = (uint8_t)(crc >> 8u);
    uint8_t original[13];
    memcpy(original, record, sizeof(original));
    MusaDwgAc1024EntityMetaPrefix out = {0};
    if (!musa_dwg_ac1024_record_entity_meta_prefix(
            record, 13u, 0u, &out) ||
        out.raw_ot != 19u || out.embedded_absolute_handle != 44u ||
        out.payload_start_byte != 3u ||
        out.data_after_entity_mode_bit != 31u ||
        out.data_after_reactors_bit != 33u ||
        out.data_after_xdic_bit != 34u ||
        out.handle_stream_start_bit != 48u ||
        out.handle_stream_end_bit != 64u ||
        out.reactor_count != 0u || out.entity_mode != 0u ||
        out.xdictionary_missing != 1u ||
        !out.original_crc16_verified ||
        memcmp(record, original, sizeof(original)))
        return 0;
    const MusaDwgAc1024EntityMetaPrefix saved = out;

    /* Rejected reserved mode does not become an owner link. */
    set_bits(record + 3u, 29u, 2u, 3u);
    if (!musa_core_crc16_arc(0xc0c1u, record, 11u, &crc)) return 0;
    record[11] = (uint8_t)crc;
    record[12] = (uint8_t)(crc >> 8u);
    if (musa_dwg_ac1024_record_entity_meta_prefix(
            record, 13u, 0u, &out) ||
        memcmp(&saved, &out, sizeof(out)))
        return 0;

    /* Valid modes 1 and 2 have no implied explicit owner ref. */
    for (uint32_t mode = 1u; mode <= 2u; mode++) {
        memcpy(record, original, sizeof(original));
        set_bits(record + 3u, 29u, 2u, mode);
        if (!musa_core_crc16_arc(0xc0c1u, record, 11u, &crc))
            return 0;
        record[11] = (uint8_t)crc;
        record[12] = (uint8_t)(crc >> 8u);
        MusaDwgAc1024EntityMetaPrefix other = {0};
        if (!musa_dwg_ac1024_record_entity_meta_prefix(
                record, 13u, 0u, &other) || other.entity_mode != mode ||
            other.data_after_xdic_bit != 34u)
            return 0;
    }

    /* Unknown class/object type, nonzero reactor overflow, truncated
       BL and XDic flag, corrupt CRC and all aliases fail atomically. */
    memcpy(record, original, sizeof(original));
    set_bits(record + 3u, 2u, 8u, 51u); /* LAYER is not an entity */
    if (!musa_core_crc16_arc(0xc0c1u, record, 11u, &crc)) return 0;
    record[11] = (uint8_t)crc;
    record[12] = (uint8_t)(crc >> 8u);
    if (musa_dwg_ac1024_record_entity_meta_prefix(
            record, 13u, 0u, &out) ||
        memcmp(&saved, &out, sizeof(out)))
        return 0;

    memcpy(record, original, sizeof(original));
    set_bits(record + 3u, 31u, 2u, 3u); /* reserved BL opcode */
    if (!musa_core_crc16_arc(0xc0c1u, record, 11u, &crc)) return 0;
    record[11] = (uint8_t)crc;
    record[12] = (uint8_t)(crc >> 8u);
    if (musa_dwg_ac1024_record_entity_meta_prefix(
            record, 13u, 0u, &out) ||
        memcmp(&saved, &out, sizeof(out)))
        return 0;

    memcpy(record, original, sizeof(original));
    record[2] = 31u; /* separate H starts at bit 33: XDic not readable */
    if (!musa_core_crc16_arc(0xc0c1u, record, 11u, &crc)) return 0;
    record[11] = (uint8_t)crc;
    record[12] = (uint8_t)(crc >> 8u);
    if (musa_dwg_ac1024_record_entity_meta_prefix(
            record, 13u, 0u, &out) ||
        memcmp(&saved, &out, sizeof(out)))
        return 0;

    memcpy(record, original, sizeof(original));
    record[12] ^= 0x40u;
    if (musa_dwg_ac1024_record_entity_meta_prefix(
            record, 13u, 0u, &out) ||
        memcmp(&saved, &out, sizeof(out)))
        return 0;
    memcpy(record, original, sizeof(original));
    if (musa_dwg_ac1024_record_entity_meta_prefix(
            record, 12u, 0u, &out) ||
        musa_dwg_ac1024_record_entity_meta_prefix(
            record, 13u, 13u, &out) ||
        musa_dwg_ac1024_record_entity_meta_prefix(
            record, 13u, 0u,
            (MusaDwgAc1024EntityMetaPrefix *)(void *)record) ||
        musa_dwg_ac1024_record_entity_meta_prefix(
            NULL, 0u, 0u, &out) ||
        memcmp(&saved, &out, sizeof(out)) ||
        memcmp(record, original, sizeof(original)))
        return 0;
    return 1;
}

/* A lookup may use ONLY an unmodified, previously recovered index.
 * Synthetic values test search boundaries; they do not authenticate DWG. */
static int test_verified_self_index_find(void) {
    MusaDwgAc1024SelfIdentity items[5] = {
        {1u, 0u, 19u, 1u, 1u},
        {5u, 13u, 17u, 1u, 1u},
        {8u, 26u, 18u, 1u, 1u},
        {256u, 39u, 7u, 1u, 1u},
        {5000u, 52u, 19u, 1u, 1u}
    };
    MusaDwgAc1024SelfIdentity result = {0};
    for (size_t i = 0u; i < 5u; i++) {
        if (!musa_dwg_ac1024_self_index_find(
                items, 5u, items[i].embedded_absolute_handle, &result) ||
            result.embedded_absolute_handle !=
                items[i].embedded_absolute_handle ||
            result.record_offset != items[i].record_offset ||
            result.raw_ot != items[i].raw_ot ||
            !result.frame_crc16_verified ||
            !result.embedded_handle_code0_verified)
            return 0;
    }
    const MusaDwgAc1024SelfIdentity saved = result;
    const uint64_t absent[] = {0u, 2u, 7u, 9u, 255u, 257u, 5001u, UINT64_MAX};
    for (size_t i = 0u; i < sizeof(absent) / sizeof(absent[0]); i++) {
        if (musa_dwg_ac1024_self_index_find(
                items, 5u, absent[i], &result) ||
            memcmp(&saved, &result, sizeof(result)))
            return 0;
    }
    if (musa_dwg_ac1024_self_index_find(
            NULL, 5u, 5u, &result) ||
        musa_dwg_ac1024_self_index_find(
            items, 0u, 5u, &result) ||
        musa_dwg_ac1024_self_index_find(
            items, SIZE_MAX, 5u, &result) ||
        musa_dwg_ac1024_self_index_find(
            items, 5u, 5u, NULL) ||
        musa_dwg_ac1024_self_index_find(
            items, 5u, 5u, &items[1]) ||
        memcmp(&saved, &result, sizeof(result)))
        return 0;

    items[2].frame_crc16_verified = 0u;
    if (musa_dwg_ac1024_self_index_find(items, 5u, 8u, &result) ||
        musa_dwg_ac1024_self_index_find(items, 5u, 5u, &result))
        return 0;
    items[2].frame_crc16_verified = 1u;
    items[2].embedded_handle_code0_verified = 0u;
    if (musa_dwg_ac1024_self_index_find(items, 5u, 8u, &result))
        return 0;
    items[2].embedded_handle_code0_verified = 1u;

    /* Neighbor duplicate/order violations are not accepted as evidence. */
    items[1].embedded_absolute_handle = 8u;
    if (musa_dwg_ac1024_self_index_find(items, 5u, 8u, &result))
        return 0;
    items[1].embedded_absolute_handle = 5u;
    items[3].embedded_absolute_handle = 8u;
    if (musa_dwg_ac1024_self_index_find(items, 5u, 8u, &result))
        return 0;
    items[3].embedded_absolute_handle = 256u;
    return memcmp(&saved, &result, sizeof(result)) == 0;
}

/* Verify canonical H -> original owner OT49 / layer OT51, not a visual
 * scene. Both target records are real CRC-backed synthetic frames. */
static int test_owner_layer_bridge(void) {
    _Alignas(MusaDwgAc1024OwnerLayerEvidence) uint8_t src[47] = {0};
    if (!make_record(src, 32u) || !make_record(src + 13u, 47u))
        return 0;
    set_bits(src + 3u, 2u, 8u, 51u);
    set_bits(src + 16u, 2u, 8u, 49u);
    uint16_t crc = 0u;
    for (size_t at = 0u; at < 26u; at += 13u) {
        if (!musa_core_crc16_arc(0xc0c1u, src + at, 11u, &crc))
            return 0;
        src[at + 11u] = (uint8_t)crc;
        src[at + 12u] = (uint8_t)(crc >> 8u);
    }
    uint8_t *const line = src + 26u, *const body = line + 3u;
    line[0] = 16u; line[2] = 48u; /* MS=16 bytes, MC=48 handle bits */
    set_bits(body, 2u, 8u, 19u); /* LINE */
    set_bits(body, 14u, 4u, 1u);
    set_bits(body, 18u, 8u, 48u); /* canonical self H */
    set_bits(body, 26u, 2u, 2u); /* zero EED */
    set_bits(body, 31u, 2u, 2u); /* zero reactors */
    set_bits(body, 33u, 1u, 1u); /* no XDictionary */
    set_bits(body, 80u, 4u, 12u); set_bits(body, 84u, 4u, 1u);
    set_bits(body, 88u, 8u, 1u);  /* owner H12 (-1) -> OT49 */
    set_bits(body, 96u, 4u, 5u); set_bits(body, 100u, 4u, 1u);
    set_bits(body, 104u, 8u, 32u); /* layer H5 -> OT51 */
    if (!musa_core_crc16_arc(0xc0c1u, line, 19u, &crc)) return 0;
    line[19u] = (uint8_t)crc; line[20u] = (uint8_t)(crc >> 8u);
    uint8_t pristine[47];
    memcpy(pristine, src, sizeof(src));
    const uint64_t offsets[3] = {0u, 13u, 26u};
    MusaDwgAc1024SelfIdentity idx[3] = {{0}};
    if (!musa_dwg_ac1024_recover_self_index(
            src, sizeof(src), offsets, 3u, idx, 3u)) return 0;
    MusaDwgAc1024OwnerLayerEvidence links = {0};
    if (!musa_dwg_ac1024_entity_owner_layer_probe(
            src, sizeof(src), idx, 3u, 48u, &links) ||
        links.owner_handle != 47u || links.layer_handle != 32u ||
        links.owner_record_offset != 13u ||
        links.layer_record_offset != 0u ||
        links.entity_record_offset != 26u ||
        links.entity_raw_ot != 19u ||
        links.owner_raw_h_code != 12u || links.layer_raw_h_code != 5u ||
        links.handle_tail_unparsed_bits != 16u ||
        !links.source_crc_verified || !links.target_records_verified ||
        memcmp(src, pristine, sizeof(src))) return 0;
    const MusaDwgAc1024OwnerLayerEvidence saved = links;
    set_bits(body, 104u, 8u, 33u); /* nonexistent target */
    if (!musa_core_crc16_arc(0xc0c1u, line, 19u, &crc)) return 0;
    line[19u] = (uint8_t)crc; line[20u] = (uint8_t)(crc >> 8u);
    if (musa_dwg_ac1024_entity_owner_layer_probe(
            src, sizeof(src), idx, 3u, 48u, &links) ||
        memcmp(&saved, &links, sizeof(saved))) return 0;
    memcpy(src, pristine, sizeof(src));
    line[20u] ^= 1u; /* CRC corruption */
    if (musa_dwg_ac1024_entity_owner_layer_probe(
            src, sizeof(src), idx, 3u, 48u, &links) ||
        memcmp(&saved, &links, sizeof(saved))) return 0;
    memcpy(src, pristine, sizeof(src));
    idx[1].raw_ot = 19u; /* source type must not be faked */
    if (musa_dwg_ac1024_entity_owner_layer_probe(
            src, sizeof(src), idx, 3u, 48u, &links) ||
        memcmp(&saved, &links, sizeof(saved))) return 0;
    idx[1].raw_ot = 49u;
    if (musa_dwg_ac1024_entity_owner_layer_probe(
            src, sizeof(src), idx, 3u, 0u, &links) ||
        musa_dwg_ac1024_entity_owner_layer_probe(
            src, sizeof(src), idx, 3u, 48u, NULL) ||
        musa_dwg_ac1024_entity_owner_layer_probe(
            src, sizeof(src), idx, 3u, 48u,
            (MusaDwgAc1024OwnerLayerEvidence *)(void *)src) ||
        memcmp(&saved, &links, sizeof(saved)) ||
        memcmp(src, pristine, sizeof(src))) return 0;
    return 1;
}


/* Exercise all four relative H wire forms against authentic in-memory,
 * CRC-backed target OT49/OT51 records. No scene or DWG writer is admitted. */
static int make_relative_links(uint8_t src[47], uint8_t entity_h,
                               uint8_t owner_code, uint8_t owner_count,
                               uint8_t owner_value, uint8_t layer_code,
                               uint8_t layer_count, uint8_t layer_value) {
    memset(src, 0, 47u);
    if (!make_record(src, 32u) || !make_record(src + 13u, 47u))
        return 0;
    set_bits(src + 3u, 2u, 8u, 51u); /* LAYER H32 */
    set_bits(src + 16u, 2u, 8u, 49u); /* BLOCK_HEADER H47 */
    uint16_t crc = 0u;
    for (size_t at = 0u; at < 26u; at += 13u) {
        if (!musa_core_crc16_arc(0xc0c1u, src + at, 11u, &crc))
            return 0;
        src[at + 11u] = (uint8_t)crc;
        src[at + 12u] = (uint8_t)(crc >> 8u);
    }
    uint8_t *const line = src + 26u;
    uint8_t *const payload = line + 3u;
    line[0] = 16u; line[2] = 48u; /* 16-byte MS, 48-bit H stream */
    set_bits(payload, 2u, 8u, 19u);
    set_bits(payload, 14u, 4u, 1u);
    set_bits(payload, 18u, 8u, entity_h);
    set_bits(payload, 26u, 2u, 2u); /* no EED */
    set_bits(payload, 31u, 2u, 2u); /* no reactors */
    set_bits(payload, 33u, 1u, 1u); /* no XDictionary */
    size_t at = 80u;
    set_bits(payload, at, 4u, owner_code); at += 4u;
    set_bits(payload, at, 4u, owner_count); at += 4u;
    if (owner_count == 1u) {
        set_bits(payload, at, 8u, owner_value); at += 8u;
    }
    set_bits(payload, at, 4u, layer_code); at += 4u;
    set_bits(payload, at, 4u, layer_count); at += 4u;
    if (layer_count == 1u) {
        set_bits(payload, at, 8u, layer_value); at += 8u;
    }
    if (at > 128u || !musa_core_crc16_arc(0xc0c1u, line, 19u, &crc))
        return 0;
    line[19u] = (uint8_t)crc;
    line[20u] = (uint8_t)(crc >> 8u);
    return 1;
}

static int test_relative_owner_layer_codes(void) {
    uint8_t src[47] = {0};
    uint8_t saved_source[47] = {0};
    MusaDwgAc1024SelfIdentity index[3] = {{0}};
    MusaDwgAc1024OwnerLayerEvidence links = {0};
    /* H8 means self-1 (owner 47); HC means self-16 (layer 32). */
    if (!make_relative_links(src, 48u, 8u, 0u, 0u, 12u, 1u, 16u))
        return 0;
    const uint64_t backward_offsets[] = {0u, 13u, 26u};
    if (!musa_dwg_ac1024_recover_self_index(
            src, sizeof(src), backward_offsets, 3u, index, 3u))
        return 0;
    memcpy(saved_source, src, sizeof(src));
    if (!musa_dwg_ac1024_entity_owner_layer_probe(
            src, sizeof(src), index, 3u, 48u, &links) ||
        links.owner_handle != 47u || links.layer_handle != 32u ||
        links.owner_raw_h_code != 8u || links.layer_raw_h_code != 12u ||
        links.owner_record_offset != 13u || links.layer_record_offset != 0u ||
        links.handle_tail_unparsed_bits != 24u ||
        !links.target_records_verified ||
        memcmp(saved_source, src, sizeof(src)))
        return 0;
    const MusaDwgAc1024OwnerLayerEvidence backward = links;

    /* HA adds 16 to self 31 for owner H47, H6 adds one for layer H32.
       The recovered handle order is NOT the physical record-offset order. */
    if (!make_relative_links(src, 31u, 10u, 1u, 16u, 6u, 0u, 0u))
        return 0;
    const uint64_t forward_offsets[] = {26u, 0u, 13u};
    if (!musa_dwg_ac1024_recover_self_index(
            src, sizeof(src), forward_offsets, 3u, index, 3u))
        return 0;
    memcpy(saved_source, src, sizeof(src));
    if (!musa_dwg_ac1024_entity_owner_layer_probe(
            src, sizeof(src), index, 3u, 31u, &links) ||
        links.owner_handle != 47u || links.layer_handle != 32u ||
        links.owner_raw_h_code != 10u || links.layer_raw_h_code != 6u ||
        links.entity_record_offset != 26u ||
        links.handle_tail_unparsed_bits != 24u ||
        memcmp(saved_source, src, sizeof(src)))
        return 0;

    /* A zero-payload H6 with count=1 is malformed; a valid relative
       identity pointing at a nonexistent owner is not a proven link. */
    if (!make_relative_links(src, 31u, 6u, 1u, 47u, 6u, 0u, 0u))
        return 0;
    if (musa_dwg_ac1024_entity_owner_layer_probe(
            src, sizeof(src), index, 3u, 31u, &links))
        return 0;
    if (!make_relative_links(src, 31u, 10u, 1u, 17u, 6u, 0u, 0u))
        return 0;
    if (musa_dwg_ac1024_entity_owner_layer_probe(
            src, sizeof(src), index, 3u, 31u, &links))
        return 0;
    /* HC underflow, invalid reference code, absent referenced OT and
       corrupt CRC never become owner/layer evidence. */
    if (!make_relative_links(src, 31u, 12u, 1u, 32u, 6u, 0u, 0u))
        return 0;
    if (musa_dwg_ac1024_entity_owner_layer_probe(
            src, sizeof(src), index, 3u, 31u, &links))
        return 0;
    if (!make_relative_links(src, 31u, 3u, 1u, 16u, 6u, 0u, 0u))
        return 0;
    if (musa_dwg_ac1024_entity_owner_layer_probe(
            src, sizeof(src), index, 3u, 31u, &links))
        return 0;
    if (!make_relative_links(src, 31u, 10u, 1u, 16u, 6u, 0u, 0u))
        return 0;
    src[46u] ^= 0x40u;
    if (musa_dwg_ac1024_entity_owner_layer_probe(
            src, sizeof(src), index, 3u, 31u, &links))
        return 0;
    /* All rejected probes preserve last successful evidence. */
    if (links.entity_handle != 31u || links.owner_handle != 47u ||
        links.layer_handle != 32u ||
        backward.entity_handle != 48u ||
        backward.owner_raw_h_code != 8u)
        return 0;
    return 1;
}


/* Original CRC-authenticated R2010 LAYER OT51 numerical FLAGS, NOT
 * an interpreted visual state. Offsets are determined by original
 * BS/BL/B common-object fields and then the documented LAYER values.
 * The source record is not modified on success or on any refusal. */
static int test_layer_raw_flags_preflight(void) {
    _Alignas(MusaDwgAc1024LayerRawFlagsEvidence)
    uint8_t source[21] = {0};
    source[0] = 16u; /* 16-byte payload, CRC at 19 */
    source[2] = 16u; /* 16 H bits; DATA ends at payload bit112 */
    uint8_t *const p = source + 3u;
    set_bits(p, 2u, 8u, 51u); /* original LAYER OT51 */
    set_bits(p, 14u, 4u, 1u);
    set_bits(p, 18u, 8u, 42u); /* original absolute self H42 */
    set_bits(p, 26u, 2u, 2u); /* BS zero EED */
    set_bits(p, 28u, 2u, 2u); /* BL zero reactors */
    set_bits(p, 30u, 1u, 1u); /* XDictionary missing */
    set_bits(p, 31u, 1u, 0u); /* referenced 64-bit flag */
    set_bits(p, 32u, 2u, 2u); /* BS zero xref idx+1 */
    set_bits(p, 34u, 1u, 0u); /* xref-dependent false */
    set_bits(p, 35u, 2u, 1u); /* BS raw 8-bit flags */
    set_bits(p, 37u, 8u, 0xabu); /* raw 70/290/370 numeric field */
    uint16_t crc = 0u;
    if (!musa_core_crc16_arc(0xc0c1u, source, 19u, &crc))
        return 0;
    source[19] = (uint8_t)crc;
    source[20] = (uint8_t)(crc >> 8u);
    const uint64_t offsets[1] = {0u};
    MusaDwgAc1024SelfIdentity idx[1] = {{0}};
    if (!musa_dwg_ac1024_recover_self_index(
            source, sizeof(source), offsets, 1u, idx, 1u))
        return 0;
    uint8_t pristine[sizeof(source)];
    memcpy(pristine, source, sizeof(source));
    MusaDwgAc1024LayerRawFlagsEvidence flags = {0};
    if (!musa_dwg_ac1024_layer_raw_flags_probe(
            source, sizeof(source), idx, 1u, 42u, &flags) ||
        flags.layer_handle != 42u ||
        flags.layer_record_offset != 0u ||
        flags.raw_flags_start_bit != 31u ||
        flags.raw_flags_end_bit != 45u ||
        flags.raw_values_70_290_370 != 0xabu ||
        flags.unmapped_bits != 0u ||
        flags.entry_64_flag != 0u ||
        flags.xref_index_plus_one != 0u ||
        flags.xref_dependent != 0u ||
        flags.xdictionary_missing != 1u ||
        flags.frozen_bit != 1u || flags.on_bit != 1u ||
        flags.frozen_new_viewports_bit != 0u ||
        flags.locked_bit != 1u || flags.plotting_bit != 0u ||
        flags.lineweight_field != 5u ||
        !flags.original_record_crc_verified ||
        flags.display_accepted != 0u ||
        memcmp(source, pristine, sizeof(source)))
        return 0;
    const MusaDwgAc1024LayerRawFlagsEvidence saved = flags;
    if (musa_dwg_ac1024_layer_raw_flags_failure_stage(
            source, sizeof(source), idx, 1u, 42u) != MUSA_DWG_LAYER_FLAGS_OK ||
        musa_dwg_ac1024_layer_raw_flags_failure_stage(
            NULL, sizeof(source), idx, 1u, 42u) !=
                MUSA_DWG_LAYER_FLAGS_ARGUMENT)
        return 0;

    /* Unknown upper numeric bits are exposed, never ignored or rendered. */
    set_bits(p, 35u, 2u, 0u); /* BS raw16 */
    set_bits(p, 37u, 8u, 0xe0u); /* low */
    set_bits(p, 45u, 8u, 0x80u); /* high -> raw 0x80e0 */
    if (!musa_core_crc16_arc(0xc0c1u, source, 19u, &crc))
        return 0;
    source[19] = (uint8_t)crc;
    source[20] = (uint8_t)(crc >> 8u);
    if (!musa_dwg_ac1024_layer_raw_flags_probe(
            source, sizeof(source), idx, 1u, 42u, &flags) ||
        flags.raw_values_70_290_370 != 0x80e0u ||
        flags.unmapped_bits != 0x8000u ||
        flags.lineweight_field != 7u ||
        flags.display_accepted != 0u)
        return 0;
    memcpy(source, pristine, sizeof(source));
    flags = saved;

    /* Presence of XDictionary is preserved, but its target is not
     * verified here, so it cannot authorize visibility or drawing. */
    set_bits(p, 30u, 1u, 0u);
    if (!musa_core_crc16_arc(0xc0c1u, source, 19u, &crc))
        return 0;
    source[19] = (uint8_t)crc;
    source[20] = (uint8_t)(crc >> 8u);
    if (!musa_dwg_ac1024_layer_raw_flags_probe(
            source, sizeof(source), idx, 1u, 42u, &flags) ||
        flags.xdictionary_missing != 0u || flags.display_accepted)
        return 0;
    memcpy(source, pristine, sizeof(source));
    flags = saved;

    /* Unsupported reactor data, CRC damage and forged OT all refuse. */
    set_bits(p, 28u, 2u, 1u); /* BL raw8 reactor count 1 */
    set_bits(p, 30u, 8u, 1u);
    if (!musa_core_crc16_arc(0xc0c1u, source, 19u, &crc))
        return 0;
    source[19] = (uint8_t)crc;
    source[20] = (uint8_t)(crc >> 8u);
    if (musa_dwg_ac1024_layer_raw_flags_probe(
            source, sizeof(source), idx, 1u, 42u, &flags) ||
        musa_dwg_ac1024_layer_raw_flags_failure_stage(
            source, sizeof(source), idx, 1u, 42u) !=
                MUSA_DWG_LAYER_FLAGS_REACTORS ||
        memcmp(&flags, &saved, sizeof(saved)))
        return 0;
    memcpy(source, pristine, sizeof(source));

    /* Raw16 BS cannot fit inside the CRC-authenticated DATA stream:
     * MC moves handle start to bit40, after preamble, before raw flags
     * end at bit53. BS selector 3 is valid 256 and must NOT be rejected. */
    source[2] = 88u; /* 128 total payload bits - 88 H bits = 40 DATA */
    set_bits(p, 35u, 2u, 0u); /* needs 16 more bits before H */
    if (!musa_core_crc16_arc(0xc0c1u, source, 19u, &crc))
        return 0;
    source[19] = (uint8_t)crc;
    source[20] = (uint8_t)(crc >> 8u);
    if (musa_dwg_ac1024_layer_raw_flags_probe(
            source, sizeof(source), idx, 1u, 42u, &flags) ||
        musa_dwg_ac1024_layer_raw_flags_failure_stage(
            source, sizeof(source), idx, 1u, 42u) !=
                MUSA_DWG_LAYER_FLAGS_NUMERIC ||
        memcmp(&flags, &saved, sizeof(saved)))
        return 0;
    memcpy(source, pristine, sizeof(source));

    source[20] ^= 1u;
    if (musa_dwg_ac1024_layer_raw_flags_probe(
            source, sizeof(source), idx, 1u, 42u, &flags) ||
        memcmp(&flags, &saved, sizeof(saved)))
        return 0;
    memcpy(source, pristine, sizeof(source));

    idx[0].raw_ot = 19u;
    if (musa_dwg_ac1024_layer_raw_flags_probe(
            source, sizeof(source), idx, 1u, 42u, &flags) ||
        memcmp(&flags, &saved, sizeof(saved)))
        return 0;
    idx[0].raw_ot = 51u;
    if (musa_dwg_ac1024_layer_raw_flags_probe(
            source, sizeof(source), idx, 1u, 0u, &flags) ||
        musa_dwg_ac1024_layer_raw_flags_probe(
            source, sizeof(source), idx, 1u, 42u, NULL) ||
        musa_dwg_ac1024_layer_raw_flags_probe(
            source, sizeof(source), idx, 1u, 42u,
            (MusaDwgAc1024LayerRawFlagsEvidence *)(void *)source) ||
        musa_dwg_ac1024_layer_raw_flags_probe(
            source, sizeof(source), idx, 1u, 42u,
            (MusaDwgAc1024LayerRawFlagsEvidence *)(void *)idx) ||
        memcmp(&flags, &saved, sizeof(saved)) ||
        memcmp(source, pristine, sizeof(source)))
        return 0;
    return 1;
}

/* Original CRC-backed OT51 with NONZERO EED: the application H5/opaque
 * frame must be terminated before the reactor and source flags. Test is
 * DATA-only; no viewport display status or APPID semantics are inferred. */
static int test_layer_nonzero_eed_raw_flags(void) {
    _Alignas(MusaDwgAc1024LayerRawFlagsEvidence)
    uint8_t source[29] = {0};
    source[0] = 24u; /* 24-byte source body, CRC at 27..28 */
    source[2] = 16u; /* H tail 16 bits at payload bit176 */
    uint8_t *const p = source + 3u;
    set_bits(p, 2u, 8u, 51u);
    set_bits(p, 14u, 4u, 1u);
    set_bits(p, 18u, 8u, 42u); /* original LAYER self H42 */
    set_bits(p, 26u, 2u, 1u); /* BS EED len1 */
    set_bits(p, 28u, 8u, 1u);
    set_bits(p, 36u, 4u, 5u); /* APPID H5, not yet target-resolved */
    set_bits(p, 40u, 4u, 1u);
    set_bits(p, 44u, 8u, 40u);
    set_bits(p, 52u, 8u, 0x70u); /* opaque application content */
    set_bits(p, 60u, 2u, 2u); /* EED terminator BS zero */
    set_bits(p, 62u, 2u, 2u); /* BL zero reactors */
    set_bits(p, 64u, 1u, 1u); /* XDictionary absent */
    set_bits(p, 65u, 1u, 0u); /* source 64-bit flag */
    set_bits(p, 66u, 2u, 2u); /* BS xref index+1 = 0 */
    set_bits(p, 68u, 1u, 0u); /* source xref-dependent */
    set_bits(p, 69u, 2u, 1u); /* BS raw layer flags as 8bit */
    set_bits(p, 71u, 8u, 0xabu);
    uint16_t crc = 0u;
    if (!musa_core_crc16_arc(0xc0c1u, source, 27u, &crc))
        return 0;
    source[27] = (uint8_t)crc;
    source[28] = (uint8_t)(crc >> 8u);
    const uint64_t offsets[1] = {0u};
    MusaDwgAc1024SelfIdentity index[1] = {{0}};
    if (!musa_dwg_ac1024_recover_self_index(
            source, sizeof(source), offsets, 1u, index, 1u))
        return 0;
    uint8_t pristine[sizeof(source)];
    memcpy(pristine, source, sizeof(source));
    MusaDwgAc1024LayerRawFlagsEvidence flags = {0};
    if (!musa_dwg_ac1024_layer_eed_raw_flags_probe(
            source, sizeof(source), index, 1u, 42u, &flags) ||
        flags.layer_handle != 42u || flags.layer_record_offset != 0u ||
        flags.raw_flags_start_bit != 65u ||
        flags.raw_flags_end_bit != 79u ||
        flags.raw_values_70_290_370 != 0xabu ||
        !flags.original_record_crc_verified ||
        flags.display_accepted != 0u ||
        flags.frozen_bit != 1u || flags.locked_bit != 1u ||
        flags.lineweight_field != 5u ||
        musa_dwg_ac1024_layer_raw_flags_probe(
            source, sizeof(source), index, 1u, 42u, &flags) ||
        memcmp(pristine, source, sizeof(source)))
        return 0;
    const MusaDwgAc1024LayerRawFlagsEvidence saved = flags;

    /* Corrupted application H code is still refused with correct CRC. */
    set_bits(p, 36u, 4u, 4u);
    if (!musa_core_crc16_arc(0xc0c1u, source, 27u, &crc))
        return 0;
    source[27] = (uint8_t)crc;
    source[28] = (uint8_t)(crc >> 8u);
    if (musa_dwg_ac1024_layer_eed_raw_flags_probe(
            source, sizeof(source), index, 1u, 42u, &flags) ||
        memcmp(&flags, &saved, sizeof(saved)))
        return 0;
    memcpy(source, pristine, sizeof(source));

    /* Unsupported reactor count must never be silently skipped. */
    set_bits(p, 62u, 2u, 1u); /* BL byte follows */
    set_bits(p, 64u, 8u, 1u);
    if (!musa_core_crc16_arc(0xc0c1u, source, 27u, &crc))
        return 0;
    source[27] = (uint8_t)crc;
    source[28] = (uint8_t)(crc >> 8u);
    if (musa_dwg_ac1024_layer_eed_raw_flags_probe(
            source, sizeof(source), index, 1u, 42u, &flags) ||
        memcmp(&flags, &saved, sizeof(saved)))
        return 0;
    memcpy(source, pristine, sizeof(source));

    /* No CRC or canonical source index bypass. */
    source[28] ^= 1u;
    if (musa_dwg_ac1024_layer_eed_raw_flags_probe(
            source, sizeof(source), index, 1u, 42u, &flags) ||
        memcmp(&flags, &saved, sizeof(saved)))
        return 0;
    memcpy(source, pristine, sizeof(source));
    index[0].raw_ot = 19u;
    if (musa_dwg_ac1024_layer_eed_raw_flags_probe(
            source, sizeof(source), index, 1u, 42u, &flags) ||
        memcmp(&flags, &saved, sizeof(saved)))
        return 0;
    index[0].raw_ot = 51u;
    if (musa_dwg_ac1024_layer_eed_raw_flags_probe(
            source, sizeof(source), index, 1u, 42u, NULL) ||
        musa_dwg_ac1024_layer_eed_raw_flags_probe(
            source, sizeof(source), index, 1u, 42u,
            (MusaDwgAc1024LayerRawFlagsEvidence *)(void *)source) ||
        musa_dwg_ac1024_layer_eed_raw_flags_probe(
            source, sizeof(source), index, 1u, 42u,
            (MusaDwgAc1024LayerRawFlagsEvidence *)(void *)index) ||
        memcmp(&flags, &saved, sizeof(saved)) ||
        memcmp(pristine, source, sizeof(source)))
        return 0;
    return 1;
}

/* CRC-valid LAYER OT51 with a nonempty EED application H5 -> APPID
 * OT67 source record, and 0 application semantics/scene permission. */
static int test_layer_eed_appid_original_target(void) {
    _Alignas(MusaDwgAc1024EedAppidEvidence)
    uint8_t records[42] = {0};
    uint8_t *const appid = records;
    uint8_t *const layer = records + 13u;
    appid[0] = 8u;
    appid[2] = 16u;
    set_bits(appid + 3u, 2u, 8u, 67u);
    set_bits(appid + 3u, 14u, 4u, 1u);
    set_bits(appid + 3u, 18u, 8u, 40u);
    uint16_t crc = 0u;
    if (!musa_core_crc16_arc(0xc0c1u, appid, 11u, &crc))
        return 0;
    appid[11] = (uint8_t)crc; appid[12] = (uint8_t)(crc >> 8u);
    layer[0] = 24u; layer[2] = 16u;
    uint8_t *const p = layer + 3u;
    set_bits(p, 2u, 8u, 51u);
    set_bits(p, 14u, 4u, 1u);
    set_bits(p, 18u, 8u, 42u);
    set_bits(p, 26u, 2u, 1u);
    set_bits(p, 28u, 8u, 1u);
    set_bits(p, 36u, 4u, 5u);
    set_bits(p, 40u, 4u, 1u);
    set_bits(p, 44u, 8u, 40u);
    set_bits(p, 52u, 8u, 0x70u);
    set_bits(p, 60u, 2u, 2u);
    set_bits(p, 62u, 2u, 2u);
    set_bits(p, 64u, 1u, 1u);
    set_bits(p, 66u, 2u, 2u);
    set_bits(p, 69u, 2u, 1u);
    set_bits(p, 71u, 8u, 0xabu);
    if (!musa_core_crc16_arc(0xc0c1u, layer, 27u, &crc))
        return 0;
    layer[27] = (uint8_t)crc; layer[28] = (uint8_t)(crc >> 8u);
    const uint64_t offsets[2] = {0u, 13u};
    MusaDwgAc1024SelfIdentity index[2] = {{0}};
    if (!musa_dwg_ac1024_recover_self_index(
            records, sizeof(records), offsets, 2u, index, 2u))
        return 0;
    uint8_t pristine[sizeof(records)];
    memcpy(pristine, records, sizeof(records));
    MusaDwgAc1024EedAppidEvidence evidence = {0};
    if (!musa_dwg_ac1024_layer_eed_appid_probe(
            records, sizeof(records), index, 2u, 42u, &evidence) ||
        evidence.chain.raw_ot != 51u ||
        evidence.chain.indexed_object_offset != 13u ||
        evidence.chain.embedded_absolute_handle != 42u ||
        evidence.chain.application_count != 1u ||
        evidence.appid_reference_count != 1u ||
        evidence.distinct_appid_count != 1u ||
        evidence.first_appid_handle != 40u ||
        evidence.first_appid_record_offset != 0u ||
        !evidence.original_appid_records_crc_verified ||
        musa_dwg_ac1024_line_eed_appid_probe(
            records, sizeof(records), index, 2u, 42u, &evidence) ||
        memcmp(records, pristine, sizeof(records)))
        return 0;
    const MusaDwgAc1024EedAppidEvidence saved = evidence;

    /* The original APPID target's CRC must match even when LAYER CRC
     * and the EED application bytes themselves are valid. */
    appid[12] ^= 1u;
    if (musa_dwg_ac1024_layer_eed_appid_probe(
            records, sizeof(records), index, 2u, 42u, &evidence) ||
        memcmp(&evidence, &saved, sizeof(saved)))
        return 0;
    memcpy(records, pristine, sizeof(records));

    index[0].raw_ot = 51u;
    if (musa_dwg_ac1024_layer_eed_appid_probe(
            records, sizeof(records), index, 2u, 42u, &evidence) ||
        memcmp(&evidence, &saved, sizeof(saved)))
        return 0;
    index[0].raw_ot = 67u;

    set_bits(p, 44u, 8u, 41u); /* dangling APPID H5 */
    if (!musa_core_crc16_arc(0xc0c1u, layer, 27u, &crc))
        return 0;
    layer[27] = (uint8_t)crc; layer[28] = (uint8_t)(crc >> 8u);
    if (musa_dwg_ac1024_layer_eed_appid_probe(
            records, sizeof(records), index, 2u, 42u, &evidence) ||
        memcmp(&evidence, &saved, sizeof(saved)))
        return 0;
    memcpy(records, pristine, sizeof(records));

    if (musa_dwg_ac1024_layer_eed_appid_probe(
            records, sizeof(records), index, 2u, 0u, &evidence) ||
        musa_dwg_ac1024_layer_eed_appid_probe(
            records, sizeof(records), index, 2u, 42u, NULL) ||
        musa_dwg_ac1024_layer_eed_appid_probe(
            records, sizeof(records), index, 2u, 42u,
            (MusaDwgAc1024EedAppidEvidence *)(void *)records) ||
        musa_dwg_ac1024_layer_eed_appid_probe(
            records, sizeof(records), index, 2u, 42u,
            (MusaDwgAc1024EedAppidEvidence *)(void *)index) ||
        memcmp(&evidence, &saved, sizeof(saved)) ||
        memcmp(records, pristine, sizeof(records)))
        return 0;
    return 1;
}

/* H-stream roles are source-order-specific: OT50 LAYER_CONTROL H4
 * then OT42 DICTIONARY H3; no subsequent LTYPE, material, plotstyle or
 * display state is inferred. All original target CRCs must remain valid. */
static int test_layer_eed_xdictionary_original_graph(void) {
    uint8_t bytes[68] = {0};
    const uint64_t offsets[4] = {0u, 13u, 26u, 39u};
    const uint16_t types[3] = {67u, 50u, 42u};
    const uint32_t handles[3] = {40u, 41u, 43u};
    uint16_t crc = 0u;
    for (size_t k = 0u; k < 3u; ++k) {
        uint8_t *const rec = bytes + 13u * k;
        if (!make_record(rec, handles[k]))
            return 0;
        set_bits(rec + 3u, 2u, 8u, types[k]);
        if (!musa_core_crc16_arc(0xc0c1u, rec, 11u, &crc))
            return 0;
        rec[11] = (uint8_t)crc;
        rec[12] = (uint8_t)(crc >> 8u);
    }
    uint8_t *const layer = bytes + 39u;
    uint8_t *const p = layer + 3u;
    layer[0] = 24u; /* 24-byte payload, record CRC at 27 */
    layer[2] = 32u; /* two role-specific Hrefs at bits160..191 */
    set_bits(p, 2u, 8u, 51u);
    set_bits(p, 14u, 4u, 1u);
    set_bits(p, 18u, 8u, 45u);
    set_bits(p, 26u, 2u, 1u); /* EED BS length 1 */
    set_bits(p, 28u, 8u, 1u);
    set_bits(p, 36u, 4u, 5u); /* original APPID H5 -> OT67/H40 */
    set_bits(p, 40u, 4u, 1u);
    set_bits(p, 44u, 8u, 40u);
    set_bits(p, 52u, 8u, 0x70u); /* opaque application data */
    set_bits(p, 60u, 2u, 2u); /* terminating BS=0 */
    set_bits(p, 62u, 2u, 2u); /* BL=0 reactors */
    set_bits(p, 64u, 1u, 0u); /* XDictionary PRESENT */
    set_bits(p, 66u, 2u, 2u); /* zero xrefindex+1 */
    set_bits(p, 69u, 2u, 1u); /* raw layer flags BS byte */
    set_bits(p, 71u, 8u, 0xabu);
    set_bits(p, 160u, 4u, 4u); /* soft pointer H4 to layer CTRL */
    set_bits(p, 164u, 4u, 1u);
    set_bits(p, 168u, 8u, 41u);
    set_bits(p, 176u, 4u, 3u); /* hard owner H3 to dictionary */
    set_bits(p, 180u, 4u, 1u);
    set_bits(p, 184u, 8u, 43u);
    if (!musa_core_crc16_arc(0xc0c1u, layer, 27u, &crc))
        return 0;
    layer[27] = (uint8_t)crc;
    layer[28] = (uint8_t)(crc >> 8u);

    MusaDwgAc1024SelfIdentity index[4] = {{0}};
    if (!musa_dwg_ac1024_recover_self_index(
            bytes, sizeof(bytes), offsets, 4u, index, 4u))
        return 0;
    uint8_t unchanged[sizeof(bytes)];
    memcpy(unchanged, bytes, sizeof(unchanged));
    MusaDwgAc1024LayerEedXDictionaryGraph result = {0};
    if (!musa_dwg_ac1024_layer_eed_xdictionary_graph_probe(
            bytes, sizeof(bytes), index, 4u, 45u, &result) ||
        result.layer_handle != 45u ||
        result.layer_record_offset != 39u ||
        result.layer_control_handle != 41u ||
        result.layer_control_record_offset != 13u ||
        result.xdictionary_handle != 43u ||
        result.xdictionary_record_offset != 26u ||
        result.remaining_handle_tail_bits != 0u ||
        result.eed_appid_reference_count != 1u ||
        result.layer_control_raw_h_code != 4u ||
        result.xdictionary_raw_h_code != 3u ||
        !result.original_records_crc_verified || result.display_accepted ||
        memcmp(unchanged, bytes, sizeof(bytes)))
        return 0;
    MusaDwgAc1024LayerEedRawHandleHeader raw = {0};
    if (!musa_dwg_ac1024_layer_eed_first_h_header_probe(
            bytes, sizeof(bytes), index, 4u, 45u, &raw) ||
        raw.layer_handle != 45u || raw.layer_record_offset != 39u ||
        raw.original_handle_start_bit != 160u ||
        raw.handle_tail_bits != 32u ||
        raw.first_h_code != 4u || raw.first_h_size != 1u ||
        !raw.record_crc_verified || raw.display_accepted ||
        memcmp(unchanged, bytes, sizeof(bytes)))
        return 0;
    const MusaDwgAc1024LayerEedRawHandleHeader raw_saved = raw;

    const MusaDwgAc1024LayerEedXDictionaryGraph preserved = result;
    if (musa_dwg_ac1024_layer_eed_xdictionary_graph_failure_stage(
            bytes, sizeof(bytes), index, 4u, 45u) !=
            MUSA_DWG_LAYER_XDIC_GRAPH_OK)
        return 0;

    /* A valid layer CRC does not authenticate a broken DICTIONARY CRC. */
    bytes[38] ^= 1u;
    /* The diagnostic proves only source first H *header*; a damaged
     * XDictionary target CRC does not alter the raw first H bits. */
    if (!musa_dwg_ac1024_layer_eed_first_h_header_probe(
            bytes, sizeof(bytes), index, 4u, 45u, &raw) ||
        memcmp(&raw, &raw_saved, sizeof(raw_saved)))
        return 0;
    if (musa_dwg_ac1024_layer_eed_xdictionary_graph_failure_stage(
            bytes, sizeof(bytes), index, 4u, 45u) !=
            MUSA_DWG_LAYER_XDIC_GRAPH_TARGET_CRC ||
        musa_dwg_ac1024_layer_eed_xdictionary_graph_probe(
            bytes, sizeof(bytes), index, 4u, 45u, &result) ||
        memcmp(&preserved, &result, sizeof(result)))
        return 0;
    memcpy(bytes, unchanged, sizeof(bytes));

    /* The source-indexed LAYER_CONTROL cannot masquerade as OT42. */
    index[1].raw_ot = 42u;
    if (musa_dwg_ac1024_layer_eed_xdictionary_graph_failure_stage(
            bytes, sizeof(bytes), index, 4u, 45u) !=
            MUSA_DWG_LAYER_XDIC_GRAPH_TARGET_CLASS ||
        musa_dwg_ac1024_layer_eed_xdictionary_graph_probe(
            bytes, sizeof(bytes), index, 4u, 45u, &result) ||
        memcmp(&preserved, &result, sizeof(result)))
        return 0;
    index[1].raw_ot = 50u;

    /* Re-sealed, dangling dictionary H3 is still refused. */
    set_bits(p, 184u, 8u, 44u);
    if (!musa_core_crc16_arc(0xc0c1u, layer, 27u, &crc))
        return 0;
    layer[27] = (uint8_t)crc;
    layer[28] = (uint8_t)(crc >> 8u);
    if (musa_dwg_ac1024_layer_eed_xdictionary_graph_failure_stage(
            bytes, sizeof(bytes), index, 4u, 45u) !=
            MUSA_DWG_LAYER_XDIC_GRAPH_TARGET_INDEX ||
        musa_dwg_ac1024_layer_eed_xdictionary_graph_probe(
            bytes, sizeof(bytes), index, 4u, 45u, &result) ||
        memcmp(&preserved, &result, sizeof(result)))
        return 0;
    memcpy(bytes, unchanged, sizeof(bytes));

    /* Wrong role H4, even pointing at the same original dictionary. */
    set_bits(p, 176u, 4u, 4u);
    if (!musa_core_crc16_arc(0xc0c1u, layer, 27u, &crc))
        return 0;
    layer[27] = (uint8_t)crc;
    layer[28] = (uint8_t)(crc >> 8u);
    if (musa_dwg_ac1024_layer_eed_xdictionary_graph_failure_stage(
            bytes, sizeof(bytes), index, 4u, 45u) !=
            MUSA_DWG_LAYER_XDIC_GRAPH_DICTIONARY_H ||
        musa_dwg_ac1024_layer_eed_xdictionary_graph_probe(
            bytes, sizeof(bytes), index, 4u, 45u, &result) ||
        memcmp(&preserved, &result, sizeof(result)))
        return 0;
    memcpy(bytes, unchanged, sizeof(bytes));

    /* Raw-prefix probe is also fail-atomic and source immutable. */
    if (musa_dwg_ac1024_layer_eed_first_h_header_probe(
            bytes, sizeof(bytes), index, 4u, 45u, NULL) ||
        musa_dwg_ac1024_layer_eed_first_h_header_probe(
            bytes, sizeof(bytes), index, 4u, 45u,
            (MusaDwgAc1024LayerEedRawHandleHeader *)(void *)bytes) ||
        musa_dwg_ac1024_layer_eed_first_h_header_probe(
            bytes, sizeof(bytes), index, 4u, 45u,
            (MusaDwgAc1024LayerEedRawHandleHeader *)(void *)index) ||
        memcmp(&raw, &raw_saved, sizeof(raw_saved)))
        return 0;
    /* A CRC-resealed H2 ownership code is NOT the H4 soft-pointer
     * required for LAYER_CONTROL; do not infer a valid parent target. */
    set_bits(p, 160u, 4u, 2u);
    if (!musa_core_crc16_arc(0xc0c1u, layer, 27u, &crc))
        return 0;
    layer[27] = (uint8_t)crc;
    layer[28] = (uint8_t)(crc >> 8u);
    if (musa_dwg_ac1024_layer_eed_xdictionary_graph_failure_stage(
            bytes, sizeof(bytes), index, 4u, 45u) !=
            MUSA_DWG_LAYER_XDIC_GRAPH_CONTROL_H ||
        musa_dwg_ac1024_layer_eed_xdictionary_graph_probe(
            bytes, sizeof(bytes), index, 4u, 45u, &result) ||
        memcmp(&preserved, &result, sizeof(result)))
        return 0;
    memcpy(bytes, unchanged, sizeof(bytes));

    /* No aliasing, NULL, forged handle or source mutation. */
    if (musa_dwg_ac1024_layer_eed_xdictionary_graph_probe(
            bytes, sizeof(bytes), index, 4u, 44u, &result) ||
        musa_dwg_ac1024_layer_eed_xdictionary_graph_probe(
            bytes, sizeof(bytes), index, 4u, 45u, NULL) ||
        musa_dwg_ac1024_layer_eed_xdictionary_graph_probe(
            bytes, sizeof(bytes), index, 4u, 45u,
            (MusaDwgAc1024LayerEedXDictionaryGraph *)(void *)bytes) ||
        musa_dwg_ac1024_layer_eed_xdictionary_graph_probe(
            bytes, sizeof(bytes), index, 4u, 45u,
            (MusaDwgAc1024LayerEedXDictionaryGraph *)(void *)index) ||
        memcmp(&preserved, &result, sizeof(result)) ||
        memcmp(unchanged, bytes, sizeof(bytes)))
        return 0;

    /* An added, CRC-sealed trailing raw H5 header remains untrusted:
     * only its eight header bits and in-bounds payload size are exposed. */
    uint8_t with_tail[72] = {0};
    memcpy(with_tail, unchanged, sizeof(unchanged));
    uint8_t *const extended_layer = with_tail + 39u;
    uint8_t *const extended_body = extended_layer + 3u;
    extended_layer[0] = 27u; /* 27-byte payload, CRC at offset 30 */
    extended_layer[2] = 56u; /* 4 H refs at bits 160 through 215 */
    set_bits(extended_body, 192u, 4u, 5u);
    set_bits(extended_body, 196u, 4u, 1u);
    set_bits(extended_body, 200u, 8u, 44u);
    set_bits(extended_body, 208u, 4u, 5u); /* opaque next H5, no payload */
    set_bits(extended_body, 212u, 4u, 0u);
    if (!musa_core_crc16_arc(0xc0c1u, extended_layer, 30u, &crc))
        return 0;
    extended_layer[30] = (uint8_t)crc;
    extended_layer[31] = (uint8_t)(crc >> 8u);
    MusaDwgAc1024SelfIdentity tail_index[4] = {{0}};
    if (!musa_dwg_ac1024_recover_self_index(
            with_tail, sizeof(with_tail), offsets, 4u, tail_index, 4u))
        return 0;
    MusaDwgAc1024LayerEedXDictionaryGraph tail_graph = {0};
    MusaDwgAc1024LayerEedTailRawHeader tail_raw = {0};
    if (!musa_dwg_ac1024_layer_eed_xdictionary_graph_probe(
            with_tail, sizeof(with_tail), tail_index, 4u, 45u,
            &tail_graph) ||
        tail_graph.remaining_handle_tail_bits != 24u ||
        tail_graph.display_accepted ||
        !musa_dwg_ac1024_layer_eed_tail_raw_header_probe(
            with_tail, sizeof(with_tail), tail_index, 4u, 45u,
            &tail_raw) ||
        tail_raw.layer_handle != 45u ||
        tail_raw.layer_record_offset != 39u ||
        tail_raw.original_tail_start_bit != 192u ||
        tail_raw.remaining_handle_tail_bits != 24u ||
        tail_raw.raw_h_code != 5u || tail_raw.raw_h_size != 1u ||
        !tail_raw.original_records_crc_verified ||
        tail_raw.display_accepted)
        return 0;
    const MusaDwgAc1024LayerEedTailRawHeader tail_saved = tail_raw;

    MusaDwgAc1024LayerEedTailShape shape = {0};
    if (!musa_dwg_ac1024_layer_eed_tail_shape_probe(
            with_tail, sizeof(with_tail), tail_index, 4u, 45u, &shape) ||
        shape.layer_handle != 45u || shape.layer_record_offset != 39u ||
        shape.tail_start_bit != 192u || shape.tail_bit_count != 24u ||
        shape.h_token_count != 2u ||
        shape.raw_h5_zero_length_count != 1u ||
        shape.other_raw_h_count != 1u || shape.raw_payload_octets != 1u ||
        shape.raw_h5_nonzero_length_count != 1u ||
        shape.raw_non_h5_zero_length_count != 0u ||
        shape.raw_non_h5_nonzero_length_count != 0u ||
        !shape.original_records_crc_verified || shape.display_accepted)
        return 0;
    const MusaDwgAc1024LayerEedTailShape shape_saved = shape;
    if (musa_dwg_ac1024_layer_eed_tail_shape_failure_stage(
            with_tail, sizeof(with_tail), tail_index, 4u, 45u) !=
            MUSA_DWG_LAYER_TAIL_SHAPE_OK)
        return 0;
    /* An H4 with zero bytes is counted as RAW other-code/zero-size
     * evidence only. It cannot turn into a verified handle target. */
    set_bits(extended_body, 208u, 4u, 4u);
    if (!musa_core_crc16_arc(0xc0c1u, extended_layer, 30u, &crc))
        return 0;
    extended_layer[30] = (uint8_t)crc;
    extended_layer[31] = (uint8_t)(crc >> 8u);
    MusaDwgAc1024LayerEedTailShape other_zero_shape = {0};
    if (!musa_dwg_ac1024_layer_eed_tail_shape_probe(
            with_tail, sizeof(with_tail), tail_index, 4u, 45u,
            &other_zero_shape) ||
        other_zero_shape.h_token_count != 2u ||
        other_zero_shape.raw_h5_zero_length_count != 0u ||
        other_zero_shape.raw_h5_nonzero_length_count != 1u ||
        other_zero_shape.raw_non_h5_zero_length_count != 1u ||
        other_zero_shape.raw_non_h5_nonzero_length_count != 0u ||
        other_zero_shape.raw_payload_octets != 1u ||
        other_zero_shape.display_accepted)
        return 0;
    set_bits(extended_body, 208u, 4u, 5u);
    if (!musa_core_crc16_arc(0xc0c1u, extended_layer, 30u, &crc))
        return 0;
    extended_layer[30] = (uint8_t)crc;
    extended_layer[31] = (uint8_t)(crc >> 8u);
    uint8_t extended_pristine[sizeof(with_tail)];
    memcpy(extended_pristine, with_tail, sizeof(with_tail));

    /* Valid first H, but the second H length is impossible in its
     * remaining bits. Stage points to the failing token, not raw data. */
    set_bits(extended_body, 212u, 4u, 15u);
    if (!musa_core_crc16_arc(0xc0c1u, extended_layer, 30u, &crc))
        return 0;
    extended_layer[30] = (uint8_t)crc;
    extended_layer[31] = (uint8_t)(crc >> 8u);
    if (musa_dwg_ac1024_layer_eed_tail_shape_failure_stage(
            with_tail, sizeof(with_tail), tail_index, 4u, 45u) !=
            MUSA_DWG_LAYER_TAIL_SHAPE_LENGTH ||
        musa_dwg_ac1024_layer_eed_tail_shape_probe(
            with_tail, sizeof(with_tail), tail_index, 4u, 45u, &shape) ||
        memcmp(&shape, &shape_saved, sizeof(shape_saved)))
        return 0;
    memcpy(with_tail, extended_pristine, sizeof(with_tail));

    /* The two-link-only source cannot claim a successful full-tail scan. */
    if (musa_dwg_ac1024_layer_eed_tail_shape_failure_stage(
            unchanged, sizeof(unchanged), index, 4u, 45u) !=
            MUSA_DWG_LAYER_TAIL_SHAPE_PRECONDITION ||
        musa_dwg_ac1024_layer_eed_tail_shape_probe(
            unchanged, sizeof(unchanged), index, 4u, 45u, &shape) ||
        memcmp(&shape, &shape_saved, sizeof(shape_saved)))
        return 0;
    /* No remaining H bits: a valid two-link graph is NOT a tail proof. */
    if (musa_dwg_ac1024_layer_eed_tail_raw_header_probe(
            unchanged, sizeof(unchanged), index, 4u, 45u, &tail_raw) ||
        memcmp(&tail_raw, &tail_saved, sizeof(tail_saved)))
        return 0;

    /* Resigned original CRC does not legitimize an overlong H value. */
    set_bits(extended_body, 196u, 4u, 15u);
    if (!musa_core_crc16_arc(0xc0c1u, extended_layer, 30u, &crc))
        return 0;
    extended_layer[30] = (uint8_t)crc;
    extended_layer[31] = (uint8_t)(crc >> 8u);
    if (musa_dwg_ac1024_layer_eed_tail_raw_header_probe(
            with_tail, sizeof(with_tail), tail_index, 4u, 45u,
            &tail_raw) ||
        memcmp(&tail_raw, &tail_saved, sizeof(tail_saved)))
        return 0;
    /* A CRC-resealed oversized token must remain refused before restore. */
    if (musa_dwg_ac1024_layer_eed_tail_shape_probe(
            with_tail, sizeof(with_tail), tail_index, 4u, 45u, &shape) ||
        memcmp(&shape, &shape_saved, sizeof(shape_saved)))
        return 0;
    memcpy(with_tail, extended_pristine, sizeof(with_tail));


    /* Synthetic 59-bit H stream: two complete tokens and exactly
     * three terminal bits. Neither all-zero nor nonzero means padding. */
    uint8_t with_fragment[72] = {0};
    memcpy(with_fragment, extended_pristine, sizeof(with_fragment));
    uint8_t *const fragment_layer = with_fragment + 39u;
    uint8_t *const fragment_body = fragment_layer + 3u;
    fragment_layer[0] = 28u;
    fragment_layer[2] = 59u; /* H stream starts at bit 165 */
    for (size_t bit = 160u; bit < 224u; ++bit)
        set_bits(fragment_body, bit, 1u, 0u);
    set_bits(fragment_body, 165u, 4u, 4u);
    set_bits(fragment_body, 169u, 4u, 1u);
    set_bits(fragment_body, 173u, 8u, 41u);
    set_bits(fragment_body, 181u, 4u, 3u);
    set_bits(fragment_body, 185u, 4u, 1u);
    set_bits(fragment_body, 189u, 8u, 43u);
    set_bits(fragment_body, 197u, 4u, 5u);
    set_bits(fragment_body, 201u, 4u, 1u);
    set_bits(fragment_body, 205u, 8u, 44u);
    set_bits(fragment_body, 213u, 4u, 5u);
    set_bits(fragment_body, 217u, 4u, 0u);
    set_bits(fragment_body, 221u, 3u, 0u);
    if (!musa_core_crc16_arc(0xc0c1u, fragment_layer, 31u, &crc))
        return 0;
    fragment_layer[31] = (uint8_t)crc;
    fragment_layer[32] = (uint8_t)(crc >> 8u);
    MusaDwgAc1024SelfIdentity fragment_index[4] = {{0}};
    if (!musa_dwg_ac1024_recover_self_index(
            with_fragment, sizeof(with_fragment), offsets, 4u,
            fragment_index, 4u))
        return 0;
    MusaDwgAc1024LayerEedTerminalFragment fragment = {0};
    if (musa_dwg_ac1024_layer_eed_tail_shape_failure_stage(
            with_fragment, sizeof(with_fragment), fragment_index,
            4u, 45u) != MUSA_DWG_LAYER_TAIL_SHAPE_SHORT_HEADER ||
        !musa_dwg_ac1024_layer_eed_terminal_fragment_probe(
            with_fragment, sizeof(with_fragment), fragment_index,
            4u, 45u, &fragment) ||
        fragment.layer_handle != 45u ||
        fragment.layer_record_offset != 39u ||
        fragment.complete_h_token_count != 2u ||
        fragment.complete_h5_zero_token_count != 1u ||
        fragment.terminal_fragment_bit_count != 3u ||
        fragment.terminal_fragment_raw_pattern != 0u ||
        fragment.terminal_fragment_start_mod8 != 5u ||
        fragment.terminal_fragment_end_mod8 != 0u ||
        fragment.prefix_h_token_count != 2u ||
        fragment.prefix_h_code[0] != 5u ||
        fragment.prefix_h_code[1] != 5u ||
        fragment.prefix_h_size[0] != 1u ||
        fragment.prefix_h_size[1] != 0u ||
        fragment.prefix_h_code[2] != 0u ||
        fragment.prefix_h_size[2] != 0u ||
        !fragment.terminal_fragment_all_zero ||
        !fragment.original_records_crc_verified ||
        fragment.display_accepted)
        return 0;
    /* The same 3-bit tail can be nonzero: no interpretation is made. */
    set_bits(fragment_body, 221u, 3u, 5u);
    if (!musa_core_crc16_arc(0xc0c1u, fragment_layer, 31u, &crc))
        return 0;
    fragment_layer[31] = (uint8_t)crc;
    fragment_layer[32] = (uint8_t)(crc >> 8u);
    if (!musa_dwg_ac1024_layer_eed_terminal_fragment_probe(
            with_fragment, sizeof(with_fragment), fragment_index,
            4u, 45u, &fragment) ||
        fragment.terminal_fragment_all_zero ||
        fragment.terminal_fragment_raw_pattern != 5u ||
        fragment.terminal_fragment_start_mod8 != 5u ||
        fragment.terminal_fragment_end_mod8 != 0u ||
        fragment.terminal_fragment_bit_count != 3u ||
        fragment.complete_h_token_count != 2u ||
        fragment.prefix_h_token_count != 2u ||
        fragment.prefix_h_code[0] != 5u ||
        fragment.prefix_h_size[0] != 1u ||
        fragment.display_accepted)
        return 0;
    if (musa_dwg_ac1024_layer_eed_terminal_fragment_probe(
            extended_pristine, sizeof(extended_pristine), tail_index,
            4u, 45u, &fragment))
        return 0;
    fragment_layer[32] ^= 1u;
    if (musa_dwg_ac1024_layer_eed_terminal_fragment_probe(
            with_fragment, sizeof(with_fragment), fragment_index,
            4u, 45u, &fragment) ||
        fragment.terminal_fragment_bit_count != 3u)
        return 0;
    fragment_layer[32] ^= 1u;
    if (musa_dwg_ac1024_layer_eed_terminal_fragment_probe(
            with_fragment, sizeof(with_fragment), fragment_index,
            4u, 45u, (MusaDwgAc1024LayerEedTerminalFragment *)(void *)with_fragment) ||
        musa_dwg_ac1024_layer_eed_terminal_fragment_probe(
            with_fragment, sizeof(with_fragment), fragment_index,
            4u, 45u, NULL))
        return 0;

    /* A raw H5/1 value does not become a layer reference simply by
     * being syntactically complete. The synthetic value 44 has no
     * independently indexed original record; H5/0 has no numeric value. */
    MusaDwgAc1024LayerTailCandidateCensus candidates = {0};
    if (!musa_dwg_ac1024_layer_eed_tail_candidate_census_probe(
            with_fragment, sizeof(with_fragment), fragment_index, 4u, 45u,
            &candidates) ||
        candidates.complete_prefix_h_tokens != 2u ||
        candidates.absolute_candidates != 1u ||
        candidates.verified_index_matches != 0u ||
        candidates.unmatched_or_unverified != 1u ||
        candidates.nonabsolute_or_empty_tokens != 1u ||
        candidates.verified_ot49 != 0u ||
        candidates.verified_ot51 != 0u ||
        candidates.verified_other_ot != 0u ||
        candidates.verified_placeholder_ot80 != 0u ||
        candidates.verified_ltype_ot57 != 0u ||
        candidates.unresolved_dynamic_class_ot_ge500 != 0u ||
        candidates.verified_other_raw_ot != 0u ||
        candidates.matched_ordinal_mask != 0u ||
        candidates.verified_raw_ot_by_ordinal[0] != 0u ||
        candidates.verified_raw_ot_by_ordinal[1] != 0u ||
        !candidates.original_records_crc_verified ||
        candidates.display_accepted)
        return 0;

    /* Repoint only the raw absolute H5 value to an independently
     * CRC-backed OT42 record; classify it as OTHER without granting a
     * drawing role. Source remains unchanged after restoring bytes. */
    /* A two-token/3-bit synthetic fragment must never be promoted to a
     * three-target LAYER property proof. Failure is strictly atomic. */
    MusaDwgAc1024LayerTailTypeTriplet triplet = {0};
    triplet.layer_handle = UINT64_C(999);
    const MusaDwgAc1024LayerTailTypeTriplet saved_triplet = triplet;
    MusaDwgAc1024LayerTailAlignmentWitness alignment = {0};
    alignment.terminal_raw_value = 127u;
    const MusaDwgAc1024LayerTailAlignmentWitness saved_alignment = alignment;
    if (musa_dwg_ac1024_layer_eed_tail_alignment_witness(
            with_fragment, sizeof(with_fragment), fragment_index,
            4u, 45u, &alignment) ||
        musa_dwg_ac1024_layer_eed_tail_alignment_witness(
            with_fragment, sizeof(with_fragment), fragment_index,
            4u, 45u,
            (MusaDwgAc1024LayerTailAlignmentWitness *)(void *)with_fragment) ||
        musa_dwg_ac1024_layer_eed_tail_alignment_witness(
            with_fragment, sizeof(with_fragment), fragment_index,
            4u, 0u, &alignment) ||
        memcmp(&alignment, &saved_alignment, sizeof(alignment)))
        return 0;
    if (musa_dwg_ac1024_layer_eed_tail_type_triplet_probe(
            with_fragment, sizeof(with_fragment), fragment_index,
            4u, 45u, &triplet) ||
        musa_dwg_ac1024_layer_eed_tail_type_triplet_probe(
            with_fragment, sizeof(with_fragment), fragment_index,
            4u, 45u,
            (MusaDwgAc1024LayerTailTypeTriplet *)(void *)with_fragment) ||
        musa_dwg_ac1024_layer_eed_tail_type_triplet_probe(
            with_fragment, sizeof(with_fragment), fragment_index,
            4u, 0u, &triplet) ||
        memcmp(&triplet, &saved_triplet, sizeof(triplet)))
        return 0;

    const MusaDwgAc1024LayerTailCandidateCensus old_candidates = candidates;
    set_bits(fragment_body, 205u, 8u, 43u);
    if (!musa_core_crc16_arc(0xc0c1u, fragment_layer, 31u, &crc))
        return 0;
    fragment_layer[31] = (uint8_t)crc;
    fragment_layer[32] = (uint8_t)(crc >> 8u);
    if (!musa_dwg_ac1024_layer_eed_tail_candidate_census_probe(
            with_fragment, sizeof(with_fragment), fragment_index, 4u, 45u,
            &candidates) ||
        candidates.complete_prefix_h_tokens != 2u ||
        candidates.absolute_candidates != 1u ||
        candidates.verified_index_matches != 1u ||
        candidates.unmatched_or_unverified != 0u ||
        candidates.nonabsolute_or_empty_tokens != 1u ||
        candidates.verified_ot49 != 0u ||
        candidates.verified_ot51 != 0u ||
        candidates.verified_other_ot != 1u ||
        candidates.verified_placeholder_ot80 != 0u ||
        candidates.verified_ltype_ot57 != 0u ||
        candidates.unresolved_dynamic_class_ot_ge500 != 0u ||
        candidates.verified_other_raw_ot != 1u ||
        candidates.matched_ordinal_mask != 1u ||
        candidates.verified_raw_ot_by_ordinal[0] != 42u ||
        candidates.verified_raw_ot_by_ordinal[1] != 0u ||
        candidates.verified_raw_ot_by_ordinal[2] != 0u ||
        candidates.verified_raw_ot_by_ordinal[3] != 0u ||
        candidates.display_accepted)
        return 0;
    const MusaDwgAc1024LayerTailCandidateCensus matched_candidates = candidates;
    fragment_layer[32] ^= 1u;
    if (musa_dwg_ac1024_layer_eed_tail_candidate_census_probe(
            with_fragment, sizeof(with_fragment), fragment_index, 4u, 45u,
            &candidates) ||
        memcmp(&candidates, &matched_candidates, sizeof(candidates)))
        return 0;
    fragment_layer[32] ^= 1u;
    set_bits(fragment_body, 205u, 8u, 44u);
    if (!musa_core_crc16_arc(0xc0c1u, fragment_layer, 31u, &crc))
        return 0;
    fragment_layer[31] = (uint8_t)crc;
    fragment_layer[32] = (uint8_t)(crc >> 8u);
    if (!musa_dwg_ac1024_layer_eed_tail_candidate_census_probe(
            with_fragment, sizeof(with_fragment), fragment_index, 4u, 45u,
            &candidates) ||
        memcmp(&candidates, &old_candidates, sizeof(candidates)) ||
        musa_dwg_ac1024_layer_eed_tail_candidate_census_probe(
            extended_pristine, sizeof(extended_pristine),
            tail_index, 4u, 45u, &candidates) ||
        musa_dwg_ac1024_layer_eed_tail_candidate_census_probe(
            with_fragment, sizeof(with_fragment), fragment_index, 4u, 45u,
            (MusaDwgAc1024LayerTailCandidateCensus *)(void *)with_fragment) ||
        musa_dwg_ac1024_layer_eed_tail_candidate_census_probe(
            with_fragment, sizeof(with_fragment), fragment_index, 4u, 45u,
            (MusaDwgAc1024LayerTailCandidateCensus *)(void *)fragment_index) ||
        musa_dwg_ac1024_layer_eed_tail_candidate_census_probe(
            with_fragment, sizeof(with_fragment), fragment_index, 4u, 45u,
            NULL))
        return 0;

    /* Three additional authentic source records, outside the verified
     * layer/APPID/XDictionary chain: fixed 80 PLACEHOLDER, dynamically
     * declared 505, fixed 57 LTYPE. No class-name or property role is
     * assigned by these numeric-only checks. */
    uint8_t with_known_targets[111] = {0};
    memcpy(with_known_targets, with_fragment, sizeof(with_fragment));
    const uint64_t known_offsets[7] = {0u,13u,26u,39u,72u,85u,98u};
    const uint16_t expected_raw[3] = {80u,505u,57u};
    for (size_t k = 0u; k < 3u; ++k) {
        uint8_t *const rec = with_known_targets + known_offsets[4u + k];
        if (!make_record(rec, (uint32_t)(46u + k))) return 0;
        if (k == 1u) {
            /* R2010 selector 1 encodes raw OT 496 + 9 = 505, NOT
             * a resolved material/class name. Self-H remains at bit 10. */
            set_bits(rec + 3u, 0u, 2u, 1u);
            set_bits(rec + 3u, 2u, 8u, 9u);
        } else {
            set_bits(rec + 3u, 2u, 8u, expected_raw[k]);
        }
        if (!musa_core_crc16_arc(0xc0c1u, rec, 11u, &crc))
            return 0;
        rec[11] = (uint8_t)crc;
        rec[12] = (uint8_t)(crc >> 8u);
        MusaDwgAc1024SelfPrefix prefix = {0};
        if (!musa_dwg_ac1024_record_self_prefix(
                with_known_targets, sizeof(with_known_targets),
                known_offsets[4u + k], &prefix) ||
            prefix.raw_ot != expected_raw[k] ||
            prefix.embedded_absolute_handle != (uint64_t)(46u + k) ||
            !prefix.frame_crc16_verified)
            return 0;
    }
    MusaDwgAc1024SelfIdentity known_index[7] = {{0}};
    if (!musa_dwg_ac1024_recover_self_index(
            with_known_targets, sizeof(with_known_targets),
            known_offsets, 7u, known_index, 7u))
        return 0;
    for (size_t k = 0u; k < 3u; ++k) {
        /* The one complete numeric H5 token starts at bit 205 of
         * this otherwise unchanged CRC-backed fragment witness. */
        set_bits(with_known_targets + 42u, 205u, 8u, (uint32_t)(46u + k));
        if (!musa_core_crc16_arc(
                0xc0c1u, with_known_targets + 39u, 31u, &crc))
            return 0;
        with_known_targets[70] = (uint8_t)crc;
        with_known_targets[71] = (uint8_t)(crc >> 8u);
        if (!musa_dwg_ac1024_layer_eed_tail_candidate_census_probe(
                with_known_targets, sizeof(with_known_targets),
                known_index, 7u, 45u, &candidates) ||
            candidates.verified_index_matches != 1u ||
            candidates.unmatched_or_unverified != 0u ||
            candidates.verified_other_ot != 1u ||
            candidates.verified_placeholder_ot80 != (size_t)(k == 0u) ||
            candidates.unresolved_dynamic_class_ot_ge500 !=
                (size_t)(k == 1u) ||
            candidates.verified_ltype_ot57 != (size_t)(k == 2u) ||
            candidates.verified_other_raw_ot != 0u ||
            candidates.verified_raw_ot_by_ordinal[0] != expected_raw[k] ||
            candidates.matched_ordinal_mask != 1u ||
            candidates.display_accepted)
            return 0;
    }
    /* Even an indexed dynamic OT must lose its match when its source
     * CRC is damaged. Remaining source relationships stay non-renderable. */
    with_known_targets[97] ^= 1u; /* OT505 record CRC */
    set_bits(with_known_targets + 42u, 205u, 8u, 47u);
    if (!musa_core_crc16_arc(0xc0c1u, with_known_targets + 39u, 31u, &crc))
        return 0;
    with_known_targets[70] = (uint8_t)crc;
    with_known_targets[71] = (uint8_t)(crc >> 8u);
    if (!musa_dwg_ac1024_layer_eed_tail_candidate_census_probe(
            with_known_targets, sizeof(with_known_targets),
            known_index, 7u, 45u, &candidates) ||
        candidates.verified_index_matches != 0u ||
        candidates.unmatched_or_unverified != 1u ||
        candidates.unresolved_dynamic_class_ot_ge500 != 0u ||
        candidates.display_accepted)
        return 0;
    /* A damaged original layer CRC or aliased output is always refused. */
    extended_layer[31] ^= 1u;
    if (musa_dwg_ac1024_layer_eed_tail_raw_header_probe(
            with_tail, sizeof(with_tail), tail_index, 4u, 45u,
            &tail_raw) ||
        memcmp(&tail_raw, &tail_saved, sizeof(tail_saved)) ||
        musa_dwg_ac1024_layer_eed_tail_shape_probe(
            with_tail, sizeof(with_tail), tail_index, 4u, 45u,
            &shape) ||
        memcmp(&shape, &shape_saved, sizeof(shape_saved)))
        return 0;
    memcpy(with_tail, extended_pristine, sizeof(with_tail));
    if (musa_dwg_ac1024_layer_eed_tail_raw_header_probe(
            with_tail, sizeof(with_tail), tail_index, 4u, 45u,
            (MusaDwgAc1024LayerEedTailRawHeader *)(void *)with_tail) ||
        musa_dwg_ac1024_layer_eed_tail_raw_header_probe(
            with_tail, sizeof(with_tail), tail_index, 4u, 45u,
            (MusaDwgAc1024LayerEedTailRawHeader *)(void *)tail_index) ||
        musa_dwg_ac1024_layer_eed_tail_raw_header_probe(
            with_tail, sizeof(with_tail), tail_index, 4u, 45u, NULL) ||
        memcmp(&tail_raw, &tail_saved, sizeof(tail_saved)) ||
        memcmp(with_tail, extended_pristine, sizeof(with_tail)))
        return 0;


    if (musa_dwg_ac1024_layer_eed_tail_shape_probe(
            with_tail, sizeof(with_tail), tail_index, 4u, 45u,
            (MusaDwgAc1024LayerEedTailShape *)(void *)with_tail) ||
        musa_dwg_ac1024_layer_eed_tail_shape_probe(
            with_tail, sizeof(with_tail), tail_index, 4u, 45u,
            (MusaDwgAc1024LayerEedTailShape *)(void *)tail_index) ||
        musa_dwg_ac1024_layer_eed_tail_shape_probe(
            with_tail, sizeof(with_tail), tail_index, 4u, 45u, NULL) ||
        memcmp(&shape, &shape_saved, sizeof(shape_saved)))
        return 0;

    return 1;
}

int main(void) {
    if (!test_owner_layer_bridge()) return 11;
    if (!test_relative_owner_layer_codes()) return 12;
    if (!test_verified_self_index_find()) return 10;
    if (!test_self_prefix_cursor()) return 6;
    if (!test_zero_eed_subset()) return 7;
    if (!test_eed_chain_bounded()) return 13;
    if (!test_layer_raw_flags_preflight()) return 14;
    if (!test_layer_nonzero_eed_raw_flags()) return 15;
    if (!test_layer_eed_appid_original_target()) return 16;
    if (!test_layer_eed_xdictionary_original_graph()) return 17;
    if (!test_no_proxy_subset()) return 8;
    if (!test_entity_meta_subset()) return 9;
    _Alignas(MusaDwgAc1024SelfIdentity) uint8_t objects[26] = {0};
    if (!make_record(objects, 1u) ||
        !make_record(objects + 13u, 2u))
        return 1;
    const uint64_t offsets[2] = {0u, 13u};
    const uint64_t duplicates[2] = {0u, 0u};
    const uint64_t reversed[2] = {13u, 0u};
    MusaDwgAc1024SelfIdentity records[2] = {{0}};
    uint8_t preserved_source[26];
    memcpy(preserved_source, objects, sizeof(objects));
    if (!musa_dwg_ac1024_recover_self_index(
            objects, sizeof(objects), offsets, 2u, records, 2u) ||
        records[0].embedded_absolute_handle != 1u ||
        records[1].embedded_absolute_handle != 2u ||
        records[0].record_offset != 0u ||
        records[1].record_offset != 13u ||
        records[0].raw_ot != 19u || records[1].raw_ot != 19u ||
        !records[0].frame_crc16_verified ||
        !records[1].embedded_handle_code0_verified ||
        memcmp(objects, preserved_source, sizeof(objects)))
        return 2;
    MusaDwgAc1024SelfIdentity saved[2];
    memcpy(saved, records, sizeof(saved));
    /* Exercise the actual CRC-verified recovered array, not only
       the synthetic search-order fixtures. */
    MusaDwgAc1024SelfIdentity match = {0};
    if (!musa_dwg_ac1024_self_index_find(records, 2u, 1u, &match) ||
        match.record_offset != 0u || match.raw_ot != 19u ||
        !musa_dwg_ac1024_self_index_find(records, 2u, 2u, &match) ||
        match.record_offset != 13u ||
        musa_dwg_ac1024_self_index_find(records, 2u, 3u, &match))
        return 11;
    if (musa_dwg_ac1024_recover_self_index(
            objects, sizeof(objects), duplicates, 2u, records, 2u) ||
        musa_dwg_ac1024_recover_self_index(
            objects, sizeof(objects), reversed, 2u, records, 2u) ||
        memcmp(saved, records, sizeof(saved)))
        return 3;
    objects[25] ^= 0x11u; /* corrupt second frame CRC */
    if (musa_dwg_ac1024_recover_self_index(
            objects, sizeof(objects), offsets, 2u, records, 2u) ||
        memcmp(saved, records, sizeof(saved)))
        return 4;
    objects[25] ^= 0x11u;
    if (musa_dwg_ac1024_recover_self_index(
            objects, sizeof(objects) - 1u, offsets, 2u, records, 2u) ||
        musa_dwg_ac1024_recover_self_index(
            objects, sizeof(objects), offsets, 2u, records, 1u) ||
        musa_dwg_ac1024_recover_self_index(
            objects, sizeof(objects), offsets, 0u, records, 2u) ||
        musa_dwg_ac1024_recover_self_index(
            objects, sizeof(objects), offsets, 2u,
            (MusaDwgAc1024SelfIdentity *)(void *)objects, 2u) ||
        memcmp(saved, records, sizeof(saved)) ||
        memcmp(objects, preserved_source, sizeof(objects)))
        return 5;
    puts("Original AC1024 recovered self-H index: synthetic atomic test passed");
    return 0;
}
