/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "musacad_dwg_ac1018_scene.h"
#include "musacad_crc16.h"
#include <assert.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>

typedef struct Writer {uint8_t data[32];size_t bits;} Writer;
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
                   uint64_t self,int reactor_overflow,int named_layer){
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
    if(named_layer==3||named_layer==4){
        assert(type==50u);
        put(&w,2u,1u);byte(&w,named_layer==4?2u:1u); /* BL */
    }else if(named_layer){
        assert(type==51u);
        put(&w,2u,1u);byte(&w,4u); /* TV length BS */
        for(unsigned i=0;i<4u;i++)byte(&w,(uint8_t)"MEK0"[i]);
        put(&w,1u,1u); /* ever-referenced */
        put(&w,2u,2u); /* xrefindex+1 = 0 */
        put(&w,1u,0u); /* Xdep */
        put(&w,2u,1u);byte(&w,9u); /* flags */
        if(named_layer==2){
            put(&w,2u,2u); /* CMC BS index=0 */
            put(&w,2u,0u); /* BL raw RGB */
            byte(&w,0x33u);byte(&w,0x22u);
            byte(&w,0x11u);byte(&w,0u);
            byte(&w,0u); /* RC: no names */
        }else put(&w,2u,2u); /* incomplete prefix-only CMC */
    }
    const size_t endbit=w.bits;
    for(unsigned i=0;i<4;i++)
        for(unsigned b=0;b<8u;b++)
            setbit(&w,rl_at+(size_t)i*8u+b,
                   ((uint32_t)endbit>>(8u*i+7u-b))&1u);
    if(named_layer==3||named_layer==4){
        byte(&w,0x40u); /* NULL soft pointer root, H 4:0 */
        byte(&w,0x22u);byte(&w,0x01u);byte(&w,0x1bu);
        /* LAYER target H 2:2:01:1b, matching the scene's second handle */
        if(named_layer==4){
            byte(&w,0x22u);byte(&w,0x01u);byte(&w,0x1bu);
        } /* duplicate layer entries must be rejected by scene audit */
    }
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
/* Original R2004 MS/BS/RL/H synthetic link-only LINE.
 * The 80-bit pre-handle section has no geometry; success must not count as
 * decoded LINE coordinates. The final code-5 H points to LAYER 0x11b.
 */
static size_t make_entity_link_only(uint8_t out[64],uint32_t self,int wrong_layer,int unknown_type){
    Writer w={0};
    put(&w,2u,1u);byte(&w,unknown_type?88u:19u); /* BS numeric */
    byte(&w,80u);byte(&w,0u);byte(&w,0u);byte(&w,0u); /* RL */
    assert(self==0x120u||self==0x121u);
    byte(&w,0x02u);byte(&w,(uint8_t)(self>>8u));
    byte(&w,(uint8_t)self); /* original absolute self H */
    put(&w,2u,2u); /* zero EED */
    put(&w,1u,0u); /* no embedded graphics */
    put(&w,2u,2u); /* entity mode 2 -> no owner H */
    put(&w,2u,2u); /* zero reactors */
    put(&w,1u,1u); /* missing XDictionary */
    assert(w.bits==74u);
    while(w.bits<80u)put(&w,1u,0u);
    byte(&w,0x52u);byte(&w,0x01u);
    byte(&w,wrong_layer?0x1cu:0x1bu); /* code 5 / length 2 */
    const size_t body=(w.bits+7u)/8u;
    assert(body<32u);
    memset(out,0u,64u);
    out[0]=(uint8_t)body;out[1]=(uint8_t)(body>>8u);
    memcpy(out+2u,w.data,body);
    uint16_t crc=0u;
    assert(musa_core_crc16_arc(0xc0c1u,out,body+2u,&crc));
    out[body+2u]=(uint8_t)crc;
    out[body+3u]=(uint8_t)(crc>>8u);
    return body+4u;
}
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


/* Independently synthesized 2-target protected original AC1018 file.
 * Handle 0x33 -> LAYER @0, 0x11B -> BLOCK_HEADER @first record length.
 * A wrong embedded self H intentionally leaves outer page and object
 * CRC16 valid, testing that a matching map entry is not sufficient. */
static void construct(uint8_t file[1120],int wrong_layer_self,
                      int wrong_other_self,uint16_t other_type,
                      int named_layer){
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
    uint8_t objs[128]={0},h[18]={0};
    /* Mode 5: type-50 table + fully bit-coded, CMC-bearing type-51 LAYER,
     * both in one original protected R2004 section (no fake lookup). */
    const int table_first=(named_layer==3||named_layer==4||named_layer==5||named_layer==6||named_layer==7||named_layer==8||named_layer==9);
    const size_t first=make(objs,table_first?50u:51u,
                            wrong_layer_self?0x34u:0x33u,
                            0,(named_layer>=5&&named_layer<=9)?3:named_layer);
    const size_t second=make(objs+first,other_type,
                             wrong_other_self?0x11cu:0x11bu,
                             0,(named_layer>=5&&named_layer<=9)?2:0);
    /* Mode 6: actual 3-record table / CMC LAYER / link-only entity
     * fixture. No materialized/guessed link graph or fake DWG geometry. */
    const size_t third=(named_layer>=6&&named_layer<=9)?
        make_entity_link_only(objs+first+second,0x120u,
                              wrong_layer_self!=0,0):0u;
    const size_t fourth=(named_layer==7||named_layer==8||named_layer==9)?
        make_entity_link_only(objs+first+second+third,0x121u,
                              named_layer==9,named_layer==8):0u;
    const size_t total=first+second+third+fourth;
    assert(total<=sizeof(objs)&&first<128u&&first+second<128u);
    h[2]=0x33u;h[3]=0u; /* delta H=0x33 and zero offset */
    h[4]=0xe8u;h[5]=0x01u;h[6]=(uint8_t)first; /* H +232 */
    uint16_t crc=0u;
    size_t hlen=0u;
    if(named_layer==7||named_layer==8||named_layer==9){
        /* Indexed self H sequence: 0x33, 0x11b, 0x120, 0x121. */
        h[1]=11u;h[7]=5u;h[8]=(uint8_t)second;
        h[9]=1u;h[10]=(uint8_t)third;
        assert(musa_core_crc16_arc(0xc0c1u,h,11u,&crc));
        h[11]=(uint8_t)(crc>>8u);h[12]=(uint8_t)crc;
        h[13]=0u;h[14]=2u;
        assert(musa_core_crc16_arc(0xc0c1u,h+13u,2u,&crc));
        h[15]=(uint8_t)(crc>>8u);h[16]=(uint8_t)crc;
        hlen=17u;
    }else if(named_layer==6){
        h[1]=9u;h[7]=5u;h[8]=(uint8_t)second; /* delta H +5 */
        assert(musa_core_crc16_arc(0xc0c1u,h,9u,&crc));
        h[9]=(uint8_t)(crc>>8u);h[10]=(uint8_t)crc;
        h[11]=0u;h[12]=2u;
        assert(musa_core_crc16_arc(0xc0c1u,h+11u,2u,&crc));
        h[13]=(uint8_t)(crc>>8u);h[14]=(uint8_t)crc;
        hlen=15u;
    }else{
        h[1]=7u;
        assert(musa_core_crc16_arc(0xc0c1u,h,7u,&crc));
        h[7]=(uint8_t)(crc>>8u);h[8]=(uint8_t)crc;
        h[9]=0u;h[10]=2u;
        assert(musa_core_crc16_arc(0xc0c1u,h+9u,2u,&crc));
        h[11]=(uint8_t)(crc>>8u);h[12]=(uint8_t)crc;
        hlen=13u;
    }
    page(file,256u,11u,objs,total);
    page(file,576u,10u,h,hlen);
    uint8_t packed[288]={0};
    const uint32_t obj_packed=(uint32_t)lz_data(packed,objs,total);
    const uint32_t h_packed=(uint32_t)lz_data(packed,h,hlen);
    uint8_t layout[244]={0};put32(layout,2u);
    section(layout+20u,total,11u,"AcDb:AcDbObjects",1u,obj_packed);
    section(layout+132u,hlen,10u,"AcDb:Handles",2u,h_packed);
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
    m[0]=0u;m[1]=14u;memcpy(m+2u,map,sizeof(map));
    m[34]=0x11u;
    sys_header(file+1056u,UINT32_C(0x41630e3b),32u,35u);
}
int main(void){
    uint8_t file[1120]={0};
    construct(file,0,0,49u,0);
    MusaDwgAc1018Scene *scene=NULL;
    assert(musa_dwg_ac1018_scene_open(file,sizeof(file),&scene));
    assert(musa_dwg_ac1018_scene_handle_count(scene)==2u);
    /* Opaque/nonentity source records are retained byte-for-byte,
     * instead of disappearing because the geometry vocabulary is smaller. */
    uint8_t raw_layer[128],raw_owner[128];
    memset(raw_layer,0xC7,sizeof(raw_layer));
    memset(raw_owner,0xD5,sizeof(raw_owner));
    size_t layer_bytes=0u,owner_bytes=0u;
    MusaDwgAc1018RecordProbe layer_probe={0},owner_probe={0};
    assert(musa_dwg_ac1018_scene_copy_verified_record(
                 scene,0x33u,raw_layer,sizeof(raw_layer),
                 &layer_bytes,&layer_probe));
    assert(musa_dwg_ac1018_scene_copy_verified_record(
                 scene,0x11bu,raw_owner,sizeof(raw_owner),
                 &owner_bytes,&owner_probe));
    assert(layer_bytes>=4u&&owner_bytes>=4u&&
           layer_bytes<sizeof(raw_layer)&&owner_bytes<sizeof(raw_owner)&&
           layer_probe.handle==0x33u&&layer_probe.declared_type==51u&&
           owner_probe.handle==0x11bu&&owner_probe.declared_type==49u&&
           layer_probe.record_crc_verified&&owner_probe.record_crc_verified);
    assert(raw_layer[layer_bytes]==0xC7&&raw_owner[owner_bytes]==0xD5);
    size_t bad_count=999u;
    MusaDwgAc1018RecordProbe unchanged_probe=layer_probe;
    uint8_t layer_copy[sizeof(raw_layer)];
    memcpy(layer_copy,raw_layer,sizeof(layer_copy));
    assert(!musa_dwg_ac1018_scene_copy_verified_record(
                 scene,0x33u,raw_layer,layer_bytes-1u,
                 &bad_count,&layer_probe));
    assert(!musa_dwg_ac1018_scene_copy_verified_record(
                 scene,0x33u,raw_layer,sizeof(raw_layer),
                 (size_t*)(void*)raw_layer,&layer_probe));
    assert(!musa_dwg_ac1018_scene_copy_verified_record(
                 scene,0x33u,raw_layer,sizeof(raw_layer),
                 &bad_count,(MusaDwgAc1018RecordProbe*)(void*)scene));
    assert(bad_count==999u&&
           !memcmp(raw_layer,layer_copy,sizeof(raw_layer))&&
           !memcmp(&layer_probe,&unchanged_probe,sizeof(layer_probe)));
    MusaDwgAc1018SceneInventory inventory={0};
    assert(musa_dwg_ac1018_scene_inventory(scene,0,&inventory));
    assert(inventory.indexed_record_count==2u&&
           inventory.other_declared_records==2u&&
           inventory.decoded_lines==0u&&
           inventory.undecodable_supported_records==0u&&
           inventory.all_indexed_record_crcs_verified&&
           !inventory.only_supported_primitives_decoded);
    const MusaDwgAc1018SceneInventory saved_inventory=inventory;
    assert(!musa_dwg_ac1018_scene_inventory(scene,1,&inventory));
    assert(!memcmp(&inventory,&saved_inventory,sizeof(inventory)));
    MusaDwgAc1018NonEntityIdentity layer={0},owner={0};
    for(unsigned i=0;i<100u;i++){
        assert(musa_dwg_ac1018_scene_probe_nonentity_identity(
                   scene,0x33u,&layer));
        assert(musa_dwg_ac1018_scene_probe_nonentity_identity(
                   scene,0x11bu,&owner));
    }
    assert(layer.indexed_handle==0x33u&&layer.declared_type==51u&&
           layer.record_crc_verified&&layer.embedded_self_verified);
    assert(owner.indexed_handle==0x11bu&&owner.declared_type==49u&&
           owner.embedded_self_verified&&owner.record_crc_verified);
    MusaDwgAc1018NonEntityIdentity saved=layer;
    assert(!musa_dwg_ac1018_scene_probe_nonentity_identity(
                 scene,0x34u,&layer));
    assert(!musa_dwg_ac1018_scene_probe_nonentity_identity(
                 scene,0u,&layer));
    assert(!musa_dwg_ac1018_scene_probe_nonentity_identity(
                 scene,0x33u,NULL));
    assert(!memcmp(&layer,&saved,sizeof(saved)));
    /* The scene owns decoded bytes, independent of the original buffer. */
    file[288u]^=1u;
    assert(musa_dwg_ac1018_scene_probe_nonentity_identity(
                 scene,0x33u,&layer));
    musa_dwg_ac1018_scene_close(scene);scene=NULL;
    construct(file,1,0,49u,0);
    assert(musa_dwg_ac1018_scene_open(file,sizeof(file),&scene));
    /* Both the outer page and MS record CRC remain correct. */
    MusaDwgAc1018RecordProbe record={0};
    assert(musa_dwg_ac1018_scene_probe_record_handle(
                 scene,0x33u,&record)&&record.record_crc_verified&&
                 record.declared_type==51u);
    /* Valid page CRC and MS record CRC do not excuse a mismatched H.
     * The all-indexed inventory must fail without touching the output. */
    assert(!musa_dwg_ac1018_scene_inventory(scene,0,&inventory));
    assert(!memcmp(&inventory,&saved_inventory,sizeof(inventory)));
    saved=layer;
    assert(!musa_dwg_ac1018_scene_probe_nonentity_identity(
                 scene,0x33u,&layer));
    assert(!memcmp(&layer,&saved,sizeof(saved)));
    assert(musa_dwg_ac1018_scene_probe_nonentity_identity(
                 scene,0x11bu,&owner)); /* correct unaffected target */
    musa_dwg_ac1018_scene_close(scene);scene=NULL;
    /* Same fail-closed policy for the indexed BLOCK_HEADER self identity. */
    construct(file,0,1,49u,0);
    assert(musa_dwg_ac1018_scene_open(file,sizeof(file),&scene));
    assert(musa_dwg_ac1018_scene_probe_record_handle(
                  scene,0x11bu,&record)&&record.record_crc_verified&&
                  record.declared_type==49u);
    assert(!musa_dwg_ac1018_scene_inventory(scene,0,&inventory));
    assert(!memcmp(&inventory,&saved_inventory,sizeof(inventory)));
    musa_dwg_ac1018_scene_close(scene);scene=NULL;
    /* A CRC-backed DICTIONARY (type 42) is still opaque but its
     * embedded indexed identity must agree for non-strict inventory. */
    construct(file,0,0,42u,0);
    assert(musa_dwg_ac1018_scene_open(file,sizeof(file),&scene));
    assert(musa_dwg_ac1018_scene_inventory(scene,0,&inventory));
    assert(inventory.other_declared_records==2u&&
           !inventory.only_supported_primitives_decoded);
    assert(musa_dwg_ac1018_scene_probe_nonentity_identity(
                  scene,0x11bu,&owner)&&owner.declared_type==42u);
    musa_dwg_ac1018_scene_close(scene);scene=NULL;
    const MusaDwgAc1018SceneInventory dictionary_inventory=inventory;
    construct(file,0,1,42u,0);
    assert(musa_dwg_ac1018_scene_open(file,sizeof(file),&scene));
    assert(!musa_dwg_ac1018_scene_inventory(scene,0,&inventory));
    assert(!memcmp(&inventory,&dictionary_inventory,sizeof(inventory)));
    musa_dwg_ac1018_scene_close(scene);scene=NULL;
    /* LAYER_CONTROL (50) is not a layer or a resolved table graph.
     * The complete indexed inventory must nevertheless check its actual
     * embedded H identity, not merely its MS record CRC and type claim. */
    construct(file,0,0,50u,0);
    assert(musa_dwg_ac1018_scene_open(file,sizeof(file),&scene));
    assert(musa_dwg_ac1018_scene_inventory(scene,0,&inventory));
    assert(inventory.indexed_record_count==2u&&
           inventory.other_declared_records==2u&&
           !inventory.only_supported_primitives_decoded);
    const MusaDwgAc1018SceneInventory control_inventory=inventory;
    assert(!musa_dwg_ac1018_scene_inventory(scene,1,&inventory));
    assert(!memcmp(&inventory,&control_inventory,sizeof(inventory)));
    assert(musa_dwg_ac1018_scene_probe_nonentity_identity(
                 scene,0x11bu,&owner)&&owner.declared_type==50u&&
           owner.indexed_handle==0x11bu&&owner.record_crc_verified&&
           owner.embedded_self_verified);
    MusaDwgAc1018LayerPrefix rejected_layer={0};
    assert(!musa_dwg_ac1018_scene_read_layer_prefix(
                 scene,0x11bu,&rejected_layer));
    const MusaDwgAc1018NonEntityIdentity good_control=owner;
    musa_dwg_ac1018_scene_close(scene);scene=NULL;
    /* CRC-valid original R2004 bytes, but H map disagrees with self H. */
    construct(file,0,1,50u,0);
    assert(musa_dwg_ac1018_scene_open(file,sizeof(file),&scene));
    assert(musa_dwg_ac1018_scene_probe_record_handle(
                 scene,0x11bu,&record)&&record.declared_type==50u&&
           record.record_crc_verified);
    assert(!musa_dwg_ac1018_scene_inventory(scene,0,&inventory));
    assert(!memcmp(&inventory,&control_inventory,sizeof(inventory)));
    assert(!musa_dwg_ac1018_scene_probe_nonentity_identity(
                 scene,0x11bu,&owner));
    assert(!memcmp(&owner,&good_control,sizeof(owner)));
    musa_dwg_ac1018_scene_close(scene);scene=NULL;
    construct(file,0,0,49u,1); /* bit-coded named layer in raw DWG */
    assert(musa_dwg_ac1018_scene_open(file,sizeof(file),&scene));
    MusaDwgAc1018LayerPrefix layer_prefix={0};
    assert(musa_dwg_ac1018_scene_read_layer_prefix(
                  scene,0x33u,&layer_prefix));
    assert(!strcmp(layer_prefix.name,"MEK0")&&
           layer_prefix.name_bytes==4u&&
           layer_prefix.raw_layer_flags==9u&&
           layer_prefix.indexed_handle==0x33u&&
           layer_prefix.record_crc_verified&&
           layer_prefix.embedded_self_verified);
    MusaDwgAc1018LayerPrefix unchanged=layer_prefix;
    assert(!musa_dwg_ac1018_scene_read_layer_prefix(
                  scene,0x11bu,&layer_prefix)); /* BLOCK_HEADER != LAYER */
    assert(!musa_dwg_ac1018_scene_read_layer_prefix(
                  scene,0x34u,&layer_prefix)); /* nonexistent handle */
    assert(!musa_dwg_ac1018_scene_read_layer_prefix(
                  scene,0x33u,(MusaDwgAc1018LayerPrefix*)(void*)scene));
    assert(!memcmp(&unchanged,&layer_prefix,sizeof(unchanged)));
    musa_dwg_ac1018_scene_close(scene);scene=NULL;
    construct(file,0,0,49u,2); /* complete bounded pre-handle CMC */
    assert(musa_dwg_ac1018_scene_open(file,sizeof(file),&scene));
    MusaDwgAc1018LayerColor cmc={0};
    assert(musa_dwg_ac1018_scene_read_layer_color(
                  scene,0x33u,&cmc));
    assert(!strcmp(cmc.prefix.name,"MEK0")&&
           cmc.prefix.record_crc_verified&&
           cmc.prefix.embedded_self_verified&&
           cmc.raw_cmc_rgb==UINT32_C(0x00112233)&&
           cmc.cmc_data_bits_verified&&
           !cmc.has_color_name&&!cmc.has_book_name);
    const MusaDwgAc1018LayerColor prior=cmc;
    assert(!musa_dwg_ac1018_scene_read_layer_color(
                  scene,0x11bu,&cmc)); /* wrong type */
    assert(!musa_dwg_ac1018_scene_read_layer_color(
                  scene,0x34u,&cmc)); /* missing H */
    assert(!musa_dwg_ac1018_scene_read_layer_color(
                  scene,0x33u,(MusaDwgAc1018LayerColor*)(void*)scene));
    assert(!memcmp(&cmc,&prior,sizeof(cmc)));
    musa_dwg_ac1018_scene_close(scene);scene=NULL;
    /* A protected original DWG with LAYER_CONTROL @ 0x33 and LAYER @
     * 0x11b: resolve the actual soft-owner H and enforce the target class
     * claim and embedded self identity from the same cached scene. */
    construct(file,0,0,51u,3);
    assert(musa_dwg_ac1018_scene_open(file,sizeof(file),&scene));
    uint64_t refs[2]={UINT64_C(0xdead),UINT64_C(0xbeef)};
    MusaDwgAc1018LayerControlRefs ref_report={0};
    assert(musa_dwg_ac1018_scene_read_layer_control_refs(
                  scene,0x33u,refs,2u,&ref_report));
    assert(ref_report.indexed_handle==0x33u&&
           ref_report.entry_count==1u&&refs[0]==0x11bu&&
           refs[1]==UINT64_C(0xbeef)&&
           ref_report.crc_verified&&ref_report.embedded_self_verified);
    const MusaDwgAc1018LayerControlRefs original_report=ref_report;
    const uint64_t original_refs[2]={refs[0],refs[1]};
    assert(!musa_dwg_ac1018_scene_read_layer_control_refs(
                  scene,0x33u,refs,0u,&ref_report));
    assert(!musa_dwg_ac1018_scene_read_layer_control_refs(
                  scene,0x11bu,refs,2u,&ref_report));
    assert(!musa_dwg_ac1018_scene_read_layer_control_refs(
                  scene,0x33u,(uint64_t*)(void*)scene,2u,&ref_report));
    assert(!musa_dwg_ac1018_scene_read_layer_control_refs(
                  scene,0x33u,refs,2u,
                  (MusaDwgAc1018LayerControlRefs*)(void*)scene));
    assert(!memcmp(refs,original_refs,sizeof(refs))&&
           !memcmp(&ref_report,&original_report,sizeof(ref_report)));
    musa_dwg_ac1018_scene_close(scene);scene=NULL;
    /* Even CRC-valid LAYER_CONTROL -> LAYER targets may not repeat. */
    construct(file,0,0,51u,4);
    assert(musa_dwg_ac1018_scene_open(file,sizeof(file),&scene));
    assert(!musa_dwg_ac1018_scene_read_layer_control_refs(
                  scene,0x33u,refs,2u,&ref_report));
    assert(!memcmp(refs,original_refs,sizeof(refs))&&
           !memcmp(&ref_report,&original_report,sizeof(ref_report)));
    musa_dwg_ac1018_scene_close(scene);scene=NULL;
    /* Reverse membership search proves only uniqueness among
     * CRC/self-H validated table H references. It does not read LAYER's
     * own parent H tail or establish AcDb class semantics. */
    construct(file,0,0,51u,3);
    assert(musa_dwg_ac1018_scene_open(file,sizeof(file),&scene));
    uint64_t table_of_layer=UINT64_C(0xcafe);
    assert(musa_dwg_ac1018_scene_find_layer_control_membership(
                 scene,0x11bu,&table_of_layer)&&table_of_layer==0x33u);
    assert(!musa_dwg_ac1018_scene_find_layer_control_membership(
                 scene,0x33u,&table_of_layer));
    assert(!musa_dwg_ac1018_scene_find_layer_control_membership(
                 scene,0x999u,&table_of_layer));
    assert(!musa_dwg_ac1018_scene_find_layer_control_membership(
                 scene,0x11bu,(uint64_t*)(void*)scene));
    assert(table_of_layer==0x33u);
    musa_dwg_ac1018_scene_close(scene);scene=NULL;
    construct(file,0,0,51u,4); /* duplicate refs in one table */
    assert(musa_dwg_ac1018_scene_open(file,sizeof(file),&scene));
    assert(!musa_dwg_ac1018_scene_find_layer_control_membership(
                 scene,0x11bu,&table_of_layer));
    assert(table_of_layer==0x33u);
    musa_dwg_ac1018_scene_close(scene);scene=NULL;
    /* Complete supported type-50 table membership coverage: every
     * indexed type-51 layer appears once, no orphan or extra reference.
     * This deliberately does NOT claim LAYER embedded owner-H semantics. */
    construct(file,0,0,51u,3);
    assert(musa_dwg_ac1018_scene_open(file,sizeof(file),&scene));
    MusaDwgAc1018LayerTableCoverage coverage={0};
    assert(musa_dwg_ac1018_scene_audit_layer_table_coverage(
                  scene,&coverage));
    assert(coverage.indexed_layers==1u&&
           coverage.indexed_controls==1u&&
           coverage.resolved_layer_references==1u&&
           coverage.all_indexed_layers_have_unique_table);
    const MusaDwgAc1018LayerTableCoverage saved_coverage=coverage;
    assert(!musa_dwg_ac1018_scene_audit_layer_table_coverage(
                   scene,(MusaDwgAc1018LayerTableCoverage*)(void*)scene));
    assert(!memcmp(&coverage,&saved_coverage,sizeof(coverage)));
    musa_dwg_ac1018_scene_close(scene);scene=NULL;
    construct(file,0,0,51u,4); /* duplicate layer references */
    assert(musa_dwg_ac1018_scene_open(file,sizeof(file),&scene));
    assert(!musa_dwg_ac1018_scene_audit_layer_table_coverage(
                   scene,&coverage));
    assert(!memcmp(&coverage,&saved_coverage,sizeof(coverage)));
    musa_dwg_ac1018_scene_close(scene);scene=NULL;
    construct(file,0,0,51u,0); /* type-51 but no table */
    assert(musa_dwg_ac1018_scene_open(file,sizeof(file),&scene));
    assert(!musa_dwg_ac1018_scene_audit_layer_table_coverage(
                   scene,&coverage));
    assert(!memcmp(&coverage,&saved_coverage,sizeof(coverage)));
    musa_dwg_ac1018_scene_close(scene);scene=NULL;
    construct(file,0,0,49u,3); /* table points to a non-LAYER */
    assert(musa_dwg_ac1018_scene_open(file,sizeof(file),&scene));
    assert(!musa_dwg_ac1018_scene_audit_layer_table_coverage(
                   scene,&coverage));
    assert(!memcmp(&coverage,&saved_coverage,sizeof(coverage)));
    musa_dwg_ac1018_scene_close(scene);scene=NULL;
    construct(file,1,0,51u,3); /* table self-H mismatch, CRC valid */
    assert(musa_dwg_ac1018_scene_open(file,sizeof(file),&scene));
    assert(!musa_dwg_ac1018_scene_audit_layer_table_coverage(
                   scene,&coverage));
    assert(!memcmp(&coverage,&saved_coverage,sizeof(coverage)));
    musa_dwg_ac1018_scene_close(scene);scene=NULL;
    construct(file,0,0,49u,0); /* no type-51 layers and no table */
    assert(musa_dwg_ac1018_scene_open(file,sizeof(file),&scene));
    assert(!musa_dwg_ac1018_scene_audit_layer_table_coverage(
                   scene,&coverage));
    assert(!memcmp(&coverage,&saved_coverage,sizeof(coverage)));
    musa_dwg_ac1018_scene_close(scene);scene=NULL;
    construct(file,0,1,51u,3); /* target self-H mismatch, CRC valid */
    assert(musa_dwg_ac1018_scene_open(file,sizeof(file),&scene));
    assert(!musa_dwg_ac1018_scene_audit_layer_table_coverage(
                   scene,&coverage));
    assert(!memcmp(&coverage,&saved_coverage,sizeof(coverage)));
    musa_dwg_ac1018_scene_close(scene);scene=NULL;
    /* Full supported ASCII layer palette from a genuine protected R2004
     * binary fixture. The CMC is verified RAW, not asserted screen RGB. */
    construct(file,0,0,51u,5);
    assert(musa_dwg_ac1018_scene_open(file,sizeof(file),&scene));
    MusaDwgAc1018LayerColor colors[2]={{0}};
    colors[1].raw_cmc_rgb=UINT32_C(0xdeadbeef);
    MusaDwgAc1018LayerPaletteReport palette_report={0};
    assert(musa_dwg_ac1018_scene_read_verified_layer_palette(
                 scene,colors,2u,&palette_report));
    assert(palette_report.indexed_layers==1u&&
           palette_report.indexed_layer_controls==1u&&
           palette_report.unique_ascii_names_verified&&
           palette_report.raw_cmc_fields_verified&&
           colors[0].prefix.indexed_handle==0x11bu&&
           !strcmp(colors[0].prefix.name,"MEK0")&&
           colors[0].raw_cmc_rgb==UINT32_C(0x00112233)&&
           colors[1].raw_cmc_rgb==UINT32_C(0xdeadbeef));
    const MusaDwgAc1018LayerColor keep0=colors[0],keep1=colors[1];
    const MusaDwgAc1018LayerPaletteReport keep_report=palette_report;
    assert(!musa_dwg_ac1018_scene_read_verified_layer_palette(
                 scene,colors,0u,&palette_report));
    assert(!musa_dwg_ac1018_scene_read_verified_layer_palette(
                 scene,(MusaDwgAc1018LayerColor*)(void*)scene,
                 1u,&palette_report));
    assert(!musa_dwg_ac1018_scene_read_verified_layer_palette(
                 scene,colors,2u,
                 (MusaDwgAc1018LayerPaletteReport*)(void*)scene));
    assert(!memcmp(&colors[0],&keep0,sizeof(keep0))&&
           !memcmp(&colors[1],&keep1,sizeof(keep1))&&
           !memcmp(&palette_report,&keep_report,sizeof(keep_report)));
    /* A name lookup must never bypass the complete table/palette gate.
     * ASCII folding is deterministic across all three target platforms. */
    MusaDwgAc1018LayerColor by_name={0};
    assert(musa_dwg_ac1018_scene_find_verified_layer_ascii_name(
                 scene,"mEk0",&by_name));
    assert(by_name.prefix.indexed_handle==0x11bu&&
           !strcmp(by_name.prefix.name,"MEK0")&&
           by_name.raw_cmc_rgb==UINT32_C(0x00112233));
    const MusaDwgAc1018LayerColor named_keep=by_name;
    assert(!musa_dwg_ac1018_scene_find_verified_layer_ascii_name(
                 scene,"OTHER",&by_name));
    assert(!musa_dwg_ac1018_scene_find_verified_layer_ascii_name(
                 scene,"",&by_name));
    assert(!musa_dwg_ac1018_scene_find_verified_layer_ascii_name(
                 scene,"MEK\\n0",&by_name));
    const char invalid_utf8[3]={'M',(char)0xc3,0};
    char too_long[257];
    memset(too_long,'X',256u);too_long[256]=0;
    assert(!musa_dwg_ac1018_scene_find_verified_layer_ascii_name(
                 scene,invalid_utf8,&by_name));
    assert(!musa_dwg_ac1018_scene_find_verified_layer_ascii_name(
                 scene,too_long,&by_name));
    assert(!musa_dwg_ac1018_scene_find_verified_layer_ascii_name(
                 scene,NULL,&by_name));
    assert(!musa_dwg_ac1018_scene_find_verified_layer_ascii_name(
                 scene,"MEK0",(MusaDwgAc1018LayerColor*)(void*)scene));
    assert(!memcmp(&by_name,&named_keep,sizeof(by_name)));
    musa_dwg_ac1018_scene_close(scene);scene=NULL;
    /* Full original protected binary: TABLE -> LAYER <- entity H-tail.
     * Header-only entity link proof, not decoded LINE geometry. */
    construct(file,0,0,51u,6);
    assert(musa_dwg_ac1018_scene_open(file,sizeof(file),&scene));
    assert(musa_dwg_ac1018_scene_handle_count(scene)==3u);
    MusaDwgAc1018EntityLayerEvidence entity_layer={0};
    assert(musa_dwg_ac1018_scene_read_verified_entity_layer(
                  scene,0x120u,&entity_layer));
    assert(entity_layer.entity_handle==0x120u&&
           entity_layer.layer_control_handle==0x33u&&
           entity_layer.layer.prefix.indexed_handle==0x11bu&&
           !strcmp(entity_layer.layer.prefix.name,"MEK0")&&
           entity_layer.layer.raw_cmc_rgb==UINT32_C(0x00112233)&&
           entity_layer.fixed_entity_links_verified&&
           entity_layer.complete_layer_table_verified);
    MusaDwgAc1018Line incomplete_line={0};
    assert(!musa_dwg_ac1018_scene_read_line(
                  scene,0x120u,&incomplete_line));
    /* Batch resolves all requested entities against ONE validated
     * palette + per-layer table map. Header-only entity stays non-geometry. */
    uint64_t requested[2]={UINT64_C(0x120),UINT64_C(0x120)};
    MusaDwgAc1018EntityLayerLink batch_links[2]={{0}};
    batch_links[1].raw_cmc_rgb=UINT32_C(0xdeadbeef);
    MusaDwgAc1018EntityLayerBatchReport batch_report={0};
    assert(musa_dwg_ac1018_scene_read_entity_layer_batch(
                  scene,requested,1u,batch_links,2u,&batch_report));
    assert(batch_links[0].entity_handle==UINT64_C(0x120)&&
           batch_links[0].layer_handle==UINT64_C(0x11b)&&
           batch_links[0].layer_control_handle==UINT64_C(0x33)&&
           batch_links[0].raw_cmc_rgb==UINT32_C(0x00112233)&&
           batch_links[1].raw_cmc_rgb==UINT32_C(0xdeadbeef)&&
           batch_report.verified_entities==1u&&
           batch_report.verified_layers==1u&&
           batch_report.verified_layer_controls==1u&&
           batch_report.verified_unique_entity_handles&&
           batch_report.complete_layer_membership_verified);
    const MusaDwgAc1018EntityLayerLink kept_batch0=batch_links[0];
    const MusaDwgAc1018EntityLayerLink kept_batch1=batch_links[1];
    const MusaDwgAc1018EntityLayerBatchReport kept_batch_report=batch_report;
    /* Duplicate requested IDs cannot cause duplicate viewport instances. */
    assert(!musa_dwg_ac1018_scene_read_entity_layer_batch(
                  scene,requested,2u,batch_links,2u,&batch_report));
    requested[0]=UINT64_C(0x33); /* LAYER is not a 2D entity */
    assert(!musa_dwg_ac1018_scene_read_entity_layer_batch(
                  scene,requested,1u,batch_links,2u,&batch_report));
    requested[0]=UINT64_C(0x121); /* missing entity */
    assert(!musa_dwg_ac1018_scene_read_entity_layer_batch(
                  scene,requested,1u,batch_links,2u,&batch_report));
    requested[0]=UINT64_C(0x120);
    assert(!musa_dwg_ac1018_scene_read_entity_layer_batch(
                  scene,requested,0u,batch_links,2u,&batch_report));
    assert(!musa_dwg_ac1018_scene_read_entity_layer_batch(
                  scene,requested,1u,batch_links,0u,&batch_report));
    assert(!musa_dwg_ac1018_scene_read_entity_layer_batch(
                  scene,requested,1u,batch_links,2u,
                  (MusaDwgAc1018EntityLayerBatchReport*)(void*)scene));
    assert(!musa_dwg_ac1018_scene_read_entity_layer_batch(
                  scene,(const uint64_t*)(const void*)batch_links,
                  1u,batch_links,2u,&batch_report));
    assert(!memcmp(&batch_links[0],&kept_batch0,sizeof(kept_batch0))&&
           !memcmp(&batch_links[1],&kept_batch1,sizeof(kept_batch1))&&
           !memcmp(&batch_report,&kept_batch_report,
                   sizeof(kept_batch_report)));
    const MusaDwgAc1018EntityLayerEvidence good_entity_layer=entity_layer;
    assert(!musa_dwg_ac1018_scene_read_verified_entity_layer(
                  scene,0x11bu,&entity_layer)); /* LAYER != entity */
    assert(!musa_dwg_ac1018_scene_read_verified_entity_layer(
                  scene,0x121u,&entity_layer));
    assert(!musa_dwg_ac1018_scene_read_verified_entity_layer(
                  scene,0x120u,NULL));
    assert(!musa_dwg_ac1018_scene_read_verified_entity_layer(
                  scene,0x120u,
                  (MusaDwgAc1018EntityLayerEvidence*)(void*)scene));
    assert(!memcmp(&entity_layer,&good_entity_layer,sizeof(entity_layer)));
    musa_dwg_ac1018_scene_close(scene);scene=NULL;
    /* Original entity points to absent H although both sections are
     * CRC consistent; reject and preserve caller output. */
    construct(file,1,0,51u,6);
    assert(musa_dwg_ac1018_scene_open(file,sizeof(file),&scene));
    assert(!musa_dwg_ac1018_scene_read_verified_entity_layer(
                  scene,0x120u,&entity_layer));
    assert(!musa_dwg_ac1018_scene_read_entity_layer_batch(
                  scene,requested,1u,batch_links,2u,&batch_report));
    assert(!memcmp(&batch_links[0],&kept_batch0,sizeof(kept_batch0))&&
           !memcmp(&batch_report,&kept_batch_report,
                   sizeof(kept_batch_report)));
    assert(!memcmp(&entity_layer,&good_entity_layer,sizeof(entity_layer)));
    musa_dwg_ac1018_scene_close(scene);scene=NULL;
    /* Two independent entities sharing the same layer: process in
     * caller-specified order, not sorted H order. A bad SECOND entity must
     * invalidate the entire batch without changing the first result. */
    construct(file,0,0,51u,7);
    assert(musa_dwg_ac1018_scene_open(file,sizeof(file),&scene));
    assert(musa_dwg_ac1018_scene_handle_count(scene)==4u);
    requested[0]=UINT64_C(0x121);
    requested[1]=UINT64_C(0x120);
    assert(musa_dwg_ac1018_scene_read_entity_layer_batch(
                  scene,requested,2u,batch_links,2u,&batch_report));
    assert(batch_report.verified_entities==2u&&
           batch_report.verified_layers==1u&&
           batch_links[0].entity_handle==UINT64_C(0x121)&&
           batch_links[1].entity_handle==UINT64_C(0x120)&&
           batch_links[0].layer_handle==UINT64_C(0x11b)&&
           batch_links[1].layer_handle==UINT64_C(0x11b)&&
           batch_links[0].layer_control_handle==UINT64_C(0x33)&&
           batch_links[1].layer_control_handle==UINT64_C(0x33));
    const MusaDwgAc1018EntityLayerLink two_good0=batch_links[0];
    const MusaDwgAc1018EntityLayerLink two_good1=batch_links[1];
    const MusaDwgAc1018EntityLayerBatchReport two_good_report=batch_report;
    requested[1]=UINT64_C(0xdead); /* second entity missing */
    assert(!musa_dwg_ac1018_scene_read_entity_layer_batch(
                  scene,requested,2u,batch_links,2u,&batch_report));
    assert(!memcmp(&batch_links[0],&two_good0,sizeof(two_good0))&&
           !memcmp(&batch_links[1],&two_good1,sizeof(two_good1))&&
           !memcmp(&batch_report,&two_good_report,sizeof(two_good_report)));
    /* Enumerate ALL supported fixed numeric indexed records, not merely
     * a hand-picked caller list. Opaque/unknown records must fail closed. */
    MusaDwgAc1018IndexedLayerLinkCoverage strict={0};
    assert(musa_dwg_ac1018_scene_collect_strict_indexed_layer_links(
                  scene,batch_links,2u,&strict));
    assert(strict.indexed_records_checked==4u&&
           strict.fixed_nonentity_prefixes_checked==2u&&
           strict.supported_entity_links_checked==2u&&
           strict.verified_layers==1u&&
           strict.verified_layer_controls==1u&&
           strict.no_unclassified_record_types&&
           strict.all_supported_entity_links_verified&&
           batch_links[0].entity_handle==UINT64_C(0x120)&&
           batch_links[1].entity_handle==UINT64_C(0x121));
    const MusaDwgAc1018EntityLayerLink strict0=batch_links[0],strict1=batch_links[1];
    /* This protected raw DWG has complete supported fixed H links,
     * but its entities are header-only and have NO geometry coordinates.
     * Never count them as decoded LINE objects. */
    MusaDwgAc1018GeometryLinkEvidence evidence={0};
    assert(musa_dwg_ac1018_scene_audit_2d_geometry_vs_links(
                  scene,&evidence));
    assert(evidence.indexed_records==4u&&
           evidence.declared_supported_2d_entities==2u&&
           evidence.geometry_decoded==0u&&
           evidence.fixed_link_identities_verified==2u&&
           evidence.geometry_and_links_verified==0u&&
           evidence.geometry_only==0u&&
           evidence.links_only==2u&&
           evidence.neither_verified==0u&&
           evidence.other_declared_records==2u&&
           evidence.all_record_ranges_nonoverlapping&&
           evidence.all_indexed_record_crcs_verified);
    /* Four independent protected R2004 original H records; two are
     * fixed nonentities, two are link-only LINEs with NO coordinates.
     * Page with constant memory and preserve the original H order. */
    MusaDwgAc1018RecordDiagnosis diagnoses[3]={{0}};
    MusaDwgAc1018RecordDiagnosisPage diagnostic_page={0};
    assert(musa_dwg_ac1018_scene_read_record_diagnosis_page(
                  scene,0u,diagnoses,2u,&diagnostic_page));
    assert(diagnostic_page.total_indexed_records==4u&&
           diagnostic_page.first_index==0u&&
           diagnostic_page.records_written==2u&&
           diagnostic_page.next_index==2u&&
           diagnostic_page.supported_2d_in_page==0u&&
           diagnostic_page.unclassified_in_page==0u&&
           diagnoses[0].indexed_handle==UINT64_C(0x33)&&
           diagnoses[1].indexed_handle==UINT64_C(0x11b)&&
           diagnoses[0].declared_type==50u&&
           diagnoses[1].declared_type==51u&&
           diagnoses[0].category==MUSA_DWG_AC1018_DIAG_FIXED_NONENTITY&&
           diagnoses[1].category==MUSA_DWG_AC1018_DIAG_FIXED_NONENTITY&&
           !diagnoses[0].evidence_issues&&
           !diagnoses[1].evidence_issues);
    assert(musa_dwg_ac1018_scene_read_record_diagnosis_page(
                  scene,2u,diagnoses,2u,&diagnostic_page));
    assert(diagnostic_page.records_written==2u&&
           diagnostic_page.next_index==4u&&
           diagnostic_page.supported_2d_in_page==2u&&
           diagnostic_page.geometry_rejected_in_page==2u&&
           diagnostic_page.link_audit_rejected_in_page==0u&&
           diagnoses[0].indexed_handle==UINT64_C(0x120)&&
           diagnoses[1].indexed_handle==UINT64_C(0x121)&&
           diagnoses[0].declared_type==19u&&
           diagnoses[1].declared_type==19u&&
           diagnoses[0].category==MUSA_DWG_AC1018_DIAG_2D_ENTITY&&
           diagnoses[1].category==MUSA_DWG_AC1018_DIAG_2D_ENTITY&&
           !diagnoses[0].geometry_decoded&&
           !diagnoses[1].geometry_decoded&&
           diagnoses[0].fixed_link_identities_verified&&
           diagnoses[1].fixed_link_identities_verified&&
           diagnoses[0].evidence_issues==
             MUSA_DWG_AC1018_DIAG_GEOMETRY_REJECTED&&
           diagnoses[1].evidence_issues==
             MUSA_DWG_AC1018_DIAG_GEOMETRY_REJECTED);
    const MusaDwgAc1018RecordDiagnosis saved_diagnosis0=diagnoses[0];
    const MusaDwgAc1018RecordDiagnosis saved_diagnosis1=diagnoses[1];
    const MusaDwgAc1018RecordDiagnosisPage saved_diagnostic_page=
        diagnostic_page;
    assert(!musa_dwg_ac1018_scene_read_record_diagnosis_page(
                  scene,4u,diagnoses,2u,&diagnostic_page));
    assert(!musa_dwg_ac1018_scene_read_record_diagnosis_page(
                  scene,2u,diagnoses,0u,&diagnostic_page));
    assert(!musa_dwg_ac1018_scene_read_record_diagnosis_page(
                  scene,2u,(MusaDwgAc1018RecordDiagnosis*)(void*)scene,
                  2u,&diagnostic_page));
    assert(!musa_dwg_ac1018_scene_read_record_diagnosis_page(
                  scene,2u,diagnoses,2u,
                  (MusaDwgAc1018RecordDiagnosisPage*)(void*)diagnoses));
    assert(!memcmp(&diagnoses[0],&saved_diagnosis0,
                   sizeof(saved_diagnosis0))&&
           !memcmp(&diagnoses[1],&saved_diagnosis1,
                   sizeof(saved_diagnosis1))&&
           !memcmp(&diagnostic_page,&saved_diagnostic_page,
                   sizeof(saved_diagnostic_page)));
    const MusaDwgAc1018GeometryLinkEvidence retained_evidence=evidence;
    /* Snapshot retains fixed nonentities + two independently indexed
     * header-only LINEs; paging later never reparses source records. */
    MusaDwgAc1018DiagnosisSnapshot *snapshot=NULL;
    assert(musa_dwg_ac1018_scene_create_diagnosis_snapshot(
                  scene,&snapshot));
    assert(musa_dwg_ac1018_diagnosis_snapshot_handle_count(snapshot)==4u);
    MusaDwgAc1018RecordDiagnosis cached_rows[2]={{0}};
    MusaDwgAc1018RecordDiagnosisPage cached_page={0};
    assert(musa_dwg_ac1018_diagnosis_snapshot_read_page(
                  snapshot,0u,cached_rows,2u,&cached_page));
    assert(cached_page.records_written==2u&&
           cached_page.next_index==2u&&
           !cached_page.geometry_rejected_in_page&&
           cached_rows[0].indexed_handle==UINT64_C(0x33)&&
           cached_rows[1].indexed_handle==UINT64_C(0x11b)&&
           cached_rows[0].category==
                MUSA_DWG_AC1018_DIAG_FIXED_NONENTITY);
    assert(musa_dwg_ac1018_diagnosis_snapshot_read_page(
                  snapshot,2u,cached_rows,2u,&cached_page));
    assert(cached_page.records_written==2u&&
           cached_page.supported_2d_in_page==2u&&
           cached_page.geometry_rejected_in_page==2u&&
           cached_page.link_audit_rejected_in_page==0u&&
           cached_rows[0].indexed_handle==UINT64_C(0x120)&&
           cached_rows[1].indexed_handle==UINT64_C(0x121)&&
           cached_rows[0].fixed_link_identities_verified&&
           cached_rows[1].fixed_link_identities_verified);

    assert(!musa_dwg_ac1018_scene_audit_2d_geometry_vs_links(
                  scene,(MusaDwgAc1018GeometryLinkEvidence*)(void*)scene));
    assert(!memcmp(&retained_evidence,&evidence,sizeof(evidence)));
    const MusaDwgAc1018IndexedLayerLinkCoverage keep_strict=strict;
    assert(!musa_dwg_ac1018_scene_collect_strict_indexed_layer_links(
                  scene,batch_links,1u,&strict)); /* no partial batch */
    assert(!musa_dwg_ac1018_scene_collect_strict_indexed_layer_links(
                  scene,batch_links,0u,&strict));
    assert(!musa_dwg_ac1018_scene_collect_strict_indexed_layer_links(
                  scene,batch_links,2u,
                  (MusaDwgAc1018IndexedLayerLinkCoverage*)(void*)batch_links));
    assert(!musa_dwg_ac1018_scene_collect_strict_indexed_layer_links(
                  scene,(MusaDwgAc1018EntityLayerLink*)(void*)scene,
                  2u,&strict));
    assert(!memcmp(&strict,&keep_strict,sizeof(strict))&&
           !memcmp(&batch_links[0],&strict0,sizeof(strict0))&&
           !memcmp(&batch_links[1],&strict1,sizeof(strict1)));
    musa_dwg_ac1018_scene_close(scene);scene=NULL;
    /* Strong ownership boundary: cached results remain accessible after
     * the original scene's cached decompressed DWG bytes are destroyed. */
    assert(musa_dwg_ac1018_diagnosis_snapshot_read_page(
                  snapshot,3u,cached_rows,2u,&cached_page));
    assert(cached_page.records_written==1u&&
           cached_rows[0].indexed_handle==UINT64_C(0x121)&&
           cached_rows[0].evidence_issues==
                 MUSA_DWG_AC1018_DIAG_GEOMETRY_REJECTED);
    musa_dwg_ac1018_diagnosis_snapshot_close(snapshot);
    construct(file,1,0,51u,7); /* broken complete table/target graph */
    assert(musa_dwg_ac1018_scene_open(file,sizeof(file),&scene));
    assert(!musa_dwg_ac1018_scene_read_record_diagnosis_page(
                  scene,0u,diagnoses,2u,&diagnostic_page));
    assert(!memcmp(&diagnostic_page,&saved_diagnostic_page,
                   sizeof(saved_diagnostic_page))&&
           !memcmp(&diagnoses[0],&saved_diagnosis0,
                   sizeof(saved_diagnosis0)));
    MusaDwgAc1018DiagnosisSnapshot *invalid_snapshot=NULL;
    assert(!musa_dwg_ac1018_scene_create_diagnosis_snapshot(
                  scene,&invalid_snapshot)&&!invalid_snapshot);
    /* Wrong embedded nonentity self-H is fatal even with valid CRC. */
    assert(!musa_dwg_ac1018_scene_audit_2d_geometry_vs_links(
                  scene,&evidence));
    assert(!memcmp(&retained_evidence,&evidence,sizeof(evidence)));
    assert(!musa_dwg_ac1018_scene_collect_strict_indexed_layer_links(
                  scene,batch_links,2u,&strict));
    assert(!memcmp(&strict,&keep_strict,sizeof(strict))&&
           !memcmp(&batch_links[0],&strict0,sizeof(strict0))&&
           !memcmp(&batch_links[1],&strict1,sizeof(strict1)));
    musa_dwg_ac1018_scene_close(scene);scene=NULL;
    construct(file,0,0,51u,8); /* type 88 instead of second LINE: refuse partial */
    assert(musa_dwg_ac1018_scene_open(file,sizeof(file),&scene));
    /* Unknown type is not ignored: retain it in opaque count, while
     * the separate STRICT admission API rightly refuses this scene. */
    assert(musa_dwg_ac1018_scene_audit_2d_geometry_vs_links(
                  scene,&evidence));
    assert(evidence.indexed_records==4u&&
           evidence.declared_supported_2d_entities==1u&&
           evidence.geometry_decoded==0u&&
           evidence.fixed_link_identities_verified==1u&&
           evidence.links_only==1u&&
           evidence.other_declared_records==3u);
    /* Unknown type may be metadata or unsupported drawable: record its
     * indexed H without pretending the object is safely rendered. */
    assert(musa_dwg_ac1018_scene_read_record_diagnosis_page(
                  scene,3u,diagnoses,1u,&diagnostic_page));
    assert(diagnostic_page.records_written==1u&&
           diagnostic_page.next_index==4u&&
           diagnostic_page.unclassified_in_page==1u&&
           diagnoses[0].indexed_handle==UINT64_C(0x121)&&
           diagnoses[0].declared_type==88u&&
           diagnoses[0].category==MUSA_DWG_AC1018_DIAG_UNCLASSIFIED&&
           diagnoses[0].evidence_issues==
             MUSA_DWG_AC1018_DIAG_UNCLASSIFIED_TYPE);
    assert(!musa_dwg_ac1018_scene_collect_strict_indexed_layer_links(
                  scene,batch_links,2u,&strict));
    assert(!memcmp(&strict,&keep_strict,sizeof(strict))&&
           !memcmp(&batch_links[0],&strict0,sizeof(strict0))&&
           !memcmp(&batch_links[1],&strict1,sizeof(strict1)));
    musa_dwg_ac1018_scene_close(scene);scene=NULL;
    /* A CRC-valid unsupported geometry payload AND dangling layer H
     * must be reported as NEITHER, separately from another valid link-only
     * entity. The fixed table/palette graph is otherwise unaltered. */
    construct(file,0,0,51u,9);
    assert(musa_dwg_ac1018_scene_open(file,sizeof(file),&scene));
    assert(musa_dwg_ac1018_scene_audit_2d_geometry_vs_links(
                  scene,&evidence));
    assert(evidence.declared_supported_2d_entities==2u&&
           evidence.geometry_decoded==0u&&
           evidence.fixed_link_identities_verified==1u&&
           evidence.links_only==1u&&evidence.neither_verified==1u);
    assert(musa_dwg_ac1018_scene_read_record_diagnosis_page(
                  scene,2u,diagnoses,2u,&diagnostic_page));
    assert(diagnostic_page.geometry_rejected_in_page==2u&&
           diagnostic_page.link_audit_rejected_in_page==1u&&
           diagnoses[0].indexed_handle==UINT64_C(0x120)&&
           diagnoses[0].evidence_issues==
              MUSA_DWG_AC1018_DIAG_GEOMETRY_REJECTED&&
           diagnoses[1].indexed_handle==UINT64_C(0x121)&&
           diagnoses[1].evidence_issues==
             (MUSA_DWG_AC1018_DIAG_GEOMETRY_REJECTED|
              MUSA_DWG_AC1018_DIAG_LINK_AUDIT_REJECTED));
    assert(!musa_dwg_ac1018_scene_collect_strict_indexed_layer_links(
                  scene,batch_links,2u,&strict));
    musa_dwg_ac1018_scene_close(scene);scene=NULL;
    /* No false success for a readable name with broken CMC, membership
     * or forged embedded identity. The output must remain unchanged. */
    construct(file,0,0,51u,3); /* self-valid but truncated CMC */
    assert(musa_dwg_ac1018_scene_open(file,sizeof(file),&scene));
    assert(!musa_dwg_ac1018_scene_read_verified_layer_palette(
                  scene,colors,2u,&palette_report));
    assert(!musa_dwg_ac1018_scene_find_verified_layer_ascii_name(
                 scene,"MEK0",&by_name));
    assert(!memcmp(&by_name,&named_keep,sizeof(by_name)));
    assert(!memcmp(&colors[0],&keep0,sizeof(keep0))&&
           !memcmp(&colors[1],&keep1,sizeof(keep1))&&
           !memcmp(&palette_report,&keep_report,sizeof(keep_report)));
    musa_dwg_ac1018_scene_close(scene);scene=NULL;
    construct(file,0,0,51u,4); /* duplicate H */
    assert(musa_dwg_ac1018_scene_open(file,sizeof(file),&scene));
    assert(!musa_dwg_ac1018_scene_read_verified_layer_palette(
                  scene,colors,2u,&palette_report));
    assert(!musa_dwg_ac1018_scene_find_verified_layer_ascii_name(
                 scene,"MEK0",&by_name));
    assert(!memcmp(&by_name,&named_keep,sizeof(by_name)));
    assert(!memcmp(&colors[0],&keep0,sizeof(keep0))&&
           !memcmp(&palette_report,&keep_report,sizeof(keep_report)));
    musa_dwg_ac1018_scene_close(scene);scene=NULL;
    construct(file,0,1,51u,5); /* valid CRC, forged embedded self-H */
    assert(musa_dwg_ac1018_scene_open(file,sizeof(file),&scene));
    assert(!musa_dwg_ac1018_scene_read_verified_layer_palette(
                  scene,colors,2u,&palette_report));
    assert(!musa_dwg_ac1018_scene_find_verified_layer_ascii_name(
                 scene,"MEK0",&by_name));
    assert(!memcmp(&by_name,&named_keep,sizeof(by_name)));
    assert(!memcmp(&colors[0],&keep0,sizeof(keep0))&&
           !memcmp(&palette_report,&keep_report,sizeof(keep_report)));
    musa_dwg_ac1018_scene_close(scene);scene=NULL;
    /* The target exists and has an intact CRC, but is not a LAYER. */
    construct(file,0,0,49u,3);
    assert(musa_dwg_ac1018_scene_open(file,sizeof(file),&scene));
    assert(!musa_dwg_ac1018_scene_read_layer_control_refs(
                  scene,0x33u,refs,2u,&ref_report));
    assert(!memcmp(refs,original_refs,sizeof(refs))&&
           !memcmp(&ref_report,&original_report,sizeof(ref_report)));
    musa_dwg_ac1018_scene_close(scene);scene=NULL;
    /* Target type and CRC are correct, but embedded self-H is forged. */
    construct(file,0,1,51u,3);
    assert(musa_dwg_ac1018_scene_open(file,sizeof(file),&scene));
    assert(!musa_dwg_ac1018_scene_read_layer_control_refs(
                  scene,0x33u,refs,2u,&ref_report));
    assert(!memcmp(refs,original_refs,sizeof(refs))&&
           !memcmp(&ref_report,&original_report,sizeof(ref_report)));
    musa_dwg_ac1018_scene_close(scene);
    return 0;
}
