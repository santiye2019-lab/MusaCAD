/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "musacad_dwg_ac1018_scene.h"
#include "musacad_crc16.h"
#include <assert.h>
#include <math.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
/* 100% hand-synthesized protected raw DWG with an encoded LINE common
 * handle-tail. Other referenced records intentionally remain opaque:
 * index membership is tested, not ownership/layer CLASSES semantics. */
static void put32(uint8_t *p,uint32_t v){
    p[0]=(uint8_t)v;p[1]=(uint8_t)(v>>8u);
    p[2]=(uint8_t)(v>>16u);p[3]=(uint8_t)(v>>24u);
}
static void put64(uint8_t *p,uint64_t v){
    put32(p,(uint32_t)v);put32(p+4u,(uint32_t)(v>>32u));
}
static uint32_t adler(uint32_t seed,const uint8_t *p,size_t n){
    uint32_t a=seed&65535u,b=seed>>16u;
    for(size_t i=0u;i<n;++i){a=(a+p[i])%65521u;b=(b+a)%65521u;}
    return (b<<16u)|a;
}
static void sys_header(uint8_t *p,uint32_t type,
                       uint32_t expanded,uint32_t packed){
    put32(p,type);put32(p+4u,expanded);put32(p+8u,packed);
    put32(p+12u,2u);put32(p+16u,0u);
    put32(p+16u,adler(adler(0u,p,20u),p+20u,packed));
}
/* A real bounded R2004 LZ stream: literal initial bytes, then
 * offset-one run to exactly the fixed 29,696-byte physical block. */
static size_t lz_data(uint8_t *p,const uint8_t *prefix,size_t n){
    assert(n>=4u && n<=273u && n<29696u-300u);
    size_t at=0u;
    if(n<=18u)p[at++]=(uint8_t)(n-3u);
    else {p[at++]=0u;p[at++]=(uint8_t)(n-18u);}
    memcpy(p+at,prefix,n);at+=n;
    const size_t remaining=29696u-n,delta=remaining-33u;
    assert(delta>255u);
    p[at++]=0x20u;p[at++]=0u;
    const size_t zeros=(delta-256u)/255u;
    const size_t extra=delta-255u-zeros*255u;
    assert(extra>=1u&&extra<=255u);
    for(size_t i=0u;i<zeros;i++)p[at++]=0u;
    p[at++]=(uint8_t)extra;
    p[at++]=0u;p[at++]=0u;p[at++]=0x11u;
    return at;
}
static void page(uint8_t *file,size_t start,uint32_t sid,
                 const uint8_t *prefix,size_t n){
    uint8_t *payload=file+start+32u;
    const size_t packed=lz_data(payload,prefix,n);
    assert(packed<288u);
    uint32_t words[8]={UINT32_C(0x4163043b),sid,
          (uint32_t)packed,(uint32_t)packed,0u,0u,0u,0u};
    words[7]=adler(0u,payload,packed);
    uint8_t raw_header[32]={0};
    for(size_t i=0u;i<8u;i++)put32(raw_header+i*4u,words[i]);
    words[6]=adler(words[7],raw_header,32u);
    const uint32_t mask=UINT32_C(0x4164536b)^(uint32_t)start;
    for(size_t i=0u;i<8u;i++)
        put32(file+start+4u*i,words[i]^mask);
}
static void section(uint8_t *p,uint64_t n,uint32_t section_id,
                    const char *name,uint32_t pid,uint32_t packed){
    put64(p,n);put32(p+8u,1u);
    put32(p+20u,2u);put32(p+24u,section_id);
    memcpy(p+32u,name,strlen(name));
    put32(p+96u,pid);put32(p+100u,packed);put64(p+104u,0u);
}


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

/* Build CRC-verified *target* record envelopes independently.
 * The 8-byte payload contains a BS numeric type claim, not a trustworthy
 * LAYER/owner AcDb class: target self H and semantic type still pending. */
