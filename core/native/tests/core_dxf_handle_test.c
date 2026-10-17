/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "musacad_dxf_handle.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

static void valid(const char *text,const char *canonical,uint64_t expected){
    uint64_t value=0;
    char formatted[17]={0};
    assert(musa_core_dxf_handle_parse(text,strlen(text),&value));
    assert(value==expected);
    assert(musa_core_dxf_handle_format(value,formatted));
    assert(strcmp(formatted,canonical)==0);
}
static void invalid(const char *text){
    uint64_t unchanged=777;
    assert(!musa_core_dxf_handle_parse(text,strlen(text),&unchanged));
    assert(unchanged==777);
}
int main(void){
    valid("000a1","A1",0xa1);
    valid("ff","FF",0xff);
    valid("FfFfFfFfFfFfFfFf","FFFFFFFFFFFFFFFF",UINT64_MAX);
    valid("8000000000000000","8000000000000000",UINT64_C(0x8000000000000000));
    invalid("");invalid("0");invalid("000000");invalid("-1");
    invalid("GG");invalid("ABCDEF12345678901");invalid("0xFF");
    invalid("A B");invalid("F\n");
    const char nonascii[]={(char)0xc3,(char)0x85,0};
    invalid(nonascii);
    uint64_t first=0,seed=0;
    assert(musa_core_dxf_handle_reserve(0x3ff,2,&first,&seed));
    assert(first==0x400&&seed==0x402);
    assert(musa_core_dxf_handle_reserve(UINT64_C(0xfffffffffffffffc),2,&first,&seed));
    assert(first==UINT64_C(0xfffffffffffffffd)&&seed==UINT64_C(0xffffffffffffffff));
    assert(!musa_core_dxf_handle_reserve(UINT64_MAX-1u,1,&first,&seed));
    assert(!musa_core_dxf_handle_reserve(0xffu,0,&first,&seed));
    assert(!musa_core_dxf_handle_reserve(0xffu,100001,&first,&seed));
    assert(!musa_core_dxf_handle_reserve(0xfeu,1,&first,&seed));
    puts("PASS: portable DXF unsigned 64-bit handle identity and collision-safe allocation");
    return 0;
}
