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

#ifdef __cplusplus
}
#endif
#endif
