/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "musacad_dwg_ac1018_line.h"
#include "musacad_dwg_ac1018_lwpolyline.h"
#include "musacad_dwg_ac1018_reactor_links.h"
#include <assert.h>
#include <stdint.h>
#include <string.h>
/* Independently generated tiny R2004 records with one synthetic reactor.
 * All handles and XYZ coordinates are artificial. No private DWG data. */
static const uint8_t line_record[] = {
    0x2c,0x00,0x44,0xca,0xc0,0x40,0x00,0x00,0x80,0x48,0x20,0x80,
    0xe4,0x20,0x08,0x00,0x00,0x00,0x00,0x00,0x00,0x02,0x06,0x00,
    0x00,0x00,0x00,0x00,0x00,0x20,0x80,0x00,0x00,0x00,0x00,0x00,
    0x00,0x10,0x80,0x78,0x20,0xa8,0x2e,0xaa,0x26,0x60,0x34,0xe3,
};
static const uint8_t lw_record[] = {
    0x2d,0x00,0x53,0x4d,0x00,0x40,0x00,0x00,0x80,0x48,0x20,0x80,
    0xe4,0x20,0x09,0x02,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x40,
    0x00,0x00,0x00,0x00,0x00,0x00,0x08,0x40,0xc0,0x00,0x00,0x00,
    0x00,0x00,0x04,0x10,0x0c,0x10,0x54,0x17,0x55,0x13,0x30,0xc7,
    0x5c,
};
static void test_line(void){
    MusaDwgAc1015HandleEntry map[4]={
        {0x33u,1u},{0x75u,2u},{0x11bu,3u},{0x120u,0u}
    };
    uint64_t targets[2]={0x9u,0xau};
    MusaDwgAc1018Line line={0};
    MusaDwgAc1018ReactorAudit audit={0};
    /* Backward compatible: old API will NEVER ignore reactor links. */
    assert(!musa_dwg_ac1018_read_line_record(
        line_record,sizeof(line_record),0u,0x120u,&line));
    assert(musa_dwg_ac1018_read_line_with_reactors(
        line_record,sizeof(line_record),0u,0x120u,map,4u,
        targets,2u,&audit,&line));
    assert(line.handle==0x120u && line.start[0]==2.0 &&
           line.start[1]==3.0 && line.end[0]==4.0 &&
           line.end[1]==3.0 && line.start[2]==0.0);
    assert(audit.geometry_decoded==1u && audit.record_crc_verified &&
           audit.handle_membership_verified && audit.owner_handle==0x11bu &&
           audit.layer_handle==0x33u && audit.reactor_count==1u);
    assert(targets[0]==0x75u && targets[1]==0xau);
    MusaDwgAc1018Line saved_line=line;
    MusaDwgAc1018ReactorAudit saved_audit=audit;
    uint64_t saved_targets[2]={targets[0],targets[1]};
    /* A wrong handle or dangling target rejects without mutating outputs. */
    assert(!musa_dwg_ac1018_read_line_with_reactors(
        line_record,sizeof(line_record),0u,0x121u,map,4u,
        targets,2u,&audit,&line));
    assert(!musa_dwg_ac1018_read_line_with_reactors(
        line_record,sizeof(line_record),0u,0x120u,map,4u,
        targets,0u,&audit,&line));
    MusaDwgAc1015HandleEntry wrong[4];
    memcpy(wrong,map,sizeof(wrong));
    wrong[1].handle=0x76u;
    assert(!musa_dwg_ac1018_read_line_with_reactors(
        line_record,sizeof(line_record),0u,0x120u,wrong,4u,
        targets,2u,&audit,&line));
    uint8_t corrupt[sizeof(line_record)];
    memcpy(corrupt,line_record,sizeof(corrupt));corrupt[20]^=1u;
    assert(!musa_dwg_ac1018_read_line_with_reactors(
        corrupt,sizeof(corrupt),0u,0x120u,map,4u,
        targets,2u,&audit,&line));
    assert(!memcmp(&saved_line,&line,sizeof(line)) &&
           !memcmp(&saved_audit,&audit,sizeof(audit)) &&
           !memcmp(saved_targets,targets,sizeof(targets)));
}
static void test_lw(void){
    const MusaDwgAc1015HandleEntry map[4]={
        {0x33u,1u},{0x75u,2u},{0x11bu,3u},{0x120u,0u}
    };
    MusaDwgAc1018LWVertex vertices[3]={{0}};
    MusaDwgAc1018LWPolyline info={0};
    MusaDwgAc1018ReactorAudit audit={0};
    uint64_t target[2]={0xabu,0xcdu};
    assert(!musa_dwg_ac1018_read_lwpolyline_record(
        lw_record,sizeof(lw_record),0u,0x120u,vertices,3u,&info));
    assert(musa_dwg_ac1018_read_lwpolyline_with_reactors(
        lw_record,sizeof(lw_record),0u,0x120u,map,4u,
        vertices,3u,target,2u,&audit,&info));
    assert(info.vertex_count==2u && info.handle==0x120u &&
           audit.geometry_decoded && audit.layer_handle==0x33u &&
           audit.reactor_count==1u && target[0]==0x75u);
    assert(vertices[0].x==2.0 && vertices[0].y==3.0 &&
           vertices[1].x==4.0 && vertices[1].y==3.0);
    double world[3]={0};
    assert(musa_dwg_ac1018_lwpolyline_vertex_to_wcs(&info,&vertices[1],world));
    assert(world[0]==4.0 && world[1]==3.0 && world[2]==0.0);
    MusaDwgAc1018LWVertex preserved_vertices[3];
    memcpy(preserved_vertices,vertices,sizeof(vertices));
    MusaDwgAc1018LWPolyline preserved_info=info;
    MusaDwgAc1018ReactorAudit preserved_audit=audit;
    const uint64_t preserve[2]={target[0],target[1]};
    assert(!musa_dwg_ac1018_read_lwpolyline_with_reactors(
        lw_record,sizeof(lw_record),0u,0x120u,map,4u,
        vertices,1u,target,2u,&audit,&info));
    assert(!musa_dwg_ac1018_read_lwpolyline_with_reactors(
        lw_record,sizeof(lw_record)-1u,0u,0x120u,map,4u,
        vertices,3u,target,2u,&audit,&info));
    assert(!memcmp(preserved_vertices,vertices,sizeof(vertices)) &&
           !memcmp(&preserved_info,&info,sizeof(info)) &&
           !memcmp(&preserved_audit,&audit,sizeof(audit)) &&
           !memcmp(preserve,target,sizeof(target)));
}
int main(void){test_line();test_lw();return 0;}
