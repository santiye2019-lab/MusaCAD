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
/* Real CRC-backed synthetic graph: LAYER OT51/H32, BLOCK_HEADER OT49/H47
 * and LINE OT19/H48 in the original handle-index ordering. */
static int make_graph_target(uint8_t *dst, unsigned type, unsigned h) {
    memset(dst, 0, 13u);
    dst[0] = 8u;
    dst[2] = 16u;
    uint8_t *const p = dst + 3u;
    set_bits(p, 2u, 8u, type);
    set_bits(p, 14u, 4u, 1u);
    set_bits(p, 18u, 8u, h);
    uint16_t crc = 0u;
    if (!musa_core_crc16_arc(0xc0c1u, dst, 11u, &crc))
        return 0;
    dst[11] = (uint8_t)crc;
    dst[12] = (uint8_t)(crc >> 8u);
    return 1;
}

static int test_authenticated_line_graph(void) {
    uint8_t source[63] = {0};
    if (!make_graph_target(source, 51u, 32u) ||
        !make_graph_target(source + 13u, 49u, 47u) ||
        !make_line(source + 26u))
        return 0;

    uint8_t *const entity = source + 26u;
    uint8_t *const p = entity + 3u;
    set_bits(p, 18u, 8u, 48u); /* LINE canonical self H */
    entity[2] = 48u; /* 48-bit H tail begins at bit 208 */
    set_bits(p, 208u, 4u, 12u); /* owner relative H: H48-1 = H47 */
    set_bits(p, 212u, 4u, 1u);
    set_bits(p, 216u, 8u, 1u);
    set_bits(p, 224u, 4u, 5u); /* absolute layer H32 */
    set_bits(p, 228u, 4u, 1u);
    set_bits(p, 232u, 8u, 32u);
    if (!seal(entity))
        return 0;

    const uint64_t offsets[3] = {0u, 13u, 26u};
    MusaDwgAc1024SelfIdentity idx[3] = {{0}};
    if (!musa_dwg_ac1024_recover_self_index(
            source, sizeof(source), offsets, 3u, idx, 3u))
        return 0;

    uint8_t source_before[sizeof(source)];
    memcpy(source_before, source, sizeof(source));
    MusaDwgAc1024LineGraphEvidence attested = {0};
    if (!musa_dwg_ac1024_line_graph_probe(
            source, sizeof(source), idx, 3u, 48u, &attested) ||
        attested.geometry.indexed_handle != 48u ||
        attested.geometry.start[0] != 1.5 ||
        attested.geometry.end[0] != 1.5 ||
        attested.geometry.start[1] != -2.0 ||
        attested.geometry.end[1] != -2.0 ||
        attested.geometry.geometry_start_bit != 73u ||
        attested.geometry.geometry_end_bit != 208u ||
        attested.owner_layer.entity_raw_ot != 19u ||
        attested.owner_layer.entity_record_offset != 26u ||
        attested.owner_layer.owner_handle != 47u ||
        attested.owner_layer.layer_handle != 32u ||
        !attested.owner_layer.source_crc_verified ||
        !attested.owner_layer.target_records_verified ||
        memcmp(source, source_before, sizeof(source)))
        return 0;

    const MusaDwgAc1024LineGraphEvidence witness = attested;
    /* XDictionary-present is DATA-only evidence: its extra H stream
     * must not be silently admitted by a strict owner/layer graph. */
    set_bits(p, 33u, 1u, 0u);
    MusaDwgAc1024LineFields xdic_geometry = {0};
    if (!seal(entity) ||
        !musa_dwg_ac1024_line_fields_probe(
            source, sizeof(source), 26u, &xdic_geometry) ||
        xdic_geometry.indexed_handle != 48u ||
        xdic_geometry.start[0] != 1.5 ||
        musa_dwg_ac1024_line_graph_probe(
            source, sizeof(source), idx, 3u, 48u, &attested) ||
        memcmp(&witness, &attested, sizeof(witness)))
        return 0;
    memcpy(source, source_before, sizeof(source));
    /* Valid CRC and provisional model-space DATA are NOT owner evidence.
     * A strict graph join must still refuse mode 2 atomically. */
    set_bits(p, 29u, 2u, 2u);
    if (!seal(entity) ||
        musa_dwg_ac1024_line_graph_probe(
            source, sizeof(source), idx, 3u, 48u, &attested) ||
        memcmp(&witness, &attested, sizeof(witness)))
        return 0;

    memcpy(source, source_before, sizeof(source));
    source[12] ^= 1u; /* CORRUPT original authenticated layer record */
    if (musa_dwg_ac1024_line_graph_probe(
            source, sizeof(source), idx, 3u, 48u, &attested) ||
        memcmp(&witness, &attested, sizeof(witness)))
        return 0;

    memcpy(source, source_before, sizeof(source));
    entity[36] ^= 1u; /* CORRUPT LINE CRC */
    if (musa_dwg_ac1024_line_graph_probe(
            source, sizeof(source), idx, 3u, 48u, &attested) ||
        memcmp(&witness, &attested, sizeof(witness)))
        return 0;

    memcpy(source, source_before, sizeof(source));
    idx[1].raw_ot = 19u; /* do not trust mutated owner-index class */
    if (musa_dwg_ac1024_line_graph_probe(
            source, sizeof(source), idx, 3u, 48u, &attested) ||
        memcmp(&witness, &attested, sizeof(witness)))
        return 0;
    idx[1].raw_ot = 49u;

    if (musa_dwg_ac1024_line_graph_probe(
            source, sizeof(source), idx, 3u, 0u, &attested) ||
        musa_dwg_ac1024_line_graph_probe(
            source, sizeof(source), idx, 3u, 48u, NULL) ||
        musa_dwg_ac1024_line_graph_probe(
            source, sizeof(source), idx, 3u, 48u,
            (MusaDwgAc1024LineGraphEvidence *)(void *)source) ||
        musa_dwg_ac1024_line_graph_probe(
            source, sizeof(source), idx, 3u, 48u,
            (MusaDwgAc1024LineGraphEvidence *)(void *)idx) ||
        memcmp(&witness, &attested, sizeof(witness)) ||
        memcmp(source, source_before, sizeof(source)))
        return 0;
    return 1;
}


