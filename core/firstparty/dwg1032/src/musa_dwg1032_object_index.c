/* SPDX-License-Identifier: MIT */
#include "musa_dwg1032_object_index.h"
#include <stdint.h>
#include <stddef.h>
static int overlaps(const void *a, size_t an, const void *b, size_t bn) {
    if (!an || !bn) return 0;
    const uintptr_t x=(uintptr_t)a, y=(uintptr_t)b;
    return x<=y ? y-x<an : x-y<bn;
}
static int disjoint_index(const MusaDwg1032ObjectIndex *index,
                          const void *out, size_t out_size) {
    return index && out && index->count<=SIZE_MAX/sizeof(*index->entries) &&
           !overlaps(index,sizeof(*index),out,out_size) &&
           !overlaps(index->entries,index->count*sizeof(*index->entries),out,out_size);
}
int musa_dwg1032_object_index_init(MusaDwg1032ObjectIndex *index,
                                    const MusaDwg1032ObjectLocation *entries,
                                    size_t count, uint64_t section_bytes) {
    if (!index || (count && !entries) ||
        count>SIZE_MAX/sizeof(*entries) ||
        overlaps(index,sizeof(*index),entries,count*sizeof(*entries))) return 0;
    uint64_t previous=0;
    for (size_t i=0;i<count;++i) {
        const MusaDwg1032ObjectLocation e=entries[i];
        if (!e.handle || e.handle<=previous || !e.length ||
            e.offset>section_bytes || e.length>section_bytes-e.offset)
            return 0;
        previous=e.handle;
    }
    const MusaDwg1032ObjectIndex candidate={entries,count,section_bytes};
    *index=candidate;
    return 1;
}
int musa_dwg1032_object_index_find(const MusaDwg1032ObjectIndex *index,
                                    uint64_t handle, size_t *position) {
    if (!disjoint_index(index,position,sizeof(*position)) ||
        (index->count && !index->entries) || !handle) return 0;
    size_t lo=0, hi=index->count;
    while (lo<hi) {
        const size_t mid=lo+(hi-lo)/2u;
        if (index->entries[mid].handle<handle) lo=mid+1u;
        else hi=mid;
    }
    if (lo==index->count || index->entries[lo].handle!=handle) return 0;
    *position=lo;
    return 1;
}
int musa_dwg1032_object_index_find_ref(const MusaDwg1032ObjectIndex *index,
                                        const MusaDwg1032Handle *reference,
                                        uint64_t base_handle,
                                        size_t *position) {
    uint64_t target=0;
    if (!index || !reference || !disjoint_index(index,position,sizeof(*position)) ||
        overlaps(reference,sizeof(*reference),position,sizeof(*position)) ||
        !musa_dwg1032_handle_resolve(reference,base_handle,&target)) return 0;
    return musa_dwg1032_object_index_find(index,target,position);
}
