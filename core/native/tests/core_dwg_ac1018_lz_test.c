/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "musacad_dwg_ac1018_lz.h"
#include <assert.h>
#include <stdint.h>
#include <string.h>
/* All streams are independently synthesized. No user DWG bytes stored. */
static void valid(void){
    const uint8_t literals[]={1,'A','B','C','D',0x11,0,0};
    char out[50]={0};
    size_t read=222u;
    assert(musa_dwg_ac1018_lz_decode(literals,sizeof(literals),
                                      (uint8_t*)out,4u,&read));
    assert(memcmp(out,"ABCD",4u)==0 && read==6u);
    const uint8_t overlap[]={1,'A','B','C','D',0x40,0,0x11};
    read=222u;
    assert(musa_dwg_ac1018_lz_decode(overlap,sizeof(overlap),
                                      (uint8_t*)out,7u,&read));
    assert(memcmp(out,"ABCDDDD",7u)==0 && read==8u);
    const uint8_t post[]={1,'A','B','C','D',0x41,0,'Z',0x11};
    assert(musa_dwg_ac1018_lz_decode(post,sizeof(post),
                                      (uint8_t*)out,8u,&read));
    assert(memcmp(out,"ABCDDDDZ",8u)==0);
    const uint8_t long_copy[]={1,'A','B','C','D',0x20,1,0,0,0x11};
    assert(musa_dwg_ac1018_lz_decode(long_copy,sizeof(long_copy),
                                      (uint8_t*)out,38u,&read));
    assert(memcmp(out,"ABCD",4u)==0);
    for(int i=4;i<38;i++)assert(out[i]=='D');
    uint8_t long_literal[22]={0,1};
    memset(long_literal+2u,'Q',19u);long_literal[21]=0x11;
    assert(musa_dwg_ac1018_lz_decode(long_literal,sizeof(long_literal),
                                      (uint8_t*)out,19u,&read));
    for(int i=0;i<19;i++)assert(out[i]=='Q');
}
static void corrupt(void){
    const uint8_t copy[]={1,'A','B','C','D',0x40,0,0x11};
    char out[7]={'U','N','T','O','U','C','H'};
    const char original[7]={'U','N','T','O','U','C','H'};
    size_t consumed=777u;
    for(size_t n=0u;n<sizeof(copy);n++){
        assert(!musa_dwg_ac1018_lz_decode(copy,n,(uint8_t*)out,7u,&consumed));
        assert(memcmp(out,original,7u)==0 && consumed==777u);
    }
    const uint8_t invalid_back[]={1,'A','B','C','D',0x40,0x7fu,0x11};
    assert(!musa_dwg_ac1018_lz_decode(invalid_back,sizeof(invalid_back),
                                       (uint8_t*)out,7u,&consumed));
    assert(!musa_dwg_ac1018_lz_decode(copy,sizeof(copy),
                                       (uint8_t*)out,6u,&consumed));
    assert(!musa_dwg_ac1018_lz_decode(copy,sizeof(copy),
                                       (uint8_t*)out,0u,&consumed));
    assert(!musa_dwg_ac1018_lz_decode(copy,sizeof(copy),
                                       (uint8_t*)out,1024u*1024u+1u,&consumed));
    assert(memcmp(out,original,7u)==0 && consumed==777u);
}
int main(void){valid();corrupt();return 0;}
