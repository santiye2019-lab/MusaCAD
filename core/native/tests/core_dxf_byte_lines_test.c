/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "musacad_dxf_byte_lines.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static int feed(MusaDxfByteLines *r,const unsigned char *data,size_t n,
                size_t *used,const char *expected,size_t count){
    const unsigned char *line=NULL;size_t bytes=0;
    int status=musa_core_dxf_byte_lines_step(r,data,n,used,&line,&bytes);
    if(status==MUSA_DXF_BYTE_LINE){
        assert(bytes==count);
        assert(memcmp(line,expected,count)==0);
    }
    return status;
}
int main(void){
    assert(musa_core_dxf_byte_lines_create(0)==NULL);
    assert(musa_core_dxf_byte_lines_create(1048577u)==NULL);
    MusaDxfByteLines *r=musa_core_dxf_byte_lines_create(5);
    assert(r);
    size_t used=99;
    assert(feed(r,(const unsigned char *)"ab\r",3,&used,"",0)==MUSA_DXF_BYTE_MORE);
    assert(used==3);
    assert(feed(r,(const unsigned char *)"c\r\nZZ",5,&used,"abc",3)==MUSA_DXF_BYTE_LINE);
    assert(used==3);
    assert(feed(r,(const unsigned char *)"ZZ\n",3,&used,"ZZ",2)==MUSA_DXF_BYTE_LINE);
    assert(feed(r,(const unsigned char *)"\n",1,&used,"",0)==MUSA_DXF_BYTE_LINE);
    assert(feed(r,(const unsigned char *)"12345",5,&used,"",0)==MUSA_DXF_BYTE_MORE);
    assert(feed(r,(const unsigned char *)"\n",1,&used,"12345",5)==MUSA_DXF_BYTE_LINE);
    assert(feed(r,(const unsigned char *)"123456",6,&used,"",0)==MUSA_DXF_BYTE_TOO_LONG);
    const unsigned char *line=NULL;size_t length=99;
    assert(musa_core_dxf_byte_lines_finish(r,&line,&length)==MUSA_DXF_BYTE_LINE);
    assert(length==5 && !memcmp(line,"12345",5));
    assert(musa_core_dxf_byte_lines_finish(r,&line,&length)==MUSA_DXF_BYTE_BAD_ARGUMENT);
    musa_core_dxf_byte_lines_destroy(r);
    r=musa_core_dxf_byte_lines_create(4);
    assert(r);
    assert(feed(r,(const unsigned char *)"a\r\rb",4,&used,"",0)==MUSA_DXF_BYTE_MORE);
    assert(musa_core_dxf_byte_lines_finish(r,&line,&length)==MUSA_DXF_BYTE_LINE);
    assert(length==2&&!memcmp(line,"ab",2));
    musa_core_dxf_byte_lines_destroy(r);
    r=musa_core_dxf_byte_lines_create(4);
    assert(r);
    assert(musa_core_dxf_byte_lines_finish(r,&line,&length)==MUSA_DXF_BYTE_EOF);
    musa_core_dxf_byte_lines_destroy(r);
    /* Record streams preserve inner CR and remove only one trailing CR.
     * Staged-save mode above deliberately keeps legacy drop-all-CR behavior.
     */
    assert(musa_core_dxf_byte_lines_create_mode(8,2)==NULL);
    assert(musa_core_dxf_byte_lines_create_mode(8,-1)==NULL);
    r=musa_core_dxf_byte_lines_create_mode(8,MUSA_DXF_CR_TRAILING_ONLY);
    assert(r);
    assert(feed(r,(const unsigned char *)"A\rB\r",4,&used,"",0)==MUSA_DXF_BYTE_MORE);
    assert(feed(r,(const unsigned char *)"\n",1,&used,"A\rB",3)==MUSA_DXF_BYTE_LINE);
    assert(feed(r,(const unsigned char *)"x\r\r",3,&used,"",0)==MUSA_DXF_BYTE_MORE);
    assert(musa_core_dxf_byte_lines_finish(r,&line,&length)==MUSA_DXF_BYTE_LINE);
    assert(length==2&&!memcmp(line,"x\r",2));
    musa_core_dxf_byte_lines_destroy(r);
    r=musa_core_dxf_byte_lines_create_mode(4,MUSA_DXF_CR_TRAILING_ONLY);
    assert(r);
    assert(feed(r,(const unsigned char *)"a\rb\n",4,&used,"a\rb",3)==MUSA_DXF_BYTE_LINE);
    assert(feed(r,(const unsigned char *)"12345",5,&used,"",0)==MUSA_DXF_BYTE_TOO_LONG);
    musa_core_dxf_byte_lines_destroy(r);
        puts("PASS: portable bounded DXF byte line framing CRLF LF split EOF and cap");
    return 0;
}