/* Synthetic R2010 model/paper LINE omits the owner H: its H stream
 * begins with a layer reference. Only the CRC-backed original OT51
 * layer may be joined; do NOT synthesize an owner/block. */
static int test_implicit_line_layer_graph(void) {
    uint8_t source[50] = {0};
    if (!make_graph_target(source, 51u, 32u) ||
        !make_line(source + 13u))
        return 0;
    uint8_t *const entity = source + 13u;
    uint8_t *const p = entity + 3u;
    set_bits(p, 18u, 8u, 48u); /* canonical LINE self H */
    set_bits(p, 29u, 2u, 2u);  /* implicit model-space owner */
    set_bits(p, 240u, 4u, 5u); /* first H is the layer, no owner H */
    set_bits(p, 244u, 4u, 1u);
    set_bits(p, 248u, 8u, 32u);
    if (!seal(entity)) return 0;
    const uint64_t offsets[2] = {0u, 13u};
    MusaDwgAc1024SelfIdentity idx[2] = {{0}};
    if (!musa_dwg_ac1024_recover_self_index(
            source, sizeof(source), offsets, 2u, idx, 2u))
        return 0;

    uint8_t before[sizeof(source)];
    memcpy(before, source, sizeof(before));
    MusaDwgAc1024ImplicitLineLayerEvidence witness = {0};
    if (!musa_dwg_ac1024_line_implicit_layer_probe(
            source, sizeof(source), idx, 2u, 48u, &witness) ||
        witness.geometry.indexed_handle != 48u ||
        witness.geometry.start[0] != 1.5 ||
        witness.geometry.end[1] != -2.0 ||
        witness.geometry.geometry_start_bit != 73u ||
        witness.geometry.geometry_end_bit != 208u ||
        witness.implicit_layer.entity_handle != 48u ||
        witness.implicit_layer.entity_record_offset != 13u ||
        witness.implicit_layer.layer_handle != 32u ||
        witness.implicit_layer.layer_record_offset != 0u ||
        witness.implicit_layer.entity_mode != 2u ||
        witness.implicit_layer.layer_raw_h_code != 5u ||
        witness.implicit_layer.entity_raw_ot != 19u ||
        witness.implicit_layer.handle_tail_unparsed_bits != 0u ||
        !witness.implicit_layer.source_crc_verified ||
        !witness.implicit_layer.layer_crc_verified ||
        memcmp(before, source, sizeof(before)))
        return 0;
    const MusaDwgAc1024ImplicitLineLayerEvidence saved = witness;

    /* Same original fields in paper space; no owner is asserted. */
    set_bits(p, 29u, 2u, 1u);
    if (!seal(entity) ||
        !musa_dwg_ac1024_line_implicit_layer_probe(
            source, sizeof(source), idx, 2u, 48u, &witness) ||
        witness.implicit_layer.entity_mode != 1u ||
        witness.implicit_layer.layer_handle != 32u ||
        witness.geometry.start[1] != -2.0)
        return 0;
    witness = saved;
    memcpy(source, before, sizeof(source));

    /* Explicit owner mode cannot use this layer-only witness. */
    set_bits(p, 29u, 2u, 0u);
    if (!seal(entity) ||
        musa_dwg_ac1024_line_implicit_layer_probe(
            source, sizeof(source), idx, 2u, 48u, &witness) ||
        memcmp(&witness, &saved, sizeof(saved)))
        return 0;
    memcpy(source, before, sizeof(source));

    set_bits(p, 248u, 8u, 35u); /* H target does not exist */
    if (!seal(entity) ||
        musa_dwg_ac1024_line_implicit_layer_probe(
            source, sizeof(source), idx, 2u, 48u, &witness) ||
        memcmp(&witness, &saved, sizeof(saved)))
        return 0;
    memcpy(source, before, sizeof(source));

    source[12] ^= 1u; /* original layer CRC must be authenticated */
    if (musa_dwg_ac1024_line_implicit_layer_probe(
            source, sizeof(source), idx, 2u, 48u, &witness) ||
        memcmp(&witness, &saved, sizeof(saved)))
        return 0;
    memcpy(source, before, sizeof(source));

    entity[36] ^= 1u; /* LINE CRC, no partial output */
    if (musa_dwg_ac1024_line_implicit_layer_probe(
            source, sizeof(source), idx, 2u, 48u, &witness) ||
        memcmp(&witness, &saved, sizeof(saved)))
        return 0;
    memcpy(source, before, sizeof(source));

    idx[0].raw_ot = 49u; /* index target class is not a layer */
    if (musa_dwg_ac1024_line_implicit_layer_probe(
            source, sizeof(source), idx, 2u, 48u, &witness) ||
        memcmp(&witness, &saved, sizeof(saved)))
        return 0;
    idx[0].raw_ot = 51u;

    if (musa_dwg_ac1024_line_implicit_layer_probe(
            source, sizeof(source), idx, 2u, 0u, &witness) ||
        musa_dwg_ac1024_line_implicit_layer_probe(
            source, sizeof(source), idx, 2u, 48u, NULL) ||
        musa_dwg_ac1024_line_implicit_layer_probe(
            source, sizeof(source), idx, 2u, 48u,
            (MusaDwgAc1024ImplicitLineLayerEvidence *)(void *)source) ||
        musa_dwg_ac1024_line_implicit_layer_probe(
            source, sizeof(source), idx, 2u, 48u,
            (MusaDwgAc1024ImplicitLineLayerEvidence *)(void *)idx) ||
        memcmp(&witness, &saved, sizeof(saved)) ||
        memcmp(source, before, sizeof(source)))
        return 0;
    return 1;
}

/* Original R2010 XDictionary-present LINE H graph.
 * Independent CRC-backed source records: layer OT51/H32, dictionary
 * OT42/H34, block OT49/H47 and LINE OT19/H48. No external CAD reader,
 * unverified layout owner, render acceptance or source mutation. */