static size_t opaque_target(uint8_t *out,uint8_t type){
    const size_t data_len=8u;
    memset(out,0,data_len+4u);
    out[0]=(uint8_t)data_len;
    out[2]=(uint8_t)(0x40u|(type>>2u));
    out[3]=(uint8_t)((type&0x03u)<<6u);
    uint16_t crc=0u;
    assert(musa_core_crc16_arc(0xc0c1u,out,data_len+2u,&crc));
    out[data_len+2u]=(uint8_t)crc;
    out[data_len+3u]=(uint8_t)(crc>>8u);
    return data_len+4u;
}
static size_t hmap(uint8_t *h,int reactor){
    size_t pos=2u;
    memset(h,0u,20u);
    h[pos++]=0x33u;h[pos++]=0u; /* LAYER claim h=0x33 @0 */
    if(reactor){h[pos++]=0x42u;h[pos++]=12u;} /* 0x75 @12 */
    /* 0x11B @12 (without reactor) or @24 */
    h[pos++]=reactor?0xa6u:0xe8u;h[pos++]=1u;h[pos++]=12u;
    /* entity 0x120 @24 or @36 */
    h[pos++]=5u;h[pos++]=12u;
    h[1]=(uint8_t)pos;
    uint16_t crc=0u;
    assert(musa_core_crc16_arc(0xc0c1u,h,pos,&crc));
    h[pos++]=(uint8_t)(crc>>8u);h[pos++]=(uint8_t)crc;
    h[pos++]=0u;h[pos++]=2u;
    assert(musa_core_crc16_arc(0xc0c1u,h+pos-2u,2u,&crc));
    h[pos++]=(uint8_t)(crc>>8u);h[pos++]=(uint8_t)crc;
    assert(pos==(reactor?17u:15u));
    return pos;
}
/* corrupt_target: 1 layer, 2 owner, 3 reactor (reactor case only).
 * Outer physical DWG page checksum is regenerated; the inner object
 * CRC is intentionally left wrong. */
