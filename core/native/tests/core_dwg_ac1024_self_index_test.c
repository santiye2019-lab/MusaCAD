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

int main(void) {
    if (!test_verified_self_index_find()) return 10;
    if (!test_self_prefix_cursor()) return 6;
    if (!test_zero_eed_subset()) return 7;
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