static int test_xdictionary_original_handle_graph(void) {
    uint8_t source[92] = {0};
    if (!make_graph_target(source, 51u, 32u) ||
        !make_graph_target(source + 13u, 42u, 34u) ||
        !make_graph_target(source + 26u, 49u, 47u) ||
        !make_line(source + 39u))
        return 0;
    uint8_t *const record = source + 39u;
    uint8_t *const payload = record + 3u;
    record[0] = 48u; /* 48 data bytes, not the old 32-byte fixture */
    record[2] = 48u; /* H stream: 3 canonical 16-bit handles */
    record[35] = 0u; record[36] = 0u; /* discarded old short CRC */
    set_bits(payload, 18u, 8u, 48u);
    set_bits(payload, 33u, 1u, 0u); /* XDictionary is present */
    set_bits(payload, 336u, 4u, 4u); /* owner soft pointer H47 */
    set_bits(payload, 340u, 4u, 1u);
    set_bits(payload, 344u, 8u, 47u);
    set_bits(payload, 352u, 4u, 3u); /* XDictionary hard owner H34 */
    set_bits(payload, 356u, 4u, 1u);
    set_bits(payload, 360u, 8u, 34u);
    set_bits(payload, 368u, 4u, 5u); /* layer hard pointer H32 */
    set_bits(payload, 372u, 4u, 1u);
    set_bits(payload, 376u, 8u, 32u);

    uint16_t crc = 0u;
    if (!musa_core_crc16_arc(0xc0c1u, record, 51u, &crc)) return 0;
    record[51] = (uint8_t)crc;
    record[52] = (uint8_t)(crc >> 8u);
    const uint64_t offsets[4] = {0u, 13u, 26u, 39u};
    MusaDwgAc1024SelfIdentity idx[4] = {{0}};
    if (!musa_dwg_ac1024_recover_self_index(
            source, sizeof(source), offsets, 4u, idx, 4u))
        return 0;

    uint8_t pristine[sizeof(source)];
    memcpy(pristine, source, sizeof(pristine));
    MusaDwgAc1024LineXDictionaryGraph graph = {0};
    MusaDwgAc1024LineFields provisional = {0};
    MusaDwgAc1024LineGraphEvidence old_graph = {0};
    if (!musa_dwg_ac1024_line_xdictionary_graph_probe(
            source, sizeof(source), idx, 4u, 48u, &graph) ||
        !musa_dwg_ac1024_line_fields_probe(
            source, sizeof(source), 39u, &provisional) ||
        musa_dwg_ac1024_line_graph_probe(
            source, sizeof(source), idx, 4u, 48u, &old_graph) ||
        graph.entity_handle != 48u || graph.entity_record_offset != 39u ||
        graph.entity_mode != 0u || graph.owner_handle != 47u ||
        graph.owner_record_offset != 26u || graph.owner_raw_h_code != 4u ||
        graph.xdictionary_handle != 34u ||
        graph.xdictionary_record_offset != 13u ||
        graph.xdictionary_raw_h_code != 3u ||
        graph.layer_handle != 32u || graph.layer_record_offset != 0u ||
        graph.layer_raw_h_code != 5u ||
        graph.handle_tail_unparsed_bits != 0u ||
        !graph.original_records_crc_verified ||
        provisional.indexed_handle != 48u ||
        provisional.start[0] != 1.5 || provisional.end[1] != -2.0 ||
        memcmp(pristine, source, sizeof(source)))
        return 0;
    const MusaDwgAc1024LineXDictionaryGraph saved = graph;

    /* Broken original DICTIONARY CRC cannot impersonate valid XDic. */
    source[25] ^= 1u;
    if (musa_dwg_ac1024_line_xdictionary_graph_probe(
            source, sizeof(source), idx, 4u, 48u, &graph) ||
        memcmp(&graph, &saved, sizeof(saved))) return 0;
    memcpy(source, pristine, sizeof(source));

    /* A hard owner H must have a canonical code, not a soft pointer. */
    set_bits(payload, 352u, 4u, 4u);
    if (!musa_core_crc16_arc(0xc0c1u, record, 51u, &crc)) return 0;
    record[51] = (uint8_t)crc;
    record[52] = (uint8_t)(crc >> 8u);
    if (musa_dwg_ac1024_line_xdictionary_graph_probe(
            source, sizeof(source), idx, 4u, 48u, &graph) ||
        memcmp(&graph, &saved, sizeof(saved))) return 0;
    memcpy(source, pristine, sizeof(source));

    /* Implicit model space: no owner H is serialized. The XDictionary
     * and LAYER references move to the beginning of the H stream. */
    set_bits(payload, 29u, 2u, 2u);
    set_bits(payload, 336u, 4u, 3u);
    set_bits(payload, 340u, 4u, 1u);
    set_bits(payload, 344u, 8u, 34u);
    set_bits(payload, 352u, 4u, 5u);
    set_bits(payload, 356u, 4u, 1u);
    set_bits(payload, 360u, 8u, 32u);
    set_bits(payload, 368u, 16u, 0u);
    if (!musa_core_crc16_arc(0xc0c1u, record, 51u, &crc)) return 0;
    record[51] = (uint8_t)crc;
    record[52] = (uint8_t)(crc >> 8u);
    if (!musa_dwg_ac1024_line_xdictionary_graph_probe(
            source, sizeof(source), idx, 4u, 48u, &graph) ||
        graph.entity_mode != 2u || graph.owner_handle != 0u ||
        graph.owner_record_offset != 0u ||
        graph.xdictionary_handle != 34u || graph.layer_handle != 32u ||
        graph.handle_tail_unparsed_bits != 16u)
        return 0;

    memcpy(source, pristine, sizeof(source));
    idx[1].raw_ot = 49u; /* an index-level fake is insufficient */
    if (musa_dwg_ac1024_line_xdictionary_graph_probe(
            source, sizeof(source), idx, 4u, 48u, &graph)) return 0;
    idx[1].raw_ot = 42u;

    if (musa_dwg_ac1024_line_xdictionary_graph_probe(
            source, sizeof(source), idx, 4u, 0u, &graph) ||
        musa_dwg_ac1024_line_xdictionary_graph_probe(
            source, sizeof(source), idx, 4u, 48u, NULL) ||
        musa_dwg_ac1024_line_xdictionary_graph_probe(
            source, sizeof(source), idx, 4u, 48u,
            (MusaDwgAc1024LineXDictionaryGraph *)(void *)source) ||
        musa_dwg_ac1024_line_xdictionary_graph_probe(
            source, sizeof(source), idx, 4u, 48u,
            (MusaDwgAc1024LineXDictionaryGraph *)(void *)idx) ||
        memcmp(source, pristine, sizeof(source)))
        return 0;
    return 1;
}

