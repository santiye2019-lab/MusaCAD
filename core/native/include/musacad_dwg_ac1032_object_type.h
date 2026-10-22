/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef MUSACAD_DWG_AC1032_OBJECT_TYPE_H
#define MUSACAD_DWG_AC1032_OBJECT_TYPE_H
#include <stddef.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
/* R2010+ OT on the ORIGINAL verified AC1032 object data stream:
 * 00 => raw byte; 01 => raw byte + 0x1f0; 10 => raw little-endian
 * short; 11 => reserved and deliberately rejected.
 * This is a raw class/type NUMBER, not a full AcDbObject/geometry decode.
 */
typedef struct MusaDwgAc1032ObjectType {
    uint64_t indexed_handle;
    uint64_t object_section_offset;
    uint16_t type_code;
    uint8_t ot_opcode;
    uint8_t type_field_bits;
    uint8_t record_crc_verified;
} MusaDwgAc1032ObjectType;
/* Caller supplies original logical AcDb:AcDbObjects bytes extracted
 * from the authenticated AC1032 file and indexed H source location.
 * Reuses the checked MS/MC/CRC record envelope. Source and output
 * never change upon failure, aliases rejected. C11 for all platforms.
 */
int musa_dwg_ac1032_object_type_bytes(
    const uint8_t *logical_objects,size_t logical_bytes,
    uint64_t indexed_handle,uint64_t object_section_offset,
    MusaDwgAc1032ObjectType *out);
/* End-to-end original AC1032 file handle lookup + physical pages + H
 * index + CRC record validation + raw R2010+ OT. NOT any renderable
 * entity or full class/owner/proxy interpretation. */
int musa_dwg_ac1032_object_type_file(
    const uint8_t *file,size_t file_bytes,uint64_t indexed_handle,
    MusaDwgAc1032ObjectType *out);
#ifdef __cplusplus
}
#endif
#endif
