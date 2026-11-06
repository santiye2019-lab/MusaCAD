/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "musacad_dwg_ac1018_lz.h"
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
typedef struct LZCursor {
    const uint8_t *src;
    size_t len,at,limit,used;
    uint8_t *dst;
} LZCursor;
static int next_byte(LZCursor *c,uint8_t *out){
    if(c->at>=c->len)return 0;
    *out=c->src[c->at++];return 1;
}
static int literal_header(LZCursor *c,size_t *length,int *has_opcode,
                          uint8_t *opcode){
    uint8_t b=0u;
    if(!next_byte(c,&b))return 0;
    if(b>=1u&&b<=15u){*length=(size_t)b+3u;*has_opcode=0;return 1;}
    if(b==0u){
        size_t n=15u;
        for(;;){
            if(!next_byte(c,&b))return 0;
            if(b){
                if(c->limit<3u||n>c->limit||(size_t)b>c->limit-n||
                   n+(size_t)b>c->limit-3u)return 0;
                *length=n+(size_t)b+3u;*has_opcode=0;return 1;
            }
            if(n>c->limit||255u>c->limit-n)return 0;
            n+=255u;
        }
    }
    *length=0u;*has_opcode=1;*opcode=b;return 1;
}
static int long_match(LZCursor *c,size_t *length){
    uint8_t b=0u;
    if(!next_byte(c,&b))return 0;
    if(b){*length=(size_t)b;return 1;}
    size_t n=255u;
    for(;;){
        if(!next_byte(c,&b))return 0;
        if(b){
            if(n>c->limit||(size_t)b>c->limit-n)return 0;
            *length=n+(size_t)b;return 1;
        }
        if(n>c->limit||255u>c->limit-n)return 0;
        n+=255u;
    }
}
static int long_offset(LZCursor *c,size_t *back,size_t *post){
    uint8_t a=0u,b=0u;
    if(!next_byte(c,&a)||!next_byte(c,&b))return 0;
    *back=((size_t)a>>2u)|((size_t)b<<6u);
    *post=(size_t)(a&3u);
    return 1;
}
static int copy_literals(LZCursor *c,size_t n){
    if(n>c->limit-c->used||n>c->len-c->at)return 0;
    memcpy(c->dst+c->used,c->src+c->at,n);
    c->at+=n;c->used+=n;return 1;
}
/* Decode exactly one bounded stream; shared by production and diagnostics.
 * All decoded bytes remain in scratch until a fully correct 0x11 terminal. */
