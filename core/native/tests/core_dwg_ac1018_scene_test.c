/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "musacad_dwg_ac1018_scene.h"
#include "musacad_crc16.h"
#include <assert.h>
#include <math.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
/* Entire raw AC1018 drawing is independently assembled from existing
 * synthetic LINE and LWPOLYLINE unit-test records; no user-owned DWG. */
static const uint8_t line_record[]={
    0x38,0x00,0x44,0xe9,0x40,0x40,0x00,0x00,0x81,0xc3,0xe1,0x74,
    0x21,0xd0,0x00,0x00,0x00,0x00,0x06,0x01,0x8e,0x06,0x00,0x00,
    0x00,0x00,0x00,0x00,0x02,0x80,0x00,0x00,0x00,0x00,0x00,0x00,
    0x22,0x81,0x80,0x00,0x00,0x00,0x00,0x00,0x10,0xe0,0x00,0x00,
    0x00,0x00,0x00,0x00,0x06,0x20,0x1a,0x90,0x0d,0x50,0x1e,0x52,
};
static const uint8_t poly_record[]={
    0x31,0x00,0x53,0x5c,0x00,0x40,0x00,0x00,0x81,0x03,0xa1,0x75,
    0x21,0xd9,0x03,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xc0,0x00,0x00,0x00,0x00,
    0x00,0x0a,0x50,0x03,0x00,0x00,0x00,0x00,0x00,0x00,0x24,0x40,
    0x52,0x01,0xab,0xdc,0xc1,
};
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

/* Two strictly sorted handles, 0x40E poly @0, 0x70F LINE @53,
 * followed by the CRC-checked empty page. */
