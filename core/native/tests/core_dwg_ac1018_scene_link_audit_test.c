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
                   unsigned xdict_missing,int nonzero_pad,unsigned entity_type){
    Writer w={0};
    put(&w,2u,1u);octet(&w,(uint8_t)entity_type); /* BS entity claim */
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
                        unsigned xdict_missing,unsigned entity_type){
    Writer w={0};
    put(&w,2u,1u);octet(&w,(uint8_t)entity_type);
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

static void make_index(uint8_t h[17],int with_reactor,size_t *out_n){
    memset(h,0,17u);
    size_t pos=2u;
    h[pos++]=0x33u;h[pos++]=0u; /* layer h=0x33 @0 */
    if(with_reactor){
        h[pos++]=0x42u;h[pos++]=1u; /* reactor h=0x75 @1 */
    }
    /* owner h=0x11B; 0x11B-0x33=232, -0x75=166 */
    h[pos++]=with_reactor?0xa6u:0xe8u;
    h[pos++]=1u;
    h[pos++]=1u; /* object offset delta 1 */
    h[pos++]=5u;
    h[pos++]=1u; /* self h=0x120, offset 2 or 3 */
    h[0]=0u;h[1]=(uint8_t)pos;
    uint16_t crc=0u;
    assert(musa_core_crc16_arc(0xc0c1u,h,pos,&crc));
    h[pos++]=(uint8_t)(crc>>8u);h[pos++]=(uint8_t)crc;
    h[pos++]=0u;h[pos++]=2u;
    assert(musa_core_crc16_arc(0xc0c1u,h+pos-2u,2u,&crc));
    h[pos++]=(uint8_t)(crc>>8u);h[pos++]=(uint8_t)crc;
    assert(pos==(with_reactor?17u:15u));
    *out_n=pos;
}
static void build(uint8_t f[1120],int with_reactor,unsigned entity_type){
    memset(f,0u,1120u);memcpy(f,"AC1018",6u);
    uint8_t head[0x6c]={0};
    memcpy(head,"AcFssFcAJMB",11u);
    put32(head+0x10u,0x6cu);
    put32(head+0x40u,4u);put32(head+0x50u,4u);
    put32(head+0x5cu,3u);put64(head+0x54u,800u);
    uint32_t state=1u;
    for(size_t i=0;i<sizeof(head);i++){
        state=state*UINT32_C(0x343fd)+UINT32_C(0x269ec3);
        f[0x80u+i]=(uint8_t)(head[i]^(uint8_t)(state>>16u));
    }
    uint8_t objects[256]={0};
    const size_t prefix=with_reactor?3u:2u;
    const size_t rlen=with_reactor?
           make(objects+prefix,0u,1u,0,entity_type):
           make_zero(objects+prefix,0u,1u,entity_type);
    assert(prefix+rlen<sizeof(objects));
    uint8_t handles[17]={0};size_t handle_bytes=0u;
    make_index(handles,with_reactor,&handle_bytes);
    page(f,256u,11u,objects,prefix+rlen);
    page(f,576u,10u,handles,handle_bytes);
    uint8_t packed[288]={0};
    const uint32_t opacked=(uint32_t)lz_data(
                    packed,objects,prefix+rlen);
    const uint32_t hpacked=(uint32_t)lz_data(
                    packed,handles,handle_bytes);
    uint8_t layout[244]={0};
    put32(layout,2u);
    section(layout+20u,prefix+rlen,11u,
            "AcDb:AcDbObjects",1u,opacked);
    section(layout+132u,handle_bytes,10u,
            "AcDb:Handles",2u,hpacked);
    uint8_t *sect=f+768u+20u;
    sect[0]=0u;sect[1]=226u;
    memcpy(sect+2u,layout,sizeof(layout));sect[246]=0x11u;
    sys_header(f+768u,UINT32_C(0x4163003b),244u,247u);
    uint8_t map[32]={0};
    put32(map,1u);put32(map+4u,320u);
    put32(map+8u,2u);put32(map+12u,192u);
    put32(map+16u,3u);put32(map+20u,288u);
    put32(map+24u,4u);put32(map+28u,64u);
    uint8_t *m=f+1056u+20u;
    m[0]=0u;m[1]=14u;memcpy(m+2u,map,32u);
    m[34]=0x11u;
    sys_header(f+1056u,UINT32_C(0x41630e3b),32u,35u);
}
int main(void){
    uint8_t file[1120];build(file,0,19u);
    MusaDwgAc1018Scene *scene=NULL;
    assert(musa_dwg_ac1018_scene_open(file,sizeof(file),&scene));
    assert(musa_dwg_ac1018_scene_handle_count(scene)==3u);
    MusaDwgAc1018ReactorAudit checked={0};
    assert(musa_dwg_ac1018_scene_audit_links(scene,0x120u,
                                              NULL,0u,&checked));
    /* Repeated scene H queries must use the immutable verified map
     * without reallocation or repeated O(N) validation. */
    for(int i=0;i<200;i++){
        MusaDwgAc1018ReactorAudit pass={0};
        assert(musa_dwg_ac1018_scene_audit_links(
                         scene,0x120u,NULL,0u,&pass));
        assert(pass.record_crc_verified&&pass.handle_membership_verified&&
               pass.owner_handle==0x11bu&&pass.layer_handle==0x33u&&
               pass.reactor_count==0u);
    }
    assert(checked.handle==0x120u&&checked.owner_handle==0x11bu&&
           checked.layer_handle==0x33u&&
           checked.xdictionary_handle==0u&&
           checked.reactor_count==0u&&checked.record_crc_verified&&
           checked.handle_membership_verified&&!checked.geometry_decoded);
    /* A declared LINE with valid H references may still contain
     * unsupported geometry. No false semantic acceptance. */
    MusaDwgAc1018Line line={0};
    assert(!musa_dwg_ac1018_scene_read_line(scene,0x120u,&line));
    MusaDwgAc1018ReactorAudit saved=checked;
    assert(!musa_dwg_ac1018_scene_audit_links(
                      scene,0x120u,NULL,0u,
                      (MusaDwgAc1018ReactorAudit*)(void*)scene));
    assert(musa_dwg_ac1018_scene_handle_count(scene)==3u);
    assert(musa_dwg_ac1018_scene_audit_links(scene,0x120u,
                                             NULL,0u,&checked));
    assert(!memcmp(&checked,&saved,sizeof(saved)));
    assert(!musa_dwg_ac1018_scene_audit_links(scene,0x11bu,
                                               NULL,0u,&checked));
    assert(!memcmp(&checked,&saved,sizeof(saved)));
    file[288u]^=1u;
    assert(musa_dwg_ac1018_scene_audit_links(scene,0x120u,
                                               NULL,0u,&checked));
    MusaDwgAc1018Scene *invalid=scene;
    assert(!musa_dwg_ac1018_scene_open(file,sizeof(file),&invalid));
    assert(invalid==scene);
    musa_dwg_ac1018_scene_close(scene);
    scene=NULL;
    build(file,1,19u);
    assert(musa_dwg_ac1018_scene_open(file,sizeof(file),&scene));
    assert(musa_dwg_ac1018_scene_handle_count(scene)==4u);
    uint64_t reactors[2]={999u,888u};
    assert(musa_dwg_ac1018_scene_audit_links(scene,0x120u,
                                               reactors,2u,&checked));
    assert(checked.reactor_count==1u&&checked.owner_handle==0x11bu&&
           checked.layer_handle==0x33u&&reactors[0]==0x75u&&
           reactors[1]==888u);
    for(int i=0;i<200;i++){
        uint64_t found[2]={999u,888u};
        MusaDwgAc1018ReactorAudit pass={0};
        assert(musa_dwg_ac1018_scene_audit_links(
                         scene,0x120u,found,2u,&pass));
        assert(found[0]==0x75u&&found[1]==888u&&
               pass.reactor_count==1u&&
               pass.record_crc_verified&&pass.handle_membership_verified);
    }
    saved=checked;const uint64_t original_reactor=reactors[0];
    assert(!musa_dwg_ac1018_scene_audit_links(scene,0x120u,
                                                NULL,0u,&checked));
    assert(!musa_dwg_ac1018_scene_audit_links(scene,0x120u,
                                                reactors,0u,&checked));
    assert(!memcmp(&checked,&saved,sizeof(saved))&&
           reactors[0]==original_reactor);
    musa_dwg_ac1018_scene_close(scene);scene=NULL;
    /* Independently synthesize positive- and zero-reactor ARC/CIRCLE
     * common H tails. Numerical type 16 must still fail closed.
     * None of the fixture payloads contains accepted curve geometry. */
    const unsigned claims[]={17u,18u,77u,16u};
    for(size_t k=0;k<sizeof(claims)/sizeof(claims[0]);k++){
        for(int with_reactor=0;with_reactor<=1;with_reactor++){
            build(file,with_reactor,claims[k]);
            assert(musa_dwg_ac1018_scene_open(file,sizeof(file),&scene));
            MusaDwgAc1018RecordProbe probe={0};
            assert(musa_dwg_ac1018_scene_probe_record_handle(
                       scene,0x120u,&probe));
            assert(probe.record_crc_verified&&
                   probe.declared_type==claims[k]);
            MusaDwgAc1018ReactorAudit audit={0};
            uint64_t targets[2]={999u,888u};
            if(claims[k]==16u){
                assert(!musa_dwg_ac1018_scene_audit_links(
                             scene,0x120u,targets,2u,&audit));
                assert(!audit.handle&&targets[0]==999u&&targets[1]==888u);
            }else{
                assert(musa_dwg_ac1018_scene_audit_links(
                             scene,0x120u,targets,2u,&audit));
                assert(audit.handle==0x120u&&audit.owner_handle==0x11bu&&
                       audit.layer_handle==0x33u&&
                       audit.reactor_count==(unsigned)with_reactor&&
                       audit.record_crc_verified&&audit.handle_membership_verified&&
                       !audit.geometry_decoded);
                if(with_reactor)
                    assert(targets[0]==0x75u&&targets[1]==888u);
                else assert(targets[0]==999u&&targets[1]==888u);
                if(claims[k]==17u||claims[k]==18u){
                    MusaDwgAc1018Curve curve={0};
                    assert(!musa_dwg_ac1018_scene_read_curve(
                                 scene,0x120u,&curve));
                }
            }
            musa_dwg_ac1018_scene_close(scene);scene=NULL;
        }
    }
    return 0;
}
