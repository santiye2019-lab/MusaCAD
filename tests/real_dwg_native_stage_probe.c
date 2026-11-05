/*
 * MusaCAD first-party, read-only DWG native decoder-stage probe.
 *
 * Reports only verifiable container / handle-index gates for supported
 * versions. Never claims geometry, ownership, color, visibility or DWG edit
 * correctness and never changes the original input.
 */
#include "musacad_dwg_signature.h"
#include "musacad_dwg_ac1015.h"
#include "musacad_dwg_ac1018_file_index.h"
#include "musacad_dwg_ac1018_page_map.h"
#include "musacad_dwg_ac1018_sections.h"
#include "musacad_dwg_ac1018_scene.h"
#include "musacad_dwg_ac1018_handles.h"
#include "musacad_crc16.h"
#include "musacad_dwg_ac1015_values.h"
#include "musacad_dwg_ac1024_record.h"
#include "musacad_dwg_ac1024_self_index.h"
#include "musacad_dwg_ac1024_line.h"
#include "musacad_dwg_ac1015_handle_refs.h"
#include "musacad_dwg_ac1032_page_map.h"
#include "musacad_dwg_ac1032_handles.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_REAL_DWG_BYTES (256u * 1024u * 1024u)

static int load_source(const char *path, uint8_t **out, size_t *out_bytes) {
    FILE *stream = fopen(path, "rb");
    if (!stream) return 0;
    if (fseek(stream, 0, SEEK_END) != 0) {
        fclose(stream); return 0;
    }
    long length = ftell(stream);
    if (length < 6 || (unsigned long)length > MAX_REAL_DWG_BYTES ||
        fseek(stream, 0, SEEK_SET) != 0) {
        fclose(stream); return 0;
    }
    uint8_t *data = (uint8_t *)malloc((size_t)length);
    if (!data) {
        fclose(stream); return 0;
    }
    const size_t n = fread(data, 1u, (size_t)length, stream);
    const int closed = fclose(stream);
    if (n != (size_t)length || closed != 0) {
        free(data); return 0;
    }
    *out = data; *out_bytes = (size_t)length;
    return 1;
}

static const char *signature_name(MusaCoreDwgSignature format) {
    switch (format) {
    case MUSA_DWG_R12: return "AC1009";
    case MUSA_DWG_R13: return "AC1012";
    case MUSA_DWG_R14: return "AC1014";
    case MUSA_DWG_2000: return "AC1015";
    case MUSA_DWG_2004: return "AC1018";
    case MUSA_DWG_2007: return "AC1021";
    case MUSA_DWG_2010: return "AC1024";
    case MUSA_DWG_2013: return "AC1027";
    case MUSA_DWG_2018_2026: return "AC1032";
    default: return "unknown";
    }
}

/*
 * AC1024/R2010 is not yet supported by the original MusaCAD decoder.
 * On a disposable, in-memory copy of the source, explore whether the
 * documented older R2004-family container *shape* matches existing strict
 * native page-map checks. This is NOT a decoder and NEVER accepted evidence.
 * The 6 signature bytes are restored before the caller continues.
 */
/* Diagnostic-only R2004-family MS envelope/CRC: NO type or geometry
   decoding. Offset belongs to an independently CRC-checked handle map.
   Return false for malformed ranges instead of reading out of bounds. */
static int ac1024_compare_offsets_u64(const void *left, const void *right) {
    const uint64_t a = *(const uint64_t *)left;
    const uint64_t b = *(const uint64_t *)right;
    return (a > b) - (a < b);
}

/* These statuses are only evidence about a borrowed R2004-style
   record frame at a candidate offset, NOT supported AC1024 geometry. */
enum Ac1024FrameStatus {
    AC1024_FRAME_TOO_SHORT = 1,
    AC1024_FRAME_INVALID_MS = 2,
    AC1024_FRAME_EXCEEDS_SECTION = 3,
    AC1024_FRAME_CRC_MISMATCH = 4,
    AC1024_FRAME_CRC_OK = 5
};

static enum Ac1024FrameStatus ac1024_candidate_frame_status(
    const uint8_t *objects, size_t object_bytes, uint64_t offset,
    size_t *out_frame_span) {
    if (!objects || offset >= (uint64_t)object_bytes)
        return AC1024_FRAME_TOO_SHORT;
    const size_t at = (size_t)offset;
    const size_t remaining = object_bytes - at;
    if (remaining < 6u)
        return AC1024_FRAME_TOO_SHORT;
    const uint8_t *p = objects + at;
    const uint16_t first = (uint16_t)p[0] | ((uint16_t)p[1] << 8u);
    size_t prefix = 2u, payload = (size_t)first;
    if ((first & 0x8000u) != 0u) {
        const uint16_t hi = (uint16_t)p[2] | ((uint16_t)p[3] << 8u);
        if (!hi || (hi & 0x8000u))
            return AC1024_FRAME_INVALID_MS;
        prefix = 4u;
        payload = (size_t)(first & 0x7fffu) | ((size_t)hi << 15u);
    }
    if (payload < 8u)
        return AC1024_FRAME_INVALID_MS;
    if (prefix > remaining - 2u || payload > remaining - prefix - 2u)
        return AC1024_FRAME_EXCEEDS_SECTION;
    if (out_frame_span)
        *out_frame_span = prefix + payload + 2u;
    uint16_t computed = 0u;
    if (!musa_core_crc16_arc(
          0xc0c1u, p, prefix + payload, &computed))
        return AC1024_FRAME_CRC_MISMATCH;
    const size_t at_crc = prefix + payload;
    const uint16_t stored = (uint16_t)p[at_crc] |
                            ((uint16_t)p[at_crc + 1u] << 8u);
    return computed == stored ? AC1024_FRAME_CRC_OK :
                                AC1024_FRAME_CRC_MISMATCH;
}

static int ac1024_candidate_frame_crc(
    const uint8_t *objects, size_t object_bytes, uint64_t offset) {
    return ac1024_candidate_frame_status(
        objects, object_bytes, offset, NULL) == AC1024_FRAME_CRC_OK;
}

/* Read-only R2010 hypothesis from the published record framing:
   an unsigned MC handle-stream bit-length marker follows the MS size.
   Evaluate whether the *encoded MC byte count* accounts for the
   observed gap, then CRC-check the corresponding expanded frame.
   This does NOT decode object type, self handle, layer, owner or geometry.
   Return 0 for unreadable framing; return 1 with explicit CRC outcome.
   No input bytes are modified, and outputs are set only on full framing. */
static int ac1024_r2010_mc_frame(
    const uint8_t *objects, size_t object_bytes, uint64_t offset,
    size_t *out_span, size_t *out_mc_bytes, int *out_crc_ok) {
    if (!objects || !out_span || !out_mc_bytes || !out_crc_ok ||
        offset >= (uint64_t)object_bytes) return 0;
    const size_t at = (size_t)offset;
    const size_t remaining = object_bytes - at;
    if (remaining < 6u) return 0;
    const uint8_t *p = objects + at;
    const uint16_t first = (uint16_t)p[0] |
                           ((uint16_t)p[1] << 8u);
    size_t ms_prefix = 2u;
    size_t payload = (size_t)first;
    if (first & 0x8000u) {
        if (remaining < 8u) return 0;
        const uint16_t hi = (uint16_t)p[2] |
                            ((uint16_t)p[3] << 8u);
        if (!hi || (hi & 0x8000u)) return 0;
        ms_prefix = 4u;
        payload = (size_t)(first & 0x7fffu) |
                  ((size_t)hi << 15u);
    }
    /* Do not borrow AC1018's arbitrary 8-byte minimum: the actual
       AC1024 CRC and OT must validate independently for short records. */
    if (payload < 2u || ms_prefix > remaining - 2u ||
        payload > remaining - ms_prefix - 2u) return 0;
    const size_t old_span = ms_prefix + payload + 2u;
    size_t mc_len = 0u;
    int terminated = 0;
    for (; mc_len < 10u; mc_len++) {
        if (mc_len >= remaining - ms_prefix) return 0;
        if ((p[ms_prefix + mc_len] & 0x80u) == 0u) {
            ++mc_len;
            terminated = 1;
            break;
        }
    }
    if (!terminated || !mc_len || mc_len > 10u || mc_len >= remaining ||
        old_span > remaining - mc_len) return 0;
    const size_t new_span = old_span + mc_len;
    const size_t crc_at = new_span - 2u;
    uint16_t computed = 0u;
    if (!musa_core_crc16_arc(0xc0c1u, p, crc_at, &computed))
        return 0;
    const uint16_t stored = (uint16_t)p[crc_at] |
                            ((uint16_t)p[crc_at + 1u] << 8u);
    *out_span = new_span;
    *out_mc_bytes = mc_len;
    *out_crc_ok = computed == stored;
    return 1;
}

/* AC1024/R2010 OT declaration from the authenticated DATA stream:
   bit-pair selector; 0 -> raw byte; 1 -> byte + 0x1f0;
   2/3 -> two little-endian raw bytes. The caller MUST first verify
   record-frame CRC. This is a raw type DECLARATION only, not entity
   geometry, CLASSES, owner/self handle or renderer acceptance. */
static int ac1024_r2010_declared_ot(
    const uint8_t *objects, size_t object_bytes, uint64_t offset,
    size_t mc_bytes, uint16_t *out_type) {
    if (!objects || !out_type || !mc_bytes || mc_bytes > 10u ||
        offset >= (uint64_t)object_bytes) return 0;
    const size_t at = (size_t)offset;
    const size_t left = object_bytes - at;
    if (left < 6u) return 0;
    const uint8_t *p = objects + at;
    const uint16_t first = (uint16_t)p[0] |
                           ((uint16_t)p[1] << 8u);
    size_t ms_prefix = 2u;
    size_t payload = (size_t)first;
    if (first & 0x8000u) {
        if (left < 6u) return 0;
        const uint16_t hi = (uint16_t)p[2] |
                            ((uint16_t)p[3] << 8u);
        if (!hi || (hi & 0x8000u)) return 0;
        ms_prefix = 4u;
        payload = (size_t)(first & 0x7fffu) | ((size_t)hi << 15u);
    }
    if (payload < 2u || ms_prefix > left ||
        mc_bytes > left - ms_prefix ||
        payload > left - ms_prefix - mc_bytes ||
        payload > SIZE_MAX / 8u) return 0;
    MusaDwgAc1015Bits bits = {0};
    if (!musa_dwg_ac1015_bits_init(
            &bits, p + ms_prefix + mc_bytes, payload, 0u,
            payload * 8u)) return 0;
    uint32_t selector = 0u, low = 0u, high = 0u;
    if (!musa_dwg_ac1015_bits_read_u(&bits, 2u, &selector) ||
        !musa_dwg_ac1015_bits_read_u(&bits, 8u, &low)) return 0;
    uint32_t raw = low;
    if (selector == 1u) raw += 0x1f0u;
    else if (selector >= 2u) {
        if (!musa_dwg_ac1015_bits_read_u(&bits, 8u, &high))
            return 0;
        raw |= high << 8u;
    }
    if (raw > UINT16_MAX) return 0;
    *out_type = (uint16_t)raw;
    return 1;
}

/* TEST-ONLY independent hypothesis: a handle-map MC final byte might
   reserve bit 0x40 rather than treating it as a magnitude bit.
   The original immutable handle-map pages are CRC checked again.
   This candidate must NEVER be used to accept a production index until
   independently verified against embedded self-H across the real corpus. */
static int ac1024_hmap_last_six_candidate(
    const uint8_t *section, size_t section_bytes,
    uint64_t *handles, size_t capacity, size_t *out_count) {
    if (!section || !handles || !out_count || !capacity)
        return 0;
    size_t cursor = 0u, count = 0u;
    uint64_t current = 0u;
    while (cursor < section_bytes) {
        if (section_bytes - cursor < 4u) return 0;
        const size_t start = cursor;
        const size_t page_bytes = ((size_t)section[start] << 8u) |
                                  section[start + 1u];
        if (page_bytes < 2u || page_bytes > 2040u ||
            page_bytes > section_bytes - start - 2u) return 0;
        const size_t end = start + page_bytes;
        const uint16_t stored = (uint16_t)(
            ((uint16_t)section[end] << 8u) | section[end + 1u]);
        uint16_t computed = 0u;
        if (!musa_core_crc16_arc(
                0xc0c1u, section + start, page_bytes, &computed) ||
            stored != computed) return 0;
        cursor = start + 2u;
        if (page_bytes == 2u) {
            if (end + 2u != section_bytes || !count) return 0;
            *out_count = count;
            return 1;
        }
        while (cursor < end) {
            uint64_t delta = 0u;
            int ended = 0;
            for (unsigned j = 0u; j < 10u; j++) {
                if (cursor >= end) return 0;
                const uint8_t b = section[cursor++];
                const int last = !(b & 0x80u);
                const uint64_t value = (uint64_t)(
                    b & (last ? 0x3fu : 0x7fu));
                if (j * 7u >= 64u ||
                    value > (UINT64_MAX >> (j * 7u))) return 0;
                delta |= value << (j * 7u);
                if (last) { ended = 1; break; }
            }
            if (!ended || !delta || delta > UINT64_MAX - current ||
                count >= capacity) return 0;
            /* A separate signed MC location delta follows. Only its
               bounded byte extent is needed for the handle hypothesis. */
            int loc_ended = 0;
            for (unsigned j = 0u; j < 10u; j++) {
                if (cursor >= end) return 0;
                const uint8_t b = section[cursor++];
                if (!(b & 0x80u)) { loc_ended = 1; break; }
            }
            if (!loc_ended) return 0;
            current += delta;
            handles[count++] = current;
        }
        if (cursor != end) return 0;
        cursor = end + 2u;
    }
    return 0;
}

