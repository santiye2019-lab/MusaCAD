/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef MUSACAD_DWG_AC1015_VALUES_H
#define MUSACAD_DWG_AC1015_VALUES_H
#include <stddef.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
/* Bounded, read-only R2000 bit stream. No Android or third-party dependency.
 * All readers preserve cursor AND output on failure. bit_end is exclusive.
 * This is a FIELD decoder, not an entity or geometry acceptance API. */
typedef struct MusaDwgAc1015Bits {
    const uint8_t *data;
    size_t size_bytes;
    size_t bit_pos;
    size_t bit_end;
} MusaDwgAc1015Bits;

int musa_dwg_ac1015_bits_init(MusaDwgAc1015Bits *out,
    const uint8_t *data,size_t bytes,size_t start_bit,size_t end_bit);
int musa_dwg_ac1015_bits_read_u(MusaDwgAc1015Bits *c,unsigned bits,uint32_t *out);
int musa_dwg_ac1015_bits_read_rl(MusaDwgAc1015Bits *c,uint32_t *out);
int musa_dwg_ac1015_bits_read_bs(MusaDwgAc1015Bits *c,uint16_t *out);
int musa_dwg_ac1015_bits_read_bl(MusaDwgAc1015Bits *c,uint32_t *out);
int musa_dwg_ac1015_bits_read_rd(MusaDwgAc1015Bits *c,double *out);
int musa_dwg_ac1015_bits_read_bd(MusaDwgAc1015Bits *c,double *out);
int musa_dwg_ac1015_bits_read_dd(MusaDwgAc1015Bits *c,double base,double *out);
int musa_dwg_ac1015_bits_read_bt(MusaDwgAc1015Bits *c,double *out);
int musa_dwg_ac1015_bits_read_be(MusaDwgAc1015Bits *c,double out_xyz[3]);
#ifdef __cplusplus
}
#endif
#endif
