/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "musacad_dwg_ac1018_layer_control.h"
#include "musacad_crc16.h"
#include <assert.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>

typedef struct Writer {uint8_t data[128];size_t bits;} Writer;
static void put(Writer *w,unsigned n,uint32_t value){
    assert(n<=32u&&w->bits+n<=sizeof(w->data)*8u);
    for(unsigned i=0u;i<n;i++){
        const unsigned shift=n-1u-i;
        const size_t at=w->bits++;
        if((value>>shift)&1u)w->data[at/8u]|=(uint8_t)(1u<<(7u-at%8u));
    }
}
static void byte(Writer *w,uint32_t v){put(w,8u,v);}
static void bl(Writer *w,uint32_t value){
    if(!value){put(w,2u,2u);return;}
    if(value<256u){put(w,2u,1u);byte(w,value);return;}
    put(w,2u,0u);
    for(unsigned i=0u;i<4u;i++)byte(w,value>>(8u*i));
}
static void absolute_h(Writer *w,uint8_t code,uint8_t value){
    byte(w,((uint32_t)code<<4u)|1u);
    byte(w,value);
}
static void patch_u32_le(Writer *w,size_t at,uint32_t value){
    for(unsigned i=0u;i<32u;i++){
        const size_t pos=at+i;
        const uint8_t mask=(uint8_t)(1u<<(7u-pos%8u));
        if((value>>(8u*(i/8u)+7u-i%8u))&1u)
            w->data[pos/8u]|=mask;
        else w->data[pos/8u]&=(uint8_t)~mask;
    }
}
/* Original R2004 object bits: BS type, RL data end, H self, BS EED,
 * BL reactor count, B xdict, BL numentries, NULL H, soft-owner layer H.
 * No synthetic "magic success" shortcut in the tested reader. */
static size_t make(uint8_t dst[132],uint16_t type,uint8_t self,
                   uint32_t count,uint8_t root_code,uint8_t layer_code,
                   int short_tail,int reactors,int xdict_missing){
    Writer w={0};
    put(&w,2u,1u);byte(&w,type);
    const size_t rl_at=w.bits;
    for(unsigned i=0u;i<4u;i++)byte(&w,0u);
    absolute_h(&w,0u,self);
    put(&w,2u,2u); /* EED zero */
    bl(&w,reactors?1u:0u);
    put(&w,1u,xdict_missing?1u:0u);
    bl(&w,count);
    const uint32_t end=(uint32_t)w.bits;
    patch_u32_le(&w,rl_at,end);
    byte(&w,(uint32_t)root_code<<4u); /* canonical NULL soft ptr 4.0 */
    const uint32_t written=short_tail?1u:count>2u?2u:count;
    for(uint32_t i=0u;i<written;i++)
        absolute_h(&w,layer_code,(uint8_t)(0x44u+i*0x11u));
    const size_t payload=(w.bits+7u)/8u;
    assert(payload+4u<=132u && payload>=8u);
    memset(dst,0u,132u);
    dst[0]=(uint8_t)payload;dst[1]=(uint8_t)(payload>>8u);
    memcpy(dst+2u,w.data,payload);
    uint16_t crc=0u;
    assert(musa_core_crc16_arc(0xc0c1u,dst,payload+2u,&crc));
    dst[payload+2u]=(uint8_t)crc;
    dst[payload+3u]=(uint8_t)(crc>>8u);
    return payload+4u;
}
int main(void){
    uint8_t record[132]={0},copy[132]={0};
    size_t n=make(record,50u,0x33u,2u,4u,2u,0,0,1);
    memcpy(copy,record,sizeof(record));
    MusaDwgAc1018LayerControlRefs result={0};
    uint64_t handles[3]={0};
    assert(musa_dwg_ac1018_layer_control_refs_record(
               record,n,0u,0x33u,handles,3u,&result));
    assert(result.indexed_handle==0x33u&&result.entry_count==2u&&
           result.crc_verified&&result.embedded_self_verified&&
           result.supported_handle_roles_verified);
    assert(handles[0]==0x44u&&handles[1]==0x55u&&handles[2]==0u);
    MusaDwgAc1018LayerControlRefs saved=result;
    uint64_t previous[3];memcpy(previous,handles,sizeof(handles));
    assert(musa_dwg_ac1018_layer_control_refs_record(
               record,n,0u,0x33u,NULL,0u,&result));
    assert(!memcmp(&result,&saved,sizeof(saved)));
    assert(!memcmp(record,copy,sizeof(record))); /* never modify DWG */
    assert(!musa_dwg_ac1018_layer_control_refs_record(
               record,n,0u,0x34u,handles,3u,&result));
    assert(!musa_dwg_ac1018_layer_control_refs_record(
               record,n-1u,0u,0x33u,handles,3u,&result));
    assert(!musa_dwg_ac1018_layer_control_refs_record(
               record,n,0u,0x33u,handles,1u,&result));
    assert(!musa_dwg_ac1018_layer_control_refs_record(
               record,n,0u,0x33u,(uint64_t*)(void*)record,2u,&result));
    assert(!musa_dwg_ac1018_layer_control_refs_record(
               record,n,0u,0x33u,handles,3u,
               (MusaDwgAc1018LayerControlRefs*)(void*)record));
    assert(!musa_dwg_ac1018_layer_control_refs_record(
               record,n,0u,0x33u,handles,3u,NULL));
    assert(!memcmp(&result,&saved,sizeof(saved))&&
           !memcmp(handles,previous,sizeof(handles)));
    record[n-1u]^=1u;
    assert(!musa_dwg_ac1018_layer_control_refs_record(
               record,n,0u,0x33u,handles,3u,&result));
    assert(!memcmp(&result,&saved,sizeof(saved))&&
           !memcmp(handles,previous,sizeof(handles)));
    const struct Variant {
        uint16_t type; uint32_t count; uint8_t null_code; uint8_t h_code;
        int short_tail,reactors,xdict_missing;
    } bad[]={
        {51u,2u,4u,2u,0,0,1}, /* not LAYER_CONTROL */
        {50u,0u,4u,2u,0,0,1}, /* zero entries */
        {50u,4097u,4u,2u,0,0,1}, /* resource budget */
        {50u,2u,3u,2u,0,0,1}, /* false root-null H role */
        {50u,2u,4u,5u,0,0,1}, /* layer H must soft-own */
        {50u,2u,4u,2u,1,0,1}, /* declared count > encoded refs */
        {50u,2u,4u,2u,0,1,1}, /* reactors not yet supported */
        {50u,2u,4u,2u,0,0,0}, /* optional XDict not yet supported */
    };
    for(size_t i=0u;i<sizeof(bad)/sizeof(bad[0]);i++){
        const struct Variant *v=&bad[i];
        n=make(record,v->type,0x33u,v->count,v->null_code,
               v->h_code,v->short_tail,v->reactors,v->xdict_missing);
        assert(!musa_dwg_ac1018_layer_control_refs_record(
                     record,n,0u,0x33u,handles,3u,&result));
        assert(!memcmp(&result,&saved,sizeof(saved))&&
               !memcmp(handles,previous,sizeof(handles)));
    }
    return 0;
}
