/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "musacad_dxf_handle_set.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static int add(MusaDxfHandleSet *set,const char *s){
    return musa_core_dxf_handle_set_insert(set,s,strlen(s));
}
int main(void){
    assert(musa_core_dxf_handle_set_create(0)==NULL);
    assert(musa_core_dxf_handle_set_create(1500001u)==NULL);
    MusaDxfHandleSet *set=musa_core_dxf_handle_set_create(300);
    assert(set);
    assert(add(set,"  000a\r")==MUSA_DXF_HANDLE_ADDED);
    assert(add(set,"A")==MUSA_DXF_HANDLE_DUPLICATE);
    assert(add(set,"a")==MUSA_DXF_HANDLE_DUPLICATE);
    assert(add(set,"0000")==MUSA_DXF_HANDLE_INVALID);
    assert(add(set,"0xA")==MUSA_DXF_HANDLE_INVALID);
    assert(add(set,"-1")==MUSA_DXF_HANDLE_INVALID);
    assert(add(set,"G")==MUSA_DXF_HANDLE_INVALID);
    assert(add(set,"FFFFFFFFFFFFFFFF")==MUSA_DXF_HANDLE_ADDED);
    assert(add(set,"ffffffffffffffff")==MUSA_DXF_HANDLE_DUPLICATE);
    for(unsigned int i=1;i<299;i++){
        char id[24];
        snprintf(id,sizeof(id),"%X",i+65536u);
        assert(add(set,id)==MUSA_DXF_HANDLE_ADDED);
    }
    assert(musa_core_dxf_handle_set_count(set)==300u);
    assert(add(set,"10001")==MUSA_DXF_HANDLE_DUPLICATE);
    assert(add(set,"BCAD000")==MUSA_DXF_HANDLE_LIMIT);
    assert(musa_core_dxf_handle_set_count(set)==300u);
    musa_core_dxf_handle_set_destroy(set);
    set=musa_core_dxf_handle_set_create(1);
    assert(set);
    assert(add(set,"5")==MUSA_DXF_HANDLE_ADDED);
    assert(add(set," 05 ")==MUSA_DXF_HANDLE_DUPLICATE);
    assert(add(set,"6")==MUSA_DXF_HANDLE_LIMIT);
    musa_core_dxf_handle_set_destroy(set);
    assert(musa_core_dxf_handle_set_count(NULL)==0u);
    assert(musa_core_dxf_handle_set_insert(NULL,"A",1)==MUSA_DXF_HANDLE_INVALID);
    puts("PASS: cross-platform bounded source-safe DXF 5/105 handle uniqueness");
    return 0;
}
