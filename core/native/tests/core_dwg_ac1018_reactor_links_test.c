/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "musacad_dwg_ac1018_reactor_links.h"
#include "musacad_crc16.h"
#include <assert.h>
#include <stdint.h>
#include <string.h>
/* Every fixture is generated here, not copied from a user's DWG.
 * The payload includes only a synthetic common header + dummy geometry.
 * This test does NOT assert decoded LINE vertex coordinates. */
typedef struct Writer {
    uint8_t bytes[128];
    size_t bits;
} Writer;
static void put(Writer *w,unsigned n,uint32_t v){
    assert(w&&n<=32u&&w->bits+n<sizeof(w->bytes)*8u);
    for(unsigned i=0u;i<n;i++){
        const unsigned shift=n-i-1u;
        const unsigned bit=(v>>shift)&1u;
        const size_t pos=w->bits++;
        if(bit)w->bytes[pos/8u]|=(uint8_t)(1u<<(7u-pos%8u));
    }
}
static void octet(Writer *w,uint8_t v){put(w,8u,v);}
static void H(Writer *w,uint8_t code,uint8_t len,uint32_t val){
    octet(w,(uint8_t)((code<<4u)|len));
    for(unsigned i=0u;i<len;i++) {
        const unsigned shift=(unsigned)(8u*(len-1u-i));
        octet(w,(uint8_t)(val>>shift));
    }
}
static size_t make(uint8_t output[128],unsigned mode,
                   unsigned xdict_missing,int nonzero_pad){
    Writer w={0};
    put(&w,2u,1u);octet(&w,19u); /* BS LINE type */
    /* RL pre-handle bit count = 88 */
    const unsigned endbit=nonzero_pad ? 91u : 88u;
    octet(&w,(uint8_t)endbit);octet(&w,0);octet(&w,0);octet(&w,0);
    H(&w,0u,2u,0x120u); /* self handle */
    put(&w,2u,2u); /* BS EED zero */
    put(&w,1u,0u); /* no embedded graphics */
    put(&w,2u,mode);
    put(&w,2u,1u);octet(&w,1u); /* BL count reactor = 1 */
    put(&w,1u,xdict_missing);
    assert(w.bits==82u);
    while(w.bits<endbit)put(&w,1u,0u); /* dummy bounded geometry */
    if(mode==0u)H(&w,12u,1u,5u); /* owner = self - 5 */
    H(&w,4u,1u,0x75u); /* reactor */
    if(!xdict_missing)H(&w,3u,1u,0x44u); /* extension dictionary */
    H(&w,5u,1u,0x33u); /* layer */
    if(nonzero_pad){put(&w,2u,3u);}
    const size_t payload=(w.bits+7u)/8u;
    assert(payload<125u);
    output[0]=(uint8_t)payload;output[1]=(uint8_t)(payload>>8u);
    memcpy(output+2u,w.bytes,payload);
    uint16_t crc=0u;
    assert(musa_core_crc16_arc(0xc0c1u,output,2u+payload,&crc));
    output[payload+2u]=(uint8_t)crc;
    output[payload+3u]=(uint8_t)(crc>>8u);
    return payload+4u;
}
/* Separate independently synthesized zero-reactor entity link record.
 * Same fixed header and H role grammar, but BL reactor count is exactly 0.
 * This fixture does not assert geometric LINE coordinates. */
