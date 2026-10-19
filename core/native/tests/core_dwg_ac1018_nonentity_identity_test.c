/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "musacad_dwg_ac1018_nonentity.h"
#include "musacad_crc16.h"
#include <assert.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>

typedef struct Writer {uint8_t data[64];size_t bits;} Writer;
static void setbit(Writer *w,size_t at,uint32_t bit){
    assert(at<sizeof(w->data)*8u);
    const uint8_t mask=(uint8_t)(1u<<(7u-at%8u));
    if(bit)w->data[at/8u]|=mask;
    else w->data[at/8u]&=(uint8_t)~mask;
}
static void put(Writer *w,unsigned n,uint32_t v){
    assert(n<=32u&&w->bits+n<=sizeof(w->data)*8u);
    for(unsigned i=0u;i<n;i++)
        setbit(w,w->bits++,(v>>(n-1u-i))&1u);
}
static void byte(Writer *w,uint8_t b){put(w,8u,b);}
/* Synthetic R2004 nonentity: BS, RL endbit, absolute H, BS EED=0,
 * BL reactors=0, B missing XDict. Everything is real bit framing;
 * no opaque target shortcut and no third-party DWG libraries. */
static size_t make(uint8_t out[64],uint16_t type,
                   uint64_t self,int reactor_overflow){
    Writer w={0};
    assert(type<256u&&self&&self<=UINT32_MAX);
    put(&w,2u,1u);byte(&w,(uint8_t)type); /* BS byte */
    const size_t rl_at=w.bits;
    for(int i=0;i<4;i++)byte(&w,0u); /* RL patched after header */
    unsigned len=0u;
    for(uint64_t v=self;v;v>>=8u)len++;
    assert(len>=1u&&len<=4u);
    byte(&w,(uint8_t)len); /* absolute code 0 + H count */
    for(unsigned i=0;i<len;i++)
        byte(&w,(uint8_t)(self>>(8u*(len-1u-i))));
    put(&w,2u,2u); /* BS EED zero */
    if(reactor_overflow){put(&w,2u,1u);byte(&w,65u);}
    else put(&w,2u,2u); /* BL reactor zero */
    put(&w,1u,1u); /* missing extension dictionary */
    const size_t endbit=w.bits;
    for(unsigned i=0;i<4;i++)
        for(unsigned b=0;b<8u;b++)
            setbit(&w,rl_at+(size_t)i*8u+b,
                   ((uint32_t)endbit>>(8u*i+7u-b))&1u);
    const size_t body=(w.bits+7u)/8u;
    assert(body<sizeof(w.data)&&body<UINT16_MAX);
    memset(out,0u,64u);
    out[0]=(uint8_t)body;out[1]=(uint8_t)(body>>8u);
    memcpy(out+2u,w.data,body);
    uint16_t crc=0u;
    assert(musa_core_crc16_arc(0xc0c1u,out,body+2u,&crc));
    out[body+2u]=(uint8_t)crc;
    out[body+3u]=(uint8_t)(crc>>8u);
    return body+4u;
}

/* Real R2004 MS+CRC framing, BS/RL/H/EED common prefix, then
 * TV name, reference bit, BS xref index, xdep and raw flags.
 * The color BS stub is kept unclassified by the prefix decoder. */
