/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "musacad_dwg_signature.h"
#include <string.h>

MusaCoreDwgSignature musa_core_dwg_identify_signature(
    const unsigned char *bytes,size_t length) {
    if(bytes==NULL||length<6)return MUSA_DWG_SHORT_HEADER;
    for(size_t i=0;i<6;i++)
        if(bytes[i]<0x20u||bytes[i]>0x7eu)return MUSA_DWG_NON_ASCII;
    if(memcmp(bytes,"AC1009",6)==0)return MUSA_DWG_R12;
    if(memcmp(bytes,"AC1012",6)==0)return MUSA_DWG_R13;
    if(memcmp(bytes,"AC1014",6)==0)return MUSA_DWG_R14;
    if(memcmp(bytes,"AC1015",6)==0)return MUSA_DWG_2000;
    if(memcmp(bytes,"AC1018",6)==0)return MUSA_DWG_2004;
    if(memcmp(bytes,"AC1021",6)==0)return MUSA_DWG_2007;
    if(memcmp(bytes,"AC1024",6)==0)return MUSA_DWG_2010;
    if(memcmp(bytes,"AC1027",6)==0)return MUSA_DWG_2013;
    if(memcmp(bytes,"AC1032",6)==0)return MUSA_DWG_2018_2026;
    return MUSA_DWG_UNKNOWN;
}