/* A genuine CRC-backed R2010 LINE EED application is inserted
 * between the canonical self-H and common entity DATA. Copying its
 * original common and geometry bits shifted by exactly 34 bits proves
 * the new EED API does not guess the LINE decoder's starting cursor.
 * There is no owner H/scene/AutoCAD geometric parity claim. */
static int test_eed_line_xyz_provisional(void) {
    uint8_t base[37] = {0};
    if (!make_line(base)) return 0;
    uint8_t record[45] = {0};
    record[0] = 40u; /* MS=40 payload bytes, CRC at absolute 43 */
    record[2] = 16u; /* 16-bit H stream, begins at payload bit 304 */
    uint8_t *const p = record + 3u;
    set_bits(p, 0u, 2u, 0u);
    set_bits(p, 2u, 8u, 19u);
    set_bits(p, 10u, 4u, 0u);
    set_bits(p, 14u, 4u, 1u);
    set_bits(p, 18u, 8u, 44u);
    set_bits(p, 26u, 2u, 1u); /* EED BS length=1 */
    set_bits(p, 28u, 8u, 1u);
    set_bits(p, 36u, 4u, 5u); /* canonical APPID H5 */
    set_bits(p, 40u, 4u, 1u);
    set_bits(p, 44u, 8u, 42u);
    set_bits(p, 52u, 8u, 0x70u); /* opaque EED byte */
    set_bits(p, 60u, 2u, 2u); /* terminating BS zero */

    /* Old fixture zero-EED ended at bit 28; the new EED boundary
     * ends at bit 62. Retain all post-EED DATA bits through 240. */
    for (size_t bit = 28u; bit < 240u; ++bit) {
        const uint8_t old_bit =
            (uint8_t)((base[3u + bit / 8u] >>
                      (7u - bit % 8u)) & 1u);
        set_bits(p, bit + 34u, 1u, old_bit);
    }
    uint16_t crc = 0u;
    if (!musa_core_crc16_arc(0xc0c1u, record, 43u, &crc))
        return 0;
    record[43] = (uint8_t)crc;
    record[44] = (uint8_t)(crc >> 8u);
    uint8_t pristine[sizeof(record)];
    memcpy(pristine, record, sizeof(record));

    MusaDwgAc1024EntityMetaPrefix meta = {0};
    MusaDwgAc1024LineFields xyz = {0};
    if (!musa_dwg_ac1024_record_line_eed_meta_prefix(
            record, sizeof(record), 0u, &meta) ||
        meta.embedded_absolute_handle != 44u ||
        meta.raw_ot != 19u ||
        meta.data_after_xdic_bit != 68u ||
        !musa_dwg_ac1024_line_eed_fields_probe(
            record, sizeof(record), 0u, &xyz) ||
        xyz.indexed_handle != 44u ||
        xyz.geometry_start_bit != 107u ||
        xyz.geometry_end_bit != 242u ||
        xyz.start[0] != 1.5 || xyz.end[0] != 1.5 ||
        xyz.start[1] != -2.0 || xyz.end[1] != -2.0 ||
        xyz.start[2] != 0.0 || xyz.end[2] != 0.0 ||
        !xyz.coordinates_decoded ||
        musa_dwg_ac1024_line_fields_probe(
            record, sizeof(record), 0u, &xyz) ||
        memcmp(pristine, record, sizeof(record)))
        return 0;
    const MusaDwgAc1024LineFields saved = xyz;

    /* EED-bearing API cannot be tricked by missing APPID handle,
     * overlong EED, proxy graphics, CRC damage or source alias. */
    set_bits(p, 36u, 4u, 4u);
    if (!musa_core_crc16_arc(0xc0c1u, record, 43u, &crc))
        return 0;
    record[43] = (uint8_t)crc; record[44] = (uint8_t)(crc >> 8u);
    if (musa_dwg_ac1024_line_eed_fields_probe(
            record, sizeof(record), 0u, &xyz) ||
        memcmp(&saved, &xyz, sizeof(saved))) return 0;
    memcpy(record, pristine, sizeof(record));

    set_bits(p, 28u, 8u, 40u);
    if (!musa_core_crc16_arc(0xc0c1u, record, 43u, &crc))
        return 0;
    record[43] = (uint8_t)crc; record[44] = (uint8_t)(crc >> 8u);
    if (musa_dwg_ac1024_line_eed_fields_probe(
            record, sizeof(record), 0u, &xyz) ||
        memcmp(&saved, &xyz, sizeof(saved))) return 0;
    memcpy(record, pristine, sizeof(record));

    set_bits(p, 62u, 1u, 1u); /* graphic present; cannot skip proxy */
    if (!musa_core_crc16_arc(0xc0c1u, record, 43u, &crc))
        return 0;
    record[43] = (uint8_t)crc; record[44] = (uint8_t)(crc >> 8u);
    if (musa_dwg_ac1024_line_eed_fields_probe(
            record, sizeof(record), 0u, &xyz) ||
        memcmp(&saved, &xyz, sizeof(saved))) return 0;
    memcpy(record, pristine, sizeof(record));

    record[44] ^= 1u;
    if (musa_dwg_ac1024_line_eed_fields_probe(
            record, sizeof(record), 0u, &xyz) ||
        memcmp(&saved, &xyz, sizeof(saved))) return 0;
    memcpy(record, pristine, sizeof(record));

    if (musa_dwg_ac1024_line_eed_fields_probe(
            record, sizeof(record), 0u, NULL) ||
        musa_dwg_ac1024_line_eed_fields_probe(
            record, sizeof(record), 0u,
            (MusaDwgAc1024LineFields *)(void *)record) ||
        memcmp(&saved, &xyz, sizeof(saved)) ||
        memcmp(pristine, record, sizeof(record)))
        return 0;

    /* Valid zero-EED LINE cannot be mislabeled as having EED frames. */
    if (musa_dwg_ac1024_line_eed_fields_probe(
            base, sizeof(base), 0u, &xyz) ||
        memcmp(&saved, &xyz, sizeof(saved)))
        return 0;
    return 1;
}

