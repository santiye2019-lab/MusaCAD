/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "musacad_dxf_pair.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static void check(const char *s,int has_value,size_t prior,size_t max,
                  int wanted_status,int wanted_code){
    int out=9000;
    const int status=musa_core_dxf_pair_check(s,s?strlen(s):0,
                                              has_value,prior,max,&out);
    assert(status==wanted_status);
    if(status==MUSA_DXF_PAIR_OK)assert(out==wanted_code);
    else assert(out==9000); /* never publish a partial parse on failure */
}
int main(void){
    check("0",1,0,32000000,MUSA_DXF_PAIR_OK,0);
    check("  1071  ",1,31999999,32000000,MUSA_DXF_PAIR_OK,1071);
    check("5",1,2,5,MUSA_DXF_PAIR_OK,5); /* empty string value is permitted */
    check("5",0,2,5,MUSA_DXF_PAIR_MISSING_VALUE,0);
    check("-1",1,2,5,MUSA_DXF_PAIR_INVALID_CODE,0);
    check("1072",1,2,5,MUSA_DXF_PAIR_INVALID_CODE,0);
    check("999999999999999999",1,2,5,MUSA_DXF_PAIR_INVALID_CODE,0);
    check("\xD9\xA1\xD9\xA0",1,2,5,MUSA_DXF_PAIR_INVALID_CODE,0);
    check("5",1,5,5,MUSA_DXF_PAIR_EXCESS_LIMIT,0);
    check("5",1,6,5,MUSA_DXF_PAIR_EXCESS_LIMIT,0);
    check("5",1,0,0,MUSA_DXF_PAIR_INVALID_ARGUMENT,0);
    check(NULL,1,0,5,MUSA_DXF_PAIR_INVALID_ARGUMENT,0);
    assert(musa_core_dxf_pair_check("5",1,1,0,5,NULL)==MUSA_DXF_PAIR_INVALID_ARGUMENT);
    puts("PASS: one portable DXF pair-framing and bounded-pair policy");
    return 0;
}
