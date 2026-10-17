/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "musacad_dxf_section.h"
#include <string.h>

static void strip_ascii(const char **bytes,size_t *n){
    while(*n && (unsigned char)**bytes<=0x20u){++(*bytes);--(*n);}
    while(*n && (unsigned char)(*bytes)[*n-1]<=0x20u)--(*n);
}
static int exact(const char *v,size_t n,const char *literal){
    size_t wanted=strlen(literal);
    return n==wanted && memcmp(v,literal,n)==0;
}
static int equal_ascii_fold(const char *v,size_t n,const char *literal){
    const size_t k=strlen(literal);
    if(n!=k)return 0;
    for(size_t i=0;i<k;i++){
        unsigned char ch=(unsigned char)v[i];
        if(ch>='a'&&ch<='z')ch=(unsigned char)(ch-('a'-'A'));
        if(ch!=(unsigned char)literal[i])return 0;
    }
    return 1;
}
static int section_kind(const char *value,size_t n){
    if(equal_ascii_fold(value,n,"HEADER"))return 1;
    if(equal_ascii_fold(value,n,"ENTITIES"))return 2;
    if(equal_ascii_fold(value,n,"BLOCKS"))return 3;
    if(equal_ascii_fold(value,n,"TABLES"))return 4;
    if(equal_ascii_fold(value,n,"OBJECTS"))return 5;
    return 6;
}
int musa_core_dxf_section_step(MusaDxfSectionState *state,int code,
                               const char *value,size_t value_len,int *flags){
    if(!state||!value||!flags||code<0||code>1071)
        return MUSA_DXF_SECTION_BAD_ARGUMENT;
    const char *text=value;
    size_t n=value_len;
    strip_ascii(&text,&n);
    MusaDxfSectionState next=*state;
    int event=0;
    if(next.seen_eof)return MUSA_DXF_SECTION_AFTER_EOF;
    if(next.awaiting_name){
        if(code!=2||!n)return MUSA_DXF_SECTION_MISSING_NAME;
        next.current_kind=section_kind(text,n);
        if(next.current_kind==2){
            if(next.seen_entities)return MUSA_DXF_SECTION_DUP_ENTITIES;
            next.seen_entities=1;
        }
        next.awaiting_name=0;
    }else if(code==0){
        event|=MUSA_DXF_SECTION_RECORD_BOUNDARY;
        if(exact(text,n,"SECTION")){
            if(next.current_kind)return MUSA_DXF_SECTION_NESTED;
            next.awaiting_name=1;
        }else if(exact(text,n,"ENDSEC")){
            if(!next.current_kind)return MUSA_DXF_SECTION_ENDSEC_MISMATCH;
            next.current_kind=0;
        }else if(exact(text,n,"EOF")){
            if(next.current_kind||!next.seen_entities)
                return MUSA_DXF_SECTION_EOF_MISMATCH;
            next.seen_eof=1;
        }else{
            if(!next.current_kind)return MUSA_DXF_SECTION_OUTSIDE;
            if(next.current_kind==2 && exact(text,n,"LINE"))
                event|=MUSA_DXF_SECTION_LINE_STARTED;
        }
    }else{
        if(!next.current_kind)return MUSA_DXF_SECTION_OUTSIDE;
        if(next.current_kind>=2 && next.current_kind<=5)
            event|=MUSA_DXF_SECTION_HANDLE_AREA;
    }
    *state=next;
    *flags=event;
    return MUSA_DXF_SECTION_OK;
}
int musa_core_dxf_section_finish(const MusaDxfSectionState *state){
    if(!state)return MUSA_DXF_SECTION_BAD_ARGUMENT;
    return (!state->awaiting_name && !state->current_kind &&
            state->seen_entities && state->seen_eof)
        ?MUSA_DXF_SECTION_OK:MUSA_DXF_SECTION_INCOMPLETE;
}