static size_t make_zero(uint8_t output[128],unsigned mode,
                        unsigned xdict_missing){
    Writer w={0};
    put(&w,2u,1u);octet(&w,19u);
    const unsigned endbit=80u;
    octet(&w,(uint8_t)endbit);
    octet(&w,0u);octet(&w,0u);octet(&w,0u);
    H(&w,0u,2u,0x120u);
    put(&w,2u,2u); /* zero-sized EED */
    put(&w,1u,0u); /* no graphics */
    put(&w,2u,mode);
    put(&w,2u,2u); /* canonical BL zero uses opcode 2, no bytes */
    put(&w,1u,xdict_missing);
    assert(w.bits==74u);
    while(w.bits<endbit)put(&w,1u,0u);
    if(mode==0u)H(&w,12u,1u,5u);
    if(!xdict_missing)H(&w,3u,1u,0x44u);
    H(&w,5u,1u,0x33u);
    const size_t payload=(w.bits+7u)/8u;
    assert(payload<125u);
    output[0]=(uint8_t)payload;
    output[1]=(uint8_t)(payload>>8u);
    memcpy(output+2u,w.bytes,payload);
    uint16_t crc=0u;
    assert(musa_core_crc16_arc(0xc0c1u,output,2u+payload,&crc));
    output[payload+2u]=(uint8_t)crc;
    output[payload+3u]=(uint8_t)(crc>>8u);
    return payload+4u;
}
static void re_crc(uint8_t bytes[128],size_t length){
    uint16_t crc=0;
    assert(musa_core_crc16_arc(0xc0c1u,bytes,length-2u,&crc));
    bytes[length-2u]=(uint8_t)crc;
    bytes[length-1u]=(uint8_t)(crc>>8);
}
int main(void){
    uint8_t input[128]={0};
    const size_t len=make(input,0u,1u,0);
    MusaDwgAc1015HandleEntry map[5]={
        {0x33u,1u},{0x44u,4u},{0x75u,2u},{0x11bu,3u},{0x120u,0u}
    };
    uint64_t reactor[2]={42u,43u};
    MusaDwgAc1018ReactorAudit info={0};
    assert(musa_dwg_ac1018_read_reactor_links(input,len,0u,0x120u,
            map,5u,reactor,2u,&info));
    assert(info.handle==0x120u && info.owner_handle==0x11bu &&
           info.layer_handle==0x33u && info.xdictionary_handle==0u &&
           info.reactor_count==1u && reactor[0]==0x75u &&
           reactor[1]==43u && info.geometry_decoded==0u &&
           info.record_crc_verified && info.handle_membership_verified);
    MusaDwgAc1018ReactorAudit preserved=info;
    uint64_t before[2]={reactor[0],reactor[1]};
    /* Mode 2 entity has no owner; xdict must be separately indexed. */
    const size_t mode2len=make(input,2u,0u,1);
    assert(musa_dwg_ac1018_read_reactor_links(input,mode2len,0u,0x120u,
            map,5u,reactor,2u,&info));
    assert(info.entity_mode==2u && info.owner_handle==0u &&
           info.xdictionary_handle==0x44u && info.layer_handle==0x33u);
    preserved=info;before[0]=reactor[0];before[1]=reactor[1];
    assert(!musa_dwg_ac1018_read_reactor_links(input,mode2len-1u,0u,0x120u,
            map,5u,reactor,2u,&info));
    assert(!musa_dwg_ac1018_read_reactor_links(input,mode2len,0u,0x121u,
            map,5u,reactor,2u,&info));
    assert(!musa_dwg_ac1018_read_reactor_links(input,mode2len,0u,0x120u,
            map,5u,reactor,0u,&info));
    assert(!memcmp(&info,&preserved,sizeof(info)));
    assert(reactor[0]==before[0]&&reactor[1]==before[1]);
    MusaDwgAc1015HandleEntry wrong[5];
    memcpy(wrong,map,sizeof(wrong));
    wrong[2].handle=0x76u; /* reference to 0x75 now dangling */
    assert(!musa_dwg_ac1018_read_reactor_links(input,mode2len,0u,0x120u,
            wrong,5u,reactor,2u,&info));
    memcpy(wrong,map,sizeof(wrong));
    wrong[0]=map[1];wrong[1]=map[0]; /* unsorted */
    assert(!musa_dwg_ac1018_read_reactor_links(input,mode2len,0u,0x120u,
            wrong,5u,reactor,2u,&info));
    const size_t changed_role_bit=91u+3u; /* bit of type nibble */
    /* In mode 2 the first H is the reactor; corrupt its H type but
     * repair CRC to prove rejection comes from reference grammar. */
    input[2u+changed_role_bit/8u]^=(uint8_t)(1u<<(7u-changed_role_bit%8u));
    re_crc(input,mode2len);
    assert(!musa_dwg_ac1018_read_reactor_links(input,mode2len,0u,0x120u,
            map,5u,reactor,2u,&info));
    assert(!memcmp(&info,&preserved,sizeof(info)));
    /* The new entry point accepts strictly zero BL reactors, but never
     * weakens the original reactor-required function. */
    const size_t zlen=make_zero(input,0u,1u);
    MusaDwgAc1018ReactorAudit zero={0};
    assert(musa_dwg_ac1018_read_zero_reactor_links(input,zlen,0u,
                        0x120u,map,5u,&zero));
    assert(zero.handle==0x120u&&zero.owner_handle==0x11bu&&
           zero.layer_handle==0x33u&&zero.reactor_count==0u&&
           zero.xdictionary_handle==0u&&zero.record_crc_verified&&
           zero.handle_membership_verified&&zero.geometry_decoded==0u);
    assert(!musa_dwg_ac1018_read_reactor_links(input,zlen,0u,
                        0x120u,map,5u,reactor,2u,&info));
    const size_t zmode2=make_zero(input,2u,0u);
    assert(musa_dwg_ac1018_read_zero_reactor_links(input,zmode2,0u,
                        0x120u,map,5u,&zero));
    assert(zero.entity_mode==2u&&zero.owner_handle==0u&&
           zero.xdictionary_handle==0x44u&&zero.layer_handle==0x33u);
    MusaDwgAc1018ReactorAudit zero_copy=zero;
    input[zmode2-1u]^=1u;
    assert(!musa_dwg_ac1018_read_zero_reactor_links(input,zmode2,0u,
                        0x120u,map,5u,&zero));
    assert(!memcmp(&zero,&zero_copy,sizeof(zero)));
    make_zero(input,2u,0u);
    memcpy(wrong,map,sizeof(wrong));
    wrong[0].handle=0x34u; /* dangling absolute layer handle */
    assert(!musa_dwg_ac1018_read_zero_reactor_links(input,zmode2,0u,
                        0x120u,wrong,5u,&zero));
    assert(!memcmp(&zero,&zero_copy,sizeof(zero)));
    const size_t original_length=make(input,0u,1u,0);
    assert(!musa_dwg_ac1018_read_zero_reactor_links(input,original_length,
                        0u,0x120u,map,5u,&zero));
    assert(!memcmp(&zero,&zero_copy,sizeof(zero)));
    return 0;
}