typedef struct MusaAc1024CompatibilityHint {
    int header, physical_map, object_pages, handle_pages, handle_index;
    size_t candidate_handles;
    size_t object_section_page_count, handle_section_page_count;
    int legacy_frame_probe_completed;
    size_t legacy_frame_crc_ok, legacy_frame_crc_failed;
    size_t raw_type_17_arc, raw_type_18_circle;
    size_t raw_type_19_line, raw_type_77_lwpolyline;
    int continuous_offset_candidate_index;
    size_t continuous_candidate_handles;
    size_t continuous_frame_crc_ok, continuous_frame_crc_failed;
    int page_absolute_candidate_index;
    size_t page_absolute_candidate_handles;
    size_t page_absolute_frame_crc_ok, page_absolute_frame_crc_failed;
    int legacy_offset_distribution_measured;
    size_t legacy_distinct_offsets, legacy_duplicate_offset_entries;
    size_t legacy_zero_offset_entries, legacy_max_offset_multiplicity;
    size_t legacy_distinct_frame_crc_pass, legacy_repeated_crc_pass_entries;
    size_t legacy_frame_too_short, legacy_frame_invalid_ms;
    size_t legacy_frame_exceeds_section, legacy_frame_crc_mismatch;
    size_t legacy_shift_minus_two_crc_pass, legacy_shift_minus_one_crc_pass;
    size_t legacy_shift_plus_one_crc_pass, legacy_shift_plus_two_crc_pass;
    size_t legacy_shift_samples_tested;
    size_t legacy_next_offset_pairs, legacy_next_frame_unavailable;
    size_t legacy_next_frame_exact, legacy_next_frame_short;
    size_t legacy_next_frame_overlaps, legacy_crc_ok_next_frame_exact;
    size_t legacy_short_gap_1, legacy_short_gap_2, legacy_short_gap_3;
    size_t legacy_short_gap_4, legacy_short_gap_5to16;
    size_t legacy_short_gap_17to64, legacy_short_gap_over64;
    int mc_frame_probe_completed;
    size_t mc_frame_crc_pass, mc_frame_crc_mismatch, mc_frame_unreadable;
    size_t mc_frame_next_exact, mc_byte_len_matches_short_gap;
    size_t mc_bytes_1, mc_bytes_2, mc_bytes_3, mc_bytes_over3;
    size_t mc_ot_decoded, mc_ot_unreadable;
    size_t mc_ot_raw_1, mc_ot_raw_7, mc_ot_raw_17, mc_ot_raw_18;
    size_t mc_ot_raw_19, mc_ot_raw_44, mc_ot_raw_49;
    size_t mc_ot_raw_51, mc_ot_raw_77;
    size_t core_frame_verified, core_frame_probe_failed;
    size_t core_raw_ot_or_span_mismatch;
    size_t core_payload_bounds_verified, core_payload_bounds_rejected;
    size_t core_payload_bounds_mismatch;
    size_t mc_unreadable_invalid_ms, mc_unreadable_short_or_boundary;
    size_t mc_unreadable_other, mc_unreadable_first_half;
    size_t mc_invalid_ms_zero, mc_invalid_ms_one;
    size_t mc_invalid_ms_2to7, mc_invalid_ms_extended_zero;
    size_t mc_invalid_ms_extended_reserved, mc_invalid_ms_unclassified;
    size_t core_stream_bounds_verified, core_stream_bounds_rejected;
    size_t core_stream_bounds_mismatch, core_empty_handle_streams;
    size_t sampled_first_h_attempts, sampled_first_h_self_matches;
    size_t sampled_first_h_other, sampled_first_h_unreadable;
    size_t sampled_first_h_code0_mismatch;
    size_t sampled_first_h_absolute_other_code;
    size_t sampled_first_h_relative_code;
    size_t sampled_first_h_reverse_bytes_matches;
    size_t sampled_first_h_plus_one_matches;
    size_t sampled_first_h_minus_one_matches;
    size_t sampled_first_h_one_byte_values;
    size_t sampled_first_h_in_handle_index;
    size_t sampled_first_h_not_in_handle_index;
    size_t sampled_first_h_same_index_offset;
    size_t sampled_first_h_count2, sampled_first_h_count3;
    size_t sampled_first_h_count4to8;
    size_t sampled_scan_best_shift_bits;
    size_t sampled_scan_best_self_matches;
    size_t sampled_h_internally_distinct, sampled_h_internal_duplicates;
    size_t sampled_h_successive_pairs, sampled_h_successive_increases;
    size_t sampled_h_successive_decreases, sampled_h_successive_equals;
    size_t sampled_h_less_than_index, sampled_h_greater_than_index;
    size_t sampled_h_index_requires_over3_bytes;
    size_t sampled_h_low8_equal_index, sampled_h_low16_equal_index;
    int masked6_candidate_index_valid;
    size_t masked6_candidate_handle_count;
    size_t masked6_sample_attempts, masked6_sample_self_matches;
    size_t masked6_sample_other, masked6_sample_unreadable;
    int full_embedded_h_census_completed;
    size_t full_embedded_h_attempts, full_embedded_h_parsed_code0;
    size_t full_embedded_h_other_codes, full_embedded_h_unreadable;
    size_t full_embedded_h_self_matches, full_embedded_h_less_than_index;
    size_t full_embedded_h_greater_than_index;
    size_t full_embedded_h_neighbor_pairs, full_embedded_h_increases;
    size_t full_embedded_h_decreases, full_embedded_h_equals;
    int core_embedded_h_recovery_attempted, core_embedded_h_recovery_succeeded;
    size_t core_embedded_h_recovery_records;
    size_t core_embedded_h_recovery_original_index_matches;
    size_t sampled_zero_eed_attempts, sampled_zero_eed_verified;
    size_t sampled_zero_eed_refused;
    size_t sampled_no_proxy_verified, sampled_no_proxy_refused;
    size_t sampled_entity_meta_attempts, sampled_entity_meta_verified;
    size_t sampled_entity_meta_refused;
    size_t sampled_owner_layer_attempts, sampled_owner_layer_verified;
    size_t sampled_owner_layer_refused;
    /* CRC/self-index verified raw OT19 samples, no pixel or entity parity. */
    size_t sampled_line_attempts;
    size_t sampled_line_common_verified, sampled_line_common_refused;
    size_t sampled_line_xyz_verified, sampled_line_xyz_refused;
    /* Source-internal joint witnesses only; independent geometric parity
     * remains NOT_MEASURABLE. Counts contain no raw drawing fields. */
    size_t sampled_line_xyz_owner_layer_joined;
    size_t sampled_line_xyz_distinct_endpoints;
    size_t sampled_line_xyz_bounded_common;
    /* Disjoint refusal stages for bounded raw OT19 samples; zero drawing data. */
    size_t sampled_line_meta_refused;
    /* First failed independently CRC-bounded metadata prefix stage. */
    size_t sampled_line_meta_self_refused;
    size_t sampled_line_meta_eed_refused;
    size_t sampled_line_meta_proxy_refused;
    size_t sampled_line_meta_entity_fields_refused;
    /* Standalone bounded EED-chain re-probe; still NO geometry admission. */
    size_t sampled_line_eed_chain_attempts;
    size_t sampled_line_eed_chain_verified;
    size_t sampled_line_eed_chain_refused;
    size_t sampled_line_eed_chain_has_applications;
    /* Additional provisional LINE XYZ after authenticated nonzero EED.
     * These are NOT added to the legacy geometry/scene claims. */
    size_t sampled_line_eed_xyz_attempts;
    size_t sampled_line_eed_xyz_verified;
    size_t sampled_line_eed_xyz_refused;
    size_t sampled_line_eed_xyz_distinct_endpoints;
    /* Original EED LINE coords joined with CRC/OT49/OT51/optional OT42.
     * No inherited layer visibility, EED APPID semantics or scene claim. */
    size_t sampled_line_eed_join_attempts;
    size_t sampled_line_eed_join_verified;
    size_t sampled_line_eed_join_refused;
    size_t sampled_line_eed_join_explicit_owner;
    size_t sampled_line_eed_join_implicit_layer;
    size_t sampled_line_eed_join_xdictionary;
    /* Independent CRC/authenticated APPID OT67 target references from
     * the original EED frames; never an application-semantics claim. */
    size_t sampled_line_eed_appid_attempts;
    size_t sampled_line_eed_appid_verified;
    size_t sampled_line_eed_appid_refused;
    size_t sampled_line_eed_appid_references_verified;
    size_t sampled_line_eed_appid_distinct_references_verified;
    /* Original OT51 raw numeric layer flags, NEVER actual visibility. */
    size_t sampled_layer_raw_flags_attempts;
    size_t sampled_layer_raw_flags_verified;
    size_t sampled_layer_raw_flags_refused;
    size_t sampled_layer_flags_refused_index;
    size_t sampled_layer_flags_refused_eed;
    size_t sampled_layer_flags_refused_record;
    size_t sampled_layer_flags_refused_reactors;
    size_t sampled_layer_flags_refused_xdic_bit;
    size_t sampled_layer_flags_refused_numeric;
    size_t sampled_layer_flags_refused_other;
    size_t sampled_layer_raw_flags_frozen_bit;
    size_t sampled_layer_raw_flags_on_bit;
    size_t sampled_layer_raw_flags_locked_bit;
    size_t sampled_layer_raw_flags_unmapped_bits;
    size_t sampled_layer_raw_flags_xdictionary_present;
    /* Separate NONZERO-EED OT51 DATA-only preflight: layer numeric flags
     * remain raw evidence, never a verified visibility or CMC color. */
    size_t sampled_layer_eed_flags_attempts;
    size_t sampled_layer_eed_flags_verified;
    size_t sampled_layer_eed_flags_refused;
    size_t sampled_layer_eed_flags_frozen_bit;
    size_t sampled_layer_eed_flags_on_bit;
    size_t sampled_layer_eed_flags_locked_bit;
    size_t sampled_layer_eed_flags_unmapped_bits;
    size_t sampled_layer_eed_flags_xdictionary_present;
    /* Original nonzero-EED OT51 APPID H5 -> CRC-backed OT67 targets,
     * independent from layer numeric flags, CMC and drawing readiness. */
    size_t sampled_layer_eed_appid_attempts;
    size_t sampled_layer_eed_appid_verified;
    size_t sampled_layer_eed_appid_refused;
    size_t sampled_layer_eed_appid_refs_verified;
    /* Strict original OT50/H2 + OT42/H3 H-stream graph. Unparsed
     * trailing layer material, plotstyle, linetype and viewport refs
     * remain outside all display acceptance and visibility. */
    size_t sampled_layer_eed_xdic_graph_attempts;
    size_t sampled_layer_eed_xdic_graph_verified;
    size_t sampled_layer_eed_xdic_graph_refused;
    size_t sampled_layer_eed_xdic_refused_identity;
    size_t sampled_layer_eed_xdic_refused_eed;
    size_t sampled_layer_eed_xdic_refused_flags;
    size_t sampled_layer_eed_xdic_refused_appid;
    size_t sampled_layer_eed_xdic_refused_tail;
    size_t sampled_layer_eed_xdic_refused_control_h;
    size_t sampled_layer_eed_xdic_refused_dictionary_h;
    size_t sampled_layer_eed_xdic_refused_target_index;
    size_t sampled_layer_eed_xdic_refused_target_class;
    size_t sampled_layer_eed_xdic_refused_target_crc;
    size_t sampled_layer_eed_xdic_refused_other;
    size_t sampled_layer_eed_xdic_graph_has_tail;
    size_t sampled_line_common_after_meta_refused;
    size_t sampled_line_xyz_after_common_refused;
    /* Precedence-classified subset of common refusals AFTER meta admission. */
    size_t sampled_line_common_mode_unsupported;
    size_t sampled_line_common_reactors_unsupported;
    size_t sampled_line_common_xdic_unsupported;
    size_t sampled_line_common_fields_refused;
    /* Exact stage of a strict, re-run shared-core LINE common decoder.
     * Metadata-level and raw geometry acceptance remain separate. */
    size_t sampled_line_common_stage_color;
    size_t sampled_line_common_stage_rgb;
    size_t sampled_line_common_stage_transparency;
    size_t sampled_line_common_stage_ltype;
    /* Exact strict BD scale diagnosis; still NOT accepted for drawing. */
    size_t sampled_line_ltype_zero;
    size_t sampled_line_ltype_negative;
    size_t sampled_line_ltype_nonfinite;
    size_t sampled_line_ltype_parse_refused;
    size_t sampled_line_common_stage_appearance;
    size_t sampled_line_common_stage_geom_bound;
    size_t sampled_line_common_stage_other;
    /* Provisional DATA-only LINE XYZ in implicit layout space, never owner join. */
    size_t sampled_line_implicit_paper_xyz_verified;
    size_t sampled_line_implicit_model_xyz_verified;
    /* Same original-index graph+geometry API: no owner fabrication. */
    size_t sampled_line_graph_attempts;
    size_t sampled_line_graph_verified, sampled_line_graph_refused;
    /* Original implicit LINE layer-only graph; no owner/layout assertion. */
    size_t sampled_line_implicit_layer_attempts;
    size_t sampled_line_implicit_layer_verified;
    size_t sampled_line_implicit_layer_refused;
    /* XDictionary flag 0: DATA-only LINE XYZ; extra H unvalidated. */
    size_t sampled_line_xdic_present_attempts;
    size_t sampled_line_xdic_present_xyz_verified;
    size_t sampled_line_xdic_present_xyz_refused;
    /* Authenticated original OT42 / OT51 H graph; no scene or style. */
    size_t sampled_line_xdic_hgraph_attempts;
    size_t sampled_line_xdic_hgraph_verified;
    size_t sampled_line_xdic_hgraph_refused;
} MusaAc1024CompatibilityHint;

