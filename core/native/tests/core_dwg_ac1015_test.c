/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "musacad_dwg_ac1015.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

static const uint8_t sentinel[16]={
    0x95u,0xa0u,0x4eu,0x28u,0x99u,0x82u,0x1au,0xe5u,
    0x5eu,0x41u,0xe0u,0x5fu,0x9du,0x3au,0x4du,0x00u
};
static void put_le(uint8_t *x,uint32_t value){
    x[0]=(uint8_t)value;x[1]=(uint8_t)(value>>8);
    x[2]=(uint8_t)(value>>16);x[3]=(uint8_t)(value>>24);
}
static size_t build(uint8_t *x,size_t n,unsigned count){
    assert(n>=512u&&count>=3u&&count<=5u);
    memset(x,0,n);
    memcpy(x,"AC1015",6);
    x[12]=1u;
    put_le(x+0x0du,256u); /* preview seeker is diagnostic, unverified */
    x[0x13u]=0xe4u; x[0x14u]=0x04u;
    put_le(x+0x15u,count);
    const uint32_t starts[]={128u,224u,304u,384u,448u};
    const uint32_t lengths[]={52u,38u,64u,32u,52u};
    for(unsigned i=0u;i<count;i++){
        size_t k=0x19u+(size_t)i*9u;
        x[k]=(uint8_t)i;
        put_le(x+k+1u,starts[i]);
        put_le(x+k+5u,lengths[i]);
    }
    size_t crc_pos=0x19u+(size_t)count*9u;
    /* Independently calculated R2000 header golden checksums for fixtures. */
    static const uint16_t golden[]={0xe82fu,0x9e95u,0x3b17u};
    const uint16_t crc=golden[count-3u];
    x[crc_pos]=(uint8_t)crc;x[crc_pos+1u]=(uint8_t)(crc>>8);
    memcpy(x+crc_pos+2u,sentinel,16u);
    return crc_pos+18u;
}
static void fail_unchanged(const uint8_t *bytes,size_t n,
                           MusaDwgAc1015Locators *out){
    if(!out){
        assert(!musa_dwg_ac1015_read_locators(bytes,n,NULL));
        return;
    }
    MusaDwgAc1015Locators original=*out;
    assert(!musa_dwg_ac1015_read_locators(bytes,n,out));
    assert(memcmp(out,&original,sizeof(original))==0);
}
int main(void){
    uint8_t fixture[512],original[512];
    for(unsigned records=3;records<=5;records++){
        size_t end=build(fixture,sizeof(fixture),records);
        memcpy(original,fixture,sizeof(fixture));
        MusaDwgAc1015Locators loc={0};
        assert(musa_dwg_ac1015_read_locators(fixture,sizeof(fixture),&loc));
        assert(loc.region_count==records&&loc.header_end==end);
        static const uint16_t golden[]={0xe82fu,0x9e95u,0x3b17u};
        assert(loc.codepage==1252u);
        assert(loc.stored_header_crc==golden[records-3u]);
        assert(loc.image_seeker_unverified==256u);
        assert(loc.crc_verified==1); /* Actual header checksum verified */
        for(unsigned i=0;i<records;i++){
            assert(loc.regions[i].section_kind==i);
            assert(loc.regions[i].offset>=end);
            assert(loc.regions[i].length>0u);
        }
        assert(memcmp(original,fixture,sizeof(fixture))==0);
        fail_unchanged(fixture,end-1u,&loc);
        fail_unchanged(fixture,0x18u,&loc);

        fixture[0]='M';fail_unchanged(fixture,sizeof(fixture),&loc);
        fixture[0]='A';fixture[1]='C';fixture[2]='1';
        /* Genuine newer AC1032 has a different format, no fallthrough. */
        memcpy(fixture,"AC1032",6);
        fail_unchanged(fixture,sizeof(fixture),&loc);
        memcpy(fixture,"AC1015",6);

        fixture[end-1u]^=0x01u; /* damaged 16-byte sentinel */
        fail_unchanged(fixture,sizeof(fixture),&loc);
        fixture[end-1u]^=0x01u;
        fixture[0x15u]=6u; /* undocumented sixth locator: fail closed */
        fail_unchanged(fixture,sizeof(fixture),&loc);
        fixture[0x15u]=(uint8_t)records;
        fixture[0x19u+9u]=0u; /* duplicate 0, required class missing */
        fail_unchanged(fixture,sizeof(fixture),&loc);
        fixture[0x19u+9u]=1u;
        fixture[0x19u+9u*2u]=4u; /* missing mandatory object-map 2 */
        fail_unchanged(fixture,sizeof(fixture),&loc);
        fixture[0x19u+9u*2u]=2u;
        fixture[0x19u]=5u; /* unknown record id */
        fail_unchanged(fixture,sizeof(fixture),&loc);
        fixture[0x19u]=0u;
        put_le(fixture+0x19u+1u,0u); /* section overlaps file header */
        fail_unchanged(fixture,sizeof(fixture),&loc);
        put_le(fixture+0x19u+1u,128u);
        put_le(fixture+0x19u+5u,0u); /* zero section */
        fail_unchanged(fixture,sizeof(fixture),&loc);
        put_le(fixture+0x19u+5u,52u);
        put_le(fixture+0x19u+1u,510u);
        fail_unchanged(fixture,sizeof(fixture),&loc);
        put_le(fixture+0x19u+1u,128u);
        put_le(fixture+0x19u+9u+1u,140u); /* crossing sections */
        fail_unchanged(fixture,sizeof(fixture),&loc);
        put_le(fixture+0x19u+9u+1u,224u);
        put_le(fixture+0x19u+1u,0xfffffff0u);
        fail_unchanged(fixture,sizeof(fixture),&loc);
        put_le(fixture+0x19u+1u,128u);
        /* Unlike the earlier structural-only reader, a forged CRC fails. */
        fixture[end-18u]^=0xffu;
        fail_unchanged(fixture,sizeof(fixture),&loc);
        fixture[end-18u]^=0xffu;
        /* Changing an otherwise valid source byte must also be detected. */
        fixture[0x13u]^=0x01u;
        fail_unchanged(fixture,sizeof(fixture),&loc);
        fixture[0x13u]^=0x01u;
        assert(musa_dwg_ac1015_read_locators(fixture,sizeof(fixture),&loc));
        assert(loc.crc_verified==1);
    }
    MusaDwgAc1015Locators loc={0};
    fail_unchanged(NULL,512u,&loc);
    fail_unchanged(fixture,512u,NULL);
    puts("PASS: first-party AC1015 raw file section table extraction, bounds, sentinel and fail-closed output");
    return 0;
}
