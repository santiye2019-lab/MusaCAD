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
static const uint8_t bulged_record[] = {
    0x80,0x00,0x53,0x79,0x00,0xc0,0x00,0x00,0x81,0x0a,0xd0,0x35,
    0x11,0x20,0x10,0x20,0x38,0x5d,0x48,0x74,0x3f,0x03,0x00,0x00,
    0x00,0x00,0x00,0x00,0x3a,0x0f,0xc0,0x00,0x00,0x00,0x00,0x00,
    0x00,0xc4,0x00,0x00,0x00,0x00,0x00,0x00,0x03,0x40,0xfe,0x64,
    0x0d,0x03,0x40,0xc0,0x00,0x00,0x00,0x00,0x00,0x3e,0x0f,0xc0,
    0x00,0x00,0x00,0x00,0x00,0x04,0xb0,0x30,0x00,0x00,0x00,0x00,
    0x00,0x00,0x44,0x00,0xc0,0x00,0x00,0x00,0x00,0x00,0x05,0x50,
    0x00,0x00,0x00,0x00,0x00,0x00,0x0e,0x03,0xf8,0x00,0x00,0x00,
    0x00,0x00,0x00,0xd0,0xbf,0x00,0x00,0x00,0x00,0x00,0x00,0x30,
    0x0f,0xc0,0x00,0x00,0x00,0x00,0x00,0x0d,0x03,0xfa,0x40,0x00,
    0x00,0x00,0x00,0x00,0x0f,0x83,0xf5,0x20,0x1a,0xb0,0x25,0xb0,
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
static void make_handles(uint8_t h[14],int overlap){
    memset(h,0u,14u);
    h[1]=8u;
    h[2]=0x8eu;h[3]=0x08u;h[4]=0u;
    h[5]=0x81u;h[6]=0x06u;h[7]=overlap?0u:(uint8_t)sizeof(poly_record);
    uint16_t crc=0u;
    assert(musa_core_crc16_arc(0xc0c1u,h,8u,&crc));
    h[8]=(uint8_t)(crc>>8u);h[9]=(uint8_t)crc;
    h[10]=0u;h[11]=2u;
    assert(musa_core_crc16_arc(0xc0c1u,h+10u,2u,&crc));
    h[12]=(uint8_t)(crc>>8u);h[13]=(uint8_t)crc;
}
static void construct(uint8_t f[1120],int overlap){
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
    uint8_t h[14];make_handles(h,overlap);
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

/* Protected original R2004 bulge/width fixture with one indexed H=0x42b.
 * Keep its source geometry unmodified; no private CAD data are used. */
static void construct_bulged(uint8_t f[1120]){
    construct(f,0);
    uint8_t h[11]={0u,5u,0xabu,0x08u,0u};
    uint16_t crc=0u;
    assert(musa_core_crc16_arc(0xc0c1u,h,5u,&crc));
    h[5]=(uint8_t)(crc>>8u);h[6]=(uint8_t)crc;
    h[7]=0u;h[8]=2u;
    assert(musa_core_crc16_arc(0xc0c1u,h+7u,2u,&crc));
    h[9]=(uint8_t)(crc>>8u);h[10]=(uint8_t)crc;
    page(f,256u,11u,bulged_record,sizeof(bulged_record));
    page(f,576u,10u,h,sizeof(h));
    uint8_t packed[288]={0};
    const uint32_t po=(uint32_t)lz_data(
                  packed,bulged_record,sizeof(bulged_record));
    const uint32_t ph=(uint32_t)lz_data(packed,h,sizeof(h));
    assert(po<288u&&ph<160u);
    uint8_t layout[244]={0};
    put32(layout,2u);
    section(layout+20u,sizeof(bulged_record),11u,
            "AcDb:AcDbObjects",1u,po);
    section(layout+132u,sizeof(h),10u,
            "AcDb:Handles",2u,ph);
    uint8_t *s=f+768u+20u;
    s[0]=0u;s[1]=226u;memcpy(s+2u,layout,sizeof(layout));
    s[246]=0x11u;
    sys_header(f+768u,UINT32_C(0x4163003b),244u,247u);
}
static int near(double a,double b){return fabs(a-b)<1e-9;}
int main(void){
    uint8_t file[1120];construct(file,0);
    MusaDwgAc1018Scene *s=NULL;
    assert(musa_dwg_ac1018_scene_open(file,sizeof(file),&s));
    assert(s!=NULL&&musa_dwg_ac1018_scene_handle_count(s)==2u);
    MusaDwgAc1018RecordRangeAudit layout={0};
    assert(musa_dwg_ac1018_scene_audit_record_ranges(s,&layout));
    assert(layout.indexed_records==2u&&
           layout.covered_record_bytes==sizeof(poly_record)+sizeof(line_record)&&
           layout.unindexed_section_bytes==0u&&layout.ranges_nonoverlapping);
    MusaDwgAc1018SceneInventory inventory={0};
    for(int n=0;n<50;n++)
        assert(musa_dwg_ac1018_scene_inventory(s,1,&inventory));
    assert(inventory.indexed_record_count==2u&&
           inventory.decoded_lines==1u&&
           inventory.decoded_lwpolylines==1u&&
           inventory.decoded_arcs==0u&&inventory.decoded_circles==0u&&
           inventory.other_declared_records==0u&&
           inventory.undecodable_supported_records==0u&&
           inventory.all_indexed_record_crcs_verified&&
           inventory.only_supported_primitives_decoded);
    /* Geometry and layer-link evidence must never be conflated.
     * This protected raw AC1018 binary has fully decoded geometry but
     * no LAYER table or verified fixed-role links. */
    MusaDwgAc1018GeometryLinkEvidence evidence={0};
    assert(musa_dwg_ac1018_scene_audit_2d_geometry_vs_links(
                  s,&evidence));
    assert(evidence.indexed_records==2u&&
           evidence.declared_supported_2d_entities==2u&&
           evidence.geometry_decoded==2u&&
           evidence.fixed_link_identities_verified==0u&&
           evidence.geometry_and_links_verified==0u&&
           evidence.geometry_only==2u&&
           evidence.links_only==0u&&
           evidence.neither_verified==0u&&
           evidence.other_declared_records==0u&&
           evidence.all_record_ranges_nonoverlapping&&
           evidence.all_indexed_record_crcs_verified);
    /* Diagnostic pages retain indexed H and numeric type for every
     * record. Real decoded geometry does NOT prove layer H semantics.
     * First page has POLYLINE, second has LINE; no row is omitted. */
    MusaDwgAc1018RecordDiagnosis diagnoses[3]={{0}};
    MusaDwgAc1018RecordDiagnosisPage diagnostic_page={0};
    assert(musa_dwg_ac1018_scene_read_record_diagnosis_page(
                  s,0u,diagnoses,1u,&diagnostic_page));
    assert(diagnostic_page.total_indexed_records==2u&&
           diagnostic_page.first_index==0u&&
           diagnostic_page.records_written==1u&&
           diagnostic_page.next_index==1u&&
           diagnostic_page.supported_2d_in_page==1u&&
           diagnostic_page.geometry_rejected_in_page==0u&&
           diagnostic_page.link_audit_rejected_in_page==1u&&
           diagnostic_page.unclassified_in_page==0u&&
           diagnoses[0].indexed_handle==UINT64_C(0x40e)&&
           diagnoses[0].declared_type==77u&&
           diagnoses[0].category==MUSA_DWG_AC1018_DIAG_2D_ENTITY&&
           diagnoses[0].geometry_decoded&&
           !diagnoses[0].fixed_link_identities_verified&&
           diagnoses[0].evidence_issues==
             MUSA_DWG_AC1018_DIAG_LINK_AUDIT_REJECTED);
    assert(musa_dwg_ac1018_scene_read_record_diagnosis_page(
                  s,1u,diagnoses,2u,&diagnostic_page));
    assert(diagnostic_page.records_written==1u&&
           diagnostic_page.next_index==2u&&
           diagnoses[0].indexed_handle==UINT64_C(0x70f)&&
           diagnoses[0].declared_type==19u&&
           diagnoses[0].geometry_decoded&&
           diagnoses[0].evidence_issues==
             MUSA_DWG_AC1018_DIAG_LINK_AUDIT_REJECTED);
    assert(musa_dwg_ac1018_scene_read_record_diagnosis_page(
                  s,0u,diagnoses,2u,&diagnostic_page));
    assert(diagnostic_page.records_written==2u&&
           diagnostic_page.next_index==2u&&
           diagnoses[0].indexed_handle==UINT64_C(0x40e)&&
           diagnoses[1].indexed_handle==UINT64_C(0x70f));
    const MusaDwgAc1018RecordDiagnosis kept_diagnosis0=diagnoses[0];
    const MusaDwgAc1018RecordDiagnosis kept_diagnosis1=diagnoses[1];
    const MusaDwgAc1018RecordDiagnosisPage kept_diagnostic_page=
        diagnostic_page;
    /* One full-scene audit/materialization, then O(page) reads without
     * consulting original DWG or scene. Original H and flags preserved. */
    MusaDwgAc1018DiagnosisSnapshot *snapshot=NULL;
    assert(musa_dwg_ac1018_scene_create_diagnosis_snapshot(s,&snapshot));
    assert(musa_dwg_ac1018_diagnosis_snapshot_handle_count(snapshot)==2u);
    MusaDwgAc1018RecordDiagnosis cached_rows[3]={{0}};
    MusaDwgAc1018RecordDiagnosisPage cached_page={0};
    assert(musa_dwg_ac1018_diagnosis_snapshot_read_page(
                  snapshot,0u,cached_rows,1u,&cached_page));
    assert(cached_page.total_indexed_records==2u&&
           cached_page.records_written==1u&&
           cached_page.next_index==1u&&
           cached_page.link_audit_rejected_in_page==1u&&
           cached_rows[0].indexed_handle==UINT64_C(0x40e)&&
           cached_rows[0].geometry_decoded&&
           cached_rows[0].evidence_issues==
                MUSA_DWG_AC1018_DIAG_LINK_AUDIT_REJECTED);
    assert(musa_dwg_ac1018_diagnosis_snapshot_read_page(
                  snapshot,0u,cached_rows,2u,&cached_page));
    assert(cached_page.records_written==2u&&
           cached_page.next_index==2u&&
           cached_page.link_audit_rejected_in_page==2u&&
           cached_rows[0].indexed_handle==UINT64_C(0x40e)&&
           cached_rows[1].indexed_handle==UINT64_C(0x70f)&&
           cached_rows[1].geometry_decoded);
    MusaDwgAc1018RecordDiagnosis saved_cache0=cached_rows[0];
    MusaDwgAc1018RecordDiagnosis saved_cache1=cached_rows[1];
    MusaDwgAc1018RecordDiagnosisPage saved_cache_page=cached_page;
    assert(!musa_dwg_ac1018_diagnosis_snapshot_read_page(
                  snapshot,2u,cached_rows,2u,&cached_page));
    assert(!musa_dwg_ac1018_diagnosis_snapshot_read_page(
                  snapshot,0u,cached_rows,0u,&cached_page));
    assert(!musa_dwg_ac1018_diagnosis_snapshot_read_page(
                  snapshot,0u,
                  (MusaDwgAc1018RecordDiagnosis*)(void*)snapshot,
                  1u,&cached_page));
    assert(!musa_dwg_ac1018_diagnosis_snapshot_read_page(
                  snapshot,0u,cached_rows,2u,
                  (MusaDwgAc1018RecordDiagnosisPage*)(void*)cached_rows));
    assert(!musa_dwg_ac1018_scene_create_diagnosis_snapshot(
                  s,(MusaDwgAc1018DiagnosisSnapshot**)(void*)s));
    assert(!memcmp(&cached_rows[0],&saved_cache0,sizeof(saved_cache0))&&
           !memcmp(&cached_rows[1],&saved_cache1,sizeof(saved_cache1))&&
           !memcmp(&cached_page,&saved_cache_page,
                   sizeof(saved_cache_page)));

    assert(!musa_dwg_ac1018_scene_read_record_diagnosis_page(
                  s,2u,diagnoses,2u,&diagnostic_page));
    assert(!musa_dwg_ac1018_scene_read_record_diagnosis_page(
                  s,0u,diagnoses,0u,&diagnostic_page));
    assert(!musa_dwg_ac1018_scene_read_record_diagnosis_page(
                  s,0u,(MusaDwgAc1018RecordDiagnosis*)(void*)s,
                  2u,&diagnostic_page));
    assert(!musa_dwg_ac1018_scene_read_record_diagnosis_page(
                  s,0u,diagnoses,2u,
                  (MusaDwgAc1018RecordDiagnosisPage*)(void*)diagnoses));
    assert(!memcmp(&diagnoses[0],&kept_diagnosis0,
                   sizeof(kept_diagnosis0))&&
           !memcmp(&diagnoses[1],&kept_diagnosis1,
                   sizeof(kept_diagnosis1))&&
           !memcmp(&diagnostic_page,&kept_diagnostic_page,
                   sizeof(kept_diagnostic_page)));
    const MusaDwgAc1018GeometryLinkEvidence keep_evidence=evidence;
    assert(!musa_dwg_ac1018_scene_audit_2d_geometry_vs_links(
                  s,NULL));
    assert(!musa_dwg_ac1018_scene_audit_2d_geometry_vs_links(
                  s,(MusaDwgAc1018GeometryLinkEvidence*)(void*)s));
    assert(!memcmp(&keep_evidence,&evidence,sizeof(evidence)));
    const MusaDwgAc1018SceneInventory saved_inventory=inventory;
    assert(!musa_dwg_ac1018_scene_inventory(s,2,&inventory));
    assert(!musa_dwg_ac1018_scene_inventory(s,0,NULL));
    assert(!musa_dwg_ac1018_scene_inventory(
               s,0,(MusaDwgAc1018SceneInventory*)(void*)s));
    assert(!memcmp(&inventory,&saved_inventory,sizeof(inventory))&&
           musa_dwg_ac1018_scene_handle_count(s)==2u);
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
    /* Bounding data is computed from CRC-verified actual coordinates,
     * not the presence of an H record or guessed view dimensions. */
    MusaDwgAc1018LinearWcsBounds line_box={0},poly_box={0};
    assert(musa_dwg_ac1018_scene_read_linear_wcs_bounds(
                  s,0x70fu,&line_box));
    assert(line_box.handle==0x70fu&&line_box.type_code==19u&&
           line_box.vertex_count==2u&&line_box.segment_count==1u);
    for(size_t axis=0u;axis<3u;axis++){
        assert(near(line_box.minimum[axis],
                    fmin(line.start[axis],line.end[axis])));
        assert(near(line_box.maximum[axis],
                    fmax(line.start[axis],line.end[axis])));
    }
    assert(musa_dwg_ac1018_scene_read_linear_wcs_bounds(
                  s,0x40eu,&poly_box));
    assert(poly_box.handle==0x40eu&&poly_box.type_code==77u&&
           poly_box.vertex_count==3u&&poly_box.segment_count==2u&&
           near(poly_box.minimum[0],0.0)&&
           near(poly_box.maximum[0],12.5)&&
           near(poly_box.minimum[1],0.0)&&
           near(poly_box.maximum[1],10.0)&&
           near(poly_box.minimum[2],0.0)&&
           near(poly_box.maximum[2],0.0));
    const MusaDwgAc1018LinearWcsBounds unchanged_bounds=poly_box;
    assert(!musa_dwg_ac1018_scene_read_linear_wcs_bounds(
                  s,UINT64_C(0xdead),&poly_box));
    assert(!musa_dwg_ac1018_scene_read_linear_wcs_bounds(
                  s,0x40eu,NULL));
    assert(!musa_dwg_ac1018_scene_read_linear_wcs_bounds(
                  s,0x40eu,
                  (MusaDwgAc1018LinearWcsBounds*)(void*)s));
    assert(!memcmp(&poly_box,&unchanged_bounds,sizeof(poly_box)));
    /* Analytic signed bulge exact WCS bounds: semicircle below/above
     * a horizontal chord; 253-degree major arc; arbitrary normal +Y.
     * Endpoints-only envelopes would miss the interior extrema. */
    const double up[3]={0.0,0.0,1.0};
    const double tilted[3]={0.0,1.0,0.0};
    MusaDwgAc1018LWVertex va={0},vb={0};
    va.x=0.0;va.y=0.0;va.bulge=1.0;
    vb.x=2.0;vb.y=0.0;
    MusaDwgAc1018BulgeSegmentWcsBounds seg={0};
    assert(musa_dwg_ac1018_bulge_segment_wcs_bounds(
                  &va,&vb,up,0.0,0.0,&seg));
    assert(seg.curved&&near(seg.radius,1.0)&&
           near(seg.sweep_angle,acos(-1.0))&&
           near(seg.minimum[0],0.0)&&near(seg.maximum[0],2.0)&&
           near(seg.minimum[1],-1.0)&&near(seg.maximum[1],0.0)&&
           near(seg.minimum[2],0.0)&&near(seg.maximum[2],0.0));
    va.bulge=-1.0;
    assert(musa_dwg_ac1018_bulge_segment_wcs_bounds(
                  &va,&vb,up,0.0,0.0,&seg));
    assert(near(seg.minimum[1],0.0)&&near(seg.maximum[1],1.0)&&
           near(seg.sweep_angle,-acos(-1.0)));
    va.bulge=2.0;
    assert(musa_dwg_ac1018_bulge_segment_wcs_bounds(
                  &va,&vb,up,0.0,0.0,&seg));
    assert(seg.sweep_angle>acos(-1.0)&&
           near(seg.minimum[0],-0.25)&&
           near(seg.maximum[0],2.25)&&
           near(seg.minimum[1],-2.0)&&
           near(seg.maximum[1],0.0));
    va.bulge=1.0;
    assert(musa_dwg_ac1018_bulge_segment_wcs_bounds(
                  &va,&vb,tilted,3.5,0.5,&seg));
    assert(near(seg.minimum[0],-2.0)&&near(seg.maximum[0],0.0)&&
           near(seg.minimum[1],3.5)&&near(seg.maximum[1],4.0)&&
           near(seg.minimum[2],-1.0)&&near(seg.maximum[2],0.0));
    assert(musa_dwg_ac1018_bulge_segment_wcs_bounds(
                  &va,&vb,tilted,3.5,-0.25,&seg));
    assert(near(seg.minimum[1],3.25)&&near(seg.maximum[1],3.5));
    va.bulge=0.0;
    assert(musa_dwg_ac1018_bulge_segment_wcs_bounds(
                  &va,&vb,up,0.0,0.0,&seg));
    assert(!seg.curved&&near(seg.minimum[0],0.0)&&
           near(seg.maximum[0],2.0)&&near(seg.minimum[1],0.0)&&
           near(seg.maximum[1],0.0));
    const MusaDwgAc1018BulgeSegmentWcsBounds seg_saved=seg;
    va.bulge=1e-14;
    assert(!musa_dwg_ac1018_bulge_segment_wcs_bounds(
                  &va,&vb,up,0.0,0.0,&seg));
    va.bulge=1e9;
    assert(!musa_dwg_ac1018_bulge_segment_wcs_bounds(
                  &va,&vb,up,0.0,0.0,&seg));
    va.bulge=1.0;
    vb.x=0.0; /* a nonzero bulge needs a nonzero chord */
    assert(!musa_dwg_ac1018_bulge_segment_wcs_bounds(
                  &va,&vb,up,0.0,0.0,&seg));
    vb.x=2.0;
    assert(!musa_dwg_ac1018_bulge_segment_wcs_bounds(
                  &va,&vb,up,0.0,0.0,
                  (MusaDwgAc1018BulgeSegmentWcsBounds*)(void*)&va));
    assert(!memcmp(&seg,&seg_saved,sizeof(seg)));
    /* Original CRC-backed R2004 straight LWPOLYLINE remains supported
     * by the GENERAL bulge-aware union API as a strict subset. */
    MusaDwgAc1018BulgedPolylineWcsBounds poly_bulged={0};
    assert(musa_dwg_ac1018_scene_read_bulged_polyline_wcs_bounds(
                  s,0x40eu,&poly_bulged));
    assert(poly_bulged.handle==0x40eu&&
           poly_bulged.vertex_count==3u&&
           poly_bulged.segment_count==2u&&
           poly_bulged.curved_segment_count==0u&&
           near(poly_bulged.minimum[0],poly_box.minimum[0])&&
           near(poly_bulged.maximum[0],poly_box.maximum[0])&&
           near(poly_bulged.minimum[1],poly_box.minimum[1])&&
           near(poly_bulged.maximum[1],poly_box.maximum[1]));
    MusaDwgAc1018BulgeCenterlineAudit straight_evidence={0};
    assert(musa_dwg_ac1018_scene_audit_bulge_centerline_wcs(
                  s,0x40eu,&straight_evidence));
    assert(!straight_evidence.nonzero_constant_width&&
           !straight_evidence.nonzero_variable_width_vertices&&
           !straight_evidence.excluded_width_geometry&&
           !memcmp(&straight_evidence.centerline,&poly_bulged,
                   sizeof(poly_bulged)));
    MusaDwgAc1018BulgeWidthNeighborhood zero_width={0};
    assert(musa_dwg_ac1018_scene_audit_bulge_width_neighborhood(
                  s,0x40eu,&zero_width));
    assert(zero_width.source_max_width==0.0&&
           !zero_width.evidence.excluded_width_geometry&&
           !zero_width.complete_visible_stroke_verified);
    for(size_t k=0u;k<3u;k++){
        assert(near(zero_width.radial_pad_wcs[k],0.0)&&
               near(zero_width.padded_minimum[k],poly_bulged.minimum[k])&&
               near(zero_width.padded_maximum[k],poly_bulged.maximum[k]));
    }
    /* The R2004 protected open polyline has 2 straight zero-width
     * segments. The exact quadrilateral degenerates safely to each
     * true source LINE chord, with no guessed stroke. */
    MusaDwgAc1018StraightTaperWcs straight={0};
    assert(musa_dwg_ac1018_scene_read_straight_taper_wcs(
                  s,0x40eu,0u,&straight));
    assert(straight.handle==0x40eu&&straight.segment_index==0u&&
           !straight.widths_from_vertices&&
           near(straight.start_width,0.0)&&
           near(straight.end_width,0.0)&&
           near(straight.minimum[0],0.0)&&
           near(straight.maximum[0],12.5)&&
           near(straight.minimum[1],0.0)&&
           near(straight.maximum[1],0.0));
    assert(musa_dwg_ac1018_scene_read_straight_taper_wcs(
                  s,0x40eu,1u,&straight));
    assert(straight.segment_index==1u&&
           near(straight.minimum[0],12.5)&&
           near(straight.maximum[0],12.5)&&
           near(straight.minimum[1],0.0)&&
           near(straight.maximum[1],10.0));
    const MusaDwgAc1018StraightTaperWcs straight_saved=straight;
    assert(!musa_dwg_ac1018_scene_read_straight_taper_wcs(
                  s,0x40eu,2u,&straight)); /* open last index */
    assert(!musa_dwg_ac1018_scene_read_straight_taper_wcs(
                  s,0x70fu,0u,&straight)); /* LINE is not LW */
    assert(!musa_dwg_ac1018_scene_read_straight_taper_wcs(
                  s,0x40eu,0u,NULL));
    assert(!musa_dwg_ac1018_scene_read_straight_taper_wcs(
                  s,0x40eu,0u,
                  (MusaDwgAc1018StraightTaperWcs*)(void*)s));
    assert(!memcmp(&straight_saved,&straight,sizeof(straight)));
    const MusaDwgAc1018BulgedPolylineWcsBounds keep_bulged=
           poly_bulged;
    assert(!musa_dwg_ac1018_scene_read_bulged_polyline_wcs_bounds(
                  s,0x70fu,&poly_bulged));
    assert(!musa_dwg_ac1018_scene_read_bulged_polyline_wcs_bounds(
                  s,0x40eu,
                  (MusaDwgAc1018BulgedPolylineWcsBounds*)(void*)s));
    assert(!memcmp(&keep_bulged,&poly_bulged,sizeof(poly_bulged)));
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
    construct(file,0);file[608u]^=1u;
    assert(!musa_dwg_ac1018_scene_open(file,sizeof(file),&s)&&!s);
    construct(file,0);file[788u]^=1u;
    assert(!musa_dwg_ac1018_scene_open(file,sizeof(file),&s)&&!s);
    construct(file,0);
    assert(!musa_dwg_ac1018_scene_open(file,sizeof(file)-32u,&s)&&!s);
    musa_dwg_ac1018_scene_close(s);s=NULL;
    /* Both handles point into the SAME intact CRC-backed object record.
     * The full physical interval audit rejects overlapping ranges, even
     * though the H map and individual object CRC checks independently pass. */
    construct(file,1);
    assert(musa_dwg_ac1018_scene_open(file,sizeof(file),&s));
    MusaDwgAc1018RecordProbe a={0},b={0};
    assert(musa_dwg_ac1018_scene_probe_record_at(s,0u,&a));
    assert(musa_dwg_ac1018_scene_probe_record_at(s,1u,&b));
    assert(a.object_offset==b.object_offset&&a.record_crc_verified&&
           b.record_crc_verified);
    MusaDwgAc1018RecordRangeAudit unchanged=layout;
    assert(!musa_dwg_ac1018_scene_audit_record_ranges(s,&layout));
    assert(!memcmp(&layout,&unchanged,sizeof(layout)));
    /* Two H entries aliased to one physical CRC-backed record must
     * invalidate the entire diagnostic and preserve caller report. */
    assert(!musa_dwg_ac1018_scene_audit_2d_geometry_vs_links(
                  s,&evidence));
    assert(!memcmp(&keep_evidence,&evidence,sizeof(evidence)));
    assert(!musa_dwg_ac1018_scene_read_record_diagnosis_page(
                  s,0u,diagnoses,2u,&diagnostic_page));
    /* CRC-correct duplicate H reference targets are not accepted as
     * actual source-verified geometry for a caller-chosen indexed handle. */
    assert(!musa_dwg_ac1018_scene_read_linear_wcs_bounds(
                  s,0x70fu,&poly_box));
    assert(!memcmp(&poly_box,&unchanged_bounds,sizeof(poly_box)));
    assert(!memcmp(&diagnoses[0],&kept_diagnosis0,
                   sizeof(kept_diagnosis0))&&
           !memcmp(&diagnostic_page,&kept_diagnostic_page,
                   sizeof(kept_diagnostic_page)));
    assert(!musa_dwg_ac1018_scene_audit_record_ranges(s,
                 (MusaDwgAc1018RecordRangeAudit*)(void*)s));
    assert(!memcmp(&layout,&unchanged,sizeof(layout)));
    musa_dwg_ac1018_scene_close(s);
    /* Source scene already closed, yet snapshot owns the original H data. */
    assert(musa_dwg_ac1018_diagnosis_snapshot_read_page(
                 snapshot,1u,cached_rows,2u,&cached_page));
    assert(cached_rows[0].indexed_handle==UINT64_C(0x70f)&&
           cached_rows[0].geometry_decoded&&
           cached_page.records_written==1u);
    musa_dwg_ac1018_diagnosis_snapshot_close(snapshot);
    assert(musa_dwg_ac1018_diagnosis_snapshot_handle_count(NULL)==0u);
    construct_bulged(file);
    s=NULL;
    assert(musa_dwg_ac1018_scene_open(file,sizeof(file),&s));
    assert(musa_dwg_ac1018_scene_handle_count(s)==1u);
    MusaDwgAc1018LWPolyline complex_poly={0};
    MusaDwgAc1018LWVertex complex_vertices[4]={{0}};
    assert(musa_dwg_ac1018_scene_read_lwpolyline(
                  s,0x42bu,complex_vertices,4u,&complex_poly));
    assert(complex_poly.closed==1u&&complex_poly.vertex_count==3u&&
           complex_poly.bulge_count==3u&&
           complex_poly.width_count==3u&&
           near(complex_vertices[0].bulge,0.5)&&
           near(complex_vertices[2].bulge,-0.25));
    /* True ORIGINAL binary R2004 bulge width edges. The variable-width
     * normal follows the ARC TANGENT at each fractional sweep, rather
     * than a chord normal or concentric fixed-radius arc approximation.
     * Source segment 0: OCS (1.5,-4.5)->(2.5,-4.5), bulge +0.5,
     * r=0.625 and original widths 0.125 -> 0.25; world normal +Y. */
    MusaDwgAc1018BulgeWidthEdgePoint curved_edges={0};
    assert(musa_dwg_ac1018_scene_evaluate_bulge_width_edges(
                  s,0x42bu,0u,0.0,&curved_edges));
    assert(curved_edges.handle==0x42bu&&
           curved_edges.segment_index==0u&&
           curved_edges.widths_from_vertices&&
           near(curved_edges.radius,0.625)&&
           near(curved_edges.start_width,0.125)&&
           near(curved_edges.end_width,0.25)&&
           near(curved_edges.interpolated_width,0.125)&&
           curved_edges.signed_sweep>0.0&&
           near(curved_edges.center_wcs[0],-1.5)&&
           near(curved_edges.center_wcs[1],3.5)&&
           near(curved_edges.center_wcs[2],-4.5)&&
           near(curved_edges.left_wcs[0],-1.55)&&
           near(curved_edges.right_wcs[0],-1.45)&&
           near(curved_edges.left_wcs[2],-4.4625)&&
           near(curved_edges.right_wcs[2],-4.5375)&&
           near(curved_edges.left_thickness_wcs[1],3.75)&&
           near(curved_edges.right_thickness_wcs[1],3.75));
    assert(musa_dwg_ac1018_scene_evaluate_bulge_width_edges(
                  s,0x42bu,0u,0.5,&curved_edges));
    assert(near(curved_edges.interpolated_width,0.1875)&&
           near(curved_edges.center_wcs[0],-2.0)&&
           near(curved_edges.center_wcs[2],-4.75)&&
           near(curved_edges.left_wcs[0],-2.0)&&
           near(curved_edges.right_wcs[0],-2.0)&&
           near(curved_edges.left_wcs[2],-4.65625)&&
           near(curved_edges.right_wcs[2],-4.84375)&&
           near(curved_edges.left_thickness_wcs[1],3.75));
    assert(musa_dwg_ac1018_scene_evaluate_bulge_width_edges(
                  s,0x42bu,0u,1.0,&curved_edges));
    assert(near(curved_edges.interpolated_width,0.25)&&
           near(curved_edges.center_wcs[0],-2.5)&&
           near(curved_edges.center_wcs[2],-4.5)&&
           near(curved_edges.left_wcs[0],-2.4)&&
           near(curved_edges.right_wcs[0],-2.6)&&
           near(curved_edges.left_wcs[2],-4.425)&&
           near(curved_edges.right_wcs[2],-4.575));
    /* The CLOSED closing edge has a negative/cw bulge. Its directed
     * left edge is on the other RADIAL side from the positive arc. */
    assert(musa_dwg_ac1018_scene_evaluate_bulge_width_edges(
                  s,0x42bu,2u,0.0,&curved_edges));
    assert(curved_edges.segment_index==2u&&
           curved_edges.signed_sweep<0.0&&
           near(curved_edges.start_width,1.0)&&
           near(curved_edges.end_width,1.5)&&
           near(curved_edges.interpolated_width,1.0)&&
           curved_edges.left_wcs[0]<curved_edges.center_wcs[0]&&
           curved_edges.right_wcs[0]>curved_edges.center_wcs[0]&&
           near(hypot(curved_edges.left_wcs[0]-
                      curved_edges.right_wcs[0],
                      curved_edges.left_wcs[2]-
                      curved_edges.right_wcs[2]),1.0));
    assert(musa_dwg_ac1018_scene_evaluate_bulge_width_edges(
                  s,0x42bu,2u,0.5,&curved_edges));
    assert(near(curved_edges.interpolated_width,1.25)&&
           near(hypot(curved_edges.left_wcs[0]-
                      curved_edges.right_wcs[0],
                      curved_edges.left_wcs[2]-
                      curved_edges.right_wcs[2]),1.25));
    /* Repeated viewport sample reads must not decode the DWG each time.
     * Both snapshots own exact source H, OCS transforms and widths; their
     * materialized values match the original binary one-shot reader. */
    MusaDwgAc1018BulgeEdgeSnapshot *positive_snap=NULL;
    MusaDwgAc1018BulgeEdgeSnapshot *negative_snap=NULL;
    assert(musa_dwg_ac1018_scene_create_bulge_edge_snapshot(
                  s,0x42bu,0u,&positive_snap)&&positive_snap);
    assert(musa_dwg_ac1018_scene_create_bulge_edge_snapshot(
                  s,0x42bu,2u,&negative_snap)&&negative_snap);
    const double fractions[5]={0.0,0.25,0.5,0.75,1.0};
    for(size_t iteration=0u;iteration<5u;iteration++){
        const double t=fractions[iteration];
        for(uint32_t index=0u;index<=2u;index+=2u){
            MusaDwgAc1018BulgeWidthEdgePoint cached={0},direct={0};
            MusaDwgAc1018BulgeEdgeSnapshot *snapshot=
                index==0u?positive_snap:negative_snap;
            assert(musa_dwg_ac1018_scene_evaluate_bulge_width_edges(
                          s,0x42bu,index,t,&direct));
            assert(musa_dwg_ac1018_bulge_edge_snapshot_evaluate(
                          snapshot,t,&cached));
            assert(cached.handle==direct.handle&&
                   cached.segment_index==direct.segment_index&&
                   cached.widths_from_vertices==
                        direct.widths_from_vertices&&
                   near(cached.fraction,t)&&
                   near(cached.interpolated_width,
                        direct.interpolated_width)&&
                   near(cached.radius,direct.radius)&&
                   near(cached.signed_sweep,direct.signed_sweep));
            for(size_t axis=0u;axis<3u;axis++){
                assert(near(cached.center_wcs[axis],
                            direct.center_wcs[axis])&&
                       near(cached.left_wcs[axis],
                            direct.left_wcs[axis])&&
                       near(cached.right_wcs[axis],
                            direct.right_wcs[axis])&&
                       near(cached.left_thickness_wcs[axis],
                            direct.left_thickness_wcs[axis])&&
                       near(cached.right_thickness_wcs[axis],
                            direct.right_thickness_wcs[axis]));
            }
        }
    }
    MusaDwgAc1018BulgeWidthEdgePoint immutable_sample={0};
    assert(musa_dwg_ac1018_bulge_edge_snapshot_evaluate(
                  positive_snap,0.5,&immutable_sample));
    const MusaDwgAc1018BulgeWidthEdgePoint saved_sample=
                  immutable_sample;
    assert(!musa_dwg_ac1018_bulge_edge_snapshot_evaluate(
                  positive_snap,-0.1,&immutable_sample));
    assert(!musa_dwg_ac1018_bulge_edge_snapshot_evaluate(
                  positive_snap,1.01,&immutable_sample));
    assert(!musa_dwg_ac1018_bulge_edge_snapshot_evaluate(
                  positive_snap,NAN,&immutable_sample));
    assert(!musa_dwg_ac1018_bulge_edge_snapshot_evaluate(
                  positive_snap,0.5,NULL));
    assert(!musa_dwg_ac1018_bulge_edge_snapshot_evaluate(
                  positive_snap,0.5,
                  (MusaDwgAc1018BulgeWidthEdgePoint*)
                   (void*)positive_snap));
    assert(!memcmp(&saved_sample,&immutable_sample,
                   sizeof(saved_sample)));
    MusaDwgAc1018BulgeEdgeSnapshot *unchanged_snapshot=positive_snap;
    assert(!musa_dwg_ac1018_scene_create_bulge_edge_snapshot(
                  s,0x42bu,1u,&unchanged_snapshot)); /* straight */
    assert(!musa_dwg_ac1018_scene_create_bulge_edge_snapshot(
                  s,0x70fu,0u,&unchanged_snapshot)); /* wrong H */
    assert(!musa_dwg_ac1018_scene_create_bulge_edge_snapshot(
                  s,0x42bu,3u,&unchanged_snapshot)); /* bad index */
    assert(!musa_dwg_ac1018_scene_create_bulge_edge_snapshot(
                  s,0x42bu,0u,NULL));
    assert(!musa_dwg_ac1018_scene_create_bulge_edge_snapshot(
                  s,0x42bu,0u,
                  (MusaDwgAc1018BulgeEdgeSnapshot**)(void*)s));
    assert(unchanged_snapshot==positive_snap);
    const MusaDwgAc1018BulgeWidthEdgePoint edge_saved=curved_edges;
    assert(!musa_dwg_ac1018_scene_evaluate_bulge_width_edges(
                  s,0x42bu,1u,0.5,&curved_edges)); /* straight */
    assert(!musa_dwg_ac1018_scene_evaluate_bulge_width_edges(
                  s,0x42bu,3u,0.5,&curved_edges)); /* out of range */
    assert(!musa_dwg_ac1018_scene_evaluate_bulge_width_edges(
                  s,0x40eu,0u,0.5,&curved_edges)); /* original straight */
    assert(!musa_dwg_ac1018_scene_evaluate_bulge_width_edges(
                  s,0x42bu,0u,-0.01,&curved_edges));
    assert(!musa_dwg_ac1018_scene_evaluate_bulge_width_edges(
                  s,0x42bu,0u,1.01,&curved_edges));
    assert(!musa_dwg_ac1018_scene_evaluate_bulge_width_edges(
                  s,0x42bu,0u,NAN,&curved_edges));
    assert(!musa_dwg_ac1018_scene_evaluate_bulge_width_edges(
                  s,0x42bu,0u,0.5,NULL));
    assert(!musa_dwg_ac1018_scene_evaluate_bulge_width_edges(
                  s,0x42bu,0u,0.5,
                  (MusaDwgAc1018BulgeWidthEdgePoint*)(void*)s));
    assert(!memcmp(&edge_saved,&curved_edges,sizeof(curved_edges)));
    /* CRC-protected actual R2004 H=0x42b, closed 3-vertex LW:
     * segment 1 is a vertical OCS straight taper between the original
     * vertex[1] and vertex[2]. Its 4 exact mathematical edge corners
     * are calculable even though adjacent segments are curved.
     * OCS normal +Y makes OCS +X become WCS -X; extrusion thickness
     * contributes Y=3.5..3.75, without inventing adjacent joins. */
    MusaDwgAc1018StraightTaperWcs taper={0};
    assert(musa_dwg_ac1018_scene_read_straight_taper_wcs(
                  s,0x42bu,1u,&taper));
    const double sw=complex_vertices[1].start_width;
    const double ew=complex_vertices[1].end_width;
    assert(taper.handle==0x42bu&&taper.segment_index==1u&&
           taper.widths_from_vertices&&
           near(taper.start_width,sw)&&near(taper.end_width,ew)&&
           near(taper.thickness,0.25));
    assert(near(taper.corners[0][0],-2.5+0.5*sw)&&
           near(taper.corners[1][0],-2.5-0.5*sw)&&
           near(taper.corners[2][0],-2.5+0.5*ew)&&
           near(taper.corners[3][0],-2.5-0.5*ew));
    assert(near(taper.corners[0][1],3.5)&&
           near(taper.corners[3][1],3.5)&&
           near(taper.corners[0][2],-4.5)&&
           near(taper.corners[1][2],-4.5)&&
           near(taper.corners[2][2],5.25)&&
           near(taper.corners[3][2],5.25));
    const double halfmax=0.5*fmax(sw,ew);
    assert(near(taper.minimum[0],-2.5-halfmax)&&
           near(taper.maximum[0],-2.5+halfmax)&&
           near(taper.minimum[1],3.5)&&
           near(taper.maximum[1],3.75)&&
           near(taper.minimum[2],-4.5)&&
           near(taper.maximum[2],5.25));
    const MusaDwgAc1018StraightTaperWcs keep_taper=taper;
    assert(!musa_dwg_ac1018_scene_read_straight_taper_wcs(
                  s,0x42bu,0u,&taper)); /* bulged */
    assert(!musa_dwg_ac1018_scene_read_straight_taper_wcs(
                  s,0x42bu,2u,&taper)); /* curved closing edge */
    assert(!musa_dwg_ac1018_scene_read_straight_taper_wcs(
                  s,0x42bu,3u,&taper));
    assert(!musa_dwg_ac1018_scene_read_straight_taper_wcs(
                  s,UINT64_C(0xdead),1u,&taper));
    assert(!memcmp(&keep_taper,&taper,sizeof(taper)));
    /* Width and signed thickness are present: never report exact
     * centerline-only WCS bounds as complete original stroke geometry. */
    assert(!musa_dwg_ac1018_scene_read_bulged_polyline_wcs_bounds(
                  s,0x42bu,&poly_bulged));
    assert(!memcmp(&poly_bulged,&keep_bulged,sizeof(poly_bulged)));
    /* The original independently protected R2004 record with H=0x42b
     * contains TWO genuine signed arc bulges plus 3 variable-width pairs.
     * Decode its exact mathematically defined CENTERLINE without ever
     * claiming that the omitted widths are visible-stroke coverage. */
    MusaDwgAc1018BulgeCenterlineAudit original_bulge={0};
    assert(musa_dwg_ac1018_scene_audit_bulge_centerline_wcs(
                  s,0x42bu,&original_bulge));
    assert(original_bulge.centerline.handle==0x42bu&&
           original_bulge.centerline.vertex_count==3u&&
           original_bulge.centerline.segment_count==3u&&
           original_bulge.centerline.curved_segment_count==2u&&
           original_bulge.nonzero_constant_width&&
           original_bulge.nonzero_variable_width_vertices>=1u&&
           original_bulge.excluded_width_geometry&&
           near(original_bulge.centerline.minimum[0],
                  -3.2730946352036)&&
           near(original_bulge.centerline.maximum[0],-1.5)&&
           near(original_bulge.centerline.minimum[1],3.5)&&
           near(original_bulge.centerline.maximum[1],3.75)&&
           near(original_bulge.centerline.minimum[2],-4.75)&&
           near(original_bulge.centerline.maximum[2],5.25));
    /* Positive end-to-end original CRC-protected R2004 geometry
     * WITH variable/constant widths, tilted plane and 0.25 thickness.
     * A conservative OCS disk radius maxWidth/2=0.75 projected to WCS
     * adds 0.75 to X and Z, zero to Y for +Y extrusion. It does not
     * establish the exact DWG miter/cap/lineweight output. */
    MusaDwgAc1018BulgeWidthNeighborhood padded={0};
    assert(musa_dwg_ac1018_scene_audit_bulge_width_neighborhood(
                  s,0x42bu,&padded));
    assert(padded.evidence.centerline.handle==0x42bu&&
           padded.evidence.centerline.curved_segment_count==2u&&
           padded.evidence.nonzero_constant_width&&
           padded.evidence.nonzero_variable_width_vertices>=1u&&
           padded.evidence.excluded_width_geometry&&
           !padded.complete_visible_stroke_verified&&
           near(padded.source_max_width,1.5)&&
           near(padded.radial_pad_wcs[0],0.75)&&
           near(padded.radial_pad_wcs[1],0.0)&&
           near(padded.radial_pad_wcs[2],0.75)&&
           near(padded.padded_minimum[0],-4.0230946352036)&&
           near(padded.padded_maximum[0],-0.75)&&
           near(padded.padded_minimum[1],3.5)&&
           near(padded.padded_maximum[1],3.75)&&
           near(padded.padded_minimum[2],-5.5)&&
           near(padded.padded_maximum[2],6.0));
    const MusaDwgAc1018BulgeWidthNeighborhood kept_padded=padded;
    assert(!musa_dwg_ac1018_scene_audit_bulge_width_neighborhood(
                  s,0x70fu,&padded));
    assert(!musa_dwg_ac1018_scene_audit_bulge_width_neighborhood(
                  s,0x42bu,NULL));
    assert(!musa_dwg_ac1018_scene_audit_bulge_width_neighborhood(
                  s,0x42bu,
                  (MusaDwgAc1018BulgeWidthNeighborhood*)(void*)s));
    assert(!memcmp(&padded,&kept_padded,sizeof(padded)));
    const MusaDwgAc1018BulgeCenterlineAudit retained_bulge=
                 original_bulge;
    assert(!musa_dwg_ac1018_scene_audit_bulge_centerline_wcs(
                  s,0x70fu,&original_bulge));
    assert(!musa_dwg_ac1018_scene_audit_bulge_centerline_wcs(
                  s,0x42bu,NULL));
    assert(!musa_dwg_ac1018_scene_audit_bulge_centerline_wcs(
                  s,0x42bu,
                  (MusaDwgAc1018BulgeCenterlineAudit*)(void*)s));
    assert(!memcmp(&original_bulge,&retained_bulge,
                   sizeof(original_bulge)));
    musa_dwg_ac1018_scene_close(s);
    /* Original protected DWG/scene is gone: cached WCS edges are
     * independent and remain byte-for-byte stable across reads. */
    MusaDwgAc1018BulgeWidthEdgePoint after_close={0};
    assert(musa_dwg_ac1018_bulge_edge_snapshot_evaluate(
                  positive_snap,0.5,&after_close));
    assert(!memcmp(&after_close,&saved_sample,sizeof(after_close)));
    assert(musa_dwg_ac1018_bulge_edge_snapshot_evaluate(
                  negative_snap,1.0,&after_close));
    assert(after_close.segment_index==2u&&
           after_close.signed_sweep<0.0&&
           near(after_close.interpolated_width,1.5));
    musa_dwg_ac1018_bulge_edge_snapshot_close(negative_snap);
    musa_dwg_ac1018_bulge_edge_snapshot_close(positive_snap);
    musa_dwg_ac1018_bulge_edge_snapshot_close(NULL);
    return 0;
}
