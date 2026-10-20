/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "musacad_dwg_ac1018_file_curve.h"
#include "musacad_dwg_ac1018_scene.h"
#include "musacad_crc16.h"
#include <assert.h>
#include <math.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
/* Synthetic original protected AC1018 binary DWG, using pre-existing
 * independently synthesized ARC and CIRCLE object records. */
static const uint8_t circle_2d[] = {
    0x27,0x00,0x44,0x89,0x80,0x40,0x00,0x00,0x81,0x26,0xa1,0x74,
    0x21,0xd0,0x00,0x00,0x00,0x00,0x00,0x00,0x09,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x21,0xc0,0x80,0x00,0x00,0x00,0x00,0x00,
    0x01,0xb4,0x0d,0x48,0x04,0xed,0xa7,
};
static const uint8_t arc_3d[] = {
    0x41,0x00,0x44,0x7c,0x80,0x40,0x00,0x00,0x84,0x8d,0x21,0x74,
    0x21,0xd0,0x00,0x00,0x00,0x00,0x00,0x00,0x31,0x02,0x00,0x00,
    0x00,0x00,0x00,0x20,0x07,0x30,0x00,0x00,0x00,0x00,0x00,0x00,
    0x0f,0x43,0xf0,0x00,0x00,0x00,0x00,0x00,0x01,0xc0,0x7e,0x98,
    0x00,0x00,0x00,0x00,0x00,0x00,0xd0,0x3f,0x00,0x00,0x00,0x00,
    0x00,0x00,0x05,0x90,0x14,0x80,0x40,0x08,0x56,
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

/* Two strictly sorted handles, 0x49A CIRCLE @0, 0x1234 ARC @43,
 * followed by the CRC-checked empty page. */
static void make_handles(uint8_t h[14]){
    memset(h,0u,14u);
    h[1]=8u;
    h[2]=0x9au;h[3]=0x09u;h[4]=0u;
    h[5]=0x9au;h[6]=0x1bu;h[7]=(uint8_t)sizeof(circle_2d);
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
    uint8_t objects[sizeof(circle_2d)+sizeof(arc_3d)];
    memcpy(objects,circle_2d,sizeof(circle_2d));
    memcpy(objects+sizeof(circle_2d),arc_3d,sizeof(arc_3d));
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

static int near(double x,double y){return fabs(x-y)<1e-10;}
int main(void){
    uint8_t file[1120];construct(file);
    MusaDwgAc1018Curve circle={0},arc={0};
    assert(musa_dwg_ac1018_file_read_curve(file,sizeof(file),0x49au,&circle));
    assert(musa_dwg_ac1018_file_read_curve(file,sizeof(file),0x1234u,&arc));
    assert(circle.type_code==18u&&circle.handle==0x49au);
    assert(near(circle.center[0],2.25)&&near(circle.center[1],-8.5)&&
           near(circle.center[2],0.0)&&near(circle.radius,6.75));
    assert(near(circle.extrusion[2],1.0));
    assert(arc.type_code==17u&&arc.handle==0x1234u);
    assert(near(arc.center[0],3.5)&&near(arc.center[2],-7.125)&&
           near(arc.radius,1.25)&&near(arc.start_angle,0.25)&&
           near(arc.end_angle,5.5)&&near(arc.extrusion[1],1.0));
    const MusaDwgAc1018Curve saved_circle=circle,saved_arc=arc;
    MusaDwgAc1018Scene *s=NULL;
    assert(musa_dwg_ac1018_scene_open(file,sizeof(file),&s));
    assert(musa_dwg_ac1018_scene_handle_count(s)==2u);
    MusaDwgAc1018SceneInventory inventory={0};
    assert(musa_dwg_ac1018_scene_inventory(s,1,&inventory));
    assert(inventory.indexed_record_count==2u&&
           inventory.decoded_arcs==1u&&inventory.decoded_circles==1u&&
           inventory.decoded_lines==0u&&inventory.decoded_lwpolylines==0u&&
           inventory.other_declared_records==0u&&
           inventory.undecodable_supported_records==0u&&
           inventory.all_indexed_record_crcs_verified&&
           inventory.only_supported_primitives_decoded);
    MusaDwgAc1018RecordProbe a={0},b={0};
    assert(musa_dwg_ac1018_scene_probe_record_at(s,0u,&a));
    assert(musa_dwg_ac1018_scene_probe_record_at(s,1u,&b));
    assert(a.handle==0x49au&&a.declared_type==18u&&
           a.record_bytes==sizeof(circle_2d));
    assert(b.handle==0x1234u&&b.declared_type==17u&&
           b.object_offset==sizeof(circle_2d)&&
           b.record_bytes==sizeof(arc_3d));
    for(int i=0;i<100;i++){
        assert(musa_dwg_ac1018_scene_read_curve(s,0x49au,&circle));
        assert(musa_dwg_ac1018_scene_read_curve(s,0x1234u,&arc));
    }
    /* Analytic extrema on original CRC-protected circles/arcs, NOT
     * endpoint-only bounds. Circle is XY; ARC normal points along +Y. */
    MusaDwgAc1018CurveWcsBounds circle_box={0},arc_box={0};
    assert(musa_dwg_ac1018_scene_read_curve_wcs_bounds(
                  s,0x49au,&circle_box));
    assert(circle_box.handle==0x49au&&circle_box.type_code==18u&&
           near(circle_box.radius,6.75)&&
           near(circle_box.minimum[0],-4.5)&&
           near(circle_box.maximum[0],9.0)&&
           near(circle_box.minimum[1],-15.25)&&
           near(circle_box.maximum[1],-1.75)&&
           near(circle_box.minimum[2],0.0)&&
           near(circle_box.maximum[2],0.0));
    assert(musa_dwg_ac1018_scene_read_curve_wcs_bounds(
                  s,0x1234u,&arc_box));
    assert(arc_box.handle==0x1234u&&arc_box.type_code==17u&&
           near(arc_box.radius,1.25)&&
           near(arc_box.center_wcs[0],-3.5)&&
           near(arc_box.center_wcs[1],-7.125)&&
           near(arc_box.minimum[0],-3.5-1.25*cos(0.25))&&
           near(arc_box.maximum[0],-3.5+1.25)&&
           near(arc_box.minimum[1],-7.125)&&
           near(arc_box.maximum[1],-7.125)&&
           near(arc_box.minimum[2],arc_box.center_wcs[2]-1.25)&&
           near(arc_box.maximum[2],arc_box.center_wcs[2]+1.25));
    /* Dense independent parametric reference samples lie inside the
     * exact computed bounds; endpoints alone would miss extrema. */
    for(unsigned j=0u;j<=1000u;j++){
        const double a=arc.start_angle+
            (arc.end_angle-arc.start_angle)*(double)j/1000.0;
        const double point[3]={
            arc_box.center_wcs[0]-arc.radius*cos(a),
            arc_box.center_wcs[1],
            arc_box.center_wcs[2]+arc.radius*sin(a)};
        for(unsigned axis=0u;axis<3u;axis++){
            assert(point[axis]>=arc_box.minimum[axis]-1e-9&&
                   point[axis]<=arc_box.maximum[axis]+1e-9);
        }
    }
    const MusaDwgAc1018CurveWcsBounds saved_box=arc_box;
    assert(!musa_dwg_ac1018_scene_read_curve_wcs_bounds(
                 s,0x1235u,&arc_box));
    assert(!musa_dwg_ac1018_scene_read_curve_wcs_bounds(
                 s,0x49au,NULL));
    assert(!musa_dwg_ac1018_scene_read_curve_wcs_bounds(
                 s,0x49au,(MusaDwgAc1018CurveWcsBounds*)(void*)s));
    assert(!memcmp(&arc_box,&saved_box,sizeof(arc_box)));

    assert(memcmp(&circle,&saved_circle,sizeof(circle))==0);
    assert(memcmp(&arc,&saved_arc,sizeof(arc))==0);
    assert(!musa_dwg_ac1018_scene_read_curve(s,0u,&arc));
    assert(!musa_dwg_ac1018_scene_read_curve(s,0x1235u,&arc));
    assert(!musa_dwg_ac1018_scene_read_curve(s,0x49au,NULL));
    /* An accidental output alias must not corrupt the cached scene. */
    assert(!musa_dwg_ac1018_scene_read_curve(
                   s,0x49au,(MusaDwgAc1018Curve*)(void*)s));
    assert(musa_dwg_ac1018_scene_handle_count(s)==2u);
    assert(musa_dwg_ac1018_scene_read_curve(s,0x49au,&circle));
    assert(memcmp(&circle,&saved_circle,sizeof(circle))==0);
    MusaDwgAc1018Line line={0};
    assert(!musa_dwg_ac1018_scene_read_line(s,0x49au,&line));
    assert(memcmp(&arc,&saved_arc,sizeof(arc))==0);
    musa_dwg_ac1018_scene_close(s);
    uint8_t before[1120];memcpy(before,file,sizeof(file));
    assert(!musa_dwg_ac1018_file_read_curve(file,sizeof(file),
                   0x49au,(MusaDwgAc1018Curve*)(void*)(file+20u)));
    assert(memcmp(file,before,sizeof(file))==0);
    assert(!musa_dwg_ac1018_file_read_curve(file,sizeof(file),0x1235u,&arc));
    assert(memcmp(&arc,&saved_arc,sizeof(arc))==0);
    file[288u]^=1u; /* corrupt protected Objects page */
    assert(!musa_dwg_ac1018_file_read_curve(file,sizeof(file),0x1234u,&arc));
    assert(memcmp(&arc,&saved_arc,sizeof(arc))==0);
    construct(file);file[608u]^=1u; /* corrupt protected Handles page */
    assert(!musa_dwg_ac1018_file_read_curve(file,sizeof(file),0x49au,&circle));
    construct(file);
    assert(!musa_dwg_ac1018_file_read_curve(file,sizeof(file)-32u,0x49au,&circle));
    assert(memcmp(&circle,&saved_circle,sizeof(circle))==0);
    return 0;
}
