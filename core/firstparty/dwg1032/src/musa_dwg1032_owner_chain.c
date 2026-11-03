/* SPDX-License-Identifier: MIT */
#include "musa_dwg1032_owner_chain.h"
#include <stdint.h>
#include <stddef.h>
static int overlaps(const void *a, size_t an, const void *b, size_t bn) {
    if (!an || !bn) return 0;
    const uintptr_t x=(uintptr_t)a, y=(uintptr_t)b;
    return x<=y ? y-x<an : x-y<bn;
}
int musa_dwg1032_owner_chain_root(const MusaDwg1032ObjectIndex *index,
                                   const uint64_t *owner_handles,
                                   size_t start_position, size_t max_hops,
                                   uint64_t *root_handle, size_t *hops) {
    if (!index || !root_handle || !hops || !owner_handles ||
        !index->entries || start_position>=index->count ||
        index->count>SIZE_MAX/sizeof(*owner_handles) ||
        index->count>SIZE_MAX/sizeof(*index->entries)) return 0;
    const size_t owner_bytes=index->count*sizeof(*owner_handles);
    const size_t entry_bytes=index->count*sizeof(*index->entries);
    if (overlaps(owner_handles,owner_bytes,index,sizeof(*index)) ||
        overlaps(owner_handles,owner_bytes,index->entries,entry_bytes) ||
        overlaps(root_handle,sizeof(*root_handle),hops,sizeof(*hops)) ||
        overlaps(root_handle,sizeof(*root_handle),index,sizeof(*index)) ||
        overlaps(hops,sizeof(*hops),index,sizeof(*index)) ||
        overlaps(root_handle,sizeof(*root_handle),owner_handles,owner_bytes) ||
        overlaps(hops,sizeof(*hops),owner_handles,owner_bytes) ||
        overlaps(root_handle,sizeof(*root_handle),index->entries,entry_bytes) ||
        overlaps(hops,sizeof(*hops),index->entries,entry_bytes)) return 0;
    size_t at=start_position;
    size_t depth=0;
    for (;;) {
        /* A caller may accidentally reuse an index after mutating its
         * backing storage. Reject malformed visited entries before
         * publishing a root, without a full-index scan per query. */
        const MusaDwg1032ObjectLocation entry=index->entries[at];
        if (!entry.handle || !entry.length ||
            entry.offset>index->section_bytes ||
            entry.length>index->section_bytes-entry.offset) return 0;
        const uint64_t parent=owner_handles[at];
        if (!parent) {
            *root_handle=entry.handle;
            *hops=depth;
            return 1;
        }
        /* More than count-1 edges without a root implies a cycle. */
        if (depth>=max_hops || depth>=index->count) return 0;
        if (!musa_dwg1032_object_index_find(index,parent,&at)) return 0;
        ++depth;
    }
}
