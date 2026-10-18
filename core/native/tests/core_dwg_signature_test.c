/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "musacad_dwg_signature.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static void recognized(const char *signature,MusaCoreDwgSignature version){
    assert(musa_core_dwg_identify_signature(
        (const unsigned char*)signature,strlen(signature))==version);
}
int main(void){
    recognized("AC1009",MUSA_DWG_R12);
    recognized("AC1012",MUSA_DWG_R13);
    recognized("AC1014",MUSA_DWG_R14);
    recognized("AC1015",MUSA_DWG_2000);
    recognized("AC1018",MUSA_DWG_2004);
    recognized("AC1021",MUSA_DWG_2007);
    recognized("AC1024",MUSA_DWG_2010);
    recognized("AC1027",MUSA_DWG_2013);
    recognized("AC1032",MUSA_DWG_2018_2026);
    recognized("AC1032EXTRA",MUSA_DWG_2018_2026);
    recognized("AC2026",MUSA_DWG_UNKNOWN); /* Invented magic not accepted. */
    recognized("AC1033",MUSA_DWG_UNKNOWN);
    const unsigned char garbage[6]={0,1,2,3,4,5};
    assert(musa_core_dwg_identify_signature(garbage,6)==MUSA_DWG_NON_ASCII);
    assert(musa_core_dwg_identify_signature(garbage,5)==MUSA_DWG_SHORT_HEADER);
    assert(musa_core_dwg_identify_signature(NULL,6)==MUSA_DWG_SHORT_HEADER);
    assert(musa_core_dwg_identify_signature(
        (const unsigned char*)"AC10",4)==MUSA_DWG_SHORT_HEADER);
    unsigned char nonprint[6]={'A','C','1','0','3',0xff};
    assert(musa_core_dwg_identify_signature(nonprint,6)==MUSA_DWG_NON_ASCII);
    /* Reference-format version matrix: a printable 6-byte DWG header can
     * be followed by BINARY payload. Header recognition MUST NOT inspect
     * later bytes, change the source, or imply an object decoder exists. */
    struct VersionHeader { const char *name; MusaCoreDwgSignature version; };
    static const struct VersionHeader formats[]={
        {"AC1009",MUSA_DWG_R12},{"AC1012",MUSA_DWG_R13},
        {"AC1014",MUSA_DWG_R14},{"AC1015",MUSA_DWG_2000},
        {"AC1018",MUSA_DWG_2004},{"AC1021",MUSA_DWG_2007},
        {"AC1024",MUSA_DWG_2010},{"AC1027",MUSA_DWG_2013},
        {"AC1032",MUSA_DWG_2018_2026}
    };
    for(size_t j=0;j<sizeof(formats)/sizeof(formats[0]);j++){
        unsigned char input[37],original[37];
        memcpy(input,formats[j].name,6);
        for(size_t k=6;k<sizeof(input);k++)
            input[k]=(unsigned char)(k*29u+j*7u);
        memcpy(original,input,sizeof(input));
        /* The scanner must never reject genuine binary payload bytes
         * just because data AFTER the six-byte format header isn't ASCII. */
        assert(musa_core_dwg_identify_signature(input,sizeof(input))==
               formats[j].version);
        assert(memcmp(input,original,sizeof(input))==0);
        for(size_t short_length=0;short_length<6;short_length++)
            assert(musa_core_dwg_identify_signature(input,short_length)==
                   MUSA_DWG_SHORT_HEADER);
        input[5]=0xffu; /* The actual signature, unlike payload, is ASCII. */
        assert(musa_core_dwg_identify_signature(input,sizeof(input))==
               MUSA_DWG_NON_ASCII);
    }
    puts("PASS: shared CAD DWG signatures and AC1032 vs AutoCAD 2026 distinction");
    return 0;
}
