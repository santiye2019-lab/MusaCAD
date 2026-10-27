/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef MUSACAD_DWG_AC1032_SOURCE_UTF8_H
#define MUSACAD_DWG_AC1032_SOURCE_UTF8_H
#include <stddef.h>
#include <stdint.h>

/* Bounded, strictly terminated UTF-8 scalar validation for independently
 * CRC-authenticated R2018 OT51/OT1/OT44 source strings. A valid scalar
 * sequence is NOT a font, display, layer visibility or paint decision.
 * No reads beyond capacity; reject embedded NUL, overlong encodings,
 * surrogate code points, truncation and out-of-range scalars. */
static inline int musa_dwg_ac1032_source_utf8_valid(
    const char *text,size_t n,size_t capacity,int allow_empty){
    if(!text||n>=capacity||(!allow_empty&&!n))return 0;
    size_t i=0u;
    while(i<n){
        const unsigned char lead=(unsigned char)text[i];
        if(!lead)return 0;
        if(lead<0x80u){i++;continue;}
        uint32_t scalar=0u,min_scalar=0u;
        size_t tails=0u;
        if(lead>=0xc2u&&lead<=0xdfu){
            scalar=(uint32_t)(lead&0x1fu);min_scalar=0x80u;tails=1u;
        }else if(lead>=0xe0u&&lead<=0xefu){
            scalar=(uint32_t)(lead&0x0fu);min_scalar=0x800u;tails=2u;
        }else if(lead>=0xf0u&&lead<=0xf4u){
            scalar=(uint32_t)(lead&0x07u);min_scalar=0x10000u;tails=3u;
        }else return 0;
        if(tails>n-i-1u)return 0;
        for(size_t k=1u;k<=tails;k++){
            const unsigned char tail=(unsigned char)text[i+k];
            if((tail&0xc0u)!=0x80u)return 0;
            scalar=(scalar<<6u)|(uint32_t)(tail&0x3fu);
        }
        if(scalar<min_scalar||scalar>0x10ffffu||
           (scalar>=0xd800u&&scalar<=0xdfffu))return 0;
        i+=tails+1u;
    }
    return text[n]=='\0';
}
#endif
