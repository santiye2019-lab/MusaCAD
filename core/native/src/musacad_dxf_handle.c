/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "musacad_dxf_handle.h"
#include <stdint.h>

int musa_core_dxf_handle_parse(const char *hex,size_t length,uint64_t *out){
    if(!hex||!out||length==0||length>16)return 0;
    uint64_t n=0;
    for(size_t i=0;i<length;i++){
        const unsigned char ch=(unsigned char)hex[i];
        const int digit=(ch>='0'&&ch<='9')?(int)(ch-'0'):
            (ch>='A'&&ch<='F')?(int)(ch-'A')+10:
            (ch>='a'&&ch<='f')?(int)(ch-'a')+10:-1;
        if(digit<0)return 0;
        n=(n<<4)|(uint64_t)digit;
    }
    if(n==0)return 0; /* DXF entity handle zero cannot identify an entity */
    *out=n;
    return 1;
}
int musa_core_dxf_handle_format(uint64_t value,char out[17]){
    if(!out||value==0)return 0;
    static const char digits[]="0123456789ABCDEF";
    char tmp[16];
    size_t n=0;
    do{
        tmp[n++]=digits[value&15u];
        value>>=4;
    }while(value&&n<16);
    for(size_t i=0;i<n;i++)out[i]=tmp[n-1-i];
    out[n]='\0';
    return 1;
}
int musa_core_dxf_handle_reserve(uint64_t maximum,size_t count,
                                 uint64_t *first,uint64_t *next_seed){
    if(!first||!next_seed||count==0||count>100000u||
       maximum<0xffu||maximum>UINT64_MAX-(uint64_t)count-1u)
        return 0;
    *first=maximum+1u;
    *next_seed=maximum+1u+(uint64_t)count;
    return 1;
}
