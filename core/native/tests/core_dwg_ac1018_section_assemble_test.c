/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "musacad_dwg_ac1018_section_assemble.h"
#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#define PAGE 29696u
/* Synthetic full-length R2004 literal page, no original CAD data. */
static size_t literal_page(uint8_t *out,uint8_t value) {
    size_t at=0u,remaining=PAGE-18u;
    out[at++]=0;
    while(remaining>255u){out[at++]=0;remaining-=255u;}
    assert(remaining>=1u&&remaining<=255u);
    out[at++]=(uint8_t)remaining;
    memset(out+at,value,PAGE);at+=PAGE;
    out[at++]=0x11u;
    return at;
}
int main(void) {
    const size_t logical=PAGE+7u;
    uint8_t *file=(uint8_t*)malloc(2u*(PAGE+150u));
    uint8_t *out=(uint8_t*)malloc(logical);
    assert(file&&out);
    const size_t first=literal_page(file,'X');
    const size_t second=literal_page(file+first,'Y');
    MusaDwgAc1018PageInput pages[2]={
        {0u,first,0u,1u},{first,second,PAGE,1u}
    };
    memset(out,0xa5,logical);
    assert(musa_dwg_ac1018_assemble_section(
        file,first+second,pages,2u,out,logical));
    for(size_t i=0;i<PAGE;++i)assert(out[i]=='X');
    for(size_t i=PAGE;i<logical;++i)assert(out[i]=='Y');
    memset(out,0xa5,logical);
    pages[1].header_verified=0u;
    assert(!musa_dwg_ac1018_assemble_section(
        file,first+second,pages,2u,out,logical));
    pages[1].header_verified=1u;
    pages[1].logical_offset=PAGE-1u;
    assert(!musa_dwg_ac1018_assemble_section(
        file,first+second,pages,2u,out,logical));
    pages[1].logical_offset=PAGE;
    assert(!musa_dwg_ac1018_assemble_section(
        file,first+second,pages,1u,out,logical));
    assert(!musa_dwg_ac1018_assemble_section(
        file,first+second,pages,2u,out,PAGE-1u));
    pages[1].packed_length=second-1u;
    assert(!musa_dwg_ac1018_assemble_section(
        file,first+second,pages,2u,out,logical));
    pages[1].packed_length=second;
    pages[1].packed_offset=first+second+1u;
    assert(!musa_dwg_ac1018_assemble_section(
        file,first+second,pages,2u,out,logical));
    pages[1].packed_offset=first;
    for(size_t i=0;i<logical;++i)assert(out[i]==0xa5);
    free(file);free(out);return 0;
}
