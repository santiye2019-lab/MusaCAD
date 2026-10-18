/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef MUSACAD_CRC16_H
#define MUSACAD_CRC16_H
#include <stddef.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
/* Reflected CRC-16/ARC, polynomial 0xA001. Independently written,
 * no borrowed CRC lookup table. Caller supplies seed for chunked updates.
 * The R2000 file-header caller uses seed 0 and count-specific XOR masks.
 */
int musa_core_crc16_arc(uint16_t seed,const uint8_t *bytes,size_t count,
                        uint16_t *out);
#ifdef __cplusplus
}
#endif
#endif
