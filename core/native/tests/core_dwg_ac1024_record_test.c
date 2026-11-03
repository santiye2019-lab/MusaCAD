/* SPDX-License-Identifier: MIT
 * Portable, self-contained source-integrity and R2010 frame regression.
 * Synthetic bytes are purpose-built; no third-party CAD source/library.
 */
#include "musacad_dwg_ac1024_record.h"
#include "musacad_crc16.h"
#include <stdint.h>
#include <stdio.h>
#include <string.h>

static int test_one(size_t mc_bytes) {
    _Alignas(MusaDwgAc1024RecordFrame) uint8_t frame[64] = {0};
    frame[0] = 8u; /* MS payload length excludes separate unsigned MC */
    for (size_t i = 0u; i < mc_bytes; i++)
        frame[2u + i] = (uint8_t)(
            i + 1u == mc_bytes ? 0x01u : 0x81u);
    const size_t body = 2u + mc_bytes;
    /* R2010 OT 2-bit selector 0, followed by raw byte 0x13 (LINE
       numeric declaration); preserve the remaining synthetic bits. */
    frame[body] = 0x04u; frame[body + 1u] = 0xc0u;
    frame[body + 2u] = 0x22u;
    const size_t crc_at = body + 8u;
    uint16_t crc = 0u;
    if (!musa_core_crc16_arc(0xc0c1u, frame, crc_at, &crc))
        return 0;
    frame[crc_at] = (uint8_t)crc;
    frame[crc_at + 1u] = (uint8_t)(crc >> 8u);
    uint8_t untouched[64];
    memcpy(untouched, frame, sizeof(frame));
    MusaDwgAc1024RecordFrame evidence = {0};
    if (!musa_dwg_ac1024_record_probe(
          frame, crc_at + 2u, 0u, &evidence) ||
        !evidence.original_crc16_verified ||
        evidence.framed_bytes != crc_at + 2u ||
        evidence.ms_object_bytes != 8u ||
        evidence.ms_marker_bytes != 2u ||
        evidence.mc_marker_bytes != mc_bytes ||
        evidence.raw_object_type != 19u ||
        evidence.handle_stream_bits !=
          (mc_bytes == 1u ? 1u :
           mc_bytes == 2u ? 129u : 16513u) ||
        memcmp(frame, untouched, sizeof(frame)))
        return 0;
    MusaDwgAc1024DataBounds bounds = {0};
    if (!musa_dwg_ac1024_record_data_bounds(
          frame, crc_at + 2u, 0u, &bounds) ||
        !bounds.record_crc16_verified ||
        bounds.raw_object_type != 19u ||
        bounds.payload_start_byte != 2u + mc_bytes ||
        bounds.payload_bytes != 8u ||
        bounds.type_end_bit != 10u ||
        bounds.payload_end_bit != 64u)
        return 0;
    const MusaDwgAc1024DataBounds saved_bounds = bounds;
    const MusaDwgAc1024RecordFrame saved = evidence;
    frame[crc_at] ^= 0x10u;
    if (musa_dwg_ac1024_record_data_bounds(
          frame, crc_at + 2u, 0u, &bounds) ||
        memcmp(&saved_bounds, &bounds, sizeof(saved_bounds)) ||
        musa_dwg_ac1024_record_probe(
          frame, crc_at + 2u, 0u, &evidence) ||
        memcmp(&saved, &evidence, sizeof(saved)))
        return 0;
    frame[crc_at] ^= 0x10u;
    if (musa_dwg_ac1024_record_probe(
          frame, crc_at + 1u, 0u, &evidence) ||
        memcmp(&saved, &evidence, sizeof(saved)))
        return 0;
    /* A caller-owned output pointing INTO source is always refused. */
    if (musa_dwg_ac1024_record_probe(
          frame, crc_at + 2u, 0u,
          (MusaDwgAc1024RecordFrame *)(void *)frame) ||
        musa_dwg_ac1024_record_data_bounds(
          frame, crc_at + 2u, 0u,
          (MusaDwgAc1024DataBounds *)(void *)frame) ||
        memcmp(frame, untouched, sizeof(frame)))
        return 0;
    return 1;
}

static int test_ot_variants(void) {
    const uint8_t selectors[] = {0u, 1u, 2u, 3u};
    const uint16_t expected[] = {0u, 0x1f0u, 0u, 0u};
    for (size_t i = 0u; i < 4u; i++) {
        uint8_t frame[32] = {0};
        frame[0] = 8u; frame[2] = 1u;
        frame[3] = (uint8_t)(selectors[i] << 6u);
        uint16_t crc = 0u;
        if (!musa_core_crc16_arc(0xc0c1u, frame, 11u, &crc))
            return 0;
        frame[11] = (uint8_t)crc;
        frame[12] = (uint8_t)(crc >> 8u);
        MusaDwgAc1024RecordFrame out = {0};
        MusaDwgAc1024DataBounds bounds = {0};
        if (!musa_dwg_ac1024_record_probe(frame, 13u, 0u, &out) ||
            out.raw_object_type != expected[i] ||
            !musa_dwg_ac1024_record_data_bounds(frame, 13u, 0u, &bounds) ||
            bounds.type_end_bit != (selectors[i] >= 2u ? 18u : 10u) ||
            bounds.raw_object_type != expected[i])
            return 0;
    }
    return 1;
}

