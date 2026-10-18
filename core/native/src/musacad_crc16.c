/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "musacad_crc16.h"
int musa_core_crc16_arc(uint16_t seed,const uint8_t *bytes,size_t count,
                        uint16_t *out){
    if(!out||(!bytes&&count))return 0;
    uint16_t crc=seed;
    for(size_t i=0u;i<count;i++){
        crc=(uint16_t)(crc^(uint16_t)bytes[i]);
        for(unsigned k=0u;k<8u;k++)
            crc=(uint16_t)((crc>>1)^((crc&1u)?0xa001u:0u));
    }
    *out=crc;return 1;
}
