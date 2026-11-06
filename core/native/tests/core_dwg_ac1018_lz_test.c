/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "musacad_dwg_ac1018_lz.h"
#include <assert.h>
#include <stdint.h>
#include <string.h>
/* All streams are independently synthesized. No user DWG bytes stored. */
static void valid(void){
    const uint8_t literals[]={1,'A','B','C','D',0x11,0,0};
    char out[50]={0};
    size_t read=222u;
    assert(musa_dwg_ac1018_lz_decode(literals,sizeof(literals),
                                      (uint8_t*)out,4u,&read));
    assert(memcmp(out,"ABCD",4u)==0 && read==6u);
    const uint8_t overlap[]={1,'A','B','C','D',0x40,0,0x11};
    read=222u;
    assert(musa_dwg_ac1018_lz_decode(overlap,sizeof(overlap),
                                      (uint8_t*)out,7u,&read));
    assert(memcmp(out,"ABCDDDD",7u)==0 && read==8u);
    const uint8_t post[]={1,'A','B','C','D',0x41,0,'Z',0x11};
    assert(musa_dwg_ac1018_lz_decode(post,sizeof(post),
                                      (uint8_t*)out,8u,&read));
    assert(memcmp(out,"ABCDDDDZ",8u)==0);
    const uint8_t long_copy[]={1,'A','B','C','D',0x20,1,0,0,0x11};
    assert(musa_dwg_ac1018_lz_decode(long_copy,sizeof(long_copy),
                                      (uint8_t*)out,38u,&read));
    assert(memcmp(out,"ABCD",4u)==0);
    for(int i=4;i<38;i++)assert(out[i]=='D');
    uint8_t long_literal[22]={0,1};
    memset(long_literal+2u,'Q',19u);long_literal[21]=0x11;
    assert(musa_dwg_ac1018_lz_decode(long_literal,sizeof(long_literal),
                                      (uint8_t*)out,19u,&read));
    for(int i=0;i<19;i++)assert(out[i]=='Q');
}
static void corrupt(void){
    const uint8_t copy[]={1,'A','B','C','D',0x40,0,0x11};
    char out[7]={'U','N','T','O','U','C','H'};
    const char original[7]={'U','N','T','O','U','C','H'};
    size_t consumed=777u;
    for(size_t n=0u;n<sizeof(copy);n++){
        assert(!musa_dwg_ac1018_lz_decode(copy,n,(uint8_t*)out,7u,&consumed));
        assert(memcmp(out,original,7u)==0 && consumed==777u);
    }
    const uint8_t invalid_back[]={1,'A','B','C','D',0x40,0x7fu,0x11};
    assert(!musa_dwg_ac1018_lz_decode(invalid_back,sizeof(invalid_back),
                                       (uint8_t*)out,7u,&consumed));
    assert(!musa_dwg_ac1018_lz_decode(copy,sizeof(copy),
                                       (uint8_t*)out,6u,&consumed));
    assert(!musa_dwg_ac1018_lz_decode(copy,sizeof(copy),
                                       (uint8_t*)out,0u,&consumed));
    assert(!musa_dwg_ac1018_lz_decode(copy,sizeof(copy),
                                       (uint8_t*)out,1024u*1024u+1u,&consumed));
    assert(memcmp(out,original,7u)==0 && consumed==777u);
}

/* Diagnostic phases never invent output, and must agree with the strict
 * production decoder on successful streams. All bytes are synthetic. */
static void trace(void){
    const uint8_t good[]={1,'A','B','C','D',0x11,0,0};
    MusaDwgAc1018LzDiagnostic diag={0};
    assert(musa_dwg_ac1018_lz_diagnose(
        good,sizeof(good),4u,&diag));
    assert(diag.completed==1u&&
           diag.failure_phase==MUSA_DWG_AC1018_LZ_PHASE_NONE&&
           diag.input_consumed==6u&&diag.output_produced==4u);
    const uint8_t bad_header[]={0u};
    assert(!musa_dwg_ac1018_lz_diagnose(
        bad_header,sizeof(bad_header),4u,&diag));
    assert(diag.completed==0u&&
           diag.failure_phase==MUSA_DWG_AC1018_LZ_PHASE_LITERAL_HEADER);
    const uint8_t missing_opcode[]={1,'A','B','C','D'};
    assert(!musa_dwg_ac1018_lz_diagnose(
        missing_opcode,sizeof(missing_opcode),4u,&diag));
    assert(diag.completed==0u&&diag.output_produced==4u&&
           diag.failure_phase==MUSA_DWG_AC1018_LZ_PHASE_OPCODE);
    const uint8_t wrong_length[]={1,'A','B','C','D',0x11};
    assert(!musa_dwg_ac1018_lz_diagnose(
        wrong_length,sizeof(wrong_length),5u,&diag));
    assert(diag.completed==0u&&diag.output_produced==4u&&
           diag.failure_phase==MUSA_DWG_AC1018_LZ_PHASE_TERMINATOR);
    const uint8_t bad_back[]={1,'A','B','C','D',0x40,0x7f,0x11};
    assert(!musa_dwg_ac1018_lz_diagnose(
        bad_back,sizeof(bad_back),7u,&diag));
    assert(diag.completed==0u&&diag.output_produced==4u&&
           diag.failure_phase==MUSA_DWG_AC1018_LZ_PHASE_BACKREFERENCE);
    const uint8_t overrun[]={1,'A','B','C','D',0x40,0x00,0x11};
    assert(!musa_dwg_ac1018_lz_diagnose(
        overrun,sizeof(overrun),6u,&diag));
    assert(diag.completed==0u&&diag.output_produced==4u&&
           diag.failure_phase==MUSA_DWG_AC1018_LZ_PHASE_OUTPUT_OVERRUN&&
           diag.overflow_match_bytes==3u&&
           diag.overflow_available_bytes==2u);
    /* A larger budget may satisfy the exact synthetic LZ stream, but the
     * real source's authenticated advertised output size is authoritative:
     * production must reject the 6-byte budget without partial output. */
    MusaDwgAc1018LzDiagnostic alt={0};
    assert(musa_dwg_ac1018_lz_diagnose(
        overrun,sizeof(overrun),7u,&alt));
    assert(alt.completed==1u&&
           alt.failure_phase==MUSA_DWG_AC1018_LZ_PHASE_NONE&&
           alt.input_consumed==sizeof(overrun)&&
           alt.output_produced==7u);
    uint8_t refused[6]={'S','A','F','E','!','!'};
    const uint8_t saved[6]={'S','A','F','E','!','!'};
    size_t refused_used=999u;
    assert(!musa_dwg_ac1018_lz_decode(
        overrun,sizeof(overrun),refused,6u,&refused_used));
    assert(memcmp(refused,saved,sizeof refused)==0 &&
           refused_used==999u);
    /* Never write diagnostic fields into original source bytes. */
    uint8_t alias[32]={1,'A','B','C','D',0x11};
    uint8_t before[32];memcpy(before,alias,sizeof alias);
    assert(!musa_dwg_ac1018_lz_diagnose(
        alias,16u,4u,(MusaDwgAc1018LzDiagnostic*)alias));
    assert(memcmp(before,alias,sizeof alias)==0);
}
int main(void){valid();corrupt();trace();return 0;}