static int test_bad_prefix(void) {
    uint8_t invalid[24] = {0};
    invalid[0] = 8u;
    for (size_t i = 2u; i < 12u; i++)
        invalid[i] = 0x81u; /* MC never terminates */
    MusaDwgAc1024RecordFrame out = {0};
    if (musa_dwg_ac1024_record_probe(
          invalid, sizeof(invalid), 0u, &out))
        return 0;
    invalid[0] = 0xffu; invalid[1] = 0xffu;
    invalid[2] = 0xffu; invalid[3] = 0xffu;
    if (musa_dwg_ac1024_record_probe(
          invalid, sizeof(invalid), 0u, &out))
        return 0;
    if (musa_dwg_ac1024_record_probe(NULL, 0u, 0u, &out) ||
        musa_dwg_ac1024_record_probe(invalid, sizeof(invalid), 0u, NULL))
        return 0;
    return 1;
}

/* Genuine short frame shape: 2-byte MS body + 1-byte unsigned MC.
 * A selector 0/1 fits in 10 body bits; 2/3 needs 18 and is rejected. */
static int test_short_ot_only(void) {
    uint8_t data[16] = {0};
    data[0] = 2u; data[2] = 1u;
    data[3] = 0x04u; data[4] = 0xc0u; /* raw LINE OT (19) */
    uint16_t crc = 0u;
    if (!musa_core_crc16_arc(0xc0c1u, data, 5u, &crc))
        return 0;
    data[5] = (uint8_t)crc;
    data[6] = (uint8_t)(crc >> 8u);
    MusaDwgAc1024RecordFrame frame = {0};
    MusaDwgAc1024DataBounds bounds = {0};
    if (!musa_dwg_ac1024_record_probe(data, 7u, 0u, &frame) ||
        !musa_dwg_ac1024_record_data_bounds(data, 7u, 0u, &bounds) ||
        frame.raw_object_type != 19u || frame.framed_bytes != 7u ||
        frame.ms_object_bytes != 2u || bounds.payload_start_byte != 3u ||
        bounds.type_end_bit != 10u || bounds.payload_end_bit != 16u)
        return 0;
    for (unsigned selector = 2u; selector <= 3u; ++selector) {
        data[3] = (uint8_t)(selector << 6u);
        if (!musa_core_crc16_arc(0xc0c1u, data, 5u, &crc))
            return 0;
        data[5] = (uint8_t)crc;
        data[6] = (uint8_t)(crc >> 8u);
        const MusaDwgAc1024RecordFrame saved = frame;
        const MusaDwgAc1024DataBounds saved_bounds = bounds;
        if (musa_dwg_ac1024_record_probe(data, 7u, 0u, &frame) ||
            musa_dwg_ac1024_record_data_bounds(data, 7u, 0u, &bounds) ||
            memcmp(&saved, &frame, sizeof(saved)) ||
            memcmp(&saved_bounds, &bounds, sizeof(saved_bounds)))
            return 0;
    }
    return 1;
}

/* Only a CRC-backed MC count INSIDE the verified payload may define a
 * candidate handle-stream bit window. Neither H nor geometry is read. */
static int test_declared_handle_stream_window(void) {
    _Alignas(MusaDwgAc1024StreamBounds) uint8_t record[64] = {0};
    record[0] = 8u; /* MS body bytes */
    record[2] = 16u; /* 16 handle bits including pad, one MC byte */
    record[3] = 0x04u; record[4] = 0xc0u; /* numeric OT 19 */
    uint16_t crc = 0u;
    if (!musa_core_crc16_arc(0xc0c1u, record, 11u, &crc))
        return 0;
    record[11] = (uint8_t)crc;
    record[12] = (uint8_t)(crc >> 8u);
    MusaDwgAc1024StreamBounds out = {0};
    if (!musa_dwg_ac1024_record_stream_bounds(record, 13u, 0u, &out) ||
        !out.record_crc16_verified || out.payload_start_byte != 3u ||
        out.data_after_ot_bit != 10u || out.handle_stream_bits != 16u ||
        out.handle_stream_start_bit != 48u ||
        out.handle_stream_end_bit != 64u || out.raw_object_type != 19u)
        return 0;
    const MusaDwgAc1024StreamBounds saved = out;
    /* 55 handle bits leave 9 OT bits: reject even with valid CRC. */
    record[2] = 55u;
    if (!musa_core_crc16_arc(0xc0c1u, record, 11u, &crc))
        return 0;
    record[11] = (uint8_t)crc;
    record[12] = (uint8_t)(crc >> 8u);
    if (musa_dwg_ac1024_record_stream_bounds(record, 13u, 0u, &out) ||
        memcmp(&saved, &out, sizeof(saved)))
        return 0;
    /* 65 bits overflow the entire 64-bit body; still reject. */
    record[2] = 65u;
    if (!musa_core_crc16_arc(0xc0c1u, record, 11u, &crc))
        return 0;
    record[11] = (uint8_t)crc;
    record[12] = (uint8_t)(crc >> 8u);
    if (musa_dwg_ac1024_record_stream_bounds(record, 13u, 0u, &out) ||
        memcmp(&saved, &out, sizeof(saved)))
        return 0;
    /* No source/output alias, no release of an unverified window. */
    record[2] = 16u;
    if (!musa_core_crc16_arc(0xc0c1u, record, 11u, &crc))
        return 0;
    record[11] = (uint8_t)crc;
    record[12] = (uint8_t)(crc >> 8u);
    if (musa_dwg_ac1024_record_stream_bounds(
            record, 13u, 0u, (MusaDwgAc1024StreamBounds *)(void *)record) ||
        musa_dwg_ac1024_record_stream_bounds(record, 12u, 0u, &out) ||
        memcmp(&saved, &out, sizeof(saved)))
        return 0;
    return 1;
}

int main(void) {
    for (size_t i = 1u; i <= 3u; i++)
        if (!test_one(i)) return 1;
    if (!test_ot_variants() || !test_bad_prefix() ||
        !test_short_ot_only() || !test_declared_handle_stream_window()) return 2;
    puts("First-party AC1024 record-envelope and raw OT: pass");
    return 0;
}
