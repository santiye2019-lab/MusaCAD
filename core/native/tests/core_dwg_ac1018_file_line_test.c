/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "musacad_dwg_ac1018_file_line.h"
#include "musacad_crc16.h"
#include <assert.h>
#include <math.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
/* Independent synthetic raw R2004 container using existing published
 * zero-private-data LINE fixture. No user DWG coordinates/files included. */
static const uint8_t line_record[]={
    0x38,0x00,0x44,0xe9,0x40,0x40,0x00,0x00,0x81,0xc3,0xe1,0x74,
    0x21,0xd0,0x00,0x00,0x00,0x00,0x06,0x01,0x8e,0x06,0x00,0x00,
    0x00,0x00,0x00,0x00,0x02,0x80,0x00,0x00,0x00,0x00,0x00,0x00,
    0x22,0x81,0x80,0x00,0x00,0x00,0x00,0x00,0x10,0xe0,0x00,0x00,
    0x00,0x00,0x00,0x00,0x06,0x20,0x1a,0x90,0x0d,0x50,0x1e,0x52,
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
    assert(packed<224u);
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
static void construct(uint8_t f[1056]){
    memset(f,0u,1056u);memcpy(f,"AC1018",6u);
    uint8_t header[0x6c]={0};
    memcpy(header,"AcFssFcAJMB",11u);
    put32(header+0x10u,0x6cu);
    put32(header+0x40u,4u);put32(header+0x50u,4u);
    put32(header+0x5cu,3u);
    put64(header+0x54u,736u);
    uint32_t state=1u;
    for(size_t i=0u;i<sizeof(header);i++){
        state=state*UINT32_C(0x343fd)+UINT32_C(0x269ec3);
        f[0x80u+i]=(uint8_t)(header[i]^(uint8_t)(state>>16u));
    }
    uint8_t handle[11]={0u,5u,0x8fu,0x0eu,0u,0u,0u,0u,2u,0u,0u};
    uint16_t crc=0u;
    assert(musa_core_crc16_arc(0xc0c1u,handle,5u,&crc));
    handle[5]=(uint8_t)(crc>>8u);handle[6]=(uint8_t)crc;
    assert(musa_core_crc16_arc(0xc0c1u,handle+7u,2u,&crc));
    handle[9]=(uint8_t)(crc>>8u);handle[10]=(uint8_t)crc;
    page(f,256u,11u,line_record,sizeof(line_record));
    page(f,512u,10u,handle,sizeof(handle));
    const uint32_t opacked=(uint32_t)(sizeof(line_record)+
                      (sizeof(line_record)<=18u?1u:2u)+
                      1u+1u+(29696u-sizeof(line_record)-33u-256u)/255u+1u+3u);
    const uint32_t hpacked=(uint32_t)(sizeof(handle)+1u+
                       1u+1u+(29696u-sizeof(handle)-33u-256u)/255u+1u+3u);
    uint8_t layout[244]={0};
    put32(layout,2u);
    section(layout+20u,sizeof(line_record),11u,
            "AcDb:AcDbObjects",1u,opacked);
    section(layout+132u,11u,10u,"AcDb:Handles",2u,hpacked);
    uint8_t *s=f+704u+20u;
    s[0]=0u;s[1]=226u;memcpy(s+2u,layout,sizeof(layout));
    s[246]=0x11u;
    sys_header(f+704u,UINT32_C(0x4163003b),244u,247u);
    uint8_t map[32]={0};
    put32(map,1u);put32(map+4u,256u);
    put32(map+8u,2u);put32(map+12u,192u);
    put32(map+16u,3u);put32(map+20u,288u);
    put32(map+24u,4u);put32(map+28u,64u);
    uint8_t *m=f+992u+20u;
    m[0]=0u;m[1]=14u;memcpy(m+2u,map,32u);
    m[34]=0x11u;
    sys_header(f+992u,UINT32_C(0x41630e3b),32u,35u);
}
static int near(double a,double b){return fabs(a-b)<1e-9;}
int main(void){
    uint8_t file[1056];construct(file);
    MusaDwgAc1018Line line={0};
    assert(musa_dwg_ac1018_file_read_line(file,sizeof(file),0x70fu,&line));
    assert(line.handle==0x70fu&&near(line.start[0],-17.75)&&
           near(line.start[1],4.25)&&near(line.start[2],3.5)&&
           near(line.end[0],2.125)&&near(line.end[1],-8.5)&&
           near(line.end[2],3.5));
    MusaDwgAc1018Line unchanged=line;
    assert(!musa_dwg_ac1018_file_read_line(file,sizeof(file),0x710u,&line));
    assert(!musa_dwg_ac1018_file_read_line(file,sizeof(file),0u,&line));
    assert(!musa_dwg_ac1018_file_read_line(file,sizeof(file)-34u,0x70fu,&line));
    assert(!musa_dwg_ac1018_file_read_line(file,sizeof(file),0x70fu,
         (MusaDwgAc1018Line*)(void*)(file+16u)));
    construct(file);file[288u]^=1u;
    assert(!musa_dwg_ac1018_file_read_line(file,sizeof(file),0x70fu,&line));
    construct(file);file[544u]^=1u;
    assert(!musa_dwg_ac1018_file_read_line(file,sizeof(file),0x70fu,&line));
    construct(file);file[724u]^=1u;
    assert(!musa_dwg_ac1018_file_read_line(file,sizeof(file),0x70fu,&line));
    assert(!memcmp(&line,&unchanged,sizeof(line)));
    return 0;
}