/* R2004+ ENC truecolor BL contains method byte C2, then RRGGBB.
 * The extra 42 encoded bits shift the following common fields/geometry.
 * All source bits are derived from this first-party 2D LINE fixture,
 * not another CAD implementation or a guessed entity record. */
static int test_r2010_tagged_truecolor(void) {
    uint8_t plain[37] = {0};
    uint8_t tagged[45] = {0};
    if (!make_line(plain)) return 0;
    tagged[0] = 40u; /* MS 40-byte payload + MC 16 handle bits */
    tagged[2] = 16u;
    const uint8_t *const src = plain + 3u;
    uint8_t *const dst = tagged + 3u;
    for (size_t bit = 0u; bit < 34u; ++bit) {
        const uint32_t v = (uint32_t)((src[bit/8u] >>
                             (7u - bit%8u)) & 1u);
        set_bits(dst, bit, 1u, v);
    }
    set_bits(dst, 34u, 2u, 0u); /* BS raw16 */
    set_bits(dst, 36u, 8u, 0x00u); /* BS raw16 LOW byte */
    set_bits(dst, 44u, 8u, 0x80u); /* BS raw16 HIGH byte: 0x8000 ENC */
    set_bits(dst, 52u, 2u, 0u); /* BL raw32 */
    set_bits(dst, 54u, 8u, 0x33u); /* B */
    set_bits(dst, 62u, 8u, 0x22u); /* G */
    set_bits(dst, 70u, 8u, 0x11u); /* R */
    set_bits(dst, 78u, 8u, 0xc2u); /* truecolor method */
    for (size_t bit = 44u; bit < 240u; ++bit) {
        const uint32_t v = (uint32_t)((src[bit/8u] >>
                             (7u - bit%8u)) & 1u);
        set_bits(dst, bit + 42u, 1u, v);
    }
    uint16_t crc = 0u;
    if (!musa_core_crc16_arc(0xc0c1u, tagged, 43u, &crc))
        return 0;
    tagged[43] = (uint8_t)crc;
    tagged[44] = (uint8_t)(crc >> 8u);
    uint8_t preserved[sizeof(tagged)];
    memcpy(preserved, tagged, sizeof(tagged));
    MusaDwgAc1024LineCommon common = {0};
    MusaDwgAc1024LineFields xyz = {0};
    if (!musa_dwg_ac1024_line_common_probe(
            tagged, sizeof(tagged), 0u, &common) ||
        !musa_dwg_ac1024_line_fields_probe(
            tagged, sizeof(tagged), 0u, &xyz) ||
        musa_dwg_ac1024_line_common_failure_stage(
            tagged, sizeof(tagged), 0u) != MUSA_DWG_LINE_COMMON_OK ||
        !common.has_rgb || common.rgb_method != 0xc2u ||
        common.rgb_raw != 0xc2112233u ||
        common.rgb != 0x00112233u ||
        common.geometry_start_bit != 115u ||
        xyz.geometry_start_bit != 115u ||
        xyz.geometry_end_bit != 250u ||
        xyz.start[0] != 1.5 || xyz.end[0] != 1.5 ||
        xyz.start[1] != -2.0 || xyz.end[1] != -2.0 ||
        memcmp(preserved, tagged, sizeof(tagged)))
        return 0;
    const MusaDwgAc1024LineCommon saved = common;

    /* Unsupported high-byte method remains rejected atomically. */
    set_bits(dst, 78u, 8u, 0xc1u);
    if (!musa_core_crc16_arc(0xc0c1u, tagged, 43u, &crc))
        return 0;
    tagged[43] = (uint8_t)crc;
    tagged[44] = (uint8_t)(crc >> 8u);
    if (musa_dwg_ac1024_line_common_probe(
            tagged, sizeof(tagged), 0u, &common) ||
        musa_dwg_ac1024_line_fields_probe(
            tagged, sizeof(tagged), 0u, &xyz) ||
        musa_dwg_ac1024_line_common_failure_stage(
            tagged, sizeof(tagged), 0u) != MUSA_DWG_LINE_COMMON_RGB ||
        memcmp(&common, &saved, sizeof(saved)))
        return 0;
    memcpy(tagged, preserved, sizeof(tagged));

    /* No CRC workaround: even supported C2 must be refused if corrupt. */
    tagged[44] ^= 1u;
    if (musa_dwg_ac1024_line_common_probe(
            tagged, sizeof(tagged), 0u, &common) ||
        memcmp(&common, &saved, sizeof(saved)))
        return 0;
    return 1;
}

/* CRC-backed EED LINE: APPID H5 is bounded, then independently
 * verify original layer OT51 and, only if mode 0, owner OT49.
 * This fixture is NOT an assertion of block transforms/render readiness. */
