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
    puts("PASS: shared CAD DWG signatures and AC1032 vs AutoCAD 2026 distinction");
    return 0;
}
