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
int main(void) {
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