static int decode_lz_internal(const uint8_t *source,size_t compressed_bytes,
    uint8_t *output,size_t decompressed_bytes,size_t *consumed_bytes,
    MusaDwgAc1018LzDiagnostic *diagnostic){
    if(!source||!consumed_bytes||!compressed_bytes||!decompressed_bytes||
       decompressed_bytes>1024u*1024u||(!output&&!diagnostic))return 0;
    if(diagnostic){
        const uintptr_t a=(uintptr_t)source,b=(uintptr_t)diagnostic;
        if((a<=b&&b-a<compressed_bytes)||
           (a>b&&a-b<sizeof(*diagnostic)))return 0;
        *diagnostic=(MusaDwgAc1018LzDiagnostic){
            0u,0u,MUSA_DWG_AC1018_LZ_PHASE_ALLOC,0u};
    }
    uint8_t *scratch=(uint8_t*)malloc(decompressed_bytes);
    if(!scratch)return 0;
    LZCursor c={source,compressed_bytes,0u,decompressed_bytes,0u,scratch};
    MusaDwgAc1018LzFailurePhase phase=
        MUSA_DWG_AC1018_LZ_PHASE_LITERAL_HEADER;
    size_t literal=0u;
    int has_opcode=0,ok=0;
    uint8_t opcode=0u;
    if(!literal_header(&c,&literal,&has_opcode,&opcode))
        goto cleanup;
    phase=MUSA_DWG_AC1018_LZ_PHASE_LITERAL_COPY;
    if(!copy_literals(&c,literal))goto cleanup;
    const size_t max_steps=decompressed_bytes*2u+64u;
    for(size_t step=0u;step<max_steps;step++){
        phase=MUSA_DWG_AC1018_LZ_PHASE_OPCODE;
        if(!has_opcode&&!next_byte(&c,&opcode))goto cleanup;
        has_opcode=0;
        if(opcode==0x11u){
            phase=MUSA_DWG_AC1018_LZ_PHASE_TERMINATOR;
            if(c.used!=decompressed_bytes)goto cleanup;
            if(output)memcpy(output,scratch,decompressed_bytes);
            *consumed_bytes=c.at;
            phase=MUSA_DWG_AC1018_LZ_PHASE_NONE;
            ok=1;goto cleanup;
        }
        size_t count=0u,back=0u,post=0u;
        phase=MUSA_DWG_AC1018_LZ_PHASE_MATCH_LENGTH;
        if(opcode==0x10u){
            if(!long_match(&c,&count)||c.limit<9u||
               count>c.limit-9u)goto cleanup;
            count+=9u;
            phase=MUSA_DWG_AC1018_LZ_PHASE_MATCH_OFFSET;
            if(!long_offset(&c,&back,&post))goto cleanup;
            back+=0x3fffu;
        }else if(opcode>=0x12u&&opcode<=0x1fu){
            count=(size_t)(opcode&15u)+2u;
            phase=MUSA_DWG_AC1018_LZ_PHASE_MATCH_OFFSET;
            if(!long_offset(&c,&back,&post))goto cleanup;
            back+=0x3fffu;
        }else if(opcode==0x20u){
            if(!long_match(&c,&count)||c.limit<0x21u||
               count>c.limit-0x21u)goto cleanup;
            count+=0x21u;
            phase=MUSA_DWG_AC1018_LZ_PHASE_MATCH_OFFSET;
            if(!long_offset(&c,&back,&post))goto cleanup;
        }else if(opcode>=0x21u&&opcode<=0x3fu){
            count=(size_t)opcode-0x1eu;
            phase=MUSA_DWG_AC1018_LZ_PHASE_MATCH_OFFSET;
            if(!long_offset(&c,&back,&post))goto cleanup;
        }else if(opcode>=0x40u){
            uint8_t b=0u;
            phase=MUSA_DWG_AC1018_LZ_PHASE_MATCH_OFFSET;
            if(!next_byte(&c,&b))goto cleanup;
            count=((size_t)(opcode&0xf0u)>>4u)-1u;
            back=((size_t)b<<2u)|((size_t)(opcode&0x0cu)>>2u);
            post=(size_t)(opcode&3u);
        }else{
            phase=MUSA_DWG_AC1018_LZ_PHASE_OPCODE;
            goto cleanup;
        }
        phase=MUSA_DWG_AC1018_LZ_PHASE_BACKREFERENCE;
        if(back>=SIZE_MAX||back+1u>c.used||count>c.limit-c.used)
            goto cleanup;
        ++back;
        for(size_t i=0u;i<count;i++){
            c.dst[c.used]=c.dst[c.used-back];++c.used;
        }
        if(post==0u){
            phase=MUSA_DWG_AC1018_LZ_PHASE_LITERAL_HEADER;
            if(!literal_header(&c,&literal,&has_opcode,&opcode))
                goto cleanup;
        }else literal=post;
        phase=MUSA_DWG_AC1018_LZ_PHASE_LITERAL_COPY;
        if(!copy_literals(&c,literal))goto cleanup;
    }
    phase=MUSA_DWG_AC1018_LZ_PHASE_STEP_LIMIT;
cleanup:
    if(diagnostic){
        diagnostic->input_consumed=c.at;
        diagnostic->output_produced=c.used;
        diagnostic->failure_phase=phase;
        diagnostic->completed=(uint8_t)ok;
    }
    free(scratch);
    return ok;
}
int musa_dwg_ac1018_lz_decode(const uint8_t *source,size_t compressed_bytes,
                              uint8_t *output,size_t decompressed_bytes,
                              size_t *consumed_bytes){
    if(!output)return 0;
    return decode_lz_internal(source,compressed_bytes,output,
                              decompressed_bytes,consumed_bytes,NULL);
}
int musa_dwg_ac1018_lz_diagnose(const uint8_t *source,
    size_t compressed_bytes,size_t decompressed_bytes,
    MusaDwgAc1018LzDiagnostic *out){
    if(!out)return 0;
    size_t unused=0u;
    return decode_lz_internal(source,compressed_bytes,NULL,
                              decompressed_bytes,&unused,out);
}