static MusaAc1024CompatibilityHint ac1024_family_hint(
    uint8_t *scratch, size_t length) {
    MusaAc1024CompatibilityHint hint = {0};
    if (!scratch || length < 256u || memcmp(scratch, "AC1024", 6u) != 0) {
        return hint;
    }
    /* Re-use existing bounded structural validators only; never copy
       LibreDWG source or include GPL reference-tool headers here. */
    memcpy(scratch, "AC1018", 6u);
    MusaDwgAc1018FileMapHeader header = {0};
    MusaDwgAc1018SectionDescriptor desc = {0};
    size_t locations = 0u, object_pages = 0u, handle_pages = 0u;
    hint.header = musa_dwg_ac1018_page_map_header(scratch, length, &header);
    if (hint.header) {
        hint.physical_map = musa_dwg_ac1018_page_locations(
            scratch, length, NULL, 0u, &locations) && locations > 0u;
    }
    if (hint.physical_map) {
        hint.object_pages = musa_dwg_ac1018_section_pages(
            scratch, length, MUSA_DWG_AC1018_OBJECTS,
            NULL, 0u, &desc, &object_pages) && object_pages > 0u;
    }
    if (hint.object_pages) {
        hint.handle_pages = musa_dwg_ac1018_section_pages(
            scratch, length, MUSA_DWG_AC1018_HANDLES,
            NULL, 0u, &desc, &handle_pages) && handle_pages > 0u;
    }
    if (hint.object_pages && hint.handle_pages) {
        hint.object_section_page_count = object_pages;
        hint.handle_section_page_count = handle_pages;
    }
    if (hint.handle_pages) {
        size_t handles = 0u;
        hint.handle_index = musa_dwg_ac1018_file_handle_index(
            scratch, length, NULL, 0u, &handles) && handles > 0u;
        if (hint.handle_index) hint.candidate_handles = handles;
    }
    /* Separate bounded experiment: try the existing R2004 CRC-framed
       record vocabulary against the authenticated candidate index.
       A numeric BS type is NEVER a decoded AC1024 entity/class/owner.
       This is diagnostic and not a normal-open or rendering path. */
    if (hint.handle_index && hint.candidate_handles <= 400000u &&
        length <= 12u * 1024u * 1024u) {
        MusaDwgAc1018Scene *scene = NULL;
        if (musa_dwg_ac1018_scene_open(scratch, length, &scene) &&
            scene && musa_dwg_ac1018_scene_handle_count(scene) ==
                     hint.candidate_handles) {
            hint.legacy_frame_probe_completed = 1;
            for (size_t i = 0u; i < hint.candidate_handles; i++) {
                MusaDwgAc1018RecordProbe record = {0};
                if (!musa_dwg_ac1018_scene_probe_record_at(scene, i, &record)
                    || !record.record_crc_verified) {
                    hint.legacy_frame_crc_failed++;
                    continue;
                }
                hint.legacy_frame_crc_ok++;
                switch (record.declared_type) {
                case 17u: hint.raw_type_17_arc++; break;
                case 18u: hint.raw_type_18_circle++; break;
                case 19u: hint.raw_type_19_line++; break;
                case 77u: hint.raw_type_77_lwpolyline++; break;
                default: break;
                }
            }
        }
        musa_dwg_ac1018_scene_close(scene);
    }
    /* The 2010 handle map may use continuously advancing logical
       AcDbObjects offsets instead of the legacy R2004 per-map-page
       reset. Compare this ALTERNATE CRC-checked map in isolation.
       Do not choose either profile for the actual application yet. */
    if (hint.handle_index && length <= 12u * 1024u * 1024u &&
        hint.candidate_handles <= 400000u) {
        MusaDwgAc1018SectionDescriptor obj_info = {0}, h_info = {0};
        size_t obj_pages2 = 0u, h_pages2 = 0u;
        if (musa_dwg_ac1018_section_pages(
                scratch, length, MUSA_DWG_AC1018_OBJECTS,
                NULL, 0u, &obj_info, &obj_pages2) &&
            musa_dwg_ac1018_section_pages(
                scratch, length, MUSA_DWG_AC1018_HANDLES,
                NULL, 0u, &h_info, &h_pages2) &&
            obj_pages2 && h_pages2 && obj_info.logical_bytes > 0u &&
            h_info.logical_bytes > 0u &&
            obj_info.logical_bytes <= 96u * 1024u * 1024u &&
            h_info.logical_bytes <= 16u * 1024u * 1024u) {
            const size_t nb = (size_t)obj_info.logical_bytes;
            const size_t hb = (size_t)h_info.logical_bytes;
            uint8_t *objects = (uint8_t *)malloc(nb);
            uint8_t *handles = (uint8_t *)malloc(hb);
            size_t got_obj = 0u, got_h = 0u, candidate_count = 0u;
            if (objects && handles &&
                musa_dwg_ac1018_extract_section(
                    scratch, length, MUSA_DWG_AC1018_OBJECTS,
                    objects, nb, &got_obj) && got_obj == nb &&
                musa_dwg_ac1018_extract_section(
                    scratch, length, MUSA_DWG_AC1018_HANDLES,
                    handles, hb, &got_h) && got_h == hb &&
                musa_dwg_ac1018_handles_index_continuous_offsets(
                    handles, hb, nb, NULL, 0u, &candidate_count) &&
                candidate_count == hint.candidate_handles &&
                candidate_count <= SIZE_MAX / sizeof(MusaDwgAc1018HandleEntry)) {
                MusaDwgAc1018HandleEntry *entries =
                    (MusaDwgAc1018HandleEntry *)malloc(
                        candidate_count * sizeof(*entries));
                size_t actual = 0u;
                if (entries &&
                    musa_dwg_ac1018_handles_index_continuous_offsets(
                        handles, hb, nb, entries, candidate_count, &actual) &&
                    actual == candidate_count) {
                    hint.continuous_offset_candidate_index = 1;
                    hint.continuous_candidate_handles = actual;
                    for (size_t i = 0u; i < actual; i++) {
                        if (ac1024_candidate_frame_crc(
                              objects, nb, entries[i].object_offset))
                            hint.continuous_frame_crc_ok++;
                        else hint.continuous_frame_crc_failed++;
                    }
                }
                free(entries);
            }
            /* Count repeated logical object offsets in the original
               CRC-backed R2004 handle map. No handle IDs, offsets, raw
               drawing fields, coordinates or user text are emitted.
               An index with many collisions is NOT a proven object map. */
            size_t legacy_count = 0u;
            if (objects && handles && got_obj == nb && got_h == hb &&
                musa_dwg_ac1018_handles_index(
                    handles, hb, nb, NULL, 0u, &legacy_count) &&
                legacy_count == hint.candidate_handles &&
                legacy_count <= SIZE_MAX / sizeof(MusaDwgAc1018HandleEntry) &&
                legacy_count <= SIZE_MAX / sizeof(uint64_t)) {
                MusaDwgAc1018HandleEntry *legacy =
                    (MusaDwgAc1018HandleEntry *)malloc(
                        legacy_count * sizeof(*legacy));
                uint64_t *offsets =
                    (uint64_t *)malloc(legacy_count * sizeof(*offsets));
                size_t actual_legacy = 0u;
                if (legacy && offsets &&
                    musa_dwg_ac1018_handles_index(
                        handles, hb, nb, legacy, legacy_count,
                        &actual_legacy) && actual_legacy == legacy_count) {
                    for (size_t k = 0u; k < legacy_count; k++)
                        offsets[k] = legacy[k].object_offset;
                    qsort(offsets, legacy_count, sizeof(*offsets),
                          ac1024_compare_offsets_u64);
                    hint.legacy_offset_distribution_measured = 1;
                    hint.mc_frame_probe_completed = 1;
                    size_t start = 0u;
                    while (start < legacy_count) {
                        size_t end = start + 1u;
                        while (end < legacy_count &&
                               offsets[end] == offsets[start])
                            ++end;
                        const size_t group = end - start;
                        ++hint.legacy_distinct_offsets;
                        hint.legacy_duplicate_offset_entries += group - 1u;
                        if (offsets[start] == 0u)
                            hint.legacy_zero_offset_entries = group;
                        if (group > hint.legacy_max_offset_multiplicity)
                            hint.legacy_max_offset_multiplicity = group;
                        size_t frame_span = 0u;
                        const enum Ac1024FrameStatus status =
                            ac1024_candidate_frame_status(
                                objects, nb, offsets[start], &frame_span);
                        switch (status) {
                        case AC1024_FRAME_TOO_SHORT:
                            hint.legacy_frame_too_short += group;
                            break;
                        case AC1024_FRAME_INVALID_MS:
                            hint.legacy_frame_invalid_ms += group;
                            break;
                        case AC1024_FRAME_EXCEEDS_SECTION:
                            hint.legacy_frame_exceeds_section += group;
                            break;
                        case AC1024_FRAME_CRC_MISMATCH:
                            hint.legacy_frame_crc_mismatch += group;
                            break;
                        case AC1024_FRAME_CRC_OK:
                            ++hint.legacy_distinct_frame_crc_pass;
                            hint.legacy_repeated_crc_pass_entries += group;
                            break;
                        }
                        if (end < legacy_count) {
                            /* Adjacent offsets, not adjacent HANDLES:
                               this tests whether each old-style MS
                               length reaches the next candidate frame.
                               No object parsing, owner or spatial claims. */
                            ++hint.legacy_next_offset_pairs;
                            const uint64_t gap = offsets[end] - offsets[start];
                            if (!frame_span)
                                ++hint.legacy_next_frame_unavailable;
                            else if ((uint64_t)frame_span == gap) {
                                ++hint.legacy_next_frame_exact;
                                if (status == AC1024_FRAME_CRC_OK)
                                    ++hint.legacy_crc_ok_next_frame_exact;
                            } else if ((uint64_t)frame_span < gap) {
                                ++hint.legacy_next_frame_short;
                                const uint64_t missing = gap - (uint64_t)frame_span;
                                if (missing == 1u)
                                    ++hint.legacy_short_gap_1;
                                else if (missing == 2u)
                                    ++hint.legacy_short_gap_2;
                                else if (missing == 3u)
                                    ++hint.legacy_short_gap_3;
                                else if (missing == 4u)
                                    ++hint.legacy_short_gap_4;
                                else if (missing <= 16u)
                                    ++hint.legacy_short_gap_5to16;
                                else if (missing <= 64u)
                                    ++hint.legacy_short_gap_17to64;
                                else
                                    ++hint.legacy_short_gap_over64;
                            } else {
                                ++hint.legacy_next_frame_overlaps;
                            }
                        }
                        size_t mc_span = 0u, mc_bytes = 0u;
                        int mc_crc_ok = 0;
                        if (ac1024_r2010_mc_frame(
                              objects, nb, offsets[start],
                              &mc_span, &mc_bytes, &mc_crc_ok)) {
                            if (mc_crc_ok) {
                                hint.mc_frame_crc_pass += group;
                                uint16_t raw_ot = 0u;
                                if (ac1024_r2010_declared_ot(
                                      objects, nb, offsets[start],
                                      mc_bytes, &raw_ot)) {
                                    hint.mc_ot_decoded += group;
                                    switch (raw_ot) {
                                    case 1u: hint.mc_ot_raw_1 += group; break;
                                    case 7u: hint.mc_ot_raw_7 += group; break;
                                    case 17u: hint.mc_ot_raw_17 += group; break;
                                    case 18u: hint.mc_ot_raw_18 += group; break;
                                    case 19u: hint.mc_ot_raw_19 += group; break;
                                    case 44u: hint.mc_ot_raw_44 += group; break;
                                    case 49u: hint.mc_ot_raw_49 += group; break;
                                    case 51u: hint.mc_ot_raw_51 += group; break;
                                    case 77u: hint.mc_ot_raw_77 += group; break;
                                    default: break;
                                    }
                                } else {
                                    hint.mc_ot_unreadable += group;
                                }
                                MusaDwgAc1024RecordFrame core = {0};
                                if (!musa_dwg_ac1024_record_probe(
                                      objects, nb, offsets[start], &core)) {
                                    hint.core_frame_probe_failed += group;
                                } else {
                                    hint.core_frame_verified += group;
                                    if (core.framed_bytes != mc_span ||
                                        core.mc_marker_bytes != mc_bytes ||
                                        !core.original_crc16_verified ||
                                        !ac1024_r2010_declared_ot(
                                             objects, nb, offsets[start],
                                             mc_bytes, &raw_ot) ||
                                        core.raw_object_type != raw_ot)
                                        hint.core_raw_ot_or_span_mismatch += group;
                                    MusaDwgAc1024DataBounds bounds = {0};
                                    if (!musa_dwg_ac1024_record_data_bounds(
                                            objects, nb, offsets[start],
                                            &bounds)) {
                                        hint.core_payload_bounds_rejected += group;
                                    } else {
                                        hint.core_payload_bounds_verified += group;
                                        const size_t data_at =
                                            (size_t)offsets[start] +
                                            core.ms_marker_bytes +
                                            core.mc_marker_bytes;
                                        const size_t type_end =
                                            (objects[data_at] >> 6u) >= 2u ?
                                            18u : 10u;
                                        if (bounds.payload_start_byte != data_at ||
                                            bounds.payload_bytes != core.ms_object_bytes ||
                                            bounds.payload_end_bit !=
                                                core.ms_object_bytes * 8u ||
                                            bounds.type_end_bit != type_end ||
                                            bounds.raw_object_type !=
                                                core.raw_object_type ||
                                            !bounds.record_crc16_verified)
                                            hint.core_payload_bounds_mismatch += group;
                                    }
                                    MusaDwgAc1024StreamBounds stream = {0};
                                    if (!musa_dwg_ac1024_record_stream_bounds(
                                            objects, nb, offsets[start],
                                            &stream)) {
                                        hint.core_stream_bounds_rejected += group;
                                    } else {
                                        hint.core_stream_bounds_verified += group;
                                        if (!stream.record_crc16_verified ||
                                            stream.raw_object_type != core.raw_object_type ||
                                            stream.payload_start_byte !=
                                                bounds.payload_start_byte ||
                                            stream.data_after_ot_bit !=
                                                bounds.type_end_bit ||
                                            stream.handle_stream_end_bit !=
                                                bounds.payload_end_bit ||
                                            stream.handle_stream_start_bit <
                                                bounds.type_end_bit ||
                                            stream.handle_stream_bits !=
                                                core.handle_stream_bits ||
                                            stream.handle_stream_end_bit -
                                                stream.handle_stream_start_bit !=
                                                core.handle_stream_bits)
                                            hint.core_stream_bounds_mismatch += group;
                                        if (stream.handle_stream_bits == 0u)
                                            hint.core_empty_handle_streams += group;
                                    }
                                }
                            } else
                                hint.mc_frame_crc_mismatch += group;
                            if (mc_bytes == 1u)
                                hint.mc_bytes_1 += group;
                            else if (mc_bytes == 2u)
                                hint.mc_bytes_2 += group;
                            else if (mc_bytes == 3u)
                                hint.mc_bytes_3 += group;
                            else
                                hint.mc_bytes_over3 += group;
                            if (end < legacy_count && frame_span &&
                                offsets[end] - offsets[start] >
                                    (uint64_t)frame_span &&
                                (uint64_t)mc_bytes ==
                                    offsets[end] - offsets[start] -
                                    (uint64_t)frame_span)
                                ++hint.mc_byte_len_matches_short_gap;
                            if (end < legacy_count && mc_span &&
                                (uint64_t)mc_span ==
                                    offsets[end] - offsets[start])
                                ++hint.mc_frame_next_exact;
                        } else {
                            hint.mc_frame_unreadable += group;
                            if (status == AC1024_FRAME_INVALID_MS) {
                                hint.mc_unreadable_invalid_ms += group;
                                if (nb >= 4u && offsets[start] <= nb - 4u) {
                                    const size_t bad_at = (size_t)offsets[start];
                                    const uint16_t first_ms =
                                        (uint16_t)objects[bad_at] |
                                        ((uint16_t)objects[bad_at + 1u] << 8u);
                                    if (first_ms == 0u)
                                        hint.mc_invalid_ms_zero += group;
                                    else if (first_ms == 1u)
                                        hint.mc_invalid_ms_one += group;
                                    else if (first_ms < 8u)
                                        hint.mc_invalid_ms_2to7 += group;
                                    else if (first_ms & 0x8000u) {
                                        const uint16_t second_ms =
                                            (uint16_t)objects[bad_at + 2u] |
                                            ((uint16_t)objects[bad_at + 3u] << 8u);
                                        if (second_ms == 0u)
                                            hint.mc_invalid_ms_extended_zero += group;
                                        else if (second_ms & 0x8000u)
                                            hint.mc_invalid_ms_extended_reserved += group;
                                        else
                                            hint.mc_invalid_ms_unclassified += group;
                                    } else
                                        hint.mc_invalid_ms_unclassified += group;
                                } else {
                                    hint.mc_invalid_ms_unclassified += group;
                                }
                            } else if (status == AC1024_FRAME_TOO_SHORT ||
                                     status == AC1024_FRAME_EXCEEDS_SECTION)
                                hint.mc_unreadable_short_or_boundary += group;
                            else
                                hint.mc_unreadable_other += group;
                            if (start < legacy_count / 2u)
                                hint.mc_unreadable_first_half += group;
                        }
                        /* Fixed deterministic <=2048 offset sample
                           for shift hypotheses. The FULL source still
                           receives the original frame-reason accounting. */
                        size_t step = legacy_count / 2048u;
                        if (step == 0u) step = 1u;
                        if ((start % step) == 0u &&
                            hint.legacy_shift_samples_tested < 2048u) {
                            ++hint.legacy_shift_samples_tested;
                            const uint64_t offset = offsets[start];
                            if (offset >= 2u && ac1024_candidate_frame_crc(
                                    objects, nb, offset - 2u))
                                ++hint.legacy_shift_minus_two_crc_pass;
                            if (offset >= 1u && ac1024_candidate_frame_crc(
                                    objects, nb, offset - 1u))
                                ++hint.legacy_shift_minus_one_crc_pass;
                            if (offset < UINT64_MAX &&
                                ac1024_candidate_frame_crc(
                                    objects, nb, offset + 1u))
                                ++hint.legacy_shift_plus_one_crc_pass;
                            if (offset <= UINT64_MAX - 2u &&
                                ac1024_candidate_frame_crc(
                                    objects, nb, offset + 2u))
                                ++hint.legacy_shift_plus_two_crc_pass;
                        }
                        start = end;
                    }
                }
                /* Full-corpus embedded self-H census. This DOES NOT
                   replace or repair the existing indexed handle map.
                   Prove the published R2010 H prefix across every
                   individually CRC-backed record; never export actual
                   handles, offsets, names, geometry, or source bytes. */
                hint.full_embedded_h_census_completed = 1;
                uint64_t prior_full_h = 0u;
                int prior_full_h_valid = 0;
                for (size_t k = 0u; k < legacy_count; k++) {
                    ++hint.full_embedded_h_attempts;
                    MusaDwgAc1024StreamBounds stream = {0};
                    MusaDwgAc1015Bits cursor = {0};
                    MusaDwgAc1015HandleRef self = {0};
                    if (!legacy[k].handle ||
                        !musa_dwg_ac1024_record_stream_bounds(
                            objects, nb, legacy[k].object_offset,
                            &stream) ||
                        stream.handle_stream_start_bit <=
                            stream.data_after_ot_bit ||
                        !musa_dwg_ac1015_bits_init(
                            &cursor, objects + stream.payload_start_byte,
                            stream.handle_stream_end_bit / 8u,
                            stream.data_after_ot_bit,
                            stream.handle_stream_start_bit) ||
                        !musa_dwg_ac1015_read_handle_ref(
                            &cursor, legacy[k].handle, &self)) {
                        ++hint.full_embedded_h_unreadable;
                        prior_full_h_valid = 0;
                        continue;
                    }
                    if (self.code != 0u) {
                        ++hint.full_embedded_h_other_codes;
                        prior_full_h_valid = 0;
                        continue;
                    }
                    ++hint.full_embedded_h_parsed_code0;
                    if (self.target == legacy[k].handle)
                        ++hint.full_embedded_h_self_matches;
                    else if (self.target < legacy[k].handle)
                        ++hint.full_embedded_h_less_than_index;
                    else
                        ++hint.full_embedded_h_greater_than_index;
                    if (prior_full_h_valid) {
                        ++hint.full_embedded_h_neighbor_pairs;
                        if (self.target > prior_full_h)
                            ++hint.full_embedded_h_increases;
                        else if (self.target < prior_full_h)
                            ++hint.full_embedded_h_decreases;
                        else
                            ++hint.full_embedded_h_equals;
                    }
                    prior_full_h_valid = 1;
                    prior_full_h = self.target;
                }
                /* New shared C11 ABI, entirely separate from the
                   exploratory bit-offset and alternative-map profiles.
                   Full source CRC and canonical self-H MUST pass
                   before any recovered identity array becomes visible.
                   A successful recovered identity index does NOT
                   establish owner/layer/geometry or DWG opening. */
                hint.core_embedded_h_recovery_attempted = 1;
                if (legacy_count <= SIZE_MAX / sizeof(uint64_t) &&
                    legacy_count <=
                       SIZE_MAX / sizeof(MusaDwgAc1024SelfIdentity)) {
                    uint64_t *candidate_offsets =
                        (uint64_t *)malloc(
                            legacy_count * sizeof(*candidate_offsets));
                    MusaDwgAc1024SelfIdentity *recovered =
                        (MusaDwgAc1024SelfIdentity *)malloc(
                            legacy_count * sizeof(*recovered));
                    if (candidate_offsets && recovered) {
                        for (size_t k = 0u; k < legacy_count; k++)
                            candidate_offsets[k] = legacy[k].object_offset;
                        if (musa_dwg_ac1024_recover_self_index(
                                objects, nb, candidate_offsets,
                                legacy_count, recovered, legacy_count)) {
                            hint.core_embedded_h_recovery_succeeded = 1;
                            hint.core_embedded_h_recovery_records = legacy_count;
                            for (size_t k = 0u; k < legacy_count; k++) {
                                if (recovered[k].embedded_absolute_handle ==
                                        legacy[k].handle)
                                    ++hint.core_embedded_h_recovery_original_index_matches;
                            }
                            /* Bounded, reproducible 2048-record sample of
                               the zero-EED common-data field only. Source
                               and recovered handles never leave memory;
                               no owner/layer/geometry inference. */
                            size_t eed_stride = legacy_count / 2048u;
                            if (!eed_stride) eed_stride = 1u;
                            for (size_t k = 0u;
                                 k < legacy_count &&
                                 hint.sampled_zero_eed_attempts < 2048u;
                                 k += eed_stride) {
                                ++hint.sampled_zero_eed_attempts;
                                MusaDwgAc1024ZeroEedPrefix zero = {0};
                                if (musa_dwg_ac1024_record_zero_eed_prefix(
                                        objects, nb, recovered[k].record_offset,
                                        &zero) &&
                                    zero.embedded_absolute_handle ==
                                        recovered[k].embedded_absolute_handle)
                                    ++hint.sampled_zero_eed_verified;
                                else
                                    ++hint.sampled_zero_eed_refused;
                                MusaDwgAc1024NoProxyPrefix graphic = {0};
                                if (musa_dwg_ac1024_record_no_proxy_prefix(
                                        objects, nb, recovered[k].record_offset,
                                        &graphic) &&
                                    graphic.embedded_absolute_handle ==
                                        recovered[k].embedded_absolute_handle)
                                    ++hint.sampled_no_proxy_verified;
                                else
                                    ++hint.sampled_no_proxy_refused;
                                /* Explicit fixed-type raw OT subset only:
                                   no class guess, no raw handles, no owner,
                                   layer, insert coordinate or scene output. */
                                const uint16_t type = recovered[k].raw_ot;
                                if (type == 51u) {
                                    ++hint.sampled_layer_eed_appid_attempts;
                                    MusaDwgAc1024EedAppidEvidence layer_appid = {0};
                                    if (musa_dwg_ac1024_layer_eed_appid_probe(
                                            objects, nb, recovered, legacy_count,
                                            recovered[k].embedded_absolute_handle,
                                            &layer_appid) &&
                                        layer_appid.chain.raw_ot == 51u &&
                                        layer_appid.chain.indexed_object_offset ==
                                            recovered[k].record_offset &&
                                        layer_appid.chain.embedded_absolute_handle ==
                                            recovered[k].embedded_absolute_handle &&
                                        layer_appid.appid_reference_count ==
                                            layer_appid.chain.application_count &&
                                        layer_appid.appid_reference_count >= 1u &&
                                        layer_appid.original_appid_records_crc_verified) {
                                        ++hint.sampled_layer_eed_appid_verified;
                                        hint.sampled_layer_eed_appid_refs_verified +=
                                            layer_appid.appid_reference_count;
                                        ++hint.sampled_layer_eed_xdic_graph_attempts;
                                        MusaDwgAc1024LayerEedXDictionaryGraph layer_graph = {0};
                                        if (musa_dwg_ac1024_layer_eed_xdictionary_graph_probe(
                                                objects, nb, recovered, legacy_count,
                                                recovered[k].embedded_absolute_handle,
                                                &layer_graph) &&
                                            layer_graph.layer_handle ==
                                                recovered[k].embedded_absolute_handle &&
                                            layer_graph.layer_record_offset ==
                                                recovered[k].record_offset &&
                                            layer_graph.layer_control_handle &&
                                            layer_graph.xdictionary_handle &&
                                            layer_graph.layer_control_handle !=
                                                layer_graph.xdictionary_handle &&
                                            layer_graph.eed_appid_reference_count ==
                                                layer_appid.appid_reference_count &&
                                            layer_graph.original_records_crc_verified &&
                                            layer_graph.display_accepted == 0u) {
                                            ++hint.sampled_layer_eed_xdic_graph_verified;
                                            if (layer_graph.remaining_handle_tail_bits)
                                                ++hint.sampled_layer_eed_xdic_graph_has_tail;
                                        } else {
                                            ++hint.sampled_layer_eed_xdic_graph_refused;
                                            switch (musa_dwg_ac1024_layer_eed_xdictionary_graph_failure_stage(
                                                    objects, nb, recovered, legacy_count,
                                                    recovered[k].embedded_absolute_handle)) {
                                            case MUSA_DWG_LAYER_XDIC_GRAPH_IDENTITY:
                                                ++hint.sampled_layer_eed_xdic_refused_identity; break;
                                            case MUSA_DWG_LAYER_XDIC_GRAPH_EED:
                                                ++hint.sampled_layer_eed_xdic_refused_eed; break;
                                            case MUSA_DWG_LAYER_XDIC_GRAPH_FLAGS:
                                                ++hint.sampled_layer_eed_xdic_refused_flags; break;
                                            case MUSA_DWG_LAYER_XDIC_GRAPH_APPID:
                                                ++hint.sampled_layer_eed_xdic_refused_appid; break;
                                            case MUSA_DWG_LAYER_XDIC_GRAPH_TAIL:
                                                ++hint.sampled_layer_eed_xdic_refused_tail; break;
                                            case MUSA_DWG_LAYER_XDIC_GRAPH_CONTROL_H:
                                                ++hint.sampled_layer_eed_xdic_refused_control_h; break;
                                            case MUSA_DWG_LAYER_XDIC_GRAPH_DICTIONARY_H:
                                                ++hint.sampled_layer_eed_xdic_refused_dictionary_h; break;
                                            case MUSA_DWG_LAYER_XDIC_GRAPH_TARGET_INDEX:
                                                ++hint.sampled_layer_eed_xdic_refused_target_index; break;
                                            case MUSA_DWG_LAYER_XDIC_GRAPH_TARGET_CLASS:
                                                ++hint.sampled_layer_eed_xdic_refused_target_class; break;
                                            case MUSA_DWG_LAYER_XDIC_GRAPH_TARGET_CRC:
                                                ++hint.sampled_layer_eed_xdic_refused_target_crc; break;
                                            default:
                                                ++hint.sampled_layer_eed_xdic_refused_other; break;
                                            }
                                        }
                                    } else
                                        ++hint.sampled_layer_eed_appid_refused;
                                    ++hint.sampled_layer_eed_flags_attempts;
                                    MusaDwgAc1024LayerRawFlagsEvidence eed_layer = {0};
                                    if (musa_dwg_ac1024_layer_eed_raw_flags_probe(
                                            objects, nb, recovered, legacy_count,
                                            recovered[k].embedded_absolute_handle,
                                            &eed_layer) &&
                                        eed_layer.layer_handle ==
                                            recovered[k].embedded_absolute_handle &&
                                        eed_layer.layer_record_offset ==
                                            recovered[k].record_offset &&
                                        eed_layer.raw_flags_end_bit >
                                            eed_layer.raw_flags_start_bit &&
                                        eed_layer.original_record_crc_verified &&
                                        eed_layer.display_accepted == 0u) {
                                        ++hint.sampled_layer_eed_flags_verified;
                                        if (eed_layer.frozen_bit)
                                            ++hint.sampled_layer_eed_flags_frozen_bit;
                                        if (eed_layer.on_bit)
                                            ++hint.sampled_layer_eed_flags_on_bit;
                                        if (eed_layer.locked_bit)
                                            ++hint.sampled_layer_eed_flags_locked_bit;
                                        if (eed_layer.unmapped_bits)
                                            ++hint.sampled_layer_eed_flags_unmapped_bits;
                                        if (eed_layer.xdictionary_missing == 0u)
                                            ++hint.sampled_layer_eed_flags_xdictionary_present;
                                    } else {
                                        ++hint.sampled_layer_eed_flags_refused;
                                    }
                                    ++hint.sampled_layer_raw_flags_attempts;
                                    MusaDwgAc1024LayerRawFlagsEvidence layer = {0};
                                    if (musa_dwg_ac1024_layer_raw_flags_probe(
                                            objects, nb, recovered, legacy_count,
                                            recovered[k].embedded_absolute_handle,
                                            &layer) &&
                                        layer.layer_handle ==
                                            recovered[k].embedded_absolute_handle &&
                                        layer.layer_record_offset ==
                                            recovered[k].record_offset &&
                                        layer.raw_flags_end_bit >
                                            layer.raw_flags_start_bit &&
                                        layer.original_record_crc_verified &&
                                        layer.display_accepted == 0u) {
                                        ++hint.sampled_layer_raw_flags_verified;
                                        if (layer.frozen_bit)
                                            ++hint.sampled_layer_raw_flags_frozen_bit;
                                        if (layer.on_bit)
                                            ++hint.sampled_layer_raw_flags_on_bit;
                                        if (layer.locked_bit)
                                            ++hint.sampled_layer_raw_flags_locked_bit;
                                        if (layer.unmapped_bits)
                                            ++hint.sampled_layer_raw_flags_unmapped_bits;
                                        if (layer.xdictionary_missing == 0u)
                                            ++hint.sampled_layer_raw_flags_xdictionary_present;
                                    } else {
                                        ++hint.sampled_layer_raw_flags_refused;
                                        switch (musa_dwg_ac1024_layer_raw_flags_failure_stage(
                                                objects, nb, recovered, legacy_count,
                                                recovered[k].embedded_absolute_handle)) {
                                        case MUSA_DWG_LAYER_FLAGS_INDEX:
                                            ++hint.sampled_layer_flags_refused_index; break;
                                        case MUSA_DWG_LAYER_FLAGS_ZERO_EED:
                                            ++hint.sampled_layer_flags_refused_eed; break;
                                        case MUSA_DWG_LAYER_FLAGS_RECORD_BOUNDS:
                                            ++hint.sampled_layer_flags_refused_record; break;
                                        case MUSA_DWG_LAYER_FLAGS_REACTORS:
                                            ++hint.sampled_layer_flags_refused_reactors; break;
                                        case MUSA_DWG_LAYER_FLAGS_XDICTIONARY_BIT:
                                            ++hint.sampled_layer_flags_refused_xdic_bit; break;
                                        case MUSA_DWG_LAYER_FLAGS_NUMERIC:
                                            ++hint.sampled_layer_flags_refused_numeric; break;
                                        default:
                                            ++hint.sampled_layer_flags_refused_other; break;
                                        }
                                    }
                                }
                                int line_owner_layer_joinable = 0;
                                int line_meta_ok = 0;
                                MusaDwgAc1024EntityMetaPrefix meta = {0};
                                if (type == 7u || type == 17u ||
                                    type == 18u || type == 19u) {
                                    ++hint.sampled_entity_meta_attempts;
                                    if (musa_dwg_ac1024_record_entity_meta_prefix(
                                            objects, nb, recovered[k].record_offset,
                                            &meta) &&
                                        meta.embedded_absolute_handle ==
                                            recovered[k].embedded_absolute_handle &&
                                        meta.raw_ot == type) {
                                        ++hint.sampled_entity_meta_verified;
                                        line_meta_ok = type == 19u;
                                    } else {
                                        ++hint.sampled_entity_meta_refused;
                                    }
                                    ++hint.sampled_owner_layer_attempts;
                                    MusaDwgAc1024OwnerLayerEvidence links = {0};
                                    if (musa_dwg_ac1024_entity_owner_layer_probe(
                                            objects, nb, recovered, legacy_count,
                                            recovered[k].embedded_absolute_handle,
                                            &links) &&
                                        links.entity_handle ==
                                            recovered[k].embedded_absolute_handle) {
                                        ++hint.sampled_owner_layer_verified;
                                        line_owner_layer_joinable =
                                            type == 19u &&
                                            links.entity_record_offset ==
                                                recovered[k].record_offset &&
                                            links.entity_raw_ot == 19u &&
                                            links.source_crc_verified &&
                                            links.target_records_verified;
                                    } else {
                                        ++hint.sampled_owner_layer_refused;
                                    }
                                }
                                /* Real LFS DWG only: no guessed stream offsets,
                                 * no coordinates, source IDs or names emitted.
                                 * A numeric OT19 is only a candidate LINE. */
                                if (type == 19u) {
                                    ++hint.sampled_line_attempts;
                                    const int xdic_present =
                                        line_meta_ok && meta.xdictionary_missing == 0u;
                                    if (xdic_present)
                                        ++hint.sampled_line_xdic_present_attempts;
                                    MusaDwgAc1024LineCommon common = {0};
                                    const int common_ok =
                                        musa_dwg_ac1024_line_common_probe(
                                            objects, nb, recovered[k].record_offset,
                                            &common) &&
                                        common.indexed_handle ==
                                            recovered[k].embedded_absolute_handle &&
                                        common.record_crc_verified;
                                    if (common_ok) {
                                        ++hint.sampled_line_common_verified;
                                    } else {
                                        ++hint.sampled_line_common_refused;
                                        if (line_meta_ok) {
                                            ++hint.sampled_line_common_after_meta_refused;
                                            const uint8_t failure_stage =
                                                musa_dwg_ac1024_line_common_failure_stage(
                                                    objects, nb, recovered[k].record_offset);
                                            switch (failure_stage) {
                                            case MUSA_DWG_LINE_COMMON_COLOR:
                                                ++hint.sampled_line_common_stage_color; break;
                                            case MUSA_DWG_LINE_COMMON_RGB:
                                                ++hint.sampled_line_common_stage_rgb; break;
                                            case MUSA_DWG_LINE_COMMON_TRANSPARENCY:
                                                ++hint.sampled_line_common_stage_transparency; break;
                                            case MUSA_DWG_LINE_COMMON_LTYPE_SCALE: {
                                                ++hint.sampled_line_common_stage_ltype;
                                                const uint8_t reason =
                                                    musa_dwg_ac1024_line_ltscale_refusal_kind(
                                                        objects, nb, recovered[k].record_offset);
                                                switch (reason) {
                                                case MUSA_DWG_LINE_LTSCALE_ZERO:
                                                    ++hint.sampled_line_ltype_zero; break;
                                                case MUSA_DWG_LINE_LTSCALE_NEGATIVE:
                                                    ++hint.sampled_line_ltype_negative; break;
                                                case MUSA_DWG_LINE_LTSCALE_NONFINITE:
                                                    ++hint.sampled_line_ltype_nonfinite; break;
                                                case MUSA_DWG_LINE_LTSCALE_PARSE_REFUSED:
                                                    ++hint.sampled_line_ltype_parse_refused; break;
                                                default: break;
                                                }
                                                break;
                                            }
                                            case MUSA_DWG_LINE_COMMON_APPEARANCE:
                                                ++hint.sampled_line_common_stage_appearance; break;
                                            case MUSA_DWG_LINE_COMMON_GEOMETRY_BOUND:
                                                ++hint.sampled_line_common_stage_geom_bound; break;
                                            default:
                                                ++hint.sampled_line_common_stage_other; break;
                                            }
                                            /* First unsupported policy wins; do not conflate
                                             * it with a corrupt CRC or unreadable record. */
                                            if (meta.entity_mode > 2u)
                                                ++hint.sampled_line_common_mode_unsupported;
                                            else if (meta.reactor_count != 0u)
                                                ++hint.sampled_line_common_reactors_unsupported;
                                            else
                                                ++hint.sampled_line_common_fields_refused;
                                        } else {
                                            ++hint.sampled_line_meta_refused;
                                            MusaDwgAc1024SelfPrefix self = {0};
                                            MusaDwgAc1024ZeroEedPrefix eed = {0};
                                            MusaDwgAc1024NoProxyPrefix proxy = {0};
                                            /* Exclusive precedence, not a second
                                             * attempt to reinterpret unsupported EED. */
                                            if (!musa_dwg_ac1024_record_self_prefix(
                                                    objects, nb,
                                                    recovered[k].record_offset, &self))
                                                ++hint.sampled_line_meta_self_refused;
                                            else if (!musa_dwg_ac1024_record_zero_eed_prefix(
                                                    objects, nb,
                                                    recovered[k].record_offset, &eed)) {
                                                ++hint.sampled_line_meta_eed_refused;
                                                ++hint.sampled_line_eed_chain_attempts;
                                                MusaDwgAc1024EedChainEvidence chain = {0};
                                                if (musa_dwg_ac1024_record_eed_chain_probe(
                                                        objects, nb,
                                                        recovered[k].record_offset, &chain) &&
                                                    chain.embedded_absolute_handle ==
                                                        recovered[k].embedded_absolute_handle &&
                                                    chain.original_crc16_verified &&
                                                    chain.eed_boundaries_verified) {
                                                    ++hint.sampled_line_eed_chain_verified;
                                                    if (chain.application_count > 0u) {
                                                        ++hint.sampled_line_eed_chain_has_applications;
                                                        ++hint.sampled_line_eed_xyz_attempts;
                                                        MusaDwgAc1024LineFields eed_xyz = {0};
                                                        if (musa_dwg_ac1024_line_eed_fields_probe(
                                                                objects, nb,
                                                                recovered[k].record_offset,
                                                                &eed_xyz) &&
                                                            eed_xyz.indexed_handle ==
                                                                recovered[k].embedded_absolute_handle &&
                                                            eed_xyz.coordinates_decoded &&
                                                            eed_xyz.geometry_end_bit >
                                                                eed_xyz.geometry_start_bit) {
                                                            ++hint.sampled_line_eed_xyz_verified;
                                                            ++hint.sampled_line_eed_appid_attempts;
                                                            MusaDwgAc1024EedAppidEvidence appids = {0};
                                                            if (musa_dwg_ac1024_line_eed_appid_probe(
                                                                    objects, nb, recovered, legacy_count,
                                                                    recovered[k].embedded_absolute_handle,
                                                                    &appids) &&
                                                                appids.chain.indexed_object_offset ==
                                                                    recovered[k].record_offset &&
                                                                appids.chain.embedded_absolute_handle ==
                                                                    recovered[k].embedded_absolute_handle &&
                                                                appids.chain.application_count ==
                                                                    chain.application_count &&
                                                                appids.appid_reference_count ==
                                                                    chain.application_count &&
                                                                appids.distinct_appid_count <=
                                                                    appids.appid_reference_count &&
                                                                appids.original_appid_records_crc_verified) {
                                                                ++hint.sampled_line_eed_appid_verified;
                                                                hint.sampled_line_eed_appid_references_verified +=
                                                                    appids.appid_reference_count;
                                                                hint.sampled_line_eed_appid_distinct_references_verified +=
                                                                    appids.distinct_appid_count;
                                                            } else
                                                                ++hint.sampled_line_eed_appid_refused;
                                                            ++hint.sampled_line_eed_join_attempts;
                                                            MusaDwgAc1024LineEedJoinedEvidence joined = {0};
                                                            if (musa_dwg_ac1024_line_eed_verified_probe(
                                                                    objects, nb, recovered, legacy_count,
                                                                    recovered[k].embedded_absolute_handle,
                                                                    &joined) &&
                                                                joined.graph.handles.entity_record_offset ==
                                                                    recovered[k].record_offset &&
                                                                joined.graph.handles.entity_handle ==
                                                                    recovered[k].embedded_absolute_handle &&
                                                                joined.graph.eed.application_count ==
                                                                    chain.application_count &&
                                                                joined.geometry.indexed_handle ==
                                                                    eed_xyz.indexed_handle &&
                                                                joined.geometry.geometry_start_bit ==
                                                                    eed_xyz.geometry_start_bit &&
                                                                joined.geometry.geometry_end_bit ==
                                                                    eed_xyz.geometry_end_bit &&
                                                                joined.geometry.start[0] == eed_xyz.start[0] &&
                                                                joined.geometry.start[1] == eed_xyz.start[1] &&
                                                                joined.geometry.start[2] == eed_xyz.start[2] &&
                                                                joined.geometry.end[0] == eed_xyz.end[0] &&
                                                                joined.geometry.end[1] == eed_xyz.end[1] &&
                                                                joined.geometry.end[2] == eed_xyz.end[2]) {
                                                                ++hint.sampled_line_eed_join_verified;
                                                                if (joined.graph.handles.entity_mode == 0u)
                                                                    ++hint.sampled_line_eed_join_explicit_owner;
                                                                else
                                                                    ++hint.sampled_line_eed_join_implicit_layer;
                                                                if (joined.graph.handles.xdictionary_handle)
                                                                    ++hint.sampled_line_eed_join_xdictionary;
                                                            } else
                                                                ++hint.sampled_line_eed_join_refused;
                                                            if (eed_xyz.start[0] != eed_xyz.end[0] ||
                                                                eed_xyz.start[1] != eed_xyz.end[1] ||
                                                                eed_xyz.start[2] != eed_xyz.end[2])
                                                                ++hint.sampled_line_eed_xyz_distinct_endpoints;
                                                        } else
                                                            ++hint.sampled_line_eed_xyz_refused;
                                                    }
                                                } else
                                                    ++hint.sampled_line_eed_chain_refused;
                                            } else if (!musa_dwg_ac1024_record_no_proxy_prefix(
                                                    objects, nb,
                                                    recovered[k].record_offset, &proxy))
                                                ++hint.sampled_line_meta_proxy_refused;
                                            else
                                                ++hint.sampled_line_meta_entity_fields_refused;
                                        }
                                    }
                                    MusaDwgAc1024LineFields xyz = {0};
                                    const int xyz_ok =
                                        musa_dwg_ac1024_line_fields_probe(
                                            objects, nb, recovered[k].record_offset,
                                            &xyz) &&
                                        xyz.indexed_handle ==
                                            recovered[k].embedded_absolute_handle &&
                                        xyz.coordinates_decoded;
                                    if (xyz_ok) {
                                        ++hint.sampled_line_xyz_verified;
                                        if (xdic_present) {
                                            ++hint.sampled_line_xdic_present_xyz_verified;
                                            ++hint.sampled_line_xdic_hgraph_attempts;
                                            MusaDwgAc1024LineXDictionaryGraph xdict = {0};
                                            if (musa_dwg_ac1024_line_xdictionary_graph_probe(
                                                    objects, nb, recovered, legacy_count,
                                                    recovered[k].embedded_absolute_handle,
                                                    &xdict) &&
                                                xdict.entity_handle ==
                                                    recovered[k].embedded_absolute_handle &&
                                                xdict.entity_record_offset ==
                                                    recovered[k].record_offset &&
                                                xdict.entity_mode == meta.entity_mode &&
                                                xdict.xdictionary_handle != 0u &&
                                                xdict.layer_handle != 0u &&
                                                xdict.original_records_crc_verified)
                                                ++hint.sampled_line_xdic_hgraph_verified;
                                            else
                                                ++hint.sampled_line_xdic_hgraph_refused;
                                        }
                                        if (common_ok &&
                                            xyz.geometry_start_bit ==
                                                common.geometry_start_bit &&
                                            xyz.geometry_end_bit >
                                                xyz.geometry_start_bit &&
                                            xyz.geometry_end_bit <=
                                                common.data_end_bit) {
                                            ++hint.sampled_line_xyz_bounded_common;
                                            if (line_owner_layer_joinable) {
                                                ++hint.sampled_line_xyz_owner_layer_joined;
                                                ++hint.sampled_line_graph_attempts;
                                                MusaDwgAc1024LineGraphEvidence graph = {0};
                                                if (musa_dwg_ac1024_line_graph_probe(
                                                        objects, nb, recovered,
                                                        legacy_count,
                                                        recovered[k].embedded_absolute_handle,
                                                        &graph) &&
                                                    graph.owner_layer.entity_record_offset ==
                                                        recovered[k].record_offset &&
                                                    graph.owner_layer.entity_handle ==
                                                        recovered[k].embedded_absolute_handle &&
                                                    graph.geometry.indexed_handle ==
                                                        xyz.indexed_handle &&
                                                    graph.geometry.geometry_start_bit ==
                                                        xyz.geometry_start_bit &&
                                                    graph.geometry.geometry_end_bit ==
                                                        xyz.geometry_end_bit &&
                                                    graph.geometry.start[0] == xyz.start[0] &&
                                                    graph.geometry.start[1] == xyz.start[1] &&
                                                    graph.geometry.start[2] == xyz.start[2] &&
                                                    graph.geometry.end[0] == xyz.end[0] &&
                                                    graph.geometry.end[1] == xyz.end[1] &&
                                                    graph.geometry.end[2] == xyz.end[2])
                                                    ++hint.sampled_line_graph_verified;
                                                else
                                                    ++hint.sampled_line_graph_refused;
                                            } else if ((meta.entity_mode == 1u ||
                                                        meta.entity_mode == 2u) &&
                                                       meta.xdictionary_missing == 1u) {
                                                if (meta.entity_mode == 1u)
                                                    ++hint.sampled_line_implicit_paper_xyz_verified;
                                                else
                                                    ++hint.sampled_line_implicit_model_xyz_verified;
                                                ++hint.sampled_line_implicit_layer_attempts;
                                                MusaDwgAc1024ImplicitLineLayerEvidence implicit = {0};
                                                if (musa_dwg_ac1024_line_implicit_layer_probe(
                                                        objects, nb, recovered,
                                                        legacy_count,
                                                        recovered[k].embedded_absolute_handle,
                                                        &implicit) &&
                                                    implicit.implicit_layer.entity_mode ==
                                                        meta.entity_mode &&
                                                    implicit.implicit_layer.entity_record_offset ==
                                                        recovered[k].record_offset &&
                                                    implicit.geometry.indexed_handle ==
                                                        xyz.indexed_handle &&
                                                    implicit.geometry.geometry_start_bit ==
                                                        xyz.geometry_start_bit &&
                                                    implicit.geometry.geometry_end_bit ==
                                                        xyz.geometry_end_bit &&
                                                    implicit.geometry.start[0] == xyz.start[0] &&
                                                    implicit.geometry.start[1] == xyz.start[1] &&
                                                    implicit.geometry.start[2] == xyz.start[2] &&
                                                    implicit.geometry.end[0] == xyz.end[0] &&
                                                    implicit.geometry.end[1] == xyz.end[1] &&
                                                    implicit.geometry.end[2] == xyz.end[2])
                                                    ++hint.sampled_line_implicit_layer_verified;
                                                else
                                                    ++hint.sampled_line_implicit_layer_refused;
                                            }
                                        }
                                        if (xyz.start[0] != xyz.end[0] ||
                                            xyz.start[1] != xyz.end[1] ||
                                            xyz.start[2] != xyz.end[2])
                                            ++hint.sampled_line_xyz_distinct_endpoints;
                                    } else {
                                        ++hint.sampled_line_xyz_refused;
                                        if (xdic_present)
                                            ++hint.sampled_line_xdic_present_xyz_refused;
                                        if (common_ok)
                                            ++hint.sampled_line_xyz_after_common_refused;
                                    }
                                }
                            }
                        }
                    }
                    free(recovered);
                    free(candidate_offsets);
                }
                /* Separate fixed-budget exploratory H identity:
                   the public R2010 layout places a self-H immediately
                   after OT in the DATA stream. Only report aggregate
                   matches to the independently CRC-backed handle map.
                   Never serialize source IDs, offsets, text or geometry. */
                size_t stride = legacy_count / 2048u;
                if (!stride) stride = 1u;
                size_t shifted_self_matches[33] = {0};
                uint64_t sampled_h_values[2048] = {0};
                size_t sampled_h_count = 0u;
                uint64_t prior_h = 0u;
                for (size_t k = 0u; k < legacy_count; k += stride) {
                    if (hint.sampled_first_h_attempts >= 2048u) break;
                    ++hint.sampled_first_h_attempts;
                    MusaDwgAc1024StreamBounds stream = {0};
                    if (!musa_dwg_ac1024_record_stream_bounds(
                            objects, nb, legacy[k].object_offset,
                            &stream) || !legacy[k].handle ||
                        stream.handle_stream_start_bit <=
                            stream.data_after_ot_bit) {
                        ++hint.sampled_first_h_unreadable;
                        continue;
                    }
                    MusaDwgAc1015Bits cursor = {0};
                    MusaDwgAc1015HandleRef first = {0};
                    if (!musa_dwg_ac1015_bits_init(
                            &cursor, objects + stream.payload_start_byte,
                            stream.handle_stream_end_bit / 8u,
                            stream.data_after_ot_bit,
                            stream.handle_stream_start_bit) ||
                        !musa_dwg_ac1015_read_handle_ref(
                            &cursor, legacy[k].handle, &first)) {
                        ++hint.sampled_first_h_unreadable;
                    } else if (first.code == 0u &&
                               first.target == legacy[k].handle) {
                        ++hint.sampled_first_h_self_matches;
                    } else {
                        ++hint.sampled_first_h_other;
                        if (first.code == 0u) {
                            ++hint.sampled_first_h_code0_mismatch;
                            if (first.count == 1u)
                                ++hint.sampled_first_h_one_byte_values;
                            uint64_t reversed = 0u, raw = first.encoded_value;
                            for (size_t ri = 0u; ri < first.count; ri++) {
                                reversed = (reversed << 8u) | (raw & 0xffu);
                                raw >>= 8u;
                            }
                            if (reversed == legacy[k].handle)
                                ++hint.sampled_first_h_reverse_bytes_matches;
                            if (legacy[k].handle < UINT64_MAX &&
                                first.target == legacy[k].handle + 1u)
                                ++hint.sampled_first_h_plus_one_matches;
                            if (legacy[k].handle > 1u &&
                                first.target == legacy[k].handle - 1u)
                                ++hint.sampled_first_h_minus_one_matches;
                        }
                        else if (first.code >= 2u && first.code <= 5u)
                            ++hint.sampled_first_h_absolute_other_code;
                        else
                            ++hint.sampled_first_h_relative_code;
                    }
                    if (first.target != 0u) {
                        if (sampled_h_count != 0u) {
                            ++hint.sampled_h_successive_pairs;
                            if (first.target > prior_h)
                                ++hint.sampled_h_successive_increases;
                            else if (first.target < prior_h)
                                ++hint.sampled_h_successive_decreases;
                            else ++hint.sampled_h_successive_equals;
                        }
                        prior_h = first.target;
                        if (sampled_h_count < 2048u)
                            sampled_h_values[sampled_h_count++] =
                                first.target;
                        if (first.target < legacy[k].handle)
                            ++hint.sampled_h_less_than_index;
                        else if (first.target > legacy[k].handle)
                            ++hint.sampled_h_greater_than_index;
                        if (legacy[k].handle > UINT64_C(0xffffff))
                            ++hint.sampled_h_index_requires_over3_bytes;
                        if ((first.target & UINT64_C(0xff)) ==
                            (legacy[k].handle & UINT64_C(0xff)))
                            ++hint.sampled_h_low8_equal_index;
                        if ((first.target & UINT64_C(0xffff)) ==
                            (legacy[k].handle & UINT64_C(0xffff)))
                            ++hint.sampled_h_low16_equal_index;
                        if (first.count == 2u)
                            ++hint.sampled_first_h_count2;
                        else if (first.count == 3u)
                            ++hint.sampled_first_h_count3;
                        else if (first.count >= 4u && first.count <= 8u)
                            ++hint.sampled_first_h_count4to8;
                        /* The verified handle map is strictly monotone.
                           Binary lookup only; never print handles or offsets. */
                        size_t low = 0u, high = legacy_count;
                        while (low < high) {
                            const size_t mid = low + (high - low) / 2u;
                            if (legacy[mid].handle < first.target)
                                low = mid + 1u;
                            else high = mid;
                        }
                        if (low < legacy_count &&
                            legacy[low].handle == first.target) {
                            ++hint.sampled_first_h_in_handle_index;
                            if (legacy[low].object_offset ==
                                legacy[k].object_offset)
                                ++hint.sampled_first_h_same_index_offset;
                        } else
                            ++hint.sampled_first_h_not_in_handle_index;
                    }
                    /* 0..32-bit scan is DIAGNOSTIC ONLY: matching a
                       value by searching different start bits is not
                       a valid self-H decoder or geometry evidence. */
                    const size_t scan_bits =
                        stream.handle_stream_start_bit -
                        stream.data_after_ot_bit;
                    for (size_t shift = 0u;
                         shift <= 32u && shift < scan_bits; shift++) {
                        MusaDwgAc1015Bits alt = {0};
                        MusaDwgAc1015HandleRef candidate = {0};
                        if (musa_dwg_ac1015_bits_init(
                                &alt, objects + stream.payload_start_byte,
                                stream.handle_stream_end_bit / 8u,
                                stream.data_after_ot_bit + shift,
                                stream.handle_stream_start_bit) &&
                            musa_dwg_ac1015_read_handle_ref(
                                &alt, legacy[k].handle, &candidate) &&
                            candidate.code == 0u &&
                            candidate.target == legacy[k].handle)
                            ++shifted_self_matches[shift];
                    }
                }
                qsort(sampled_h_values, sampled_h_count,
                      sizeof(*sampled_h_values),
                      ac1024_compare_offsets_u64);
                for (size_t i = 0u; i < sampled_h_count; i++) {
                    if (i == 0u || sampled_h_values[i] !=
                        sampled_h_values[i - 1u])
                        ++hint.sampled_h_internally_distinct;
                    else
                        ++hint.sampled_h_internal_duplicates;
                }
                for (size_t shift = 0u; shift <= 32u; shift++) {
                    if (shifted_self_matches[shift] >
                        hint.sampled_scan_best_self_matches) {
                        hint.sampled_scan_best_self_matches =
                            shifted_self_matches[shift];
                        hint.sampled_scan_best_shift_bits = shift;
                    }
                }
                /* Never modify the original CRC-backed R2004 map.
                   Compare an alternative final-byte interpretation to
                   the real object-embedded self H, without publishing IDs. */
                if (legacy_count <= SIZE_MAX / sizeof(uint64_t)) {
                    uint64_t *alt_handles =
                        (uint64_t *)malloc(legacy_count * sizeof(uint64_t));
                    size_t alt_count = 0u;
                    if (alt_handles &&
                        ac1024_hmap_last_six_candidate(
                            handles, hb, alt_handles,
                            legacy_count, &alt_count) &&
                        alt_count == legacy_count) {
                        hint.masked6_candidate_index_valid = 1;
                        hint.masked6_candidate_handle_count = alt_count;
                        for (size_t k = 0u; k < legacy_count; k += stride) {
                            if (hint.masked6_sample_attempts >= 2048u) break;
                            ++hint.masked6_sample_attempts;
                            MusaDwgAc1024StreamBounds stream = {0};
                            MusaDwgAc1015Bits cursor = {0};
                            MusaDwgAc1015HandleRef first = {0};
                            if (!alt_handles[k] ||
                                !musa_dwg_ac1024_record_stream_bounds(
                                    objects, nb, legacy[k].object_offset,
                                    &stream) ||
                                stream.handle_stream_start_bit <=
                                    stream.data_after_ot_bit ||
                                !musa_dwg_ac1015_bits_init(
                                    &cursor,
                                    objects + stream.payload_start_byte,
                                    stream.handle_stream_end_bit / 8u,
                                    stream.data_after_ot_bit,
                                    stream.handle_stream_start_bit) ||
                                !musa_dwg_ac1015_read_handle_ref(
                                    &cursor, alt_handles[k], &first)) {
                                ++hint.masked6_sample_unreadable;
                            } else if (first.code == 0u &&
                                       first.target == alt_handles[k]) {
                                ++hint.masked6_sample_self_matches;
                            } else
                                ++hint.masked6_sample_other;
                        }
                    }
                    free(alt_handles);
                }
                free(offsets);
                free(legacy);
            }
            /* Third, distinct CRC-backed handle-map encoding; a missing
               continuous-index result must not be taken as permission to
               invent record positions. */
            size_t absolute_count = 0u;
            if (objects && handles && got_obj == nb && got_h == hb &&
                musa_dwg_ac1018_handles_index_page_absolute_handles(
                    handles, hb, nb, NULL, 0u, &absolute_count) &&
                absolute_count == hint.candidate_handles &&
                absolute_count <= SIZE_MAX / sizeof(MusaDwgAc1018HandleEntry)) {
                MusaDwgAc1018HandleEntry *absolute_entries =
                    (MusaDwgAc1018HandleEntry *)malloc(
                        absolute_count * sizeof(*absolute_entries));
                size_t actual_abs = 0u;
                if (absolute_entries &&
                    musa_dwg_ac1018_handles_index_page_absolute_handles(
                        handles, hb, nb, absolute_entries,
                        absolute_count, &actual_abs) &&
                    actual_abs == absolute_count) {
                    hint.page_absolute_candidate_index = 1;
                    hint.page_absolute_candidate_handles = actual_abs;
                    for (size_t j = 0u; j < actual_abs; j++) {
                        if (ac1024_candidate_frame_crc(
                              objects, nb, absolute_entries[j].object_offset))
                            hint.page_absolute_frame_crc_ok++;
                        else hint.page_absolute_frame_crc_failed++;
                    }
                }
                free(absolute_entries);
            }
            free(handles);
            free(objects);
        }
    }
    memcpy(scratch, "AC1024", 6u);
    return hint;
}

