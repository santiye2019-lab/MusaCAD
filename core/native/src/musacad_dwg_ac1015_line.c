/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "musacad_dwg_ac1015_line.h"
#include "musacad_dwg_ac1015_values.h"
#include <stddef.h>
#include <stdint.h>

/* Original, bounded R2000 LINE geometry field grammar.
 * Open Design Specification, LINE R2000+: zero-Z flag, X RD/DD,
 * Y RD/DD, optional Z RD/DD, thickness BT, extrusion BE.
 * The common entity header and handle-reference stream are NOT decoded.
 *
 * An external trusted caller provides a verified, independently derived
 * entity-specific start bit. A guessed offset can accidentally decode
 * plausible coordinates; do not treat this API as full DWG acceptance.
 */
int musa_dwg_ac1015_decode_line_fields(
    const MusaDwgAc1015ObjectRecord *record,
    size_t start_bit,
    MusaDwgAc1015LineFields *out){
    if(!record||!out||!record->data||
       record->type_code!=19u||record->record_crc_verified!=1||
       record->indexed_handle==0u||record->data_bytes==0u||
       record->data_bytes>SIZE_MAX/8u||
       (size_t)record->pre_handle_bits>record->data_bytes*8u||
       start_bit>record->pre_handle_bits)return 0;

    MusaDwgAc1015Bits c={0};
    if(!musa_dwg_ac1015_bits_init(&c,
        record->data,record->data_bytes,
        start_bit,(size_t)record->pre_handle_bits))return 0;

    MusaDwgAc1015LineFields next={0};
    next.indexed_handle=record->indexed_handle;
    next.first_field_bit=start_bit;
    uint32_t zero_z=0u;
    if(!musa_dwg_ac1015_bits_read_u(&c,1u,&zero_z)||
       !musa_dwg_ac1015_bits_read_rd(&c,&next.start[0])||
       !musa_dwg_ac1015_bits_read_dd(&c,next.start[0],&next.end[0])||
       !musa_dwg_ac1015_bits_read_rd(&c,&next.start[1])||
       !musa_dwg_ac1015_bits_read_dd(&c,next.start[1],&next.end[1]))return 0;
    if(!zero_z){
        if(!musa_dwg_ac1015_bits_read_rd(&c,&next.start[2])||
           !musa_dwg_ac1015_bits_read_dd(&c,next.start[2],&next.end[2]))return 0;
    }
    if(!musa_dwg_ac1015_bits_read_bt(&c,&next.thickness)||
       !musa_dwg_ac1015_bits_read_be(&c,next.extrusion)||
       c.bit_pos!=(size_t)record->pre_handle_bits)return 0;
    next.end_field_bit=c.bit_pos;
    next.field_layout_decoded=1;
    *out=next;
    return 1;
}
