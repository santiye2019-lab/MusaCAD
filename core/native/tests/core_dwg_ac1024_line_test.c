/* SPDX-License-Identifier: MIT
 * Synthetic R2010 LINE boundary/geometry regression, not real-DWG parity.
 */
#include "musacad_dwg_ac1024_line.h"
#include "musacad_dwg_ac1024_self_index.h"
#include "musacad_crc16.h"
#include <stdint.h>
#include <stdio.h>
#include <string.h>

static void set_bits(uint8_t *data, size_t pos, unsigned width, uint32_t value) {
    for (unsigned i = 0u; i < width; ++i) {
        const size_t k = pos + i;
        const uint8_t mask = (uint8_t)(1u << (7u - (k % 8u)));
        if ((value >> (width - i - 1u)) & 1u)
            data[k / 8u] |= mask;
        else data[k / 8u] &= (uint8_t)~mask;
    }
}
static void set_rd(uint8_t *data, size_t pos, double number) {
    uint64_t raw = 0u;
    memcpy(&raw, &number, sizeof(raw));
    for (unsigned i = 0u; i < 8u; ++i)
        set_bits(data, pos + i * 8u, 8u,
                 (uint32_t)((raw >> (8u * i)) & 0xffu));
}
static int seal(uint8_t record[37]) {
    uint16_t crc = 0u;
    if (!musa_core_crc16_arc(0xc0c1u, record, 35u, &crc)) return 0;
    record[35] = (uint8_t)crc;
    record[36] = (uint8_t)(crc >> 8u);
    return 1;
}
static int make_line(uint8_t record[37]) {
    memset(record, 0, 37u);
    record[0] = 32u; /* MS=32-byte payload */
    record[2] = 16u; /* MC=16 handle-stream bits */
    uint8_t *const p = record + 3u;
    set_bits(p, 0u, 2u, 0u);    /* OT selector */
    set_bits(p, 2u, 8u, 19u);   /* LINE */
    set_bits(p, 10u, 4u, 0u);   /* absolute self-H */
    set_bits(p, 14u, 4u, 1u);
    set_bits(p, 18u, 8u, 44u);
    set_bits(p, 26u, 2u, 2u);   /* BS zero EED */
    set_bits(p, 28u, 1u, 0u);   /* no proxy */
    set_bits(p, 29u, 2u, 0u);   /* explicit owner mode */
    set_bits(p, 31u, 2u, 2u);   /* zero reactors */
    set_bits(p, 33u, 1u, 1u);   /* XDictionary absent */
    /* AC1024 ENC begins DIRECTLY at bit 34, no R2013+ DS flag. */
    set_bits(p, 34u, 2u, 1u);   /* BS one-byte color */
    set_bits(p, 36u, 8u, 7u);   /* ACI=7 */
    set_bits(p, 44u, 2u, 1u);   /* BD linetype scale=1 */
    set_bits(p, 46u, 2u, 0u);   /* BYLAYER linetype */
    set_bits(p, 48u, 2u, 0u);   /* BYLAYER plotstyle */
    set_bits(p, 50u, 2u, 0u);   /* BYLAYER material */
    set_bits(p, 52u, 8u, 0u);   /* shadow */
    set_bits(p, 60u, 3u, 0u);   /* no full/face/edge visualstyle */
    set_bits(p, 63u, 2u, 2u);   /* BS invisible=0 */
    set_bits(p, 65u, 8u, 25u);  /* lineweight */
    set_bits(p, 73u, 1u, 1u);   /* Z values omitted */
    set_rd(p, 74u, 1.5);        /* X start */
    set_bits(p, 138u, 2u, 0u);  /* X end same as start */
    set_rd(p, 140u, -2.0);      /* Y start */
    set_bits(p, 204u, 2u, 0u);  /* Y end same as start */
    set_bits(p, 206u, 1u, 1u);  /* zero thickness */
    set_bits(p, 207u, 1u, 1u);  /* default extrusion */
    /* bit 240 begins MC-declared HANDLES; field readers must stop there. */
    return seal(record);
}
int main(void) {
    _Alignas(MusaDwgAc1024LineFields) uint8_t record[64] = {0};
    if (!make_line(record)) return 1;
    uint8_t saved[37];
    memcpy(saved, record, sizeof(saved));
    MusaDwgAc1024LineCommon common = {0};
    MusaDwgAc1024LineFields line = {0};
    if (!musa_dwg_ac1024_line_common_probe(
            record, sizeof(saved), 0u, &common) ||
        !musa_dwg_ac1024_line_fields_probe(
            record, sizeof(saved), 0u, &line) ||
        common.indexed_handle != 44u ||
        common.geometry_start_bit != 73u ||
        common.data_end_bit != 240u ||
        common.encoded_color != 7u ||
        common.color_index != 7u ||
        common.linetype_scale != 1.0 ||
        common.lineweight != 25u ||
        !common.record_crc_verified ||
        line.indexed_handle != 44u ||
        line.geometry_start_bit != 73u ||
        line.geometry_end_bit != 208u ||
        !line.coordinates_decoded ||
        line.start[0] != 1.5 || line.end[0] != 1.5 ||
        line.start[1] != -2.0 || line.end[1] != -2.0 ||
        line.start[2] != 0.0 || line.end[2] != 0.0 ||
        line.thickness != 0.0 ||
        line.extrusion[0] != 0.0 ||
        line.extrusion[1] != 0.0 ||
        line.extrusion[2] != 1.0 ||
        memcmp(saved, record, sizeof(saved))) return 2;
    const MusaDwgAc1024LineCommon old_common = common;
    const MusaDwgAc1024LineFields old_line = line;

    /* Invalid color/scale cannot be promoted into fake geometry. */
    set_bits(record + 3u, 44u, 2u, 2u); /* BD scale=0 */
    if (!seal(record) ||
        musa_dwg_ac1024_line_common_probe(
            record, sizeof(saved), 0u, &common) ||
        musa_dwg_ac1024_line_fields_probe(
            record, sizeof(saved), 0u, &line)) return 3;
    memcpy(record, saved, sizeof(saved));

    /* A no-Z record cannot be coerced into reading missing Z doubles. */
    set_bits(record + 3u, 73u, 1u, 0u);
    if (!seal(record) ||
        musa_dwg_ac1024_line_fields_probe(
            record, sizeof(saved), 0u, &line)) return 4;
    memcpy(record, saved, sizeof(saved));

    /* Invalid common flags, wrong type, CRC, short source and alias fail. */
    set_bits(record + 3u, 26u, 2u, 3u); /* EED nonzero */
    if (!seal(record) ||
        musa_dwg_ac1024_line_common_probe(
            record, sizeof(saved), 0u, &common)) return 5;
    memcpy(record, saved, sizeof(saved));
    set_bits(record + 3u, 2u, 8u, 18u); /* CIRCLE not LINE */
    if (!seal(record) ||
        musa_dwg_ac1024_line_fields_probe(
            record, sizeof(saved), 0u, &line)) return 6;
    memcpy(record, saved, sizeof(saved));
    record[36] ^= 0x01u;
    if (musa_dwg_ac1024_line_fields_probe(
            record, sizeof(saved), 0u, &line)) return 7;
    memcpy(record, saved, sizeof(saved));

    if (musa_dwg_ac1024_line_common_probe(
            record, sizeof(saved) - 1u, 0u, &common) ||
        musa_dwg_ac1024_line_fields_probe(
            record, sizeof(saved), 0u,
            (MusaDwgAc1024LineFields *)(void *)record) ||
        musa_dwg_ac1024_line_common_probe(
            record, sizeof(saved), 0u,
            (MusaDwgAc1024LineCommon *)(void *)record) ||
        memcmp(&old_common, &common, sizeof(common)) ||
        memcmp(&old_line, &line, sizeof(line)) ||
        memcmp(saved, record, sizeof(saved))) return 8;

    /* A CRC-valid original R2010 record can carry supported META syntax
     * but still be outside the narrow LINE common-data policy. The parser
     * must refuse it without changing previously authenticated outputs.
     * Each case keeps the *same* LINE OT, self-H and valid frame CRC. */
    const struct { unsigned bit; unsigned width; unsigned value; } policy[] = {
        {29u, 2u, 1u}, /* implicit entity owner mode is not yet supported */
        {31u, 2u, 3u}, /* BL(3): 256 reactors, not zero */
        {33u, 1u, 0u}  /* XDictionary exists; do not invent its reference */
    };
    for (size_t k = 0u; k < sizeof(policy) / sizeof(policy[0]); k++) {
        memcpy(record, saved, sizeof(saved));
        set_bits(record + 3u, policy[k].bit,
                 policy[k].width, policy[k].value);
        MusaDwgAc1024EntityMetaPrefix candidate = {0};
        if (!seal(record) ||
            !musa_dwg_ac1024_record_entity_meta_prefix(
                record, sizeof(saved), 0u, &candidate) ||
            candidate.raw_ot != 19u ||
            candidate.embedded_absolute_handle != 44u ||
            musa_dwg_ac1024_line_common_probe(
                record, sizeof(saved), 0u, &common) ||
            musa_dwg_ac1024_line_fields_probe(
                record, sizeof(saved), 0u, &line) ||
            memcmp(&old_common, &common, sizeof(common)) ||
            memcmp(&old_line, &line, sizeof(line)))
            return 12 + (int)k;
    }
    memcpy(record, saved, sizeof(saved));
    if (memcmp(saved, record, sizeof(saved))) return 15;

    /* Full XYZ 3D fixture: three genuinely different endpoints exercise
     * RD and absolute DD(3) instead of only the equal-point DD(0) path.
     * No real-source identity/owner/layer or display parity is inferred. */
    uint8_t record3d[69] = {0};
    record3d[0] = 64u; /* MS=64-byte payload, MC=16 handle bits */
    record3d[2] = 16u;
    memcpy(record3d + 3u, saved + 3u, 10u); /* authenticated common header */
    uint8_t *const p3 = record3d + 3u;
    set_bits(p3, 73u, 1u, 0u);  /* nonzero Z stream is present */
    set_rd(p3, 74u, 1.5);       /* start X */
    set_bits(p3, 138u, 2u, 3u); /* end X is a full RD */
    set_rd(p3, 140u, 4.5);
    set_rd(p3, 204u, -2.0);     /* start Y */
    set_bits(p3, 268u, 2u, 3u); /* end Y is a full RD */
    set_rd(p3, 270u, 6.0);
    set_rd(p3, 334u, 3.0);      /* start Z */
    set_bits(p3, 398u, 2u, 3u); /* end Z is a full RD */
    set_rd(p3, 400u, -1.0);
    set_bits(p3, 464u, 1u, 1u); /* zero thickness */
    set_bits(p3, 465u, 1u, 1u); /* default extrusion */
    uint16_t crc3 = 0u;
    if (!musa_core_crc16_arc(0xc0c1u, record3d, 67u, &crc3)) return 9;
    record3d[67] = (uint8_t)crc3;
    record3d[68] = (uint8_t)(crc3 >> 8u);
    uint8_t before3d[sizeof(record3d)];
    memcpy(before3d, record3d, sizeof(record3d));
    MusaDwgAc1024LineCommon common3d = {0};
    MusaDwgAc1024LineFields line3d = {0};
    if (!musa_dwg_ac1024_line_common_probe(
            record3d, sizeof(record3d), 0u, &common3d) ||
        !musa_dwg_ac1024_line_fields_probe(
            record3d, sizeof(record3d), 0u, &line3d) ||
        common3d.indexed_handle != 44u ||
        common3d.geometry_start_bit != 73u ||
        common3d.data_end_bit != 496u ||
        line3d.indexed_handle != 44u ||
        line3d.geometry_end_bit != 466u ||
        !line3d.coordinates_decoded ||
        line3d.start[0] != 1.5 || line3d.end[0] != 4.5 ||
        line3d.start[1] != -2.0 || line3d.end[1] != 6.0 ||
        line3d.start[2] != 3.0 || line3d.end[2] != -1.0 ||
        line3d.thickness != 0.0 ||
        line3d.extrusion[0] != 0.0 ||
        line3d.extrusion[1] != 0.0 ||
        line3d.extrusion[2] != 1.0 ||
        memcmp(before3d, record3d, sizeof(record3d))) return 10;
    const MusaDwgAc1024LineFields prior3d = line3d;
    record3d[68] ^= 1u; /* CRC corruption must not mutate caller output */
    if (musa_dwg_ac1024_line_fields_probe(
            record3d, sizeof(record3d), 0u, &line3d) ||
        memcmp(&line3d, &prior3d, sizeof(line3d))) return 11;

    puts("AC1024 LINE common/XYZ synthetic checks: pass; real-DWG parity unproven");
    return 0;
}