static size_t make_layer_prefix(uint8_t out[64],
                                unsigned type,uint32_t indexed,
                                int non_ascii,int cmc_case){
    Writer w={0};
    const char name[]="MEK-SU";
    put(&w,2u,1u);byte(&w,(uint8_t)type);
    const size_t rl_at=w.bits;
    for(int i=0;i<4;i++)byte(&w,0u);
    byte(&w,1u);byte(&w,(uint8_t)indexed); /* absolute self-H */
    put(&w,2u,2u); /* EED zero */
    put(&w,2u,2u); /* reactor BL zero */
    put(&w,1u,1u); /* XDictionary missing */
    put(&w,2u,1u);byte(&w,6u); /* TV BS byte count */
    for(size_t i=0;i<6u;i++)
        byte(&w,non_ascii&&i==0u?0xe9u:(uint8_t)name[i]);
    put(&w,1u,1u); /* ever-referenced */
    put(&w,2u,2u); /* xrefindex+1 = 0 */
    put(&w,1u,0u); /* Xdep */
    put(&w,2u,1u);byte(&w,13u); /* raw flags BS */
    if(cmc_case==0){
        put(&w,2u,2u); /* incomplete CMC: prefix-only fixture */
    }else{
        if(cmc_case==5){put(&w,2u,1u);byte(&w,1u);}
        else put(&w,2u,2u); /* R2004 CMC BS index=0 */
        put(&w,2u,0u); /* BL raw RGB: 32 bits follow */
        byte(&w,0x33u);byte(&w,0x22u);
        byte(&w,0x11u);byte(&w,0xc2u); /* method C2 RGB: 0xC2112233 */
        const uint8_t flag=(uint8_t)(
            cmc_case==2||cmc_case==4?3u:cmc_case==3?4u:0u);
        byte(&w,flag);
        if(flag&1u){
            put(&w,2u,1u);byte(&w,3u);
            byte(&w,cmc_case==4?0xe9u:'R');byte(&w,'E');byte(&w,'D');
        }
        if(flag&2u){
            put(&w,2u,1u);byte(&w,4u);
            byte(&w,'B');byte(&w,'O');
            byte(&w,'O');byte(&w,'K');
        }
    }
    const size_t endbit=w.bits;
    for(unsigned i=0u;i<4u;i++)
        for(unsigned j=0u;j<8u;j++)
            setbit(&w,rl_at+(size_t)i*8u+j,
                   ((uint32_t)endbit>>(8u*i+7u-j))&1u);
    const size_t body=(w.bits+7u)/8u;
    assert(body<sizeof(w.data)&&body+4u<64u);
    memset(out,0u,64u);
    out[0]=(uint8_t)body;out[1]=(uint8_t)(body>>8u);
    memcpy(out+2u,w.data,body);
    uint16_t crc=0u;
    assert(musa_core_crc16_arc(0xc0c1u,out,body+2u,&crc));
    out[body+2u]=(uint8_t)crc;
    out[body+3u]=(uint8_t)(crc>>8u);
    return body+4u;
}
int main(void){
    uint8_t record[64]={0};
    MusaDwgAc1018NonEntityIdentity out={0};
    const uint16_t kinds[]={51u,49u,42u};
    const uint64_t handles[]={0x33u,0x11bu,0x44u};
    for(size_t i=0;i<3u;i++){
        const size_t n=make(record,kinds[i],handles[i],0);
        assert(musa_dwg_ac1018_nonentity_identity_record(
                       record,n,0u,handles[i],&out));
        assert(out.declared_type==kinds[i]&&out.indexed_handle==handles[i]&&
               out.record_bytes==n&&out.record_crc_verified&&
               out.embedded_self_verified&&out.reactor_count==0u&&
               out.xdictionary_missing&&out.eed_skipped_bytes==0u);
        const MusaDwgAc1018NonEntityIdentity old=out;
        /* Correct record checksum but an incorrect index-to-self binding. */
        assert(!musa_dwg_ac1018_nonentity_identity_record(
                       record,n,0u,handles[i]+1u,&out));
        assert(!memcmp(&old,&out,sizeof(old)));
        /* Correct object, expected type alone cannot rewrite actual type. */
        assert(!musa_dwg_ac1018_nonentity_identity_record(
                       record,n,n,handles[i],&out));
        assert(!musa_dwg_ac1018_nonentity_identity_record(
                       record,n-1u,0u,handles[i],&out));
        assert(!musa_dwg_ac1018_nonentity_identity_record(
                       record,n,0u,handles[i],
                       (MusaDwgAc1018NonEntityIdentity*)(void*)record));
        assert(!memcmp(&old,&out,sizeof(old)));
        record[n-1u]^=0x01u;
        assert(!musa_dwg_ac1018_nonentity_identity_record(
                       record,n,0u,handles[i],&out));
        assert(!memcmp(&old,&out,sizeof(old)));
    }
    size_t n=make(record,50u,0x33u,0);
    assert(!musa_dwg_ac1018_nonentity_identity_record(
                    record,n,0u,0x33u,&out)); /* LAYER_CONTROL not LAYER */
    n=make(record,51u,0x33u,1);
    assert(!musa_dwg_ac1018_nonentity_identity_record(
                    record,n,0u,0x33u,&out)); /* BL reactor cap */

    MusaDwgAc1018LayerPrefix layer={0};
    n=make_layer_prefix(record,51u,0x33u,0,0);
    assert(musa_dwg_ac1018_layer_prefix_record(
                   record,n,0u,0x33u,&layer));
    assert(layer.indexed_handle==0x33u&&
           layer.name_bytes==6u&&!strcmp(layer.name,"MEK-SU")&&
           layer.ever_referenced==1u&&
           layer.xref_index_plus_one==0u&&
           layer.xref_dependent==0u&&
           layer.raw_layer_flags==13u&&
           layer.record_crc_verified&&layer.embedded_self_verified);
    const MusaDwgAc1018LayerPrefix good=layer;
    assert(!musa_dwg_ac1018_layer_prefix_record(
                  record,n,0u,0x34u,&layer)); /* embedded H mismatch */
    assert(!musa_dwg_ac1018_layer_prefix_record(
                  record,n-1u,0u,0x33u,&layer)); /* truncated CRC */
    assert(!musa_dwg_ac1018_layer_prefix_record(
                  record,n,0u,0x33u,
                  (MusaDwgAc1018LayerPrefix*)(void*)record)); /* alias */
    assert(!memcmp(&good,&layer,sizeof(good)));
    n=make_layer_prefix(record,49u,0x33u,0,0);
    assert(!musa_dwg_ac1018_layer_prefix_record(
                  record,n,0u,0x33u,&layer)); /* not LAYER */
    assert(!memcmp(&good,&layer,sizeof(good)));
    n=make_layer_prefix(record,51u,0x33u,1,0);
    assert(!musa_dwg_ac1018_layer_prefix_record(
                  record,n,0u,0x33u,&layer)); /* unsupported codepage */
    assert(!memcmp(&good,&layer,sizeof(good)));
    n=make_layer_prefix(record,51u,0x33u,0,0);
    record[n-1u]^=0x01u;
    assert(!musa_dwg_ac1018_layer_prefix_record(
                  record,n,0u,0x33u,&layer)); /* CRC corruption */
    assert(!memcmp(&good,&layer,sizeof(good)));
    MusaDwgAc1018LayerColor color={0};
    n=make_layer_prefix(record,51u,0x33u,0,1);
    assert(musa_dwg_ac1018_layer_color_record(
               record,n,0u,0x33u,&color));
    assert(color.cmc_data_bits_verified&&
           color.raw_cmc_index==0u&&
           color.raw_cmc_rgb==UINT32_C(0xC2112233)&&
           !strcmp(color.prefix.name,"MEK-SU")&&
           color.prefix.raw_layer_flags==13u&&
           !color.has_color_name&&!color.has_book_name&&
           !color.color_name[0]&&!color.book_name[0]);
    const MusaDwgAc1018LayerColor good_color=color;
    assert(!musa_dwg_ac1018_layer_color_record(
                  record,n,0u,0x34u,&color));
    assert(!musa_dwg_ac1018_layer_color_record(
                  record,n,0u,0x33u,
                  (MusaDwgAc1018LayerColor*)(void*)record));
    assert(!memcmp(&color,&good_color,sizeof(color)));
    n=make_layer_prefix(record,51u,0x33u,0,2);
    assert(musa_dwg_ac1018_layer_color_record(
               record,n,0u,0x33u,&color));
    assert(color.has_color_name&&color.has_book_name&&
           !strcmp(color.color_name,"RED")&&
           !strcmp(color.book_name,"BOOK"));
    const MusaDwgAc1018LayerColor names=color;
    n=make_layer_prefix(record,51u,0x33u,0,3);
    assert(!musa_dwg_ac1018_layer_color_record(
                  record,n,0u,0x33u,&color)); /* illegal flags */
    assert(!memcmp(&color,&names,sizeof(color)));
    n=make_layer_prefix(record,51u,0x33u,0,4);
    assert(!musa_dwg_ac1018_layer_color_record(
                  record,n,0u,0x33u,&color)); /* unsupported codepage */
    assert(!memcmp(&color,&names,sizeof(color)));
    n=make_layer_prefix(record,51u,0x33u,0,5);
    assert(!musa_dwg_ac1018_layer_color_record(
                  record,n,0u,0x33u,&color)); /* CMC index MUST be zero */
    assert(!memcmp(&color,&names,sizeof(color)));
    n=make_layer_prefix(record,51u,0x33u,0,0);
    assert(!musa_dwg_ac1018_layer_color_record(
                  record,n,0u,0x33u,&color)); /* incomplete CMC */
    assert(!memcmp(&color,&names,sizeof(color)));
    n=make_layer_prefix(record,51u,0x33u,0,1);
    record[n-1u]^=1u;
    assert(!musa_dwg_ac1018_layer_color_record(
                  record,n,0u,0x33u,&color)); /* CRC mismatch */
    assert(!memcmp(&color,&names,sizeof(color)));
    return 0;
}
