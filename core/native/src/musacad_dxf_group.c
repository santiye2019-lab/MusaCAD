/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "musacad_dxf_group.h"

int musa_core_dxf_group_parse(const char *text,size_t length,int *out){
    if(!text||!out||length==0||length>1048576u)return 0;
    size_t first=0,last=length;
    /* Match the explicit ASCII trim policy, not locale or Unicode digits. */
    while(first<last&&(unsigned char)text[first]<=0x20u)first++;
    while(last>first&&(unsigned char)text[last-1]<=0x20u)last--;
    if(first==last)return 0;
    if(text[first]=='+')first++;
    if(first==last)return 0;
    int value=0;
    for(size_t i=first;i<last;i++){
        const unsigned char digit=(unsigned char)text[i];
        if(digit<'0'||digit>'9')return 0;
        value=value*10+(int)(digit-'0');
        /* Check on EVERY digit so long invalid numeric fields cannot overflow. */
        if(value>1071)return 0;
    }
    *out=value;
    return 1;
}
