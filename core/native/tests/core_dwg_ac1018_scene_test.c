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
    /* Real CRC-backed open H=0x40e: two straight segments meet at
     * vertex 1 exactly, zero width. Reject open endpoint joins. */
    MusaDwgAc1018LwJoinEdgeAudit join={0};
    assert(musa_dwg_ac1018_scene_audit_lw_join_edges(
                  s,0x40eu,1u,&join));
    assert(join.handle==0x40eu&&join.vertex_index==1u&&
           join.incoming_segment_index==0u&&
           join.outgoing_segment_index==1u&&
           !join.incoming_curved&&!join.outgoing_curved&&
           join.source_widths_match&&join.edge_endpoints_match&&
           !join.complete_visible_join_verified&&
           near(join.center_wcs[0],12.5)&&
           near(join.center_wcs[1],0.0)&&
           near(join.edge_gap[0],0.0)&&
           near(join.edge_gap[1],0.0)&&
           near(join.incoming_edge_tangent_wcs[0][0],1.0)&&
           near(join.outgoing_edge_tangent_wcs[0][1],1.0));
    /* Protected original R2004 open zero-width right-angle junction:
     * bevel is the exact two coincident source edge endpoints; the
     * bounded tangent-line miter is also exactly their shared vertex.
     * None of these candidates implies a rendered join/paint stroke. */
    MusaDwgAc1018LwJoinCandidates candidate={0};
    assert(musa_dwg_ac1018_scene_lw_join_candidates(
                  s,0x40eu,1u,4.0,&candidate));
    assert(candidate.source.handle==0x40eu&&
           candidate.source.vertex_index==1u&&
           near(candidate.miter_limit,4.0)&&
           !candidate.complete_visible_join_verified);
    for(size_t side=0u;side<2u;side++){
        assert(candidate.tangent_miter_valid[side]);
        for(size_t k=0u;k<3u;k++){
            assert(near(candidate.bevel_wcs[side][0][k],
                        candidate.source.incoming_edge_wcs[side][k])&&
                   near(candidate.bevel_wcs[side][1][k],
                        candidate.source.outgoing_edge_wcs[side][k])&&
                   near(candidate.miter_wcs[side][k],
                        candidate.source.center_wcs[k])&&
                   near(candidate.miter_thickness_wcs[side][k],
                        candidate.source.center_wcs[k]));
        }
    }
    const MusaDwgAc1018LwJoinCandidates saved_candidate=candidate;
    assert(!musa_dwg_ac1018_scene_lw_join_candidates(
                  s,0x40eu,0u,4.0,&candidate));
    assert(!musa_dwg_ac1018_scene_lw_join_candidates(
                  s,0x40eu,1u,0.0,&candidate));
    assert(!musa_dwg_ac1018_scene_lw_join_candidates(
                  s,0x40eu,1u,101.0,&candidate));
    assert(!musa_dwg_ac1018_scene_lw_join_candidates(
                  s,0x40eu,1u,NAN,&candidate));
    assert(!musa_dwg_ac1018_scene_lw_join_candidates(
                  s,0x40eu,1u,4.0,NULL));
    assert(!musa_dwg_ac1018_scene_lw_join_candidates(
                  s,0x40eu,1u,4.0,
                  (MusaDwgAc1018LwJoinCandidates*)(void*)s));
    assert(!memcmp(&saved_candidate,&candidate,sizeof(candidate)));
    /* Single-pass, CRC-protected original R2004 H=0x40e:
     * two exact straight 0-width segments and their sole open bevel
     * bridge. No repeated per-segment source parsing. */
    MusaDwgAc1018LwRibbonBevelWcsEnvelope whole_open={0};
    assert(musa_dwg_ac1018_scene_enclose_lw_ribbon_bevel_wcs(
                s,0x40eu,0.02,32768u,&whole_open));
    assert(whole_open.handle==0x40eu&&
           whole_open.vertex_count==3u&&
           whole_open.segment_count==2u&&
           whole_open.straight_segments==2u&&
           whole_open.curved_segments==0u&&
           whole_open.curve_intervals_total==0u&&
           whole_open.bevel_bridge_count==1u&&
           whole_open.discontinuous_width_junctions==0u&&
           whole_open.bevel_bridge_endpoints_enclosed&&
           !whole_open.all_miters_enclosed&&
           !whole_open.all_round_joins_enclosed&&
           !whole_open.complete_visible_stroke_verified&&
           near(whole_open.maximum_bevel_edge_gap,0.0)&&
           near(whole_open.minimum[0],0.0)&&
           near(whole_open.maximum[0],12.5)&&
           near(whole_open.minimum[1],0.0)&&
           near(whole_open.maximum[1],10.0)&&
           near(whole_open.minimum[2],0.0)&&
           near(whole_open.maximum[2],0.0));
    /* Whole original H=0x40E open right-angle source has two coincident
     * zero-width edge miter candidates and no geometric protrusion. */
    MusaDwgAc1018LwRibbonMiterWcsEnvelope open_miter={0};
    assert(musa_dwg_ac1018_scene_enclose_lw_ribbon_miter_wcs(
                s,0x40eu,0.02,32768u,4.0,&open_miter));
    assert(open_miter.valid_miter_candidates==2u&&
           open_miter.rejected_miter_candidates==0u&&
           near(open_miter.miter_limit,4.0)&&
           open_miter.bounded_tangent_candidates_enclosed&&
           !open_miter.complete_visible_stroke_verified);
    for(size_t axis=0;axis<3;axis++){
        assert(near(open_miter.minimum[axis],whole_open.minimum[axis])&&
               near(open_miter.maximum[axis],whole_open.maximum[axis]));
    }
    const MusaDwgAc1018LwRibbonMiterWcsEnvelope
           keep_open_miter=open_miter;
    assert(!musa_dwg_ac1018_scene_enclose_lw_ribbon_miter_wcs(
                s,0x40eu,0.02,32768u,0.5,&open_miter));
    assert(!musa_dwg_ac1018_scene_enclose_lw_ribbon_miter_wcs(
                s,0x40eu,0.02,32768u,101.0,&open_miter));
    assert(!musa_dwg_ac1018_scene_enclose_lw_ribbon_miter_wcs(
                s,0x40eu,0.02,32768u,NAN,&open_miter));
    assert(!musa_dwg_ac1018_scene_enclose_lw_ribbon_miter_wcs(
                s,0x40eu,0.02,32768u,4.0,NULL));
    assert(!musa_dwg_ac1018_scene_enclose_lw_ribbon_miter_wcs(
                s,0x40eu,0.02,32768u,4.0,
                (MusaDwgAc1018LwRibbonMiterWcsEnvelope*)(void*)s));
    assert(!memcmp(&keep_open_miter,&open_miter,sizeof(open_miter)));
    /* Original H=0x40E open source has no width: radial arcs cannot
     * be identified, so all degenerate corners fall back to bevel.
     * The baseline model and its exact WCS boundaries stay identical. */
    MusaDwgAc1018LwRadialBridgeWcsEnvelope open_round={0};
    assert(musa_dwg_ac1018_scene_enclose_lw_radial_bridges_wcs(
                  s,0x40eu,0.02,32768u,&open_round));
    assert(open_round.radial_bridges_enclosed==0u&&
           open_round.radial_bridges_rejected==2u&&
           open_round.variable_radius_bridges==0u&&
           open_round.all_admitted_radial_bridges_enclosed&&
           !open_round.complete_visible_stroke_verified&&
           !memcmp(&open_round.bevel,&whole_open,sizeof(whole_open)));
    for(size_t k=0u;k<3u;k++){
        assert(near(open_round.minimum[k],whole_open.minimum[k])&&
               near(open_round.maximum[k],whole_open.maximum[k]));
    }
    const MusaDwgAc1018LwRadialBridgeWcsEnvelope keep_open_round=open_round;
    assert(!musa_dwg_ac1018_scene_enclose_lw_radial_bridges_wcs(
                  s,0x40eu,NAN,32768u,&open_round));
    assert(!musa_dwg_ac1018_scene_enclose_lw_radial_bridges_wcs(
                  s,0x40eu,0.02,32768u,NULL));
    assert(!musa_dwg_ac1018_scene_enclose_lw_radial_bridges_wcs(
                  s,0x40eu,0.02,32768u,
                  (MusaDwgAc1018LwRadialBridgeWcsEnvelope*)(void*)s));
    assert(!memcmp(&keep_open_round,&open_round,sizeof(open_round)));
    const MusaDwgAc1018LwRibbonBevelWcsEnvelope keep_open=whole_open;
    assert(!musa_dwg_ac1018_scene_enclose_lw_ribbon_bevel_wcs(
                s,0x70fu,0.02,32768u,&whole_open));
    assert(!musa_dwg_ac1018_scene_enclose_lw_ribbon_bevel_wcs(
                s,0x40eu,0.0,32768u,&whole_open));
    assert(!musa_dwg_ac1018_scene_enclose_lw_ribbon_bevel_wcs(
                s,0x40eu,NAN,32768u,&whole_open));
    assert(!musa_dwg_ac1018_scene_enclose_lw_ribbon_bevel_wcs(
                s,0x40eu,0.02,15u,&whole_open));
    assert(!musa_dwg_ac1018_scene_enclose_lw_ribbon_bevel_wcs(
                s,0x40eu,0.02,65537u,&whole_open));
    assert(!musa_dwg_ac1018_scene_enclose_lw_ribbon_bevel_wcs(
                s,0x40eu,0.02,32768u,NULL));
    assert(!musa_dwg_ac1018_scene_enclose_lw_ribbon_bevel_wcs(
                s,0x40eu,0.02,32768u,
                (MusaDwgAc1018LwRibbonBevelWcsEnvelope*)(void*)s));
    assert(!memcmp(&whole_open,&keep_open,sizeof(whole_open)));
    /* In the original H=0x40E open 90deg LW the width is zero.
     * Never invent a positive-area radial fill at zero radius. */
    double zero_base[8][3]={{0}},zero_top[8][3]={{0}};
    MusaDwgAc1018LwRadialFan zero_report={0};
    zero_base[0][0]=17.0;zero_top[0][0]=19.0;
    zero_report.handle=UINT64_C(0x12345);
    assert(!musa_dwg_ac1018_scene_tessellate_lw_radial_fan(
        s,0x40eu,1u,0u,0.01,7u,zero_base,zero_top,8u,
        &zero_report));
    assert(near(zero_base[0][0],17.0)&&
           near(zero_top[0][0],19.0)&&
           zero_report.handle==UINT64_C(0x12345));
    const MusaDwgAc1018LwJoinEdgeAudit saved_straight_join=join;
    assert(!musa_dwg_ac1018_scene_audit_lw_join_edges(
                  s,0x40eu,0u,&join));
    assert(!musa_dwg_ac1018_scene_audit_lw_join_edges(
                  s,0x40eu,2u,&join));
    assert(!musa_dwg_ac1018_scene_audit_lw_join_edges(
                  s,0x70fu,1u,&join));
    assert(!musa_dwg_ac1018_scene_audit_lw_join_edges(
                  s,0x40eu,1u,NULL));
    assert(!musa_dwg_ac1018_scene_audit_lw_join_edges(
                  s,0x40eu,1u,
                  (MusaDwgAc1018LwJoinEdgeAudit*)(void*)s));
    assert(!memcmp(&saved_straight_join,&join,sizeof(join)));
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
    /* Original CRC-backed R2004 H=0x42B; both nonlinear variable-width
     * signed edges are enclosed for every t, not just their endpoints.
     * The public numerical certificate compares analytic outer interval
     * extrema with genuinely evaluated inner extrema at dense fractions. */
    MusaDwgAc1018BulgeEdgeWcsEnvelope posbox={0},negbox={0};
    assert(musa_dwg_ac1018_bulge_edge_snapshot_enclose(
                  positive_snap,0.02,32768u,&posbox));
    assert(musa_dwg_ac1018_bulge_edge_snapshot_enclose(
                  negative_snap,0.02,32768u,&negbox));
    assert(posbox.handle==0x42bu&&posbox.segment_index==0u&&
           negbox.handle==0x42bu&&negbox.segment_index==2u&&
           posbox.intervals_used>=16u&&negbox.intervals_used>=16u&&
           posbox.max_outward_gap<=0.02&&
           negbox.max_outward_gap<=0.02&&
           !posbox.full_stroke_verified&&!negbox.full_stroke_verified);
    for(unsigned sample=0u;sample<=1000u;sample++){
        const double t=(double)sample/1000.0;
        for(size_t kind=0u;kind<2u;kind++){
            MusaDwgAc1018BulgeWidthEdgePoint v={0};
            MusaDwgAc1018BulgeEdgeSnapshot *snapshot=
                kind==0u?positive_snap:negative_snap;
            const MusaDwgAc1018BulgeEdgeWcsEnvelope *bounds=
                kind==0u?&posbox:&negbox;
            assert(musa_dwg_ac1018_bulge_edge_snapshot_evaluate(
                          snapshot,t,&v));
            const double *points[4]={
                v.left_wcs,v.right_wcs,
                v.left_thickness_wcs,v.right_thickness_wcs};
            for(size_t j=0u;j<4u;j++){
                for(size_t axis=0u;axis<3u;axis++){
                    assert(points[j][axis]>=bounds->minimum[axis]-1e-9&&
                           points[j][axis]<=bounds->maximum[axis]+1e-9);
                }
            }
        }
    }
    const MusaDwgAc1018BulgeEdgeWcsEnvelope original_box=posbox;
    assert(!musa_dwg_ac1018_bulge_edge_snapshot_enclose(
                  positive_snap,1e-12,16u,&posbox));
    assert(!musa_dwg_ac1018_bulge_edge_snapshot_enclose(
                  positive_snap,0.0,32768u,&posbox));
    assert(!musa_dwg_ac1018_bulge_edge_snapshot_enclose(
                  positive_snap,NAN,32768u,&posbox));
    assert(!musa_dwg_ac1018_bulge_edge_snapshot_enclose(
                  positive_snap,0.02,15u,&posbox));
    assert(!musa_dwg_ac1018_bulge_edge_snapshot_enclose(
                  positive_snap,0.02,65537u,&posbox));
    assert(!musa_dwg_ac1018_bulge_edge_snapshot_enclose(
                  positive_snap,0.02,32768u,NULL));
    assert(!musa_dwg_ac1018_bulge_edge_snapshot_enclose(
                  positive_snap,0.02,32768u,
                  (MusaDwgAc1018BulgeEdgeWcsEnvelope*)
                   (void*)positive_snap));
    assert(!memcmp(&original_box,&posbox,sizeof(posbox)));
    /* Two true CRC-checked binary R2004 curved LW segments (opposing
     * signs and variable widths): independently compare every WCS mesh
     * cross section to the original-source immutable edge snapshots,
     * then every cell midpoint to the ideal curve. These explicit
     * 4-triangle-per-cell strip faces were previously unavailable. */
    for(uint32_t index=0u;index<=2u;index+=2u){
        MusaDwgAc1018BulgeEdgeSnapshot *snap=
             index==0u?positive_snap:negative_snap;
        MusaDwgAc1018BulgeRibbonSection mesh[513]={0};
        MusaDwgAc1018BulgeRibbonStrip strip={0};
        assert(musa_dwg_ac1018_scene_tessellate_bulge_ribbon_strip(
                    s,0x42bu,index,0.01,512u,mesh,513u,&strip));
        assert(strip.handle==0x42bu&&strip.segment_index==index&&
               strip.cell_count>=1u&&strip.cell_count<=512u&&
               strip.section_count==strip.cell_count+1u&&
               strip.triangles_per_face==2u*strip.cell_count&&
               strip.triangles_both_faces==4u*strip.cell_count&&
               strip.maximum_chord_deviation_bound<=0.01&&
               !strip.full_visible_stroke_verified);
        for(uint32_t i=0u;i<=strip.cell_count;i++){
            const double t=(double)i/(double)strip.cell_count;
            MusaDwgAc1018BulgeWidthEdgePoint e={0};
            assert(musa_dwg_ac1018_bulge_edge_snapshot_evaluate(
                          snap,t,&e));
            const double *expected[4]={
                  e.left_wcs,e.right_wcs,
                  e.left_thickness_wcs,e.right_thickness_wcs};
            const double *got[4]={
                  mesh[i].left_wcs,mesh[i].right_wcs,
                  mesh[i].left_thickness_wcs,mesh[i].right_thickness_wcs};
            for(size_t edge=0u;edge<4u;edge++)
                for(size_t k=0u;k<3u;k++)
                    assert(near(got[edge][k],expected[edge][k])&&
                           got[edge][k]>=strip.minimum[k]-1e-9&&
                           got[edge][k]<=strip.maximum[k]+1e-9);
            if(i==strip.cell_count)continue;
            MusaDwgAc1018BulgeWidthEdgePoint mid={0};
            const double midpoint=((double)i+0.5)/
                                     (double)strip.cell_count;
            assert(musa_dwg_ac1018_bulge_edge_snapshot_evaluate(
                          snap,midpoint,&mid));
            const double *truth[2]={mid.left_wcs,mid.right_wcs};
            const double *m0[2]={mesh[i].left_wcs,
                                   mesh[i].right_wcs};
            const double *m1[2]={mesh[i+1u].left_wcs,
                                   mesh[i+1u].right_wcs};
            for(size_t side=0u;side<2u;side++){
                double error2=0.0;
                for(size_t k=0u;k<3u;k++){
                    const double interp=0.5*(m0[side][k]+
                                              m1[side][k]);
                    error2+=(interp-truth[side][k])*
                            (interp-truth[side][k]);
                }
                assert(sqrt(error2)<=
                        strip.maximum_chord_deviation_bound+1e-9);
            }
        }
        /* Extrude the genuine original signed-bulge source sections
         * into explicitly indexed base/top, left/right wall and endcap
         * triangles. No guessed DWG joins, no source mutation. */
        uint32_t shell_faces[8u*512u+4u][3]={{0}};
        MusaDwgAc1018BulgeRibbonIndexedShell shell={0};
        assert(musa_dwg_ac1018_bulge_ribbon_strip_indexed_shell(
                    mesh,&strip,shell_faces,8u*512u+4u,&shell));
        assert(shell.source_handle==0x42bu&&
               shell.source_segment_index==index&&
               shell.vertex_count==4u*(strip.cell_count+1u)&&
               shell.triangle_count==8u*strip.cell_count+4u&&
               shell.base_triangles==2u*strip.cell_count&&
               shell.translated_triangles==2u*strip.cell_count&&
               shell.left_wall_triangles==2u*strip.cell_count&&
               shell.right_wall_triangles==2u*strip.cell_count&&
               shell.endpoint_triangles==4u&&
               !shell.full_visible_stroke_verified&&
               !shell.global_watertightness_verified);
        assert(near(shell.signed_extrusion_offset_wcs[0],0.0)&&
               near(shell.signed_extrusion_offset_wcs[1],0.25)&&
               near(shell.signed_extrusion_offset_wcs[2],0.0));
        for(uint32_t face=0u;face<shell.triangle_count;face++)
            for(size_t corner=0u;corner<3u;corner++)
                assert(shell_faces[face][corner]<shell.vertex_count);
        assert(shell_faces[0][0]==0u&&
               shell_faces[0][1]==4u&&
               shell_faces[0][2]==1u);
        assert(shell_faces[8u*strip.cell_count][0]==0u&&
               shell_faces[8u*strip.cell_count][1]==1u&&
               shell_faces[8u*strip.cell_count][2]==2u);
        assert(shell_faces[shell.triangle_count-1u][0]==
                   4u*strip.cell_count+1u);
        const MusaDwgAc1018BulgeRibbonIndexedShell saved_shell=shell;
        shell_faces[0][0]=UINT32_C(56789);
        assert(!musa_dwg_ac1018_bulge_ribbon_strip_indexed_shell(
                    mesh,&strip,shell_faces,1u,&shell));
        assert(shell_faces[0][0]==UINT32_C(56789)&&
               !memcmp(&shell,&saved_shell,sizeof(shell)));
        MusaDwgAc1018BulgeRibbonSection malformed[513]={{0}};
        memcpy(malformed,mesh,strip.section_count*sizeof(mesh[0]));
        malformed[0].right_thickness_wcs[1]+=0.125;
        assert(!musa_dwg_ac1018_bulge_ribbon_strip_indexed_shell(
                    malformed,&strip,shell_faces,8u*512u+4u,&shell));
        assert(shell_faces[0][0]==UINT32_C(56789)&&
               !memcmp(&shell,&saved_shell,sizeof(shell)));
        assert(!musa_dwg_ac1018_bulge_ribbon_strip_indexed_shell(
                    mesh,&strip,(uint32_t (*)[3])(void*)mesh,
                    8u*512u+4u,&shell));
        assert(!musa_dwg_ac1018_bulge_ribbon_strip_indexed_shell(
                    mesh,&strip,shell_faces,8u*512u+4u,NULL));
        assert(!memcmp(&shell,&saved_shell,sizeof(shell)));
        MusaDwgAc1018BulgeRibbonSection sentinel[2]={0};
        sentinel[0].left_wcs[0]=321.0;
        const MusaDwgAc1018BulgeRibbonStrip saved_strip=strip;
        assert(!musa_dwg_ac1018_scene_tessellate_bulge_ribbon_strip(
                     s,0x42bu,index,1e-12,1u,sentinel,2u,&strip));
        assert(near(sentinel[0].left_wcs[0],321.0)&&
               !memcmp(&strip,&saved_strip,sizeof(strip)));
    }
    MusaDwgAc1018BulgeRibbonSection invalid_mesh[513]={0};
    MusaDwgAc1018BulgeRibbonStrip invalid_strip={0};
    invalid_mesh[0].left_wcs[0]=678.0;
    invalid_strip.handle=UINT64_C(0xabcd);
    assert(!musa_dwg_ac1018_scene_tessellate_bulge_ribbon_strip(
                s,0x40eu,0u,0.01,512u,invalid_mesh,513u,
                &invalid_strip)); /* original straight only */
    assert(!musa_dwg_ac1018_scene_tessellate_bulge_ribbon_strip(
                s,0x42bu,1u,0.01,512u,invalid_mesh,513u,
                &invalid_strip)); /* selected straight segment */
    assert(!musa_dwg_ac1018_scene_tessellate_bulge_ribbon_strip(
                s,0x42bu,0u,NAN,512u,invalid_mesh,513u,
                &invalid_strip));
    assert(!musa_dwg_ac1018_scene_tessellate_bulge_ribbon_strip(
                s,0x42bu,0u,0.01,512u,invalid_mesh,1u,
                &invalid_strip));
    assert(!musa_dwg_ac1018_scene_tessellate_bulge_ribbon_strip(
                s,0x42bu,0u,0.01,512u,NULL,513u,
                &invalid_strip));
    assert(!musa_dwg_ac1018_scene_tessellate_bulge_ribbon_strip(
                s,0x42bu,0u,0.01,512u,invalid_mesh,513u,
                (MusaDwgAc1018BulgeRibbonStrip*)(void*)s));
    assert(near(invalid_mesh[0].left_wcs[0],678.0)&&
           invalid_strip.handle==UINT64_C(0xabcd));
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
    /* Protected original R2004 H=0x42B CLOSED, with positive and
     * negative signed bulges plus one straight TAPER: the single-read
     * complete ribbon/bevel numerical box must enclose EACH independently
     * tested curve envelope and all four true tapered straight corners.
     * An affine bevel between endpoints cannot escape that union. */
    MusaDwgAc1018LwRibbonBevelWcsEnvelope whole_complex={0};
    assert(musa_dwg_ac1018_scene_enclose_lw_ribbon_bevel_wcs(
                s,0x42bu,0.02,32768u,&whole_complex));
    assert(whole_complex.handle==0x42bu&&
           whole_complex.vertex_count==3u&&
           whole_complex.segment_count==3u&&
           whole_complex.curved_segments==2u&&
           whole_complex.straight_segments==1u&&
           whole_complex.bevel_bridge_count==3u&&
           whole_complex.discontinuous_width_junctions>=1u&&
           whole_complex.curve_intervals_total>=32u&&
           whole_complex.maximum_curve_gap<=0.02&&
           whole_complex.maximum_bevel_edge_gap>0.0&&
           whole_complex.bevel_bridge_endpoints_enclosed&&
           !whole_complex.all_miters_enclosed&&
           !whole_complex.all_round_joins_enclosed&&
           !whole_complex.complete_visible_stroke_verified);
    for(size_t axis=0u;axis<3u;axis++){
        const double expected_min=
              fmin(fmin(posbox.minimum[axis],negbox.minimum[axis]),
                   taper.minimum[axis]);
        const double expected_max=
              fmax(fmax(posbox.maximum[axis],negbox.maximum[axis]),
                   taper.maximum[axis]);
        assert(near(whole_complex.minimum[axis],expected_min)&&
               near(whole_complex.maximum[axis],expected_max));
    }
    /* Full R2004 H=0x42B mixed curved/straight, variable width and
     * closed junctions: compare the single-pass miter union with the
     * independently validated per-junction candidate reader.
     * Only valid tangent-line intersections count, not DWG join styles. */
    MusaDwgAc1018LwRibbonMiterWcsEnvelope whole_miter={0};
    assert(musa_dwg_ac1018_scene_enclose_lw_ribbon_miter_wcs(
                 s,0x42bu,0.02,32768u,8.0,&whole_miter));
    assert(whole_miter.bounded_tangent_candidates_enclosed&&
           !whole_miter.complete_visible_stroke_verified&&
           whole_miter.valid_miter_candidates+
             whole_miter.rejected_miter_candidates==6u);
    assert(!memcmp(&whole_miter.bevel,&whole_complex,
                   sizeof(whole_complex)));
    double expected_min[3]={0},expected_max[3]={0};
    for(size_t axis=0;axis<3;axis++){
        expected_min[axis]=whole_complex.minimum[axis];
        expected_max[axis]=whole_complex.maximum[axis];
    }
    uint32_t expected_candidates=0u;
    for(uint32_t vertex=0;vertex<3;vertex++){
        MusaDwgAc1018LwJoinCandidates single_join={0};
        assert(musa_dwg_ac1018_scene_lw_join_candidates(
                       s,0x42bu,vertex,8.0,&single_join));
        for(size_t side=0;side<2;side++){
            if(!single_join.tangent_miter_valid[side])continue;
            expected_candidates++;
            for(size_t axis=0;axis<3;axis++){
                const double low=fmin(single_join.miter_wcs[side][axis],
                              single_join.miter_thickness_wcs[side][axis]);
                const double high=fmax(single_join.miter_wcs[side][axis],
                              single_join.miter_thickness_wcs[side][axis]);
                if(low<expected_min[axis])expected_min[axis]=low;
                if(high>expected_max[axis])expected_max[axis]=high;
            }
        }
    }
    assert(expected_candidates==whole_miter.valid_miter_candidates);
    for(size_t axis=0;axis<3;axis++){
        assert(near(whole_miter.minimum[axis],expected_min[axis])&&
               near(whole_miter.maximum[axis],expected_max[axis])&&
               whole_miter.minimum[axis]<=whole_complex.minimum[axis]&&
               whole_miter.maximum[axis]>=whole_complex.maximum[axis]);
    }
    MusaDwgAc1018LwRibbonMiterWcsEnvelope restricted={0};
    assert(musa_dwg_ac1018_scene_enclose_lw_ribbon_miter_wcs(
                 s,0x42bu,0.02,32768u,1.0,&restricted));
    assert(restricted.valid_miter_candidates<=
           whole_miter.valid_miter_candidates);
    for(size_t axis=0;axis<3;axis++){
        assert(restricted.minimum[axis]>=whole_miter.minimum[axis]-1e-8&&
               restricted.maximum[axis]<=whole_miter.maximum[axis]+1e-8);
    }
    const MusaDwgAc1018LwRibbonMiterWcsEnvelope kept_miter=whole_miter;
    assert(!musa_dwg_ac1018_scene_enclose_lw_ribbon_miter_wcs(
                 s,0x42bu,1e-12,16u,8.0,&whole_miter));
    assert(!musa_dwg_ac1018_scene_enclose_lw_ribbon_miter_wcs(
                 s,0x42bu,0.02,32768u,0.0,&whole_miter));
    assert(!musa_dwg_ac1018_scene_enclose_lw_ribbon_miter_wcs(
                 s,0x42bu,0.02,32768u,8.0,
                 (MusaDwgAc1018LwRibbonMiterWcsEnvelope*)(void*)s));
    assert(!memcmp(&kept_miter,&whole_miter,sizeof(whole_miter)));
    /* R2004 original closed H=0x42B: 3 joints * 2 sides, all variable
     * source radii. Independent dense intermediate source-edge connector
     * points must lie inside the analytic harmonic-interval WCS envelope,
     * with signed thickness Y=3.50..3.75. Not an AutoCAD join policy. */
    MusaDwgAc1018LwRadialBridgeWcsEnvelope whole_round={0};
    assert(musa_dwg_ac1018_scene_enclose_lw_radial_bridges_wcs(
                  s,0x42bu,0.02,32768u,&whole_round));
    /* Exactly six source side transitions are CONSIDERED. A genuine
     * source zero-width or antipodal side is fail-closed and stays under
     * the original bevel envelope instead of fabricating a round arc. */
    assert(whole_round.radial_bridges_enclosed+
           whole_round.radial_bridges_rejected==6u);
    assert(whole_round.radial_bridges_enclosed>=1u);
    assert(whole_round.variable_radius_bridges<=
           whole_round.radial_bridges_enclosed);
    assert(whole_round.all_admitted_radial_bridges_enclosed);
    assert(!whole_round.complete_visible_stroke_verified);
    assert(!memcmp(&whole_round.bevel,&whole_complex,
                   sizeof(whole_complex)));
    assert(near(whole_round.minimum[1],3.5)&&
           near(whole_round.maximum[1],3.75));
    for(size_t k=0u;k<3u;k++){
        assert(whole_round.minimum[k]<=whole_complex.minimum[k]&&
               whole_round.maximum[k]>=whole_complex.maximum[k]);
    }
    for(uint32_t vertex=0u;vertex<3u;vertex++){
        MusaDwgAc1018LwJoinEdgeAudit j={0};
        assert(musa_dwg_ac1018_scene_audit_lw_join_edges(
                      s,0x42bu,vertex,&j));
        for(size_t side=0u;side<2u;side++){
            /* +Y extrusion: true OCS X/Y map to world -X/Z.
             * Using atan2 of WCS Z/X radial vectors is an independent
             * physical-circle parameterization with the same short arc
             * (reflection reverses sweep, not the circle's point set). */
            const double x=j.center_wcs[0],z=j.center_wcs[2];
            const double dx0=j.incoming_edge_wcs[side][0]-x;
            const double dz0=j.incoming_edge_wcs[side][2]-z;
            const double dx1=j.outgoing_edge_wcs[side][0]-x;
            const double dz1=j.outgoing_edge_wcs[side][2]-z;
            const double r0=hypot(dx0,dz0),r1=hypot(dx1,dz1);
            const double a0=atan2(dz0,dx0),a1=atan2(dz1,dx1);
            const double sweep=atan2(sin(a1-a0),cos(a1-a0));
            /* The production engine REFUSES zero-radius and
             * near-antipodal geometry; independently reproduce those
             * explicit admissions before checking continuous coverage. */
            if(!(r0>0.0)||!(r1>0.0)||
               fabs(sweep)>=acos(-1.0)-1e-12)
                continue;
            for(uint32_t sample=0u;sample<=1000u;sample++){
                const double t=(double)sample/1000.0;
                const double radius=(1.0-t)*r0+t*r1;
                const double angle=a0+t*sweep;
                const double vx=x+radius*cos(angle);
                const double vz=z+radius*sin(angle);
                assert(vx>=whole_round.minimum[0]-1e-9&&
                       vx<=whole_round.maximum[0]+1e-9&&
                       vz>=whole_round.minimum[2]-1e-9&&
                       vz<=whole_round.maximum[2]+1e-9&&
                       j.center_wcs[1]>=whole_round.minimum[1]-1e-9&&
                       j.center_wcs[1]+0.25<=
                                           whole_round.maximum[1]+1e-9);
            }
        }
    }
    const MusaDwgAc1018LwRadialBridgeWcsEnvelope saved_round=
                 whole_round;
    assert(!musa_dwg_ac1018_scene_enclose_lw_radial_bridges_wcs(
                  s,0x42bu,1e-12,16u,&whole_round));
    assert(!musa_dwg_ac1018_scene_enclose_lw_radial_bridges_wcs(
                  s,0x42bu,0.02,15u,&whole_round));
    assert(!musa_dwg_ac1018_scene_enclose_lw_radial_bridges_wcs(
                  s,0x42bu,0.02,32768u,
                  (MusaDwgAc1018LwRadialBridgeWcsEnvelope*)(void*)s));
    assert(!memcmp(&saved_round,&whole_round,sizeof(whole_round)));
    const MusaDwgAc1018LwRibbonBevelWcsEnvelope keep_whole=
             whole_complex;
    assert(!musa_dwg_ac1018_scene_enclose_lw_ribbon_bevel_wcs(
                s,0x42bu,1e-12,16u,&whole_complex));
    assert(!musa_dwg_ac1018_scene_enclose_lw_ribbon_bevel_wcs(
                s,0x42bu,0.02,32768u,NULL));
    assert(!musa_dwg_ac1018_scene_enclose_lw_ribbon_bevel_wcs(
                s,0x42bu,0.02,32768u,
                (MusaDwgAc1018LwRibbonBevelWcsEnvelope*)(void*)s));
    assert(!memcmp(&keep_whole,&whole_complex,sizeof(whole_complex)));
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
    /* True source H=0x42b has TWO curved and one tapered straight
     * edge: audit both curve->line and line->curve junctions, plus
     * closed vertex zero connecting the two signed bulges. */
    MusaDwgAc1018LwJoinEdgeAudit complex_join={0};
    MusaDwgAc1018BulgeWidthEdgePoint boundary_sample={0};
    assert(musa_dwg_ac1018_scene_audit_lw_join_edges(
                  s,0x42bu,1u,&complex_join));
    assert(complex_join.handle==0x42bu&&
           complex_join.vertex_index==1u&&
           complex_join.incoming_segment_index==0u&&
           complex_join.outgoing_segment_index==1u&&
           complex_join.incoming_curved&&!complex_join.outgoing_curved&&
           !complex_join.complete_visible_join_verified&&
           near(complex_join.incoming_end_width,0.25)&&
           near(complex_join.outgoing_start_width,sw)&&
           near(complex_join.center_wcs[0],-2.5)&&
           near(complex_join.center_wcs[1],3.5)&&
           near(complex_join.center_wcs[2],-4.5));
    assert(musa_dwg_ac1018_bulge_edge_snapshot_evaluate(
                  positive_snap,1.0,&boundary_sample));
    for(size_t side=0u;side<2u;side++){
        const double *incoming=side==0u?
                   boundary_sample.left_wcs:boundary_sample.right_wcs;
        for(size_t k=0u;k<3u;k++){
            assert(near(complex_join.incoming_edge_wcs[side][k],
                        incoming[k])&&
                   near(complex_join.outgoing_edge_wcs[side][k],
                        taper.corners[side][k]));
        }
        assert(near(complex_join.incoming_edge_thickness_wcs[side][1],
                    3.75)&&
               near(complex_join.outgoing_edge_thickness_wcs[side][1],
                    3.75));
        const double gap=hypot(
                  complex_join.incoming_edge_wcs[side][0]-
                  complex_join.outgoing_edge_wcs[side][0],
                  complex_join.incoming_edge_wcs[side][2]-
                  complex_join.outgoing_edge_wcs[side][2]);
        assert(near(complex_join.edge_gap[side],gap));
        for(size_t dir=0u;dir<2u;dir++){
            const double *t=dir==0u?
               complex_join.incoming_edge_tangent_wcs[side]:
               complex_join.outgoing_edge_tangent_wcs[side];
            assert(near(hypot(hypot(t[0],t[1]),t[2]),1.0));
        }
    }
    assert(complex_join.source_widths_match==
           (uint8_t)(fabs(0.25-sw)<=
                     1e-10*(1.0+0.25+fabs(sw)+7.0)));
    assert(musa_dwg_ac1018_scene_audit_lw_join_edges(
                  s,0x42bu,2u,&complex_join));
    assert(!complex_join.incoming_curved&&
           complex_join.outgoing_curved&&
           complex_join.incoming_segment_index==1u&&
           complex_join.outgoing_segment_index==2u&&
           near(complex_join.incoming_end_width,ew)&&
           near(complex_join.outgoing_start_width,1.0));
    assert(musa_dwg_ac1018_bulge_edge_snapshot_evaluate(
                  negative_snap,0.0,&boundary_sample));
    for(size_t side=0u;side<2u;side++){
        const double *outward=side==0u?
                  boundary_sample.left_wcs:boundary_sample.right_wcs;
        for(size_t k=0u;k<3u;k++){
            assert(near(complex_join.incoming_edge_wcs[side][k],
                        taper.corners[2u+side][k])&&
                   near(complex_join.outgoing_edge_wcs[side][k],
                        outward[k]));
        }
    }
    assert(musa_dwg_ac1018_scene_audit_lw_join_edges(
                  s,0x42bu,0u,&complex_join));
    assert(complex_join.incoming_curved&&
           complex_join.outgoing_curved&&
           complex_join.incoming_segment_index==2u&&
           complex_join.outgoing_segment_index==0u&&
           near(complex_join.incoming_end_width,1.5)&&
           near(complex_join.outgoing_start_width,0.125));
    assert(musa_dwg_ac1018_bulge_edge_snapshot_evaluate(
                  negative_snap,1.0,&boundary_sample));
    for(size_t k=0u;k<3u;k++){
        assert(near(complex_join.incoming_edge_wcs[0][k],
                    boundary_sample.left_wcs[k]));
    }
    /* Genuine protected R2004 H=0x42B has mixed curved and tapered
     * straight edges with signed extrusion. For every one of its 3
     * joins, bevel bridge points are EXACT source boundary endpoints;
     * finite miter candidates are bounded tangent intersections, NOT
     * a claim about DWG rendered join style or round/miter fills. */
    for(uint32_t vertex=0u;vertex<3u;vertex++){
        MusaDwgAc1018LwJoinCandidates joined={0};
        assert(musa_dwg_ac1018_scene_lw_join_candidates(
                  s,0x42bu,vertex,8.0,&joined));
        assert(joined.source.handle==0x42bu&&
               joined.source.vertex_index==vertex&&
               !joined.complete_visible_join_verified&&
               near(joined.miter_limit,8.0));
        const double allowed=8.0*0.5*fmax(
                          joined.source.incoming_end_width,
                          joined.source.outgoing_start_width);
        for(size_t side=0u;side<2u;side++){
            for(size_t k=0u;k<3u;k++){
                assert(near(joined.bevel_wcs[side][0][k],
                            joined.source.incoming_edge_wcs[side][k])&&
                       near(joined.bevel_wcs[side][1][k],
                            joined.source.outgoing_edge_wcs[side][k])&&
                       near(joined.bevel_thickness_wcs[side][0][k],
                            joined.source.incoming_edge_thickness_wcs[side][k])&&
                       near(joined.bevel_thickness_wcs[side][1][k],
                            joined.source.outgoing_edge_thickness_wcs[side][k]));
            }
            if(joined.tangent_miter_valid[side]){
                assert(fabs(joined.incoming_extension[side])<=
                       allowed+1e-7&&
                       fabs(joined.outgoing_extension[side])<=
                       allowed+1e-7);
                for(size_t k=0u;k<3u;k++){
                    const double from_in=
                       joined.source.incoming_edge_wcs[side][k]+
                       joined.incoming_extension[side]*
                       joined.source.incoming_edge_tangent_wcs[side][k];
                    const double from_out=
                       joined.source.outgoing_edge_wcs[side][k]+
                       joined.outgoing_extension[side]*
                       joined.source.outgoing_edge_tangent_wcs[side][k];
                    assert(near(joined.miter_wcs[side][k],from_in)&&
                           near(joined.miter_wcs[side][k],from_out)&&
                           near(joined.miter_thickness_wcs[side][k]-
                                joined.miter_wcs[side][k],
                                k==1u?0.25:0.0));
                }
            }
        }
    }
    /* A very small miter budget can reject a spike on either side,
     * while ALWAYS preserving the source bevel bridge geometry. */
    MusaDwgAc1018LwJoinCandidates strict_join={0};
    assert(musa_dwg_ac1018_scene_lw_join_candidates(
                  s,0x42bu,1u,1.0,&strict_join));
    assert(!strict_join.complete_visible_join_verified);
    assert(near(strict_join.source.center_wcs[1],3.5));
    const MusaDwgAc1018LwJoinCandidates keep_strict=strict_join;
    assert(!musa_dwg_ac1018_scene_lw_join_candidates(
                  s,UINT64_C(0xbad),1u,8.0,&strict_join));
    assert(!musa_dwg_ac1018_scene_lw_join_candidates(
                  s,0x42bu,3u,8.0,&strict_join));
    assert(!memcmp(&keep_strict,&strict_join,sizeof(strict_join)));
    const MusaDwgAc1018LwJoinEdgeAudit joined_saved=complex_join;
    assert(!musa_dwg_ac1018_scene_audit_lw_join_edges(
                  s,0x42bu,3u,&complex_join));
    assert(!musa_dwg_ac1018_scene_audit_lw_join_edges(
                  s,UINT64_C(0xbeef),0u,&complex_join));
    assert(!musa_dwg_ac1018_scene_audit_lw_join_edges(
                  s,0x42bu,1u,NULL));
    assert(!musa_dwg_ac1018_scene_audit_lw_join_edges(
                  s,0x42bu,1u,
                  (MusaDwgAc1018LwJoinEdgeAudit*)(void*)s));
    assert(!memcmp(&joined_saved,&complex_join,sizeof(complex_join)));
    /* Genuine CRC-backed original R2004 H=0x42B variable-width
     * curved/straight mixed polyline. Fan point triangles are the
     * explicit mathematical radial-sector fill, not AutoCAD's DWG
     * selection of round/inner/outer join or final stroke topology.
     * Check WCS endpoint exactness, thickness and the upper
     * interpolation deviation by independent mid-segment evaluation. */
    uint32_t admitted_fans=0u,rejected_fans=0u;
    for(uint32_t vertex=0u;vertex<3u;vertex++){
        MusaDwgAc1018LwJoinEdgeAudit j={0};
        assert(musa_dwg_ac1018_scene_audit_lw_join_edges(
                  s,0x42bu,vertex,&j));
        for(uint32_t side=0u;side<2u;side++){
            double boundary[513][3]={{0}},thick[513][3]={{0}};
            MusaDwgAc1018LwRadialFan fan={0};
            const int ok=musa_dwg_ac1018_scene_tessellate_lw_radial_fan(
                  s,0x42bu,vertex,side,0.01,512u,
                  boundary,thick,513u,&fan);
            const double r0=0.5*j.incoming_end_width;
            const double r1=0.5*j.outgoing_start_width;
            if(!(r0>0.0)||!(r1>0.0)){
                assert(!ok);
                rejected_fans++;
                continue;
            }
            assert(ok);
            admitted_fans++;
            assert(fan.handle==0x42bu&&
                   fan.vertex_index==vertex&&fan.side==side&&
                   fan.triangle_count>0u&&fan.triangle_count<=512u&&
                   fan.boundary_point_count==fan.triangle_count+1u&&
                   fan.maximum_chord_deviation_bound<=0.01&&
                   !fan.complete_visible_join_verified&&
                   near(fan.incoming_radius,r0)&&
                   near(fan.outgoing_radius,r1));
            for(size_t k=0u;k<3u;k++){
                assert(near(boundary[0][k],
                            j.incoming_edge_wcs[side][k])&&
                       near(boundary[fan.triangle_count][k],
                            j.outgoing_edge_wcs[side][k])&&
                       near(fan.center_wcs[k],j.center_wcs[k])&&
                       near(thick[0][k],
                            j.incoming_edge_thickness_wcs[side][k])&&
                       near(thick[fan.triangle_count][k],
                            j.outgoing_edge_thickness_wcs[side][k]));
            }
            double u[3]={0},v[3]={0},w[3]={0},dot=0.0;
            for(size_t k=0u;k<3u;k++){
                u[k]=(j.incoming_edge_wcs[side][k]-
                         j.center_wcs[k])/r0;
                w[k]=(j.outgoing_edge_wcs[side][k]-
                         j.center_wcs[k])/r1;
                dot+=u[k]*w[k];
            }
            dot=fmax(-1.0,fmin(1.0,dot));
            const double theta=acos(dot);
            for(size_t k=0u;k<3u;k++)
                v[k]=(w[k]-dot*u[k])/sin(theta);
            for(uint32_t i=0u;i<fan.triangle_count;i++){
                const double t=((double)i+0.5)/
                                (double)fan.triangle_count;
                const double r=(1.0-t)*r0+t*r1;
                const double ct=cos(t*theta),st=sin(t*theta);
                double err2=0.0;
                for(size_t k=0u;k<3u;k++){
                    const double exact=j.center_wcs[k]+
                              r*(ct*u[k]+st*v[k]);
                    const double linear=0.5*(boundary[i][k]+
                                              boundary[i+1u][k]);
                    err2+=(exact-linear)*(exact-linear);
                    assert(near(thick[i][k]-boundary[i][k],
                                k==1u?0.25:0.0));
                }
                assert(sqrt(err2)<=
                         fan.maximum_chord_deviation_bound+1e-9);
            }
            /* A single triangle cannot meet a strict error budget.
             * Failure must not alter the caller's geometry or report. */
            double guard[2][3]={{0}},guard_thick[2][3]={{0}};
            guard[0][0]=77.0;
            guard_thick[0][0]=88.0;
            MusaDwgAc1018LwRadialFan retained=fan;
            assert(!musa_dwg_ac1018_scene_tessellate_lw_radial_fan(
                  s,0x42bu,vertex,side,1e-10,1u,
                  guard,guard_thick,2u,&fan));
            assert(near(guard[0][0],77.0)&&
                   near(guard_thick[0][0],88.0)&&
                   !memcmp(&retained,&fan,sizeof(fan)));
        }
    }
    assert(admitted_fans>0u&&
           admitted_fans+rejected_fans==6u);
    double scratch[513][3]={{0}},scratch_thick[513][3]={{0}};
    MusaDwgAc1018LwRadialFan last_fan={0};
    last_fan.handle=UINT64_C(0x222);
    assert(!musa_dwg_ac1018_scene_tessellate_lw_radial_fan(
         s,0x42bu,3u,0u,0.01,512u,scratch,scratch_thick,
         513u,&last_fan));
    assert(!musa_dwg_ac1018_scene_tessellate_lw_radial_fan(
         s,0x42bu,1u,2u,0.01,512u,scratch,scratch_thick,
         513u,&last_fan));
    assert(!musa_dwg_ac1018_scene_tessellate_lw_radial_fan(
         s,0x42bu,1u,0u,NAN,512u,scratch,scratch_thick,
         513u,&last_fan));
    assert(!musa_dwg_ac1018_scene_tessellate_lw_radial_fan(
         s,0x42bu,1u,0u,0.01,512u,scratch,scratch,
         513u,&last_fan));
    assert(last_fan.handle==UINT64_C(0x222));
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
    /* Envelope remains computable after source file + native scene
     * destruction. No borrowing original compressed or indexed data. */
    MusaDwgAc1018BulgeEdgeWcsEnvelope after_close_box={0};
    assert(musa_dwg_ac1018_bulge_edge_snapshot_enclose(
                  positive_snap,0.02,32768u,&after_close_box));
    for(size_t k=0u;k<3u;k++){
        assert(near(after_close_box.minimum[k],original_box.minimum[k])&&
               near(after_close_box.maximum[k],original_box.maximum[k]));
    }
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

