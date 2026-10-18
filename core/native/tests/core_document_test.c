/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "musacad_document.h"
#include <assert.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>

int main(void){
    assert(musa_document_abi_version()==1u);
    assert(musa_document_create(0)==NULL);
    assert(musa_document_create(1000001u)==NULL);
    MusaDocument *doc=musa_document_create(4);
    assert(doc);
    uint64_t a=0,b=0,c=0,d=0,unchanged=777u;
    assert(musa_document_add_line(doc,2,3,5,7,&a)==MUSA_DOCUMENT_OK && a==1u);
    assert(musa_document_add_circle(doc,10,-2,3,&b)==MUSA_DOCUMENT_OK && b==2u);
    assert(musa_document_add_arc(doc,0,0,8,45,135,&c)==MUSA_DOCUMENT_OK && c==3u);
    assert(musa_document_add_point(doc,99,1,&d)==MUSA_DOCUMENT_OK && d==4u);
    assert(musa_document_count(doc)==4u);
    assert(musa_document_add_circle(doc,0,0,1,&unchanged)==MUSA_DOCUMENT_LIMIT);
    assert(unchanged==777u);
    MusaDocumentEntity v={0};
    assert(musa_document_get(doc,a,&v)==MUSA_DOCUMENT_OK);
    assert(v.handle==1u&&v.kind==MUSA_DOCUMENT_LINE&&v.x2==5&&v.y2==7);
    assert(musa_document_translate(doc,a,-2,4)==MUSA_DOCUMENT_OK);
    assert(musa_document_get(doc,a,&v)==MUSA_DOCUMENT_OK);
    assert(v.x1==0&&v.y1==7&&v.x2==3&&v.y2==11);
    assert(musa_document_translate(doc,b,1,1)==MUSA_DOCUMENT_OK);
    assert(musa_document_get(doc,b,&v)==MUSA_DOCUMENT_OK);
    assert(v.x1==11&&v.y1==-1&&v.radius==3);
    assert(musa_document_delete(doc,b)==MUSA_DOCUMENT_OK);
    assert(musa_document_count(doc)==3u);
    assert(musa_document_get(doc,b,&v)==MUSA_DOCUMENT_NOT_FOUND);
    assert(musa_document_get_index(doc,1,&v)==MUSA_DOCUMENT_OK&&v.handle==c);
    assert(musa_document_get_index(doc,99,&v)==MUSA_DOCUMENT_NOT_FOUND);
    assert(musa_document_add_line(doc,8,9,10,11,&unchanged)==MUSA_DOCUMENT_OK);
    assert(unchanged==5u); /* no handle reuse after deletion */
    assert(musa_document_count(doc)==4u);
    assert(musa_document_translate(doc,a,NAN,0)==MUSA_DOCUMENT_INVALID);
    assert(musa_document_add_circle(doc,0,0,-1,&unchanged)==MUSA_DOCUMENT_INVALID);
    assert(musa_document_add_line(doc,INFINITY,0,0,0,&unchanged)==MUSA_DOCUMENT_INVALID);
    assert(musa_document_add_arc(doc,0,0,2,0,NAN,&unchanged)==MUSA_DOCUMENT_INVALID);
    /* Overflow must not commit even one of the two endpoints. */
    MusaDocument *extreme=musa_document_create(2);
    assert(extreme);
    uint64_t huge=0;
    assert(musa_document_add_line(extreme,1.0e308,1,0,1,&huge)==MUSA_DOCUMENT_OK);
    assert(musa_document_translate(extreme,huge,1.0e308,2)==MUSA_DOCUMENT_OVERFLOW);
    assert(musa_document_get(extreme,huge,&v)==MUSA_DOCUMENT_OK);
    assert(v.x1==1.0e308&&v.y1==1&&v.x2==0&&v.y2==1);
    musa_document_destroy(extreme);
    musa_document_destroy(doc);
    assert(musa_document_count(NULL)==0u);
    puts("PASS: first-party MusaCAD document ABI, ownership, identity and atomic edits");
    return 0;
}
