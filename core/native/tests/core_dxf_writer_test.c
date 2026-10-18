/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "musacad_dxf_writer.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void same(const MusaDocumentEntity *a,const MusaDocumentEntity *b){
    assert(a->handle==b->handle);
    assert(a->kind==b->kind);
    assert(a->x1==b->x1&&a->y1==b->y1);
    assert(a->x2==b->x2&&a->y2==b->y2);
    assert(a->radius==b->radius);
    assert(a->start_deg==b->start_deg&&a->end_deg==b->end_deg);
}
int main(void){
    MusaDocument *source=musa_document_create(8);
    assert(source);
    uint64_t line=0,circle=0,arc=0,point=0;
    assert(musa_document_add_line(source,-2.25,5.5,15.75,4.125,&line)
           ==MUSA_DOCUMENT_OK);
    assert(musa_document_add_circle(source,3,-4,0.125,&circle)
           ==MUSA_DOCUMENT_OK);
    assert(musa_document_add_arc(source,1,2,6,45,270,&arc)
           ==MUSA_DOCUMENT_OK);
    assert(musa_document_add_point(source,123.5,789.25,&point)
           ==MUSA_DOCUMENT_OK);
    assert(line==1&&circle==2&&arc==3&&point==4);
    size_t needed=0;
    assert(musa_dxf_document_write(source,NULL,0,&needed)==MUSA_DXF_DOCUMENT_OK);
    assert(needed>100&&needed<8192);
    char *out=(char *)malloc(needed+1u);
    assert(out);
    memset(out,'Q',needed+1u);
    size_t reported=0;
    assert(musa_dxf_document_write(source,out,needed,&reported)
           ==MUSA_DXF_DOCUMENT_LIMIT);
    assert(reported==needed);
    for(size_t i=0;i<=needed;i++)assert(out[i]=='Q'); /* atomic no partial output */
    assert(musa_dxf_document_write(source,out,needed+1u,&reported)
           ==MUSA_DXF_DOCUMENT_OK);
    assert(reported==needed&&out[needed]=='\0');
    assert(strstr(out,"0\nSECTION\n2\nENTITIES\n")==out);
    assert(strstr(out,"0\nENDSEC\n0\nEOF\n")!=NULL);
    MusaDocument *again=NULL;
    assert(musa_dxf_document_read((const unsigned char *)out,reported,
        8,&again)==MUSA_DXF_DOCUMENT_OK);
    assert(again);
    assert(musa_document_count(again)==4);
    for(size_t i=0;i<4;i++){
        MusaDocumentEntity a={0},b={0};
        assert(musa_document_get_index(source,i,&a)==MUSA_DOCUMENT_OK);
        assert(musa_document_get_index(again,i,&b)==MUSA_DOCUMENT_OK);
        same(&a,&b);
    }
    /* Editing the owned model and writing a second time does not change IDs. */
    assert(musa_document_translate(again,line,0.5,-2)==MUSA_DOCUMENT_OK);
    assert(musa_document_delete(again,circle)==MUSA_DOCUMENT_OK);
    size_t edited_len=0;
    assert(musa_dxf_document_write(again,NULL,0,&edited_len)==MUSA_DXF_DOCUMENT_OK);
    assert(edited_len>30&&edited_len<needed);
    char *edited=(char *)malloc(edited_len+1);
    assert(edited);
    assert(musa_dxf_document_write(again,edited,edited_len+1,&reported)
           ==MUSA_DXF_DOCUMENT_OK);
    MusaDocument *reopened=NULL;
    assert(musa_dxf_document_read((const unsigned char *)edited,reported,
        8,&reopened)==MUSA_DXF_DOCUMENT_OK);
    assert(musa_document_count(reopened)==3);
    MusaDocumentEntity actual={0};
    assert(musa_document_get(reopened,line,&actual)==MUSA_DOCUMENT_OK);
    assert(actual.x1==-1.75&&actual.y1==3.5);
    assert(musa_document_get(reopened,circle,&actual)==MUSA_DOCUMENT_NOT_FOUND);
    assert(musa_document_get(reopened,arc,&actual)==MUSA_DOCUMENT_OK);
    assert(musa_document_get(reopened,point,&actual)==MUSA_DOCUMENT_OK);
    musa_document_destroy(reopened);
    musa_document_destroy(again);
    musa_document_destroy(source);
    free(edited);free(out);

    /* Reject a document that cannot fit the strict supported DXF subset. */
    MusaDocument *large=musa_document_create(4001);
    assert(large);
    for(int i=0;i<4001;i++){
        uint64_t h=0;
        assert(musa_document_add_point(large,i,i,&h)==MUSA_DOCUMENT_OK);
    }
    needed=2;
    assert(musa_dxf_document_write(large,NULL,0,&needed)==MUSA_DXF_DOCUMENT_LIMIT);
    assert(needed==0u);
    musa_document_destroy(large);
    puts("PASS: independent DXF write, read, edit, safe capacity rejection and reopen");
    return 0;
}
