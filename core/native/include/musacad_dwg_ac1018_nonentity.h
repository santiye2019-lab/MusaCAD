/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef MUSACAD_DWG_AC1018_NONENTITY_H
#define MUSACAD_DWG_AC1018_NONENTITY_H
#include <stddef.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
/* R2004 AC1018 non-entity common header, read-only first-party C11.
 * This is only an identity PREFIX proof, not an AcDb class/owner graph.
 * Type is the record's numerical BS declaration; it is NOT CLASSES proof.
 * Callers must separately validate layer/owner/XDictionary associations.
 */
typedef struct MusaDwgAc1018NonEntityIdentity {
    uint64_t indexed_handle;
    size_t record_bytes;
    uint32_t pre_handle_bits;
    uint32_t reactor_count;
    uint32_t eed_skipped_bytes;
    uint16_t declared_type;
    uint8_t xdictionary_missing;
    uint8_t record_crc_verified;
    uint8_t embedded_self_verified;
} MusaDwgAc1018NonEntityIdentity;
/* Validate one MS-framed record at the indexed object offset: CRC16/ARC,
 * BS type, RL pre-handle boundary, absolute H code 0 matching indexed ID,
 * bounded EED, reactor BL and R2004 xdictionary-missing bit.
 * Only fixed nonentity types 42/49/51 are currently supported. They are
 * DICTIONARY/BLOCK_HEADER/LAYER numerical claims, NOT verified classes.
 * Reject unsupported types, malformed payload, CRC mismatch, wrong self,
 * aliasing, or out of range. Preserve *out unchanged on every error.
 * A valid common prefix is NOT a complete semantic object decoder.
 */
int musa_dwg_ac1018_nonentity_identity_record(
    const uint8_t *objects,size_t object_bytes,size_t object_offset,
    uint64_t indexed_handle,MusaDwgAc1018NonEntityIdentity *out);

/* First-party bounded R2004/AC1018 LAYER (numerical type 51) DATA PREFIX:
 * ASCII-only TV name, referenced bit, xref index+1, xref-dependent bit and
 * raw BS flags. Does NOT parse CMC color, table-control/linetype/plotstyle H,
 * complete AcDb class/ownership graph, or mixed-codepage text. Unsupported
 * encoding/record layout FAILS CLOSED; no metadata is silently guessed.
 * Source bytes and caller's output remain unchanged on any error.
 */
typedef struct MusaDwgAc1018LayerPrefix {
    uint64_t indexed_handle;
    char name[256]; /* NUL-terminated conservative ASCII-only subset */
    uint16_t name_bytes;
    uint16_t xref_index_plus_one;
    uint16_t raw_layer_flags;
    uint8_t ever_referenced;
    uint8_t xref_dependent;
    uint8_t record_crc_verified;
    uint8_t embedded_self_verified;
} MusaDwgAc1018LayerPrefix;
int musa_dwg_ac1018_layer_prefix_record(
    const uint8_t *objects,size_t object_bytes,size_t object_offset,
    uint64_t indexed_handle,MusaDwgAc1018LayerPrefix *out);

/* Verified R2004 LAYER pre-handle CMC data, first-party C11 subset.
 * Verifies CMC BS color index is 0 (unlike ENC), preserves BL raw RGB
 * value and RC flags with optional
 * printable ASCII color/book TV strings. Unsupported bit layouts and
 * non-ASCII name encodings fail closed. This does NOT establish final
 * on-screen RGB appearance, color book mapping, remaining H target
 * identities, or complete AcDb layer meaning.
 * Transactional and read-only: caller result unchanged on any error. */
typedef struct MusaDwgAc1018LayerColor {
    MusaDwgAc1018LayerPrefix prefix;
    uint32_t raw_cmc_rgb;
    uint16_t raw_cmc_index; /* verified zero for R2004 CMC (not ENC) */
    char color_name[256];
    char book_name[256];
    uint8_t has_color_name;
    uint8_t has_book_name;
    uint8_t cmc_data_bits_verified;
} MusaDwgAc1018LayerColor;
int musa_dwg_ac1018_layer_color_record(
    const uint8_t *objects,size_t object_bytes,size_t object_offset,
    uint64_t indexed_handle,MusaDwgAc1018LayerColor *out);
#ifdef __cplusplus
}
#endif
#endif
