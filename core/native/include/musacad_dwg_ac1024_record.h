/* SPDX-License-Identifier: MIT
 * Original MusaCAD first-party R2010/AC1024 record envelope only.
 * No LibreDWG/third-party code, no CAD geometry or writer.
 */
#ifndef MUSACAD_DWG_AC1024_RECORD_H
#define MUSACAD_DWG_AC1024_RECORD_H
#include <stddef.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif

typedef struct MusaDwgAc1024RecordFrame {
    uint64_t indexed_object_offset;
    size_t framed_bytes;      /* MS + unsigned MC + object bytes + CRC16 */
    size_t ms_object_bytes;   /* recorded object payload byte count */
    uint64_t handle_stream_bits; /* raw unsigned MC value, NOT handle graph */
    uint16_t raw_object_type;  /* R2010 OT DECLARATION, NOT resolved class */
    uint8_t ms_marker_bytes;
    uint8_t mc_marker_bytes;
    uint8_t original_crc16_verified;
} MusaDwgAc1024RecordFrame;

/* Read only ONE R2010 object frame from an independently authenticated
 * uncompressed AcDb:AcDbObjects section and caller-supplied CRC-backed
 * handle-map OFFSET. The function cannot authenticate an entire source
 * drawing, its index, embedded self-handle, owner/layer/block, geometry,
 * drawing appearance, text contents, or write any DWG.
 *
 * Strictly bounds MS length, unsigned MC handle-bit-count marker, R2010 OT
 * selector and trailing CRC16/ARC seeded 0xc0c1. The MC encoded bytes are
 * not counted in the R2010 MS body size. Only publish output on COMPLETE
 * success; reject output aliasing with source, leave input untouched.
 *
 * This is a portable first-party C11 record evidence API, NOT a normal-open
 * admission, preview decoder, or AutoCAD equivalence assertion. */
int musa_dwg_ac1024_record_probe(
    const uint8_t *objects, size_t object_bytes, uint64_t indexed_offset,
    MusaDwgAc1024RecordFrame *out);


/* Immutable bounded DATA-stream locations after full record CRC16 + raw OT.
 * Offsets are into the caller's independently verified, decompressed
 * AcDbObjects buffer, not into the original DWG file. No pointer is returned.
 * type_end_bit is the first bit AFTER OT, relative to payload_start_byte.
 * payload_end_bit is the exclusive end of the whole object payload; it is
 * NOT a verified common-entity header end or LINE field beginning.
 * R2010's separate text/handle streams are NOT interpreted here.
 * This function does NOT decode a point, line, entity, layer or block. */
typedef struct MusaDwgAc1024DataBounds {
    uint64_t indexed_object_offset;
    size_t payload_start_byte;
    size_t payload_bytes;
    size_t type_end_bit;
    size_t payload_end_bit;
    uint16_t raw_object_type;
    uint8_t record_crc16_verified;
} MusaDwgAc1024DataBounds;

/* Atomic, read-only; output must be disjoint from the source. Only accepts
 * records that the original first-party record_probe fully verifies.
 * This is the safe entry to a future AC1024 common-entity stream reader,
 * NOT authority to guess geometry starting at type_end_bit. */
int musa_dwg_ac1024_record_data_bounds(
    const uint8_t *objects, size_t object_bytes, uint64_t indexed_offset,
    MusaDwgAc1024DataBounds *out);


/* R2010 declared handle-stream window, bit offsets relative to payload.
 * MC is the stream's bit length (INCLUDING its final padding bits).
 * The stream begins at payload_bits - declared_handle_stream_bits.
 * It must be disjoint from the already-decoded OT bit prefix.
 * This does NOT decode self-H, owner/layer links, or handle references.
 * Source untouched; output atomic; caller must independently validate
 * the candidate offset against the protected section handle map. */
typedef struct MusaDwgAc1024StreamBounds {
    uint64_t indexed_object_offset;
    size_t payload_start_byte;
    size_t data_after_ot_bit;
    size_t handle_stream_start_bit;
    size_t handle_stream_end_bit;
    uint64_t handle_stream_bits;
    uint16_t raw_object_type;
    uint8_t record_crc16_verified;
} MusaDwgAc1024StreamBounds;

/* Return 1 only when full MS+MC+OT+CRC16 frame is authenticated and
 * the declared handle stream fits entirely AFTER the OT prefix.
 * Return 0 without modifying *out on any invalid or aliased output. */
int musa_dwg_ac1024_record_stream_bounds(
    const uint8_t *objects, size_t object_bytes, uint64_t indexed_offset,
    MusaDwgAc1024StreamBounds *out);

#ifdef __cplusplus
}
#endif
#endif
