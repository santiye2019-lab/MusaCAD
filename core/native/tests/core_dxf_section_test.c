/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "musacad_dxf_section.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static int feed(MusaDxfSectionState *s,int code,const char *value){
    int flags=0;
    const int status=musa_core_dxf_section_step(s,code,value,strlen(value),&flags);
    assert(status==MUSA_DXF_SECTION_OK);
    return flags;
}
static void reject(MusaDxfSectionState *s,int code,const char *value,int status){
    MusaDxfSectionState before=*s;
    int untouched=99;
    assert(musa_core_dxf_section_step(s,code,value,strlen(value),&untouched)==status);
    assert(memcmp(&before,s,sizeof(before))==0);
    assert(untouched==99);
}
int main(void){
    MusaDxfSectionState s={0};
    assert(musa_core_dxf_section_finish(&s)==MUSA_DXF_SECTION_INCOMPLETE);
    reject(&s,0,"ENDSEC",MUSA_DXF_SECTION_ENDSEC_MISMATCH);
    reject(&s,0,"EOF",MUSA_DXF_SECTION_EOF_MISMATCH);
    reject(&s,0,"LINE",MUSA_DXF_SECTION_OUTSIDE);
    feed(&s,0,"SECTION");
    reject(&s,0,"LINE",MUSA_DXF_SECTION_MISSING_NAME);
    reject(&s,2," ",MUSA_DXF_SECTION_MISSING_NAME);
    feed(&s,2," header ");
    reject(&s,0,"SECTION",MUSA_DXF_SECTION_NESTED);
    assert(feed(&s,9,"$HANDSEED")==0);
    feed(&s,0,"ENDSEC");
    feed(&s,0,"SECTION");feed(&s,2,"eNtItIeS");
    reject(&s,0,"EOF",MUSA_DXF_SECTION_EOF_MISMATCH);
    int f=feed(&s,0,"LINE");
    assert((f&(MUSA_DXF_SECTION_RECORD_BOUNDARY|MUSA_DXF_SECTION_LINE_STARTED))==3);
    assert(feed(&s,10,"1")&MUSA_DXF_SECTION_HANDLE_AREA);
    assert(feed(&s,5,"A1")&MUSA_DXF_SECTION_HANDLE_AREA);
    feed(&s,0,"ENDSEC");
    feed(&s,0,"SECTION");
    reject(&s,2,"ENTITIES",MUSA_DXF_SECTION_DUP_ENTITIES);
    feed(&s,2,"BLOCKS");feed(&s,0,"ENDBLK");feed(&s,0,"ENDSEC");
    assert(feed(&s,0,"EOF")&MUSA_DXF_SECTION_RECORD_BOUNDARY);
    assert(musa_core_dxf_section_finish(&s)==MUSA_DXF_SECTION_OK);
    reject(&s,0,"EOF",MUSA_DXF_SECTION_AFTER_EOF);
    MusaDxfSectionState incomplete={0};
    feed(&incomplete,0,"SECTION");
    assert(musa_core_dxf_section_finish(&incomplete)==MUSA_DXF_SECTION_INCOMPLETE);
    assert(musa_core_dxf_section_step(NULL,0,"X",1,&f)==MUSA_DXF_SECTION_BAD_ARGUMENT);

    /* The same canonical state machine must reject malformed LINE geometry
     * without mutating committed data, and require all four XY values.
     */
    int line=0, next=99;
    assert(musa_core_dxf_line_step(line,0,"LINE",4,
        MUSA_DXF_SECTION_RECORD_BOUNDARY|MUSA_DXF_SECTION_LINE_STARTED,&next)==0);
    assert(next==16);line=next;
    assert(musa_core_dxf_line_step(line,10,"1.25",4,0,&next)==0);
    assert(next==17);line=next;
    next=99;
    assert(musa_core_dxf_line_step(line,11,"NaN",3,0,&next)==
           MUSA_DXF_LINE_INVALID_NUMBER);
    assert(next==99);
    assert(musa_core_dxf_line_step(line,11,"1e999",5,0,&next)==
           MUSA_DXF_LINE_INVALID_NUMBER);
    assert(next==99);
    assert(musa_core_dxf_line_step(line,11,"0x1p2",5,0,&next)==
           MUSA_DXF_LINE_INVALID_NUMBER);
    assert(next==99);
    assert(musa_core_dxf_line_step(line,20,"-2.0e+3",7,0,&next)==0);
    assert(next==19);line=next;
    assert(musa_core_dxf_line_step(line,11,"0.01",4,0,&next)==0);
    assert(next==23);line=next;
    next=99;
    assert(musa_core_dxf_line_step(line,0,"ENDSEC",6,
        MUSA_DXF_SECTION_RECORD_BOUNDARY,&next)==
        MUSA_DXF_LINE_COORDINATES_MISSING);
    assert(next==99);
    assert(musa_core_dxf_line_step(line,21,"-1",2,0,&next)==0);
    assert(next==31);line=next;
    assert(musa_core_dxf_line_step(line,0,"ENDSEC",6,
        MUSA_DXF_SECTION_RECORD_BOUNDARY,&next)==0);
    assert(next==0);
    puts("PASS: portable DXF LINE coordinates, missing data, overflow, atomic state");
    puts("PASS: cross-platform source-safe DXF SECTION/ENDSEC/EOF finite state machine");
    return 0;
}