static void report(const char *signature, const char *result,
                   const char *stage, size_t evidence_count,
                   const MusaAc1024CompatibilityHint *hint) {
    printf("{\"signature\":\"%s\",\"result\":\"%s\","
           "\"native_stage\":\"%s\",\"evidence_count\":%zu,"
           "\"geometry_verified\":false,\"rendering_verified\":false,"
           "\"source_modified\":false",
           signature, result, stage, evidence_count);
    if (hint) {
        printf(",\"ac1024_compatibility_hypothesis\":{"
               "\"patched_signature_in_memory_only\":true,"
               "\"validated_original_ac1024_decoder\":false,"
               "\"page_map_header\":%s,\"physical_page_map\":%s,"
               "\"objects_section_pages\":%s,\"handles_section_pages\":%s,"
               "\"candidate_handle_index\":%s,\"candidate_handle_count\":%zu,"
               "\"object_section_page_count\":%zu,"
               "\"handle_section_page_count\":%zu,"
               "\"legacy_frame_probe_completed\":%s,"
               "\"legacy_frame_crc_ok\":%zu,\"legacy_frame_crc_failed\":%zu,"
               "\"legacy_raw_type_17_arc\":%zu,"
               "\"legacy_raw_type_18_circle\":%zu,"
               "\"legacy_raw_type_19_line\":%zu,"
               "\"legacy_raw_type_77_lwpolyline\":%zu,"
               "\"continuous_offset_candidate_index\":%s,"
               "\"continuous_candidate_handles\":%zu,"
               "\"continuous_frame_crc_ok\":%zu,"
               "\"continuous_frame_crc_failed\":%zu,"
               "\"page_absolute_candidate_index\":%s,"
               "\"page_absolute_candidate_handles\":%zu,"
               "\"page_absolute_frame_crc_ok\":%zu,"
               "\"page_absolute_frame_crc_failed\":%zu,"
               "\"legacy_offset_distribution_measured\":%s,"
               "\"legacy_distinct_offsets\":%zu,"
               "\"legacy_duplicate_offset_entries\":%zu,"
               "\"legacy_zero_offset_entries\":%zu,"
               "\"legacy_max_offset_multiplicity\":%zu,"
               "\"legacy_distinct_frame_crc_pass\":%zu,"
               "\"legacy_repeated_crc_pass_entries\":%zu,"
               "\"legacy_frame_too_short\":%zu,"
               "\"legacy_frame_invalid_ms\":%zu,"
               "\"legacy_frame_exceeds_section\":%zu,"
               "\"legacy_frame_crc_mismatch\":%zu,"
               "\"legacy_shift_minus_two_crc_pass\":%zu,"
               "\"legacy_shift_minus_one_crc_pass\":%zu,"
               "\"legacy_shift_plus_one_crc_pass\":%zu,"
               "\"legacy_shift_plus_two_crc_pass\":%zu,"
               "\"legacy_shift_samples_tested\":%zu,"
               "\"legacy_next_offset_pairs\":%zu,"
               "\"legacy_next_frame_unavailable\":%zu,"
               "\"legacy_next_frame_exact\":%zu,"
               "\"legacy_next_frame_short\":%zu,"
               "\"legacy_next_frame_overlaps\":%zu,"
               "\"legacy_crc_ok_next_frame_exact\":%zu,"
               "\"legacy_short_gap_1\":%zu,"
               "\"legacy_short_gap_2\":%zu,"
               "\"legacy_short_gap_3\":%zu,"
               "\"legacy_short_gap_4\":%zu,"
               "\"legacy_short_gap_5to16\":%zu,"
               "\"legacy_short_gap_17to64\":%zu,"
               "\"legacy_short_gap_over64\":%zu,"
               "\"mc_frame_probe_completed\":%s,"
               "\"mc_frame_crc_pass\":%zu,"
               "\"mc_frame_crc_mismatch\":%zu,"
               "\"mc_frame_unreadable\":%zu,"
               "\"mc_frame_next_exact\":%zu,"
               "\"mc_byte_len_matches_short_gap\":%zu,"
               "\"mc_bytes_1\":%zu,"
               "\"mc_bytes_2\":%zu,"
               "\"mc_bytes_3\":%zu,"
               "\"mc_bytes_over3\":%zu,"
               "\"mc_ot_decoded\":%zu,"
               "\"mc_ot_unreadable\":%zu,"
               "\"mc_ot_raw_1\":%zu,"
               "\"mc_ot_raw_7\":%zu,"
               "\"mc_ot_raw_17\":%zu,"
               "\"mc_ot_raw_18\":%zu,"
               "\"mc_ot_raw_19\":%zu,"
               "\"mc_ot_raw_44\":%zu,"
               "\"mc_ot_raw_49\":%zu,"
               "\"mc_ot_raw_51\":%zu,"
               "\"mc_ot_raw_77\":%zu,"
               "\"core_frame_verified\":%zu,"
               "\"core_frame_probe_failed\":%zu,"
               "\"core_raw_ot_or_span_mismatch\":%zu,"
               "\"core_payload_bounds_verified\":%zu,"
               "\"core_payload_bounds_rejected\":%zu,"
               "\"core_payload_bounds_mismatch\":%zu,"
               "\"mc_unreadable_invalid_ms\":%zu,"
               "\"mc_unreadable_short_or_boundary\":%zu,"
               "\"mc_unreadable_other\":%zu,"
               "\"mc_unreadable_first_half\":%zu,"
               "\"mc_invalid_ms_zero\":%zu,"
               "\"mc_invalid_ms_one\":%zu,"
               "\"mc_invalid_ms_2to7\":%zu,"
               "\"mc_invalid_ms_extended_zero\":%zu,"
               "\"mc_invalid_ms_extended_reserved\":%zu,"
               "\"mc_invalid_ms_unclassified\":%zu,"
               "\"core_stream_bounds_verified\":%zu,"
               "\"core_stream_bounds_rejected\":%zu,"
               "\"core_stream_bounds_mismatch\":%zu,"
               "\"core_empty_handle_streams\":%zu,"
               "\"sampled_first_h_attempts\":%zu,"
               "\"sampled_first_h_self_matches\":%zu,"
               "\"sampled_first_h_other\":%zu,"
               "\"sampled_first_h_unreadable\":%zu,"
               "\"sampled_first_h_code0_mismatch\":%zu,"
               "\"sampled_first_h_absolute_other_code\":%zu,"
               "\"sampled_first_h_relative_code\":%zu,"
               "\"sampled_first_h_reverse_bytes_matches\":%zu,"
               "\"sampled_first_h_plus_one_matches\":%zu,"
               "\"sampled_first_h_minus_one_matches\":%zu,"
               "\"sampled_first_h_one_byte_values\":%zu,"
               "\"sampled_first_h_in_handle_index\":%zu,"
               "\"sampled_first_h_not_in_handle_index\":%zu,"
               "\"sampled_first_h_same_index_offset\":%zu,"
               "\"sampled_first_h_count2\":%zu,"
               "\"sampled_first_h_count3\":%zu,"
               "\"sampled_first_h_count4to8\":%zu,"
               "\"sampled_scan_best_shift_bits\":%zu,"
               "\"sampled_scan_best_self_matches\":%zu,"
               "\"sampled_h_internally_distinct\":%zu,"
               "\"sampled_h_internal_duplicates\":%zu,"
               "\"sampled_h_successive_pairs\":%zu,"
               "\"sampled_h_successive_increases\":%zu,"
               "\"sampled_h_successive_decreases\":%zu,"
               "\"sampled_h_successive_equals\":%zu,"
               "\"sampled_h_less_than_index\":%zu,"
               "\"sampled_h_greater_than_index\":%zu,"
               "\"sampled_h_index_requires_over3_bytes\":%zu,"
               "\"sampled_h_low8_equal_index\":%zu,"
               "\"sampled_h_low16_equal_index\":%zu,"
               "\"masked6_candidate_index_valid\":%s,"
               "\"masked6_candidate_handle_count\":%zu,"
               "\"masked6_sample_attempts\":%zu,"
               "\"masked6_sample_self_matches\":%zu,"
               "\"masked6_sample_other\":%zu,"
               "\"masked6_sample_unreadable\":%zu,"
               "\"full_embedded_h_census_completed\":%s,"
               "\"full_embedded_h_attempts\":%zu,"
               "\"full_embedded_h_parsed_code0\":%zu,"
               "\"full_embedded_h_other_codes\":%zu,"
               "\"full_embedded_h_unreadable\":%zu,"
               "\"full_embedded_h_self_matches\":%zu,"
               "\"full_embedded_h_less_than_index\":%zu,"
               "\"full_embedded_h_greater_than_index\":%zu,"
               "\"full_embedded_h_neighbor_pairs\":%zu,"
               "\"full_embedded_h_increases\":%zu,"
               "\"full_embedded_h_decreases\":%zu,"
               "\"full_embedded_h_equals\":%zu,"
               "\"core_embedded_h_recovery_attempted\":%s,"
               "\"core_embedded_h_recovery_succeeded\":%s,"
               "\"core_embedded_h_recovery_records\":%zu,"
               "\"core_embedded_h_recovery_original_index_matches\":%zu,"
               "\"sampled_zero_eed_attempts\":%zu,"
               "\"sampled_zero_eed_verified\":%zu,"
               "\"sampled_zero_eed_refused\":%zu,"
               "\"sampled_no_proxy_verified\":%zu,"
               "\"sampled_no_proxy_refused\":%zu,"
               "\"sampled_entity_meta_attempts\":%zu,"
               "\"sampled_entity_meta_verified\":%zu,"
               "\"sampled_entity_meta_refused\":%zu,"
               "\"sampled_owner_layer_attempts\":%zu,"
               "\"sampled_owner_layer_verified\":%zu,"
               "\"sampled_owner_layer_refused\":%zu,"
               "\"sampled_line_attempts\":%zu,"
               "\"sampled_line_common_verified\":%zu,"
               "\"sampled_line_common_refused\":%zu,"
               "\"sampled_line_xyz_verified\":%zu,"
               "\"sampled_line_xyz_refused\":%zu,"
               "\"sampled_line_xyz_owner_layer_joined\":%zu,"
               "\"sampled_line_xyz_distinct_endpoints\":%zu,"
               "\"sampled_line_xyz_bounded_common\":%zu,"
               "\"sampled_line_meta_refused\":%zu,"
               "\"sampled_line_meta_self_refused\":%zu,"
               "\"sampled_line_meta_eed_refused\":%zu,"
               "\"sampled_line_meta_proxy_refused\":%zu,"
               "\"sampled_line_meta_entity_fields_refused\":%zu,"
               "\"sampled_line_eed_chain_attempts\":%zu,"
               "\"sampled_line_eed_chain_verified\":%zu,"
               "\"sampled_line_eed_chain_refused\":%zu,"
               "\"sampled_line_eed_chain_has_applications\":%zu,"
               "\"sampled_line_eed_xyz_attempts\":%zu,"
               "\"sampled_line_eed_xyz_verified\":%zu,"
               "\"sampled_line_eed_xyz_refused\":%zu,"
               "\"sampled_line_eed_xyz_distinct_endpoints\":%zu,"
               "\"sampled_line_eed_join_attempts\":%zu,"
               "\"sampled_line_eed_join_verified\":%zu,"
               "\"sampled_line_eed_join_refused\":%zu,"
               "\"sampled_line_eed_join_explicit_owner\":%zu,"
               "\"sampled_line_eed_join_implicit_layer\":%zu,"
               "\"sampled_line_eed_join_xdictionary\":%zu,"
               "\"sampled_line_eed_appid_attempts\":%zu,"
               "\"sampled_line_eed_appid_verified\":%zu,"
               "\"sampled_line_eed_appid_refused\":%zu,"
               "\"sampled_line_eed_appid_references_verified\":%zu,"
               "\"sampled_line_eed_appid_distinct_references_verified\":%zu,"
               "\"sampled_layer_raw_flags_attempts\":%zu,"
               "\"sampled_layer_raw_flags_verified\":%zu,"
               "\"sampled_layer_raw_flags_refused\":%zu,"
               "\"sampled_layer_flags_refused_index\":%zu,"
               "\"sampled_layer_flags_refused_eed\":%zu,"
               "\"sampled_layer_flags_refused_record\":%zu,"
               "\"sampled_layer_flags_refused_reactors\":%zu,"
               "\"sampled_layer_flags_refused_xdic_bit\":%zu,"
               "\"sampled_layer_flags_refused_numeric\":%zu,"
               "\"sampled_layer_flags_refused_other\":%zu,"
               "\"sampled_layer_raw_flags_frozen_bit\":%zu,"
               "\"sampled_layer_raw_flags_on_bit\":%zu,"
               "\"sampled_layer_raw_flags_locked_bit\":%zu,"
               "\"sampled_layer_raw_flags_unmapped_bits\":%zu,"
               "\"sampled_layer_raw_flags_xdictionary_present\":%zu,"
               "\"sampled_layer_eed_flags_attempts\":%zu,"
               "\"sampled_layer_eed_flags_verified\":%zu,"
               "\"sampled_layer_eed_flags_refused\":%zu,"
               "\"sampled_layer_eed_flags_frozen_bit\":%zu,"
               "\"sampled_layer_eed_flags_on_bit\":%zu,"
               "\"sampled_layer_eed_flags_locked_bit\":%zu,"
               "\"sampled_layer_eed_flags_unmapped_bits\":%zu,"
               "\"sampled_layer_eed_flags_xdictionary_present\":%zu,"
               "\"sampled_layer_eed_appid_attempts\":%zu,"
               "\"sampled_layer_eed_appid_verified\":%zu,"
               "\"sampled_layer_eed_appid_refused\":%zu,"
               "\"sampled_layer_eed_appid_refs_verified\":%zu,"
               "\"sampled_layer_eed_xdic_graph_attempts\":%zu,"
               "\"sampled_layer_eed_xdic_graph_verified\":%zu,"
               "\"sampled_layer_eed_xdic_graph_refused\":%zu,"
               "\"sampled_layer_eed_xdic_refused_identity\":%zu,"
               "\"sampled_layer_eed_xdic_refused_eed\":%zu,"
               "\"sampled_layer_eed_xdic_refused_flags\":%zu,"
               "\"sampled_layer_eed_xdic_refused_appid\":%zu,"
               "\"sampled_layer_eed_xdic_refused_tail\":%zu,"
               "\"sampled_layer_eed_xdic_refused_control_h\":%zu,"
               "\"sampled_layer_eed_xdic_refused_dictionary_h\":%zu,"
               "\"sampled_layer_eed_xdic_refused_target_index\":%zu,"
               "\"sampled_layer_eed_xdic_refused_target_class\":%zu,"
               "\"sampled_layer_eed_xdic_refused_target_crc\":%zu,"
               "\"sampled_layer_eed_xdic_refused_other\":%zu,"
               "\"sampled_layer_eed_xdic_graph_has_tail\":%zu,"
               "\"sampled_line_common_after_meta_refused\":%zu,"
               "\"sampled_line_xyz_after_common_refused\":%zu,"
               "\"sampled_line_common_mode_unsupported\":%zu,"
               "\"sampled_line_common_reactors_unsupported\":%zu,"
               "\"sampled_line_common_xdic_unsupported\":%zu,"
               "\"sampled_line_common_fields_refused\":%zu,"
               "\"sampled_line_common_stage_color\":%zu,"
               "\"sampled_line_common_stage_rgb\":%zu,"
               "\"sampled_line_common_stage_transparency\":%zu,"
               "\"sampled_line_common_stage_ltype\":%zu,"
               "\"sampled_line_ltype_zero\":%zu,"
               "\"sampled_line_ltype_negative\":%zu,"
               "\"sampled_line_ltype_nonfinite\":%zu,"
               "\"sampled_line_ltype_parse_refused\":%zu,"
               "\"sampled_line_common_stage_appearance\":%zu,"
               "\"sampled_line_common_stage_geom_bound\":%zu,"
               "\"sampled_line_common_stage_other\":%zu,"
               "\"sampled_line_implicit_paper_xyz_verified\":%zu,"
               "\"sampled_line_implicit_model_xyz_verified\":%zu,"
               "\"sampled_line_graph_attempts\":%zu,"
               "\"sampled_line_graph_verified\":%zu,"
               "\"sampled_line_graph_refused\":%zu,"
               "\"sampled_line_implicit_layer_attempts\":%zu,"
               "\"sampled_line_implicit_layer_verified\":%zu,"
               "\"sampled_line_implicit_layer_refused\":%zu,"
               "\"sampled_line_xdic_present_attempts\":%zu,"
               "\"sampled_line_xdic_present_xyz_verified\":%zu,"
               "\"sampled_line_xdic_present_xyz_refused\":%zu,"
               "\"sampled_line_xdic_hgraph_attempts\":%zu,"
               "\"sampled_line_xdic_hgraph_verified\":%zu,"
               "\"sampled_line_xdic_hgraph_refused\":%zu}",
               hint->header ? "true" : "false",
               hint->physical_map ? "true" : "false",
               hint->object_pages ? "true" : "false",
               hint->handle_pages ? "true" : "false",
               hint->handle_index ? "true" : "false",
               hint->candidate_handles,
               hint->object_section_page_count,
               hint->handle_section_page_count,
               hint->legacy_frame_probe_completed ? "true" : "false",
               hint->legacy_frame_crc_ok,
               hint->legacy_frame_crc_failed,
               hint->raw_type_17_arc,
               hint->raw_type_18_circle,
               hint->raw_type_19_line,
               hint->raw_type_77_lwpolyline,
               hint->continuous_offset_candidate_index ? "true" : "false",
               hint->continuous_candidate_handles,
               hint->continuous_frame_crc_ok,
               hint->continuous_frame_crc_failed,
               hint->page_absolute_candidate_index ? "true" : "false",
               hint->page_absolute_candidate_handles,
               hint->page_absolute_frame_crc_ok,
               hint->page_absolute_frame_crc_failed,
               hint->legacy_offset_distribution_measured ? "true" : "false",
               hint->legacy_distinct_offsets,
               hint->legacy_duplicate_offset_entries,
               hint->legacy_zero_offset_entries,
               hint->legacy_max_offset_multiplicity,
               hint->legacy_distinct_frame_crc_pass,
               hint->legacy_repeated_crc_pass_entries,
               hint->legacy_frame_too_short,
               hint->legacy_frame_invalid_ms,
               hint->legacy_frame_exceeds_section,
               hint->legacy_frame_crc_mismatch,
               hint->legacy_shift_minus_two_crc_pass,
               hint->legacy_shift_minus_one_crc_pass,
               hint->legacy_shift_plus_one_crc_pass,
               hint->legacy_shift_plus_two_crc_pass,
               hint->legacy_shift_samples_tested,
               hint->legacy_next_offset_pairs,
               hint->legacy_next_frame_unavailable,
               hint->legacy_next_frame_exact,
               hint->legacy_next_frame_short,
               hint->legacy_next_frame_overlaps,
               hint->legacy_crc_ok_next_frame_exact,
               hint->legacy_short_gap_1,
               hint->legacy_short_gap_2,
               hint->legacy_short_gap_3,
               hint->legacy_short_gap_4,
               hint->legacy_short_gap_5to16,
               hint->legacy_short_gap_17to64,
               hint->legacy_short_gap_over64,
               hint->mc_frame_probe_completed ? "true" : "false",
               hint->mc_frame_crc_pass,
               hint->mc_frame_crc_mismatch,
               hint->mc_frame_unreadable,
               hint->mc_frame_next_exact,
               hint->mc_byte_len_matches_short_gap,
               hint->mc_bytes_1,
               hint->mc_bytes_2,
               hint->mc_bytes_3,
               hint->mc_bytes_over3,
               hint->mc_ot_decoded,
               hint->mc_ot_unreadable,
               hint->mc_ot_raw_1,
               hint->mc_ot_raw_7,
               hint->mc_ot_raw_17,
               hint->mc_ot_raw_18,
               hint->mc_ot_raw_19,
               hint->mc_ot_raw_44,
               hint->mc_ot_raw_49,
               hint->mc_ot_raw_51,
               hint->mc_ot_raw_77,
               hint->core_frame_verified,
               hint->core_frame_probe_failed,
               hint->core_raw_ot_or_span_mismatch,
               hint->core_payload_bounds_verified,
               hint->core_payload_bounds_rejected,
               hint->core_payload_bounds_mismatch,
               hint->mc_unreadable_invalid_ms,
               hint->mc_unreadable_short_or_boundary,
               hint->mc_unreadable_other,
               hint->mc_unreadable_first_half,
               hint->mc_invalid_ms_zero,
               hint->mc_invalid_ms_one,
               hint->mc_invalid_ms_2to7,
               hint->mc_invalid_ms_extended_zero,
               hint->mc_invalid_ms_extended_reserved,
               hint->mc_invalid_ms_unclassified,
               hint->core_stream_bounds_verified,
               hint->core_stream_bounds_rejected,
               hint->core_stream_bounds_mismatch,
               hint->core_empty_handle_streams,
               hint->sampled_first_h_attempts,
               hint->sampled_first_h_self_matches,
               hint->sampled_first_h_other,
               hint->sampled_first_h_unreadable,
               hint->sampled_first_h_code0_mismatch,
               hint->sampled_first_h_absolute_other_code,
               hint->sampled_first_h_relative_code,
               hint->sampled_first_h_reverse_bytes_matches,
               hint->sampled_first_h_plus_one_matches,
               hint->sampled_first_h_minus_one_matches,
               hint->sampled_first_h_one_byte_values,
               hint->sampled_first_h_in_handle_index,
               hint->sampled_first_h_not_in_handle_index,
               hint->sampled_first_h_same_index_offset,
               hint->sampled_first_h_count2,
               hint->sampled_first_h_count3,
               hint->sampled_first_h_count4to8,
               hint->sampled_scan_best_shift_bits,
               hint->sampled_scan_best_self_matches,
               hint->sampled_h_internally_distinct,
               hint->sampled_h_internal_duplicates,
               hint->sampled_h_successive_pairs,
               hint->sampled_h_successive_increases,
               hint->sampled_h_successive_decreases,
               hint->sampled_h_successive_equals,
               hint->sampled_h_less_than_index,
               hint->sampled_h_greater_than_index,
               hint->sampled_h_index_requires_over3_bytes,
               hint->sampled_h_low8_equal_index,
               hint->sampled_h_low16_equal_index,
               hint->masked6_candidate_index_valid ? "true" : "false",
               hint->masked6_candidate_handle_count,
               hint->masked6_sample_attempts,
               hint->masked6_sample_self_matches,
               hint->masked6_sample_other,
               hint->masked6_sample_unreadable,
               hint->full_embedded_h_census_completed ? "true" : "false",
               hint->full_embedded_h_attempts,
               hint->full_embedded_h_parsed_code0,
               hint->full_embedded_h_other_codes,
               hint->full_embedded_h_unreadable,
               hint->full_embedded_h_self_matches,
               hint->full_embedded_h_less_than_index,
               hint->full_embedded_h_greater_than_index,
               hint->full_embedded_h_neighbor_pairs,
               hint->full_embedded_h_increases,
               hint->full_embedded_h_decreases,
               hint->full_embedded_h_equals,
               hint->core_embedded_h_recovery_attempted ? "true" : "false",
               hint->core_embedded_h_recovery_succeeded ? "true" : "false",
               hint->core_embedded_h_recovery_records,
               hint->core_embedded_h_recovery_original_index_matches,
               hint->sampled_zero_eed_attempts,
               hint->sampled_zero_eed_verified,
               hint->sampled_zero_eed_refused,
               hint->sampled_no_proxy_verified,
               hint->sampled_no_proxy_refused,
               hint->sampled_entity_meta_attempts,
               hint->sampled_entity_meta_verified,
               hint->sampled_entity_meta_refused,
               hint->sampled_owner_layer_attempts,
               hint->sampled_owner_layer_verified,
               hint->sampled_owner_layer_refused,
               hint->sampled_line_attempts,
               hint->sampled_line_common_verified,
               hint->sampled_line_common_refused,
               hint->sampled_line_xyz_verified,
               hint->sampled_line_xyz_refused,
               hint->sampled_line_xyz_owner_layer_joined,
               hint->sampled_line_xyz_distinct_endpoints,
               hint->sampled_line_xyz_bounded_common,
               hint->sampled_line_meta_refused,
               hint->sampled_line_meta_self_refused,
               hint->sampled_line_meta_eed_refused,
               hint->sampled_line_meta_proxy_refused,
               hint->sampled_line_meta_entity_fields_refused,
               hint->sampled_line_eed_chain_attempts,
               hint->sampled_line_eed_chain_verified,
               hint->sampled_line_eed_chain_refused,
               hint->sampled_line_eed_chain_has_applications,
               hint->sampled_line_eed_xyz_attempts,
               hint->sampled_line_eed_xyz_verified,
               hint->sampled_line_eed_xyz_refused,
               hint->sampled_line_eed_xyz_distinct_endpoints,
               hint->sampled_line_eed_join_attempts,
               hint->sampled_line_eed_join_verified,
               hint->sampled_line_eed_join_refused,
               hint->sampled_line_eed_join_explicit_owner,
               hint->sampled_line_eed_join_implicit_layer,
               hint->sampled_line_eed_join_xdictionary,
               hint->sampled_line_eed_appid_attempts,
               hint->sampled_line_eed_appid_verified,
               hint->sampled_line_eed_appid_refused,
               hint->sampled_line_eed_appid_references_verified,
               hint->sampled_line_eed_appid_distinct_references_verified,
               hint->sampled_layer_raw_flags_attempts,
               hint->sampled_layer_raw_flags_verified,
               hint->sampled_layer_raw_flags_refused,
               hint->sampled_layer_flags_refused_index,
               hint->sampled_layer_flags_refused_eed,
               hint->sampled_layer_flags_refused_record,
               hint->sampled_layer_flags_refused_reactors,
               hint->sampled_layer_flags_refused_xdic_bit,
               hint->sampled_layer_flags_refused_numeric,
               hint->sampled_layer_flags_refused_other,
               hint->sampled_layer_raw_flags_frozen_bit,
               hint->sampled_layer_raw_flags_on_bit,
               hint->sampled_layer_raw_flags_locked_bit,
               hint->sampled_layer_raw_flags_unmapped_bits,
               hint->sampled_layer_raw_flags_xdictionary_present,
               hint->sampled_layer_eed_flags_attempts,
               hint->sampled_layer_eed_flags_verified,
               hint->sampled_layer_eed_flags_refused,
               hint->sampled_layer_eed_flags_frozen_bit,
               hint->sampled_layer_eed_flags_on_bit,
               hint->sampled_layer_eed_flags_locked_bit,
               hint->sampled_layer_eed_flags_unmapped_bits,
               hint->sampled_layer_eed_flags_xdictionary_present,
               hint->sampled_layer_eed_appid_attempts,
               hint->sampled_layer_eed_appid_verified,
               hint->sampled_layer_eed_appid_refused,
               hint->sampled_layer_eed_appid_refs_verified,
               hint->sampled_layer_eed_xdic_graph_attempts,
               hint->sampled_layer_eed_xdic_graph_verified,
               hint->sampled_layer_eed_xdic_graph_refused,
               hint->sampled_layer_eed_xdic_refused_identity,
               hint->sampled_layer_eed_xdic_refused_eed,
               hint->sampled_layer_eed_xdic_refused_flags,
               hint->sampled_layer_eed_xdic_refused_appid,
               hint->sampled_layer_eed_xdic_refused_tail,
               hint->sampled_layer_eed_xdic_refused_control_h,
               hint->sampled_layer_eed_xdic_refused_dictionary_h,
               hint->sampled_layer_eed_xdic_refused_target_index,
               hint->sampled_layer_eed_xdic_refused_target_class,
               hint->sampled_layer_eed_xdic_refused_target_crc,
               hint->sampled_layer_eed_xdic_refused_other,
               hint->sampled_layer_eed_xdic_graph_has_tail,
               hint->sampled_line_common_after_meta_refused,
               hint->sampled_line_xyz_after_common_refused,
               hint->sampled_line_common_mode_unsupported,
               hint->sampled_line_common_reactors_unsupported,
               hint->sampled_line_common_xdic_unsupported,
               hint->sampled_line_common_fields_refused,
               hint->sampled_line_common_stage_color,
               hint->sampled_line_common_stage_rgb,
               hint->sampled_line_common_stage_transparency,
               hint->sampled_line_common_stage_ltype,
               hint->sampled_line_ltype_zero,
               hint->sampled_line_ltype_negative,
               hint->sampled_line_ltype_nonfinite,
               hint->sampled_line_ltype_parse_refused,
               hint->sampled_line_common_stage_appearance,
               hint->sampled_line_common_stage_geom_bound,
               hint->sampled_line_common_stage_other,
               hint->sampled_line_implicit_paper_xyz_verified,
               hint->sampled_line_implicit_model_xyz_verified,
               hint->sampled_line_graph_attempts,
               hint->sampled_line_graph_verified,
               hint->sampled_line_graph_refused,
               hint->sampled_line_implicit_layer_attempts,
               hint->sampled_line_implicit_layer_verified,
               hint->sampled_line_implicit_layer_refused,
               hint->sampled_line_xdic_present_attempts,
               hint->sampled_line_xdic_present_xyz_verified,
               hint->sampled_line_xdic_present_xyz_refused,
               hint->sampled_line_xdic_hgraph_attempts,
               hint->sampled_line_xdic_hgraph_verified,
               hint->sampled_line_xdic_hgraph_refused);
    }
    puts("}");
}

