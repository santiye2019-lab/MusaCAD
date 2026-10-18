/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "musacad_handle_index.h"
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define MAX_HANDLES 1000000u
typedef struct {
    uint64_t handle;
    uint32_t slot;
    uint32_t state; /* 0 never used, 1 live, 2 deleted tombstone */
} Entry;
struct MusaHandleIndex {
    Entry *entries;
    size_t slots,limit,count;
};
static uint64_t hash(uint64_t key){
    /* SplitMix64 mixing, deterministic on 32-bit/64-bit C11 machines. */
    key+=UINT64_C(0x9e3779b97f4a7c15);
    key=(key^(key>>30u))*UINT64_C(0xbf58476d1ce4e5b9);
    key=(key^(key>>27u))*UINT64_C(0x94d049bb133111eb);
    return key^(key>>31u);
}
MusaHandleIndex *musa_handle_index_create(size_t max_entries){
    if(!max_entries||max_entries>MAX_HANDLES)return NULL;
    size_t slots=16u;
    while(slots<max_entries*2u){
        if(slots>SIZE_MAX/2u)return NULL;
        slots*=2u;
    }
    if(slots>SIZE_MAX/sizeof(Entry))return NULL;
    MusaHandleIndex *index=(MusaHandleIndex *)calloc(1,sizeof(*index));
    if(!index)return NULL;
    index->entries=(Entry *)calloc(slots,sizeof(*index->entries));
    if(!index->entries){free(index);return NULL;}
    index->slots=slots;index->limit=max_entries;
    return index;
}
void musa_handle_index_destroy(MusaHandleIndex *index){
    if(!index)return;
    if(index->entries){
        memset(index->entries,0,index->slots*sizeof(*index->entries));
        free(index->entries);
    }
    memset(index,0,sizeof(*index));free(index);
}
size_t musa_handle_index_count(const MusaHandleIndex *idx){
    return idx?idx->count:0;
}
static size_t locate(const MusaHandleIndex *index,uint64_t handle){
    if(!index||!handle)return SIZE_MAX;
    const size_t mask=index->slots-1u;
    size_t i=(size_t)hash(handle)&mask;
    for(size_t probes=0;probes<index->slots;probes++){
        const Entry *e=&index->entries[i];
        if(e->state==0u)return SIZE_MAX;
        if(e->state==1u&&e->handle==handle)return i;
        i=(i+1u)&mask;
    }
    return SIZE_MAX;
}
int musa_handle_index_get(const MusaHandleIndex *idx,uint64_t handle,uint32_t *out){
    if(!idx||!out||!handle)return -1;
    size_t pos=locate(idx,handle);
    if(pos==SIZE_MAX)return 0;
    *out=idx->entries[pos].slot;
    return 1;
}
int musa_handle_index_set(MusaHandleIndex *idx,uint64_t handle,uint32_t slot){
    if(!idx||!handle)return -1;
    size_t pos=locate(idx,handle);
    if(pos==SIZE_MAX)return 0;
    idx->entries[pos].slot=slot;return 1;
}
int musa_handle_index_remove(MusaHandleIndex *idx,uint64_t handle){
    if(!idx||!handle)return -1;
    size_t pos=locate(idx,handle);
    if(pos==SIZE_MAX)return 0;
    Entry *e=&idx->entries[pos];
    e->handle=0u;e->slot=0u;e->state=2u;
    idx->count--;return 1;
}
int musa_handle_index_put(MusaHandleIndex *idx,uint64_t handle,uint32_t slot){
    if(!idx||!handle)return -1;
    const size_t mask=idx->slots-1u;
    size_t i=(size_t)hash(handle)&mask;
    size_t first_tombstone=SIZE_MAX;
    for(size_t probes=0;probes<idx->slots;probes++){
        Entry *e=&idx->entries[i];
        if(e->state==1u&&e->handle==handle)return 0;
        if(e->state==2u&&first_tombstone==SIZE_MAX)
            first_tombstone=i;
        if(e->state==0u){
            if(idx->count>=idx->limit)return -2;
            if(first_tombstone!=SIZE_MAX)e=&idx->entries[first_tombstone];
            e->handle=handle;e->slot=slot;e->state=1u;
            idx->count++;return 1;
        }
        i=(i+1u)&mask;
    }
    if(first_tombstone!=SIZE_MAX&&idx->count<idx->limit){
        Entry *e=&idx->entries[first_tombstone];
        e->handle=handle;e->slot=slot;e->state=1u;
        idx->count++;return 1;
    }
    return -2;
}
