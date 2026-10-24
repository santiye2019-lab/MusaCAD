/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "musacad_dwg_cmc.h"
#include "musacad_dwg_ac1015_values.h"
#include <stdint.h>
#include <stddef.h>
#define CMC_MAX_BYTES (64u*1024u*1024u)
static int overlap(const void *p,size_t pn,const void *q,size_t qn){
    if(!pn||!qn)return 0;
    const uintptr_t a=(uintptr_t)p,b=(uintptr_t)q;
    return a<=b?b-a<pn:a-b<qn;
}
/* CMC is NOT the entity ENC representation. In the supported R2004+
 * ordinary CMC subset the BS field must be 0. The BL is retained as
 * original packed 32-bit RGB+method evidence; the byte flags signal
 * optional Unicode/color-book strings that are deliberately unsupported
 * until the separate original DWG STRINGS data is authenticated.
 *
 * No palette/RGB interpretation is published into the renderer here. */
int musa_dwg_cmc_read_numeric_field(
    const uint8_t *raw,size_t bytes,size_t start,size_t limit,
    MusaDwgCmcField *out){
    if(!raw||!out||!bytes||bytes>CMC_MAX_BYTES||
       bytes>SIZE_MAX/8u||start>limit||
       limit>bytes*8u||overlap(raw,bytes,out,sizeof(*out)))
        return 0;
    MusaDwgAc1015Bits bits={0};
    uint16_t index=0u;uint32_t rgb=0u,flags=0u;
    if(!musa_dwg_ac1015_bits_init(&bits,raw,bytes,start,limit)||
       !musa_dwg_ac1015_bits_read_bs(&bits,&index)||index!=0u||
       !musa_dwg_ac1015_bits_read_bl(&bits,&rgb)||
       !musa_dwg_ac1015_bits_read_u(&bits,8u,&flags)||
       (flags&~3u)!=0u||
       flags!=0u)return 0;
    MusaDwgCmcField found={0};
    found.indexed_color=index;
    found.packed_rgb=rgb;
    found.start_bit=start;
    found.end_bit=bits.bit_pos;
    found.color_name_flag=0u;
    found.book_name_flag=0u;
    found.supported_numeric_only=1u;
    *out=found;
    return 1;
}
