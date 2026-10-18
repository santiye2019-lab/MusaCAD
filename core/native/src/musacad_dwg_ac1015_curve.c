/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "musacad_dwg_ac1015_curve.h"
#include "musacad_dwg_ac1015_values.h"
#include <stddef.h>
#include <stdint.h>
/* ODA R2000:
 * CIRCLE 18: center 3BD, radius BD, thickness BT, extrusion BE.
 * ARC 17: same fields followed by start/end angles BD in radians.
 * Common entity metadata and handle stream are not parsed here. */
int musa_dwg_ac1015_decode_curve_fields(
    const MusaDwgAc1015ObjectRecord *r,size_t first,
    MusaDwgAc1015CurveFields *out){
    if(!r||!out||!r->data||
       (r->type_code!=17u&&r->type_code!=18u)||
       r->record_crc_verified!=1||r->indexed_handle==0u||
       !r->data_bytes||r->data_bytes>SIZE_MAX/8u||
       (size_t)r->pre_handle_bits>r->data_bytes*8u||
       first>r->pre_handle_bits)return 0;
    MusaDwgAc1015Bits b={0};
    if(!musa_dwg_ac1015_bits_init(&b,r->data,r->data_bytes,
                                   first,r->pre_handle_bits))return 0;
    MusaDwgAc1015CurveFields next={0};
    next.indexed_handle=r->indexed_handle;
    next.type_code=r->type_code;
    next.first_field_bit=first;
    for(unsigned i=0u;i<3u;i++){
        if(!musa_dwg_ac1015_bits_read_bd(&b,&next.center[i]))return 0;
    }
    if(!musa_dwg_ac1015_bits_read_bd(&b,&next.radius)||
       next.radius<=0.0||
       !musa_dwg_ac1015_bits_read_bt(&b,&next.thickness)||
       !musa_dwg_ac1015_bits_read_be(&b,next.extrusion))return 0;
    if(r->type_code==17u){
        if(!musa_dwg_ac1015_bits_read_bd(&b,&next.start_angle)||
           !musa_dwg_ac1015_bits_read_bd(&b,&next.end_angle))return 0;
    }
    if(b.bit_pos!=r->pre_handle_bits)return 0;
    next.end_field_bit=b.bit_pos;
    next.field_layout_decoded=1;
    *out=next;
    return 1;
}
