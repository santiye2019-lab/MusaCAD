/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "musacad_dxf_handle_set.h"
#include "musacad_dxf_handle.h"
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

struct MusaDxfHandleSet {
    uint64_t *table; /* zero is an unoccupied slot; DXF handle zero is invalid */
    size_t capacity;
    size_t count;
    size_t maximum;
};

static size_t index_for(uint64_t value,size_t capacity){
    /* SplitMix64 avalanche; deterministic on every supported C11 platform. */
    value^=value>>30;
    value*=UINT64_C(0xbf58476d1ce4e5b9);
    value^=value>>27;
    value*=UINT64_C(0x94d049bb133111eb);
    value^=value>>31;
    return (size_t)value & (capacity-1u);
}
static void put_unchecked(uint64_t *table,size_t capacity,uint64_t value){
    size_t slot=index_for(value,capacity);
    while(table[slot]!=0u)slot=(slot+1u)&(capacity-1u);
    table[slot]=value;
}
MusaDxfHandleSet *musa_core_dxf_handle_set_create(size_t maximum){
    if(maximum==0||maximum>1500000u)return NULL;
    MusaDxfHandleSet *set=(MusaDxfHandleSet *)calloc(1,sizeof(*set));
    if(!set)return NULL;
    set->capacity=256u;
    set->maximum=maximum;
    set->table=(uint64_t *)calloc(set->capacity,sizeof(uint64_t));
    if(!set->table){free(set);return NULL;}
    return set;
}
static int grow(MusaDxfHandleSet *set){
    if(set->capacity>SIZE_MAX/2u/sizeof(uint64_t))
        return MUSA_DXF_HANDLE_NO_MEMORY;
    const size_t larger=set->capacity*2u;
    uint64_t *next=(uint64_t *)calloc(larger,sizeof(uint64_t));
    if(!next)return MUSA_DXF_HANDLE_NO_MEMORY;
    for(size_t i=0;i<set->capacity;i++){
        if(set->table[i])put_unchecked(next,larger,set->table[i]);
    }
    free(set->table);
    set->table=next;
    set->capacity=larger;
    return MUSA_DXF_HANDLE_ADDED;
}
int musa_core_dxf_handle_set_insert(MusaDxfHandleSet *set,
                                    const char *text,size_t length){
    if(!set||!set->table||!text||length==0)
        return MUSA_DXF_HANDLE_INVALID;
    /* Match the legacy verifier's String.trim() semantics for ASCII DXF. */
    while(length && (unsigned char)*text<=0x20u){++text;--length;}
    while(length && (unsigned char)text[length-1u]<=0x20u)--length;
    uint64_t handle=0;
    if(!musa_core_dxf_handle_parse(text,length,&handle))
        return MUSA_DXF_HANDLE_INVALID;
    size_t slot=index_for(handle,set->capacity);
    while(set->table[slot]){
        if(set->table[slot]==handle)return MUSA_DXF_HANDLE_DUPLICATE;
        slot=(slot+1u)&(set->capacity-1u);
    }
    if(set->count==set->maximum)return MUSA_DXF_HANDLE_LIMIT;
    if(set->count+1u>set->capacity/2u){
        int status=grow(set);
        if(status!=MUSA_DXF_HANDLE_ADDED)return status;
        slot=index_for(handle,set->capacity);
        while(set->table[slot])slot=(slot+1u)&(set->capacity-1u);
    }
    set->table[slot]=handle;
    ++set->count;
    return MUSA_DXF_HANDLE_ADDED;
}
size_t musa_core_dxf_handle_set_count(const MusaDxfHandleSet *set){
    return set?set->count:0u;
}
void musa_core_dxf_handle_set_destroy(MusaDxfHandleSet *set){
    if(!set)return;
    if(set->table){
        memset(set->table,0,set->capacity*sizeof(uint64_t));
        free(set->table);
    }
    memset(set,0,sizeof(*set));
    free(set);
}
