/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "musacad_dwg_ac1015_common.h"
#include "musacad_dwg_ac1015_values.h"
#include <stddef.h>
#include <stdint.h>
/* R2000 common ENTITY prefix, documented Open Design Specification:
 * BS type, RL endbit, H self-handle, EED BS length, graphic B,
 * nolinks B, color CMC/BS, linetype scale BD, 2-bit ltype/plotstyle,
 * visibility BS and lineweight RC.
 * STRICT SUBSET: EED must be zero, graphics absent. Other valid forms
 * are deferred, not silently treated as those simple fields. */
int musa_dwg_ac1015_read_simple_common(
    const MusaDwgAc1015ObjectRecord *r,
    MusaDwgAc1015SimpleCommon *out){
    if(!r||!out||!r->data||!r->record_crc_verified||
       !(r->type_code==17u||r->type_code==18u||r->type_code==19u)||
       !r->indexed_handle||r->data_bytes>SIZE_MAX/8u||
       (size_t)r->pre_handle_bits>r->data_bytes*8u)return 0;
    MusaDwgAc1015Bits c={0};
    if(!musa_dwg_ac1015_bits_init(&c,r->data,r->data_bytes,
                                  0u,(size_t)r->pre_handle_bits))return 0;
    uint16_t kind=0u,eed=0u,aci=0u,invisible=0u;
    uint32_t endbit=0u,code=0u,count=0u,part=0u,graphic=0u;
    uint32_t nolinks=0u,ltf=0u,plot=0u,weight=0u;
    uint64_t self=0u;
    double scale=0.0;
    if(!musa_dwg_ac1015_bits_read_bs(&c,&kind)||
       !musa_dwg_ac1015_bits_read_rl(&c,&endbit)||
       kind!=r->type_code||endbit!=r->pre_handle_bits||
       !musa_dwg_ac1015_bits_read_u(&c,4u,&code)||
       !musa_dwg_ac1015_bits_read_u(&c,4u,&count)||
       code!=0u||count==0u||count>8u)return 0;
    for(uint32_t i=0u;i<count;i++){
        if(!musa_dwg_ac1015_bits_read_u(&c,8u,&part))return 0;
        self=(self<<8u)|(uint64_t)part;
    }
    /* H self-handle must equal validated object-map index handle. */
    if(self!=r->indexed_handle||
       !musa_dwg_ac1015_bits_read_bs(&c,&eed)||
       eed!=0u|| /* EED payload requires an independent parser. */
       !musa_dwg_ac1015_bits_read_u(&c,1u,&graphic)||
       graphic!=0u|| /* Preview bytes must not be misread as entity data. */
       !musa_dwg_ac1015_bits_read_u(&c,1u,&nolinks)||
       !musa_dwg_ac1015_bits_read_bs(&c,&aci)||
       aci>256u||
       !musa_dwg_ac1015_bits_read_bd(&c,&scale)||
       scale<=0.0||
       !musa_dwg_ac1015_bits_read_u(&c,2u,&ltf)||
       !musa_dwg_ac1015_bits_read_u(&c,2u,&plot)||
       !musa_dwg_ac1015_bits_read_bs(&c,&invisible)||
       !musa_dwg_ac1015_bits_read_u(&c,8u,&weight)||
       c.bit_pos>r->pre_handle_bits)return 0;
    /* The handle graph and reference stream are deliberately not read:
     * callers MUST NOT mark a complete real DWG geometry object decoded. */
    MusaDwgAc1015SimpleCommon next={0};
    next.indexed_handle=r->indexed_handle;
    next.type_code=kind;next.color_index=aci;
    next.invisibility=invisible;next.nolinks=(uint8_t)nolinks;
    next.linetype_flags=(uint8_t)ltf;
    next.plotstyle_flags=(uint8_t)plot;
    next.lineweight=(uint8_t)weight;
    next.linetype_scale=scale;
    next.geometric_start_bit=c.bit_pos;next.prefix_decoded=1;
    *out=next;
    return 1;
}