static int test_eed_line_original_graph(void) {
    uint8_t base[37] = {0};
    uint8_t source[71] = {0};
    if (!make_line(base) ||
        !make_graph_target(source, 51u, 32u) ||
        !make_graph_target(source + 13u, 49u, 47u))
        return 0;
    uint8_t *const entity = source + 26u;
    uint8_t *const p = entity + 3u;
    entity[0] = 40u; /* 40 DATA/H bytes, CRC at entity[43..44] */
    entity[2] = 16u; /* implicit-space layer H is 16 bits at bit304 */
    set_bits(p, 2u, 8u, 19u);
    set_bits(p, 14u, 4u, 1u);
    set_bits(p, 18u, 8u, 48u); /* canonical LINE self H */
    set_bits(p, 26u, 2u, 1u); /* EED: BS(1) */
    set_bits(p, 28u, 8u, 1u);
    set_bits(p, 36u, 4u, 5u); /* EED APPID H5 */
    set_bits(p, 40u, 4u, 1u);
    set_bits(p, 44u, 8u, 42u);
    set_bits(p, 52u, 8u, 0x70u); /* opaque app byte */
    set_bits(p, 60u, 2u, 2u); /* zero-EED terminator */
    for (size_t bit = 28u; bit < 240u; ++bit) {
        const uint8_t v = (uint8_t)((base[3u + bit / 8u] >>
                                   (7u - bit % 8u)) & 1u);
        set_bits(p, bit + 34u, 1u, v);
    }
    set_bits(p, 63u, 2u, 2u); /* implicit MODEL space */
    set_bits(p, 304u, 4u, 5u); /* LAYER H5/H32 */
    set_bits(p, 308u, 4u, 1u);
    set_bits(p, 312u, 8u, 32u);
    uint16_t crc = 0u;
    if (!musa_core_crc16_arc(0xc0c1u, entity, 43u, &crc))
        return 0;
    entity[43] = (uint8_t)crc;
    entity[44] = (uint8_t)(crc >> 8u);

    const uint64_t offsets[3] = {0u, 13u, 26u};
    MusaDwgAc1024SelfIdentity idx[3] = {{0}};
    if (!musa_dwg_ac1024_recover_self_index(
            source, sizeof(source), offsets, 3u, idx, 3u))
        return 0;

    uint8_t pristine[sizeof(source)];
    memcpy(pristine, source, sizeof(pristine));
    MusaDwgAc1024LineEedJoinedEvidence result = {0};
    if (!musa_dwg_ac1024_line_eed_verified_probe(
            source, sizeof(source), idx, 3u, 48u, &result) ||
        result.geometry.indexed_handle != 48u ||
        result.geometry.geometry_start_bit != 107u ||
        result.geometry.geometry_end_bit != 242u ||
        result.geometry.start[0] != 1.5 ||
        result.geometry.start[1] != -2.0 ||
        result.graph.eed.application_count != 1u ||
        result.graph.eed.application_data_bytes != 1u ||
        result.graph.handles.entity_mode != 2u ||
        result.graph.handles.entity_record_offset != 26u ||
        result.graph.handles.layer_handle != 32u ||
        result.graph.handles.layer_record_offset != 0u ||
        result.graph.handles.owner_handle != 0u ||
        result.graph.handles.xdictionary_handle != 0u ||
        result.graph.handles.handle_tail_unparsed_bits != 0u ||
        !result.graph.handles.original_records_crc_verified ||
        memcmp(pristine, source, sizeof(source)))
        return 0;
    const MusaDwgAc1024LineEedJoinedEvidence saved = result;

    /* PAPER space is another implicit mode: no owner is synthesized. */
    set_bits(p, 63u, 2u, 1u);
    if (!musa_core_crc16_arc(0xc0c1u, entity, 43u, &crc))
        return 0;
    entity[43] = (uint8_t)crc;
    entity[44] = (uint8_t)(crc >> 8u);
    if (!musa_dwg_ac1024_line_eed_verified_probe(
            source, sizeof(source), idx, 3u, 48u, &result) ||
        result.graph.handles.entity_mode != 1u ||
        result.graph.handles.owner_handle != 0u ||
        result.graph.handles.layer_handle != 32u)
        return 0;
    result = saved;
    memcpy(source, pristine, sizeof(source));

    /* Explicit mode 0 has a DIFFERENT tail grammar: owner H4 followed
     * by layer H5. No reinterpretation of model-space layout occurs. */
    entity[2] = 32u; /* 32-bit tail starts at bit288 */
    set_bits(p, 63u, 2u, 0u);
    set_bits(p, 288u, 4u, 4u); /* owner H4 -> OT49/H47 */
    set_bits(p, 292u, 4u, 1u);
    set_bits(p, 296u, 8u, 47u);
    set_bits(p, 304u, 4u, 5u); /* layer H5 -> OT51/H32 */
    set_bits(p, 308u, 4u, 1u);
    set_bits(p, 312u, 8u, 32u);
    if (!musa_core_crc16_arc(0xc0c1u, entity, 43u, &crc))
        return 0;
    entity[43] = (uint8_t)crc;
    entity[44] = (uint8_t)(crc >> 8u);
    if (!musa_dwg_ac1024_line_eed_verified_probe(
            source, sizeof(source), idx, 3u, 48u, &result) ||
        result.graph.handles.entity_mode != 0u ||
        result.graph.handles.owner_handle != 47u ||
        result.graph.handles.owner_record_offset != 13u ||
        result.graph.handles.layer_handle != 32u)
        return 0;
    memcpy(source, pristine, sizeof(source));
    result = saved;

    /* A correct LINE CRC cannot bypass an invalid original LAYER CRC. */
    source[12] ^= 1u;
    if (musa_dwg_ac1024_line_eed_verified_probe(
            source, sizeof(source), idx, 3u, 48u, &result) ||
        memcmp(&saved, &result, sizeof(saved)))
        return 0;
    memcpy(source, pristine, sizeof(source));

    /* Broken EED APPID grammar with re-sealed CRC is still refused. */
    set_bits(p, 36u, 4u, 4u);
    if (!musa_core_crc16_arc(0xc0c1u, entity, 43u, &crc))
        return 0;
    entity[43] = (uint8_t)crc;
    entity[44] = (uint8_t)(crc >> 8u);
    if (musa_dwg_ac1024_line_eed_verified_probe(
            source, sizeof(source), idx, 3u, 48u, &result) ||
        memcmp(&saved, &result, sizeof(saved)))
        return 0;
    memcpy(source, pristine, sizeof(source));

    /* XDictionary-present flag means another H3 is mandatory; without
     * an authenticated DICTIONARY OT42 reference this MUST refuse. */
    set_bits(p, 67u, 1u, 0u);
    if (!musa_core_crc16_arc(0xc0c1u, entity, 43u, &crc))
        return 0;
    entity[43] = (uint8_t)crc;
    entity[44] = (uint8_t)(crc >> 8u);
    if (musa_dwg_ac1024_line_eed_verified_probe(
            source, sizeof(source), idx, 3u, 48u, &result) ||
        memcmp(&saved, &result, sizeof(saved)))
        return 0;
    memcpy(source, pristine, sizeof(source));

    /* EED and a real XDictionary can coexist. The H stream now has
     * explicit owner H4, DICTIONARY H3 and layer H5; all referenced
     * targets must be authenticated ORIGINAL OT49/OT42/OT51 records. */
    uint8_t with_xdic[84] = {0};
    memcpy(with_xdic, source, 26u); /* original LAYER and BLOCK_HEADER */
    if (!make_graph_target(with_xdic + 26u, 42u, 34u))
        return 0;
    memcpy(with_xdic + 39u, pristine + 26u, 45u);
    uint8_t *const xe = with_xdic + 39u;
    uint8_t *const xp = xe + 3u;
    xe[2] = 48u; /* three 16-bit references: handle tail starts bit272 */
    set_bits(xp, 63u, 2u, 0u); /* explicit owner */
    set_bits(xp, 67u, 1u, 0u); /* XDictionary PRESENT */
    set_bits(xp, 272u, 4u, 4u); /* BLOCK_HEADER OT49/H47 */
    set_bits(xp, 276u, 4u, 1u);
    set_bits(xp, 280u, 8u, 47u);
    set_bits(xp, 288u, 4u, 3u); /* DICTIONARY OT42/H34 */
    set_bits(xp, 292u, 4u, 1u);
    set_bits(xp, 296u, 8u, 34u);
    set_bits(xp, 304u, 4u, 5u); /* LAYER OT51/H32 */
    set_bits(xp, 308u, 4u, 1u);
    set_bits(xp, 312u, 8u, 32u);
    if (!musa_core_crc16_arc(0xc0c1u, xe, 43u, &crc))
        return 0;
    xe[43] = (uint8_t)crc;
    xe[44] = (uint8_t)(crc >> 8u);
    const uint64_t xoffsets[4] = {0u, 13u, 26u, 39u};
    MusaDwgAc1024SelfIdentity xindex[4] = {{0}};
    if (!musa_dwg_ac1024_recover_self_index(
            with_xdic, sizeof(with_xdic), xoffsets, 4u, xindex, 4u))
        return 0;
    MusaDwgAc1024LineEedJoinedEvidence xwitness = {0};
    if (!musa_dwg_ac1024_line_eed_verified_probe(
            with_xdic, sizeof(with_xdic), xindex, 4u, 48u, &xwitness) ||
        xwitness.geometry.geometry_start_bit != 107u ||
        xwitness.graph.handles.owner_handle != 47u ||
        xwitness.graph.handles.xdictionary_handle != 34u ||
        xwitness.graph.handles.layer_handle != 32u ||
        xwitness.graph.handles.entity_record_offset != 39u ||
        xwitness.graph.handles.xdictionary_record_offset != 26u ||
        xwitness.graph.handles.handle_tail_unparsed_bits != 0u ||
        !xwitness.graph.handles.original_records_crc_verified)
        return 0;
    const MusaDwgAc1024LineEedJoinedEvidence xsave = xwitness;
    with_xdic[38] ^= 1u; /* original dictionary CRC corruption */
    if (musa_dwg_ac1024_line_eed_verified_probe(
            with_xdic, sizeof(with_xdic), xindex, 4u, 48u, &xwitness) ||
        memcmp(&xsave, &xwitness, sizeof(xsave)))
        return 0;

    /* Fail-closed on fabricated index, NULL and overlapping destination. */
    idx[0].raw_ot = 49u;
    if (musa_dwg_ac1024_line_eed_verified_probe(
            source, sizeof(source), idx, 3u, 48u, &result) ||
        memcmp(&saved, &result, sizeof(saved)))
        return 0;
    idx[0].raw_ot = 51u;
    if (musa_dwg_ac1024_line_eed_verified_probe(
            source, sizeof(source), idx, 3u, 48u, NULL) ||
        musa_dwg_ac1024_line_eed_verified_probe(
            source, sizeof(source), idx, 3u, 48u,
            (MusaDwgAc1024LineEedJoinedEvidence *)(void *)source) ||
        musa_dwg_ac1024_line_eed_verified_probe(
            source, sizeof(source), idx, 3u, 48u,
            (MusaDwgAc1024LineEedJoinedEvidence *)(void *)idx) ||
        memcmp(&saved, &result, sizeof(saved)) ||
        memcmp(pristine, source, sizeof(source)))
        return 0;
    return 1;
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
    /* Diagnostic must call the SAME strict reader; no duplicate guesser,
     * caller output write, or silent support for invalid color/scale.
     * These CRC-valid mutations place faults at known DATA stages. */
    if (musa_dwg_ac1024_line_common_failure_stage(
            record, sizeof(saved), 0u) != MUSA_DWG_LINE_COMMON_OK ||
        musa_dwg_ac1024_line_common_failure_stage(
            NULL, sizeof(saved), 0u) != MUSA_DWG_LINE_COMMON_ARGUMENT)
        return 27;
    set_bits(record + 3u, 34u, 2u, 0u); /* BS raw16 color */
    set_bits(record + 3u, 36u, 16u, 257u); /* unsupported ACI */
    if (!seal(record) ||
        musa_dwg_ac1024_line_common_failure_stage(
            record, sizeof(saved), 0u) != MUSA_DWG_LINE_COMMON_COLOR ||
        musa_dwg_ac1024_line_common_probe(
            record, sizeof(saved), 0u, &common) ||
        memcmp(&old_common, &common, sizeof(common)))
        return 28;
    memcpy(record, saved, sizeof(saved));
    set_bits(record + 3u, 44u, 2u, 2u); /* BD scale=0 */
    if (!seal(record) ||
        musa_dwg_ac1024_line_common_failure_stage(
            record, sizeof(saved), 0u) !=
                MUSA_DWG_LINE_COMMON_LTYPE_SCALE ||
        musa_dwg_ac1024_line_common_probe(
            record, sizeof(saved), 0u, &common) ||
        memcmp(&old_common, &common, sizeof(common)))
        return 29;
    memcpy(record, saved, sizeof(saved));
    set_bits(record + 3u, 29u, 2u, 3u); /* reserved mode */
    if (!seal(record) ||
        musa_dwg_ac1024_line_common_failure_stage(
            record, sizeof(saved), 0u) != MUSA_DWG_LINE_COMMON_META ||
        musa_dwg_ac1024_line_common_probe(
            record, sizeof(saved), 0u, &common) ||
        memcmp(&old_common, &common, sizeof(common)))
        return 30;
    memcpy(record, saved, sizeof(saved));


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

    /* XDictionary-present changes the separate H references, NOT
     * the DATA common fields or coordinate boundaries. This fixture
     * intentionally has NO authenticated XDictionary target: only
     * provisional CRC-backed LINE DATA may be decoded, not a scene. */
    memcpy(record, saved, sizeof(saved));
    set_bits(record + 3u, 33u, 1u, 0u);
    MusaDwgAc1024EntityMetaPrefix xdic_meta = {0};
    if (!seal(record) ||
        !musa_dwg_ac1024_record_entity_meta_prefix(
            record, sizeof(saved), 0u, &xdic_meta) ||
        xdic_meta.xdictionary_missing != 0u ||
        xdic_meta.embedded_absolute_handle != 44u ||
        !musa_dwg_ac1024_line_common_probe(
            record, sizeof(saved), 0u, &common) ||
        !musa_dwg_ac1024_line_fields_probe(
            record, sizeof(saved), 0u, &line) ||
        common.indexed_handle != 44u ||
        line.indexed_handle != 44u ||
        line.start[0] != 1.5 || line.end[1] != -2.0 ||
        line.geometry_start_bit != 73u ||
        line.geometry_end_bit != 208u ||
        memcmp(record, saved, 3u))
        return 12;
    memcpy(record, saved, sizeof(saved));

    /* A nonzero reactor BL is encoded as opcode 0 + raw LE32 here.
     * Opcode 3 is RESERVED in MusaCAD's strict BL reader, not "256".
     * Rebuild this authenticated META prefix with count=1 and xdic absent.
     * The following geometry deliberately remains unsupported. */
    memcpy(record, saved, sizeof(saved));
    set_bits(record + 3u, 31u, 2u, 0u);  /* BL raw32 */
    set_bits(record + 3u, 33u, 8u, 1u);  /* count LE32 = 1 */
    set_bits(record + 3u, 41u, 24u, 0u);
    set_bits(record + 3u, 65u, 1u, 1u); /* XDictionary missing */
    MusaDwgAc1024EntityMetaPrefix reactor_meta = {0};
    if (!seal(record) ||
        !musa_dwg_ac1024_record_entity_meta_prefix(
            record, sizeof(saved), 0u, &reactor_meta) ||
        reactor_meta.reactor_count != 1u ||
        reactor_meta.entity_mode != 0u ||
        reactor_meta.xdictionary_missing != 1u ||
        musa_dwg_ac1024_line_common_probe(
            record, sizeof(saved), 0u, &common) ||
        musa_dwg_ac1024_line_fields_probe(
            record, sizeof(saved), 0u, &line) ||
        memcmp(&old_common, &common, sizeof(common)) ||
        memcmp(&old_line, &line, sizeof(line)))
        return 16;
    /* DATA bit stream is identical for valid R2010 mode 1 (paper space)
     * and mode 2 (model space). No explicit owner H is serialized in those
     * modes. Decode only coordinates: no layer, ownership or scene claim. */
    for (unsigned mode = 1u; mode <= 2u; ++mode) {
        memcpy(record, saved, sizeof(saved));
        set_bits(record + 3u, 29u, 2u, mode);
        uint8_t unchanged[sizeof(saved)];
        MusaDwgAc1024EntityMetaPrefix implicit_meta = {0};
        if (!seal(record)) return 17;
        memcpy(unchanged, record, sizeof(unchanged));
        if (!musa_dwg_ac1024_record_entity_meta_prefix(
                record, sizeof(saved), 0u, &implicit_meta) ||
            implicit_meta.entity_mode != mode ||
            implicit_meta.embedded_absolute_handle != 44u ||
            !musa_dwg_ac1024_line_common_probe(
                record, sizeof(saved), 0u, &common) ||
            !musa_dwg_ac1024_line_fields_probe(
                record, sizeof(saved), 0u, &line) ||
            !common.record_crc_verified ||
            line.indexed_handle != 44u ||
            line.geometry_start_bit != 73u ||
            line.geometry_end_bit != 208u ||
            line.start[0] != 1.5 || line.end[0] != 1.5 ||
            line.start[1] != -2.0 || line.end[1] != -2.0 ||
            line.start[2] != 0.0 || line.end[2] != 0.0 ||
            memcmp(unchanged, record, sizeof(unchanged)))
            return 18 + (int)mode;
    }
    /* The reserved mode 3 must remain refused even with a valid CRC. */
    memcpy(record, saved, sizeof(saved));
    set_bits(record + 3u, 29u, 2u, 3u);
    MusaDwgAc1024EntityMetaPrefix reserved = {0};
    if (!seal(record)) return 21;
    if (musa_dwg_ac1024_record_entity_meta_prefix(
            record, sizeof(saved), 0u, &reserved) ||
        musa_dwg_ac1024_line_common_probe(
            record, sizeof(saved), 0u, &common) ||
        musa_dwg_ac1024_line_fields_probe(
            record, sizeof(saved), 0u, &line))
        return 22;
    memcpy(record, saved, sizeof(saved));

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

    if (!test_authenticated_line_graph()) return 23;
    if (!test_implicit_line_layer_graph()) return 24;
    if (!test_xdictionary_original_handle_graph()) return 25;
    if (!test_eed_line_xyz_provisional()) return 26;
    if (!test_r2010_tagged_truecolor()) return 31;
    if (!test_eed_line_original_graph()) return 32;
    puts("AC1024 LINE common/XYZ/owner/layer synthetic checks: pass; real-DWG parity unproven");
    return 0;
}
