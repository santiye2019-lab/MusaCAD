/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef MUSACAD_DWG_AC1032_OBJECT_STREAMS_H
#define MUSACAD_DWG_AC1032_OBJECT_STREAMS_H
#include <stddef.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif

/* Immutable bit-space partition from a CRC-authenticated original AC1032
 * object. MC handle_stream_bits INCLUDES its terminal alignment padding;
 * those bits are retained rather than guessed/discarded. An OT code is not
 * proof of LINE/ARC/BLOCK geometry or an owner graph.
 */
typedef struct MusaDwgAc1032ObjectStreams {
    uint64_t indexed_handle;
    uint64_t object_section_offset;
    uint64_t total_payload_bits;
    uint64_t data_stream_bits;
    uint64_t handle_stream_start_bit;
    uint64_t handle_stream_bits;
    uint16_t raw_object_type;
    uint8_t object_type_field_bits;
    uint8_t original_record_crc_verified;
} MusaDwgAc1032ObjectStreams;
enum {
    MUSA_AC1032_STREAM_OBJECT_DATA=0,
    MUSA_AC1032_STREAM_HANDLES=1
};

/* Source-backed logical Objects input with original H index, MS/MC/CRC/OT
 * verification through the existing independently authored common engine.
 * Output never mutates on error; aliases rejected. */
int musa_dwg_ac1032_stream_layout_bytes(
    const uint8_t *objects,size_t objects_size,
    uint64_t handle,uint64_t object_offset,
    MusaDwgAc1032ObjectStreams *out);

/* Full original physical DWG + handle-index + CRC-backed layout proof.
 * No LibreDWG runtime and no Android-specific implementation. */
int musa_dwg_ac1032_stream_layout_file(
    const uint8_t *file,size_t file_bytes,uint64_t handle,
    MusaDwgAc1032ObjectStreams *out);

/* Bounded DWG MSB-first extractor FROM A PREVIOUSLY VERIFIED PAYLOAD COPY.
 * Layout and payload must come from the same record; this pure primitive
 * checks bounds/invariants but cannot re-authenticate arbitrary supplied
 * payload bytes. Returns 1-32 bits in most-significant-bit-first order.
 * Enforces DATA vs HANDLES boundaries (including terminal handle padding).
 * Output remains unchanged for invalid/truncated/overlapping buffers. */
int musa_dwg_ac1032_stream_read_bits(
    const uint8_t *payload,size_t payload_bytes,
    const MusaDwgAc1032ObjectStreams *layout,
    uint8_t stream,size_t relative_bit,unsigned width,
    uint32_t *out);
#ifdef __cplusplus
}
#endif
#endif
