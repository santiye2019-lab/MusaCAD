/* SPDX-License-Identifier: MIT */
#include "musa_dwg1032_object_map.h"
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <assert.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
/* Incremental decoding needs no full-map output buffer. A failed late
 * delta must preserve both the stream position and the last good entry. */
static void iterator_streaming(void) {
    const uint8_t bytes[]={1,10,2,0x45,3,20};
    MusaDwg1032MapIterator iterator;
    MusaDwg1032MapEntry entry={99,99};
    assert(musa_dwg1032_object_map_iterator_init(
        &iterator,bytes,sizeof(bytes),26));
    assert(musa_dwg1032_object_map_iterator_next(&iterator,&entry));
    assert(entry.handle==1 && entry.offset==10);
    assert(musa_dwg1032_object_map_iterator_next(&iterator,&entry));
    assert(entry.handle==3 && entry.offset==5);
    assert(musa_dwg1032_object_map_iterator_next(&iterator,&entry));
    assert(entry.handle==6 && entry.offset==25);
    assert(iterator.cursor.bit_position==sizeof(bytes)*8u);
    MusaDwg1032MapIterator saved=iterator;
    MusaDwg1032MapEntry last=entry;
    assert(!musa_dwg1032_object_map_iterator_next(&iterator,&entry));
    assert(memcmp(&saved,&iterator,sizeof(saved))==0);
    assert(memcmp(&last,&entry,sizeof(last))==0);
    assert(!musa_dwg1032_object_map_iterator_next(
        &iterator,(MusaDwg1032MapEntry *)(void *)&iterator));
    assert(memcmp(&saved,&iterator,sizeof(saved))==0);
    const uint8_t truncated[]={1,10,2};
    assert(musa_dwg1032_object_map_iterator_init(
        &iterator,truncated,sizeof(truncated),26));
    assert(musa_dwg1032_object_map_iterator_next(&iterator,&entry));
    saved=iterator;last=entry;
    assert(!musa_dwg1032_object_map_iterator_next(&iterator,&entry));
    assert(memcmp(&saved,&iterator,sizeof(saved))==0);
    assert(memcmp(&last,&entry,sizeof(last))==0);
    assert(!musa_dwg1032_object_map_iterator_init(
        (MusaDwg1032MapIterator *)(void *)bytes,
        bytes,sizeof(bytes),26));
    assert(!musa_dwg1032_object_map_iterator_init(
        &iterator,NULL,1,26));
}
static void iterator_large_map(void) {
    enum { N=20000 };
    static uint8_t bytes[N*2];
    for (size_t i=0;i<N;++i) {
        bytes[i*2]=1;
        bytes[i*2+1]=1;
    }
    MusaDwg1032MapIterator iterator;
    MusaDwg1032MapEntry entry={0,0};
    assert(musa_dwg1032_object_map_iterator_init(
        &iterator,bytes,sizeof(bytes),N+1u));
    for (size_t i=0;i<N;++i) {
        assert(musa_dwg1032_object_map_iterator_next(&iterator,&entry));
        assert(entry.handle==i+1u && entry.offset==i+1u);
    }
    assert(iterator.cursor.bit_position==sizeof(bytes)*8u);
    bytes[N*2-1u]=0x40; /* Invalid signed negative-zero at last entry. */
    assert(musa_dwg1032_object_map_iterator_init(
        &iterator,bytes,sizeof(bytes),N+1u));
    for (size_t i=0;i<N-1u;++i)
        assert(musa_dwg1032_object_map_iterator_next(&iterator,&entry));
    const MusaDwg1032MapIterator saved=iterator;
    const MusaDwg1032MapEntry last=entry;
    assert(!musa_dwg1032_object_map_iterator_next(&iterator,&entry));
    assert(memcmp(&saved,&iterator,sizeof(saved))==0);
    assert(memcmp(&last,&entry,sizeof(last))==0);
}
static void small(void) {
    const uint8_t payload[]={1,10,2,0x45,3,20};
    MusaDwg1032MapEntry out[3]={{99,99},{99,99},{99,99}};
    assert(musa_dwg1032_object_map_decode(payload,sizeof payload,3,26,out,3));
    assert(out[0].handle==1 && out[0].offset==10);
    assert(out[1].handle==3 && out[1].offset==5);
    assert(out[2].handle==6 && out[2].offset==25);
    const MusaDwg1032MapEntry saved=out[0];
    assert(!musa_dwg1032_object_map_decode(payload,sizeof payload-1,3,26,out,3));
    assert(!musa_dwg1032_object_map_decode(payload,sizeof payload,3,25,out,3));
    assert(!musa_dwg1032_object_map_decode(payload,sizeof payload,2,26,out,3));
    assert(!musa_dwg1032_object_map_decode(payload,sizeof payload,3,26,out,2));
    assert(out[0].handle==saved.handle && out[0].offset==saved.offset);
    const uint8_t bad_handle[]={0,10};
    assert(!musa_dwg1032_object_map_decode(bad_handle,2,1,100,out,3));
    const uint8_t bad_offset[]={1,0x41};
    assert(!musa_dwg1032_object_map_decode(bad_offset,2,1,100,out,3));
    const uint8_t noncanonical[]={0x81,0,10};
    assert(!musa_dwg1032_object_map_decode(noncanonical,3,1,100,out,3));
    assert(musa_dwg1032_object_map_decode(NULL,0,0,0,NULL,0));
    assert(!musa_dwg1032_object_map_decode(payload,sizeof payload,3,26,
        (MusaDwg1032MapEntry *)(void *)payload,3));
}
static void stress(void) {
    enum { N=10000 };
    static uint8_t bytes[N*2];
    static MusaDwg1032MapEntry out[N];
    for (size_t i=0;i<N;++i) {bytes[2*i]=1;bytes[2*i+1]=1;}
    assert(musa_dwg1032_object_map_decode(bytes,sizeof bytes,N,N+1,out,N));
    for (size_t i=0;i<N;++i) {
        assert(out[i].handle==i+1u && out[i].offset==i+1u);
    }
    bytes[N*2-1]=0x40;
    assert(!musa_dwg1032_object_map_decode(bytes,sizeof bytes,N,N+1,out,N));
    assert(out[N-1].handle==N && out[N-1].offset==N);
}
static void candidate_spans(void) {
    MusaDwg1032MapEntry entries[]={{1u,9u},{3u,0u},{5u,4u}};
    MusaDwg1032ObjectLocation out[3]={{99u,99u,99u},
                                      {99u,99u,99u},{99u,99u,99u}};
    size_t order[3]={99u,99u,99u};
    assert(musa_dwg1032_object_map_candidate_spans(
        entries,3u,12u,out,3u,order,3u));
    assert(out[0].handle==1u && out[0].offset==9u && out[0].length==3u);
    assert(out[1].handle==3u && out[1].offset==0u && out[1].length==4u);
    assert(out[2].handle==5u && out[2].offset==4u && out[2].length==5u);
    assert(order[0]==1u && order[1]==2u && order[2]==0u);
    const MusaDwg1032ObjectLocation saved=out[2];
    entries[2].offset=9u; /* Duplicate physical location. */
    assert(!musa_dwg1032_object_map_candidate_spans(
        entries,3u,12u,out,3u,order,3u));
    assert(out[2].handle==saved.handle && out[2].length==saved.length);
    entries[2].offset=12u; /* Past section end. */
    assert(!musa_dwg1032_object_map_candidate_spans(
        entries,3u,12u,out,3u,order,3u));
    entries[2].offset=4u;
    entries[2].handle=3u; /* Duplicate handle. */
    assert(!musa_dwg1032_object_map_candidate_spans(
        entries,3u,12u,out,3u,order,3u));
    entries[2].handle=5u;
    assert(!musa_dwg1032_object_map_candidate_spans(
        entries,3u,12u,out,2u,order,3u));
    assert(!musa_dwg1032_object_map_candidate_spans(
        entries,3u,12u,out,3u,order,2u));
    assert(!musa_dwg1032_object_map_candidate_spans(
        entries,3u,12u,(MusaDwg1032ObjectLocation *)(void *)entries,
        3u,order,3u));
    assert(!musa_dwg1032_object_map_candidate_spans(
        entries,3u,12u,out,3u,(size_t *)(void *)out,3u));
    assert(musa_dwg1032_object_map_candidate_spans(
        NULL,0u,0u,NULL,0u,NULL,0u));
}
static void candidate_stress(void) {
    enum { N=20000 };
    static MusaDwg1032MapEntry entries[N];
    static MusaDwg1032ObjectLocation out[N];
    static size_t order[N];
    for (size_t i=0;i<N;++i)
        entries[i]=(MusaDwg1032MapEntry){i+1u,(N-1u-i)*3u};
    assert(musa_dwg1032_object_map_candidate_spans(
        entries,N,N*3u,out,N,order,N));
    for (size_t i=0;i<N;++i) {
        assert(out[i].handle==i+1u);
        assert(out[i].offset==(N-1u-i)*3u);
        assert(out[i].length==3u);
        assert(order[i]==N-1u-i);
    }
    entries[N-1u].offset=entries[N-2u].offset;
    assert(!musa_dwg1032_object_map_candidate_spans(
        entries,N,N*3u,out,N,order,N));
    assert(out[N-1u].length==3u); /* No partial publication. */
}
int main(void) {iterator_streaming();iterator_large_map();small();stress();candidate_spans();candidate_stress();return 0;}
