/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "musacad_dxf_document.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <stdlib.h>
static int read_string(const char *s,size_t max,MusaDocument **doc){
    return musa_dxf_document_read((const unsigned char *)s,strlen(s),max,doc);
}
static void rejected(const char *s,int status){
    MusaDocument *doc=(MusaDocument *)(uintptr_t)1u;
    int value=read_string(s,16,&doc);
    assert(value==status);
    assert(doc==NULL); /* no incomplete / lossy partial model exposed */
}
int main(void){
    const char *prefix="0\r\nSECTION\r\n2\r\nENTITIES\r\n";
    char valid[2048];
    snprintf(valid,sizeof(valid),
       "%s"
       "0\nLINE\n5\n00B\n8\n0\n10\n-1.25\n20\n1e2\n11\n4\n21\n2\n"
       "0\nCIRCLE\n5\nA\n10\n0\n20\n0\n40\n2.5\n"
       "0\nARC\n5\nF\n10\n1\n20\n3\n40\n2\n50\n30\n51\n270\n"
       "0\nPOINT\n10\n10\n20\n12\n"
       "0\nENDSEC\n0\nEOF\n",prefix);
    MusaDocument *doc=NULL;
    assert(read_string(valid,10,&doc)==MUSA_DXF_DOCUMENT_OK);
    assert(doc && musa_document_count(doc)==4u);
    MusaDocumentEntity e={0};
    assert(musa_document_get_index(doc,0,&e)==MUSA_DOCUMENT_OK);
    assert(e.kind==MUSA_DOCUMENT_LINE&&e.handle==11u);
    assert(e.x1==-1.25&&e.y1==100&&e.x2==4&&e.y2==2);
    assert(musa_document_get_index(doc,1,&e)==MUSA_DOCUMENT_OK);
    assert(e.handle==10u&&e.kind==MUSA_DOCUMENT_CIRCLE&&e.radius==2.5);
    assert(musa_document_get_index(doc,2,&e)==MUSA_DOCUMENT_OK);
    assert(e.handle==15u&&e.kind==MUSA_DOCUMENT_ARC&&e.start_deg==30&&e.end_deg==270);
    assert(musa_document_get_index(doc,3,&e)==MUSA_DOCUMENT_OK);
    assert(e.handle==16u&&e.kind==MUSA_DOCUMENT_POINT);
    assert(musa_document_translate(doc,11,-1,2)==MUSA_DOCUMENT_OK);
    assert(musa_document_get(doc,11,&e)==MUSA_DOCUMENT_OK);
    assert(e.x1==-2.25&&e.y1==102&&e.x2==3&&e.y2==4);
    assert(musa_document_delete(doc,10)==MUSA_DOCUMENT_OK);
    assert(musa_document_get(doc,10,&e)==MUSA_DOCUMENT_NOT_FOUND);
    uint64_t returned=0;
    MusaDocumentEntity fake={0};
    fake.kind=MUSA_DOCUMENT_POINT;fake.handle=10;
    assert(musa_document_load_entity(doc,&fake,&returned)==MUSA_DOCUMENT_INVALID);
    assert(musa_document_add_point(doc,0,0,&returned)==MUSA_DOCUMENT_OK);
    assert(returned==17u); /* imported handles A/B/F are never reused */
    musa_document_destroy(doc);

    const char *empty="0\nSECTION\n2\nENTITIES\n0\nENDSEC\n0\nEOF";
    assert(read_string(empty,2,&doc)==MUSA_DXF_DOCUMENT_OK);
    assert(doc && musa_document_count(doc)==0u);
    musa_document_destroy(doc);
    const char *nolayer="0\nSECTION\n2\nENTITIES\n0\nPOINT\n10\n0\n20\n0\n0\nENDSEC\n0\nEOF";
    assert(read_string(nolayer,2,&doc)==MUSA_DXF_DOCUMENT_OK);
    assert(musa_document_count(doc)==1);
    musa_document_destroy(doc);
    rejected("0\nSECTION\n2\nENTITIES\n0\nLINE\n10\n0\n20\n1\n11\n2\n0\nENDSEC\n0\nEOF",
             MUSA_DXF_DOCUMENT_MALFORMED);
    rejected("0\nSECTION\n2\nENTITIES\n0\nPOINT\n10\n0\n10\n1\n20\n0\n0\nENDSEC\n0\nEOF",
             MUSA_DXF_DOCUMENT_MALFORMED);
    rejected("0\nSECTION\n2\nENTITIES\n0\nPOINT\n5\n1\n10\n0\n20\n0\n0\nPOINT\n5\n0001\n10\n2\n20\n2\n0\nENDSEC\n0\nEOF",
             MUSA_DXF_DOCUMENT_MALFORMED);
    rejected("0\nSECTION\n2\nENTITIES\n0\nPOINT\n10\nNaN\n20\n0\n0\nENDSEC\n0\nEOF",
             MUSA_DXF_DOCUMENT_MALFORMED);
    rejected("0\nSECTION\n2\nENTITIES\n0\nPOINT\n10\n1e999\n20\n0\n0\nENDSEC\n0\nEOF",
             MUSA_DXF_DOCUMENT_MALFORMED);
    rejected("0\nSECTION\n2\nENTITIES\n0\nPOINT\n10\n0\n20\n0\n30\n1\n0\nENDSEC\n0\nEOF",
             MUSA_DXF_DOCUMENT_UNSUPPORTED);
    rejected("0\nSECTION\n2\nENTITIES\n0\nPOINT\n8\nPIPES\n10\n0\n20\n0\n0\nENDSEC\n0\nEOF",
             MUSA_DXF_DOCUMENT_UNSUPPORTED);
    rejected("0\nSECTION\n2\nENTITIES\n0\nSPLINE\n0\nENDSEC\n0\nEOF",
             MUSA_DXF_DOCUMENT_UNSUPPORTED);
    rejected("0\nSECTION\n2\nHEADER\n0\nENDSEC\n0\nEOF",
             MUSA_DXF_DOCUMENT_MALFORMED);
    rejected("0\nSECTION\n2\nENTITIES\n0\nPOINT\n10\n0\n20\n0\n0\nENDSEC",
             MUSA_DXF_DOCUMENT_MALFORMED);
    rejected("0\nSECTION\n2\nENTITIES\n0\nPOINT\n10\n0\n20\n0\n0\nENDSEC\n0\nEOF\n1\nignored",
             MUSA_DXF_DOCUMENT_MALFORMED);
    rejected("0\nSECTION\n2\nENTITIES\n0\nPOINT\n10\n0\n20\n0\n0\nENDSEC\n0\nEOF\n0\nEOF",
             MUSA_DXF_DOCUMENT_MALFORMED);
    rejected("0\nSECTION\n2\nENTITIES\n0\nPOINT\n10\n0\n20\n0\n0\nENDSEC\n0\nEOF",MUSA_DXF_DOCUMENT_LIMIT);
    /* Previous fixture contains one point; max limit cannot be zero. */
    size_t big_size=6000;
    unsigned char *big=(unsigned char *)malloc(big_size);
    assert(big);
    memset(big,'1',big_size);
    assert(musa_dxf_document_read(big,big_size,2,&doc)==MUSA_DXF_DOCUMENT_MALFORMED);
    free(big);
    puts("PASS: owned first-party DXF loader, handles, geometry, corruption and fail-closed");
    return 0;
}