static void make_handles(uint8_t h[14]){
    memset(h,0u,14u);
    h[1]=8u;
    h[2]=0x8eu;h[3]=0x08u;h[4]=0u;
    h[5]=0x81u;h[6]=0x06u;h[7]=(uint8_t)sizeof(poly_record);
    uint16_t crc=0u;
    assert(musa_core_crc16_arc(0xc0c1u,h,8u,&crc));
    h[8]=(uint8_t)(crc>>8u);h[9]=(uint8_t)crc;
    h[10]=0u;h[11]=2u;
    assert(musa_core_crc16_arc(0xc0c1u,h+10u,2u,&crc));
    h[12]=(uint8_t)(crc>>8u);h[13]=(uint8_t)crc;
}
static void construct(uint8_t f[1120]){
    memset(f,0u,1120u);memcpy(f,"AC1018",6u);
    uint8_t header[0x6c]={0};
    memcpy(header,"AcFssFcAJMB",11u);
    put32(header+0x10u,0x6cu);
    put32(header+0x40u,4u);put32(header+0x50u,4u);
    put32(header+0x5cu,3u);
    put64(header+0x54u,800u);
    uint32_t state=1u;
    for(size_t i=0u;i<sizeof(header);++i){
        state=state*UINT32_C(0x343fd)+UINT32_C(0x269ec3);
        f[0x80u+i]=(uint8_t)(header[i]^(uint8_t)(state>>16u));
    }
    uint8_t objects[sizeof(poly_record)+sizeof(line_record)];
    memcpy(objects,poly_record,sizeof(poly_record));
    memcpy(objects+sizeof(poly_record),line_record,sizeof(line_record));
    uint8_t h[14];make_handles(h);
    page(f,256u,11u,objects,sizeof(objects));
    page(f,576u,10u,h,sizeof(h));
    uint8_t packed[288]={0};
    const uint32_t object_packed=(uint32_t)lz_data(
                                packed,objects,sizeof(objects));
    const uint32_t handle_packed=(uint32_t)lz_data(packed,h,sizeof(h));
    assert(object_packed<288u && handle_packed<160u);
    uint8_t layout[244]={0};
    put32(layout,2u);
    section(layout+20u,sizeof(objects),11u,
            "AcDb:AcDbObjects",1u,object_packed);
    section(layout+132u,sizeof(h),10u,
            "AcDb:Handles",2u,handle_packed);
    uint8_t *s=f+768u+20u;
    s[0]=0u;s[1]=226u;
    memcpy(s+2u,layout,sizeof(layout));
    s[246]=0x11u;
    sys_header(f+768u,UINT32_C(0x4163003b),244u,247u);
    uint8_t map[32]={0};
    put32(map,1u);put32(map+4u,320u);
    put32(map+8u,2u);put32(map+12u,192u);
    put32(map+16u,3u);put32(map+20u,288u);
    put32(map+24u,4u);put32(map+28u,64u);
    uint8_t *m=f+1056u+20u;
    m[0]=0u;m[1]=14u;
    memcpy(m+2u,map,sizeof(map));m[34]=0x11u;
    sys_header(f+1056u,UINT32_C(0x41630e3b),32u,35u);
}
static int near(double a,double b){return fabs(a-b)<1e-9;}
int main(void){
    uint8_t file[1120];construct(file);
    MusaDwgAc1018Scene *s=NULL;
    assert(musa_dwg_ac1018_scene_open(file,sizeof(file),&s));
    assert(s!=NULL&&musa_dwg_ac1018_scene_handle_count(s)==2u);
    MusaDwgAc1018RecordProbe first={0},second={0};
    assert(musa_dwg_ac1018_scene_probe_record_at(s,0u,&first));
    assert(musa_dwg_ac1018_scene_probe_record_at(s,1u,&second));
    assert(first.handle==0x40eu&&first.object_offset==0u&&
           first.declared_type==77u&&
           first.record_bytes==sizeof(poly_record)&&
           first.record_crc_verified);
    assert(second.handle==0x70fu&&
           second.object_offset==sizeof(poly_record)&&
           second.declared_type==19u&&
           second.record_bytes==sizeof(line_record)&&
           second.record_crc_verified);
    MusaDwgAc1018RecordProbe before=second;
    assert(!musa_dwg_ac1018_scene_probe_record_at(s,2u,&second));
    assert(!musa_dwg_ac1018_scene_probe_record_at(s,0u,NULL));
    assert(!memcmp(&before,&second,sizeof(second)));
    MusaDwgAc1018Line line={0};
    MusaDwgAc1018LWPolyline poly={0};
    MusaDwgAc1018LWVertex vertices[4]={{0}};
    for(int n=0;n<100;n++){
        assert(musa_dwg_ac1018_scene_read_line(s,0x70fu,&line));
        assert(near(line.start[0],-17.75)&&near(line.end[1],-8.5));
        assert(musa_dwg_ac1018_scene_read_lwpolyline(
                       s,0x40eu,vertices,4u,&poly));
        assert(poly.vertex_count==3u&&poly.closed==0u);
        assert(near(vertices[1].x,12.5)&&near(vertices[2].y,10.0));
    }
    double wcs[3]={0};
    assert(musa_dwg_ac1018_lwpolyline_vertex_to_wcs(
                   &poly,&vertices[2],wcs));
    assert(near(wcs[0],12.5)&&near(wcs[1],10.0)&&near(wcs[2],0.0));
    MusaDwgAc1018Line line_before=line;
    MusaDwgAc1018LWPolyline poly_before=poly;
    MusaDwgAc1018LWVertex old_vertices[4];
    memcpy(old_vertices,vertices,sizeof(vertices));
    /* All public cached-scene readers must reject a caller output pointed
     * at the opaque scene itself, leaving its index and geometry intact. */
    assert(!musa_dwg_ac1018_scene_probe_record_at(
                  s,0u,(MusaDwgAc1018RecordProbe*)(void*)s));
    assert(!musa_dwg_ac1018_scene_probe_record_handle(
                  s,0x70fu,(MusaDwgAc1018RecordProbe*)(void*)s));
    assert(!musa_dwg_ac1018_scene_read_line(
                  s,0x70fu,(MusaDwgAc1018Line*)(void*)s));
    assert(!musa_dwg_ac1018_scene_read_lwpolyline(
                  s,0x40eu,(MusaDwgAc1018LWVertex*)(void*)s,4u,&poly));
    assert(!musa_dwg_ac1018_scene_read_lwpolyline(
                  s,0x40eu,vertices,4u,(MusaDwgAc1018LWPolyline*)(void*)s));
    assert(musa_dwg_ac1018_scene_handle_count(s)==2u);
    MusaDwgAc1018RecordProbe post_alias={0};
    assert(musa_dwg_ac1018_scene_probe_record_at(s,1u,&post_alias));
    assert(!memcmp(&post_alias,&before,sizeof(before)));
    assert(musa_dwg_ac1018_scene_read_line(s,0x70fu,&line));
    assert(!memcmp(&line,&line_before,sizeof(line)));
    assert(!memcmp(&poly,&poly_before,sizeof(poly)));
    assert(!memcmp(vertices,old_vertices,sizeof(vertices)));
    assert(!musa_dwg_ac1018_scene_read_line(s,0x40eu,&line));
    assert(!musa_dwg_ac1018_scene_read_lwpolyline(
                   s,0x70fu,vertices,4u,&poly));
    assert(!musa_dwg_ac1018_scene_read_lwpolyline(
                   s,0x40eu,vertices,2u,&poly));
    assert(!musa_dwg_ac1018_scene_read_lwpolyline(
                   s,0u,vertices,4u,&poly));
    assert(!musa_dwg_ac1018_scene_read_lwpolyline(s,0x40eu,
                   (MusaDwgAc1018LWVertex*)(void*)&poly,4u,&poly));
    assert(!memcmp(&line,&line_before,sizeof(line)));
    assert(!memcmp(&poly,&poly_before,sizeof(poly)));
    assert(!memcmp(vertices,old_vertices,sizeof(vertices)));
    /* Invalid new source never displaces or corrupts a valid scene. */
    file[288u]^=1u;
    assert(!musa_dwg_ac1018_scene_open(file,sizeof(file),&s));
    assert(musa_dwg_ac1018_scene_handle_count(s)==2u);
    assert(musa_dwg_ac1018_scene_read_line(s,0x70fu,&line));
    musa_dwg_ac1018_scene_close(s);
    s=NULL;
    assert(!musa_dwg_ac1018_scene_open(file,sizeof(file),&s)&&!s);
    construct(file);file[608u]^=1u;
    assert(!musa_dwg_ac1018_scene_open(file,sizeof(file),&s)&&!s);
    construct(file);file[788u]^=1u;
    assert(!musa_dwg_ac1018_scene_open(file,sizeof(file),&s)&&!s);
    construct(file);
    assert(!musa_dwg_ac1018_scene_open(file,sizeof(file)-32u,&s)&&!s);
    musa_dwg_ac1018_scene_close(s);
    return 0;
}