static void build_targets(uint8_t file[1120],int reactor,int corrupt_target){
    memset(file,0u,1120u);memcpy(file,"AC1018",6u);
    uint8_t head[0x6c]={0};
    memcpy(head,"AcFssFcAJMB",11u);
    put32(head+0x10u,0x6cu);
    put32(head+0x40u,4u);put32(head+0x50u,4u);
    put32(head+0x5cu,3u);put64(head+0x54u,800u);
    uint32_t state=1u;
    for(size_t i=0;i<sizeof(head);i++){
        state=state*UINT32_C(0x343fd)+UINT32_C(0x269ec3);
        file[0x80u+i]=(uint8_t)(head[i]^(uint8_t)(state>>16u));
    }
    uint8_t objects[256]={0};
    size_t count=0u;
    count+=opaque_target(objects+count,50u); /* declared layer type */
    if(reactor)count+=opaque_target(objects+count,70u);
    count+=opaque_target(objects+count,60u); /* declared owner type */
    count+=reactor?make(objects+count,0u,1u,0):
                   make_zero(objects+count,0u,1u);
    assert(count<sizeof(objects));
    if(corrupt_target==1)objects[4u]^=1u;
    if(corrupt_target==2)objects[reactor?28u:16u]^=1u;
    if(corrupt_target==3&&reactor)objects[16u]^=1u;
    uint8_t handles[20]={0};
    const size_t map_bytes=hmap(handles,reactor);
    page(file,256u,11u,objects,count);
    page(file,576u,10u,handles,map_bytes);
    uint8_t packed[288]={0};
    const uint32_t opacked=(uint32_t)lz_data(packed,objects,count);
    const uint32_t hpacked=(uint32_t)lz_data(packed,handles,map_bytes);
    uint8_t layout[244]={0};put32(layout,2u);
    section(layout+20u,count,11u,
            "AcDb:AcDbObjects",1u,opacked);
    section(layout+132u,map_bytes,10u,
            "AcDb:Handles",2u,hpacked);
    uint8_t *sect=file+768u+20u;
    sect[0]=0u;sect[1]=226u;
    memcpy(sect+2u,layout,sizeof(layout));sect[246]=0x11u;
    sys_header(file+768u,UINT32_C(0x4163003b),244u,247u);
    uint8_t map[32]={0};
    put32(map,1u);put32(map+4u,320u);
    put32(map+8u,2u);put32(map+12u,192u);
    put32(map+16u,3u);put32(map+20u,288u);
    put32(map+24u,4u);put32(map+28u,64u);
    uint8_t *m=file+1056u+20u;
    m[0]=0u;m[1]=14u;memcpy(m+2u,map,32u);
    m[34]=0x11u;
    sys_header(file+1056u,UINT32_C(0x41630e3b),32u,35u);
}
int main(void){
    uint8_t file[1120];build_targets(file,0,0);
    MusaDwgAc1018Scene *scene=NULL;
    assert(musa_dwg_ac1018_scene_open(file,sizeof(file),&scene));
    assert(musa_dwg_ac1018_scene_handle_count(scene)==3u);
    MusaDwgAc1018LinkedRecordAudit audit={0};
    assert(musa_dwg_ac1018_scene_audit_link_record_envelopes(
                        scene,0x120u,NULL,0u,&audit));
    assert(audit.link_roles.record_crc_verified&&
           audit.link_roles.handle_membership_verified&&
           audit.link_roles.owner_handle==0x11bu&&
           audit.link_roles.layer_handle==0x33u&&
           audit.link_roles.reactor_count==0u&&
           audit.owner_record.declared_type==60u&&
           audit.owner_record.record_crc_verified&&
           audit.layer_record.declared_type==50u&&
           audit.layer_record.record_crc_verified&&
           audit.xdictionary_record.handle==0u);
    MusaDwgAc1018RecordProbe record={0};
    assert(musa_dwg_ac1018_scene_probe_record_handle(scene,0x33u,&record));
    assert(record.handle==0x33u&&record.declared_type==50u&&
           record.object_offset==0u&&record.record_bytes==12u);
    const MusaDwgAc1018RecordProbe saved=record;
    assert(!musa_dwg_ac1018_scene_probe_record_handle(scene,0x34u,&record));
    assert(!musa_dwg_ac1018_scene_probe_record_handle(scene,0u,&record));
    assert(!memcmp(&record,&saved,sizeof(record)));
    const MusaDwgAc1018LinkedRecordAudit old=audit;
    assert(!musa_dwg_ac1018_scene_audit_link_record_envelopes(
                   scene,0x34u,NULL,0u,&audit));
    assert(!memcmp(&audit,&old,sizeof(audit)));
    file[288u]^=1u; /* source copied in scene */
    assert(musa_dwg_ac1018_scene_audit_link_record_envelopes(
                   scene,0x120u,NULL,0u,&audit));
    musa_dwg_ac1018_scene_close(scene);scene=NULL;
    build_targets(file,1,0);
    assert(musa_dwg_ac1018_scene_open(file,sizeof(file),&scene));
    assert(musa_dwg_ac1018_scene_handle_count(scene)==4u);
    MusaDwgAc1018RecordProbe reactors[2]={{0}};
    assert(musa_dwg_ac1018_scene_audit_link_record_envelopes(
                  scene,0x120u,reactors,2u,&audit));
    assert(audit.link_roles.reactor_count==1u&&
           audit.owner_record.declared_type==60u&&
           audit.layer_record.declared_type==50u&&
           reactors[0].handle==0x75u&&
           reactors[0].declared_type==70u&&
           reactors[0].record_crc_verified);
    MusaDwgAc1018LinkedRecordAudit kept=audit;
    MusaDwgAc1018RecordProbe kept_reactor=reactors[0];
    assert(!musa_dwg_ac1018_scene_audit_link_record_envelopes(
                  scene,0x120u,NULL,0u,&audit));
    assert(!musa_dwg_ac1018_scene_audit_link_record_envelopes(
                  scene,0x120u,reactors,0u,&audit));
    assert(!musa_dwg_ac1018_scene_audit_link_record_envelopes(
                  scene,0x120u,(MusaDwgAc1018RecordProbe*)(void*)&audit,
                  1u,&audit));
    assert(!memcmp(&audit,&kept,sizeof(audit))&&
           !memcmp(&reactors[0],&kept_reactor,sizeof(kept_reactor)));
    musa_dwg_ac1018_scene_close(scene);scene=NULL;
    for(int damage=1;damage<=3;damage++){
        build_targets(file,1,damage);
        assert(musa_dwg_ac1018_scene_open(file,sizeof(file),&scene));
        MusaDwgAc1018ReactorAudit links={0};
        uint64_t members[1]={0};
        /* An indexed H target exists: the older API still admits it. */
        assert(musa_dwg_ac1018_scene_audit_links(
                      scene,0x120u,members,1u,&links));
        /* But CRC of one target record is wrong: new audit fails. */
        assert(!musa_dwg_ac1018_scene_audit_link_record_envelopes(
                      scene,0x120u,reactors,2u,&audit));
        assert(!memcmp(&audit,&kept,sizeof(audit))&&
               !memcmp(&reactors[0],&kept_reactor,sizeof(kept_reactor)));
        musa_dwg_ac1018_scene_close(scene);scene=NULL;
    }
    return 0;
}