int main(int argc, char **argv) {
    if (argc != 2) {
        fputs("usage: real_dwg_native_stage_probe FILE.dwg\n", stderr);
        return 2;
    }
    uint8_t *file = NULL;
    size_t bytes = 0u;
    if (!load_source(argv[1], &file, &bytes)) {
        fputs("real DWG input is inaccessible, too short or exceeds 256 MiB\n",
              stderr);
        return 2;
    }
    const MusaCoreDwgSignature format =
        musa_core_dwg_identify_signature(file, bytes);
    const char *name = signature_name(format);
    const char *result = "not_covered_by_this_native_probe";
    const char *stage = "signature_only";
    size_t count = 0u;
    MusaAc1024CompatibilityHint hint = {0};
    const MusaAc1024CompatibilityHint *hint_for_report = NULL;

    if (format == MUSA_DWG_2000) {
        MusaDwgAc1015Locators loc = {0};
        if (musa_dwg_ac1015_read_locators(file, bytes, &loc) &&
            loc.crc_verified && loc.region_count > 0u) {
            result = "accepted";
            stage = "ac1015_crc_verified_section_locators";
            count = loc.region_count;
        } else {
            result = "rejected";
            stage = "ac1015_section_locators";
        }
    } else if (format == MUSA_DWG_2004) {
        MusaDwgAc1018FileMapHeader header = {0};
        MusaDwgAc1018SectionDescriptor desc = {0};
        size_t locations = 0u, objects = 0u, handles = 0u;
        result = "rejected";
        if (!musa_dwg_ac1018_page_map_header(file, bytes, &header)) {
            stage = "ac1018_page_map_header";
        } else if (!musa_dwg_ac1018_page_locations(
                       file, bytes, NULL, 0u, &locations) || !locations) {
            stage = "ac1018_physical_page_locations";
        } else if (!musa_dwg_ac1018_section_pages(
                       file, bytes, MUSA_DWG_AC1018_OBJECTS,
                       NULL, 0u, &desc, &objects) || !objects) {
            stage = "ac1018_objects_section_pages";
        } else if (!musa_dwg_ac1018_section_pages(
                       file, bytes, MUSA_DWG_AC1018_HANDLES,
                       NULL, 0u, &desc, &handles) || !handles) {
            stage = "ac1018_handles_section_pages";
        } else if (!musa_dwg_ac1018_file_handle_index(
                       file, bytes, NULL, 0u, &count) || !count) {
            stage = "ac1018_crc_backed_handle_index";
            count = 0u;
        } else {
            result = "accepted";
            stage = "ac1018_crc_backed_handle_index";
        }
    } else if (format == MUSA_DWG_2010) {
        hint = ac1024_family_hint(file, bytes);
        hint_for_report = &hint;
        /* The compatibility hint does not authenticate an AC1024 reader.
           Never expose candidate handles as an accepted entity index. */
        result = "not_covered_by_this_native_probe";
        stage = "ac1024_family_hypothesis_only";
        count = 0u;
    } else if (format == MUSA_DWG_2018_2026) {
        MusaDwgAc1032FileMapHeader header = {0};
        size_t locations = 0u;
        if (!musa_dwg_ac1032_page_map_header(file, bytes, &header)) {
            result = "rejected";
            stage = "ac1032_page_map_header";
        } else if (musa_dwg_ac1032_page_locations(
                       file, bytes, NULL, 0u, &locations) && locations > 0u) {
            result = "accepted";
            stage = "ac1032_physical_page_locations";
            count = locations;
            size_t handles = 0u;
            if (musa_dwg_ac1032_handles_index(
                    file, bytes, NULL, 0u, &handles) && handles > 0u) {
                stage = "ac1032_crc_backed_handle_index";
                count = handles;
            }
        } else {
            result = "rejected";
            stage = "ac1032_physical_page_locations";
        }
    } else if (format == MUSA_DWG_UNKNOWN ||
               format == MUSA_DWG_SHORT_HEADER ||
               format == MUSA_DWG_NON_ASCII) {
        result = "rejected";
        stage = "invalid_signature";
    }
    report(name, result, stage, count, hint_for_report);
    free(file);
    return 0;
}
