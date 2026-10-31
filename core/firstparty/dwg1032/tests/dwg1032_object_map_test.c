/* SPDX-License-Identifier: MIT */
#include "musa_dwg1032_object_map.h"
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <assert.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
static int same_iterator(const MusaDwg1032MapIterator *a,
                         const MusaDwg1032MapIterator *b) {
    return a->cursor.bytes==b->cursor.bytes &&
           a->cursor.byte_count==b->cursor.byte_count &&
           a->cursor.bit_position==b->cursor.bit_position &&
           a->handle==b->handle && a->offset==b->offset &&
           a->section_bytes==b->section_bytes &&
           a->entries_read==b->entries_read;
}
static int same_entry(const MusaDwg1032MapEntry *a,
                      const MusaDwg1032MapEntry *b) {
    return a->handle==b->handle && a->offset==b->offset;
}
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
    assert(same_iterator(&saved,&iterator));
    assert(same_entry(&last,&entry));
    assert(!musa_dwg1032_object_map_iterator_next(
        &iterator,(MusaDwg1032MapEntry *)(void *)&iterator));
    assert(same_iterator(&saved,&iterator));
    const uint8_t truncated[]={1,10,2};
    assert(musa_dwg1032_object_map_iterator_init(
        &iterator,truncated,sizeof(truncated),26));
    assert(musa_dwg1032_object_map_iterator_next(&iterator,&entry));
    saved=iterator;last=entry;
    assert(!musa_dwg1032_object_map_iterator_next(&iterator,&entry));
    assert(same_iterator(&saved,&iterator));
    assert(same_entry(&last,&entry));
    assert(!musa_dwg1032_object_map_iterator_init(
        (MusaDwg1032MapIterator *)(void *)bytes,
        bytes,sizeof(bytes),26));
    assert(!musa_dwg1032_object_map_iterator_init(
        &iterator,NULL,1,26));
}
static void iterator_completion(void) {
    const uint8_t bytes[]={1u,10u,2u,0x45u,3u,20u};
    MusaDwg1032MapIterator it;
    MusaDwg1032MapEntry entry={0,0};
    assert(musa_dwg1032_object_map_iterator_init(
        &it,bytes,sizeof(bytes),26u));
    assert(!musa_dwg1032_object_map_iterator_finished(&it,0u));
    assert(musa_dwg1032_object_map_iterator_next(&it,&entry));
    assert(it.entries_read==1u);
    assert(!musa_dwg1032_object_map_iterator_finished(&it,1u));
    assert(musa_dwg1032_object_map_iterator_page(&it,&entry,1u,1u));
    assert(it.entries_read==2u);
    assert(!musa_dwg1032_object_map_iterator_finished(&it,2u));
    assert(musa_dwg1032_object_map_iterator_next(&it,&entry));
    assert(it.entries_read==3u);
    assert(musa_dwg1032_object_map_iterator_finished(&it,3u));
    assert(!musa_dwg1032_object_map_iterator_finished(&it,2u));
    assert(!musa_dwg1032_object_map_iterator_finished(NULL,3u));
    const MusaDwg1032MapIterator saved=it;
    it.entries_read=SIZE_MAX;
    assert(!musa_dwg1032_object_map_iterator_next(&it,&entry));
    assert(it.entries_read==SIZE_MAX &&
           it.cursor.bit_position==saved.cursor.bit_position);
    it=saved;
    it.cursor.bit_position=0u;
    assert(!musa_dwg1032_object_map_iterator_finished(&it,3u));
    assert(musa_dwg1032_object_map_iterator_init(&it,NULL,0u,0u));
    assert(musa_dwg1032_object_map_iterator_finished(&it,0u));
    assert(!musa_dwg1032_object_map_iterator_finished(&it,1u));
}
static void iterator_paged_atomic(void) {
    uint8_t bytes[]={1,10,2,0x45,3,20};
    MusaDwg1032MapIterator iterator;
    MusaDwg1032MapEntry out[3]={{99,99},{99,99},{99,99}};
    assert(musa_dwg1032_object_map_iterator_init(
        &iterator,bytes,sizeof(bytes),26));
    assert(musa_dwg1032_object_map_iterator_page(
        &iterator,out,2,3));
    assert(out[0].handle==1 && out[0].offset==10);
    assert(out[1].handle==3 && out[1].offset==5);
    assert(out[2].handle==99 && out[2].offset==99);
    assert(iterator.cursor.bit_position==32u);
    MusaDwg1032MapIterator saved=iterator;
    MusaDwg1032MapEntry snapshot[3];
    memcpy(snapshot,out,sizeof(out));
    bytes[5]=0x40; /* Late negative zero: reject entire requested page. */
    assert(!musa_dwg1032_object_map_iterator_page(
        &iterator,out,1,3));
    assert(same_iterator(&saved,&iterator));
    assert(memcmp(snapshot,out,sizeof(out))==0);
    bytes[5]=20;
    assert(!musa_dwg1032_object_map_iterator_page(
        &iterator,out,2,3)); /* One pair exists, not two. */
    assert(same_iterator(&saved,&iterator));
    assert(memcmp(snapshot,out,sizeof(out))==0);
    assert(!musa_dwg1032_object_map_iterator_page(
        &iterator,out,1,0));
    assert(!musa_dwg1032_object_map_iterator_page(
        &iterator,(MusaDwg1032MapEntry *)(void *)bytes,1,1));
    assert(!musa_dwg1032_object_map_iterator_page(
        &iterator,(MusaDwg1032MapEntry *)(void *)&iterator,1,1));
    assert(same_iterator(&saved,&iterator));
    assert(musa_dwg1032_object_map_iterator_page(
        &iterator,out+2,1,1));
    assert(out[2].handle==6 && out[2].offset==25);
    assert(iterator.cursor.bit_position==sizeof(bytes)*8u);
    saved=iterator;
    assert(musa_dwg1032_object_map_iterator_page(
        &iterator,NULL,0,0));
    assert(same_iterator(&saved,&iterator));
    assert(!musa_dwg1032_object_map_iterator_page(
        NULL,out,1,3));
    assert(!musa_dwg1032_object_map_iterator_page(
        &iterator,NULL,1,1));
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
    assert(same_iterator(&saved,&iterator));
    assert(same_entry(&last,&entry));
}
static void iterator_paged_large(void) {
    enum { N=20000, PAGE=257 };
    static uint8_t bytes[N*2];
    for (size_t i=0;i<N;++i) {
        bytes[i*2]=1u;
        bytes[i*2+1u]=1u;
    }
    MusaDwg1032MapIterator iterator;
    MusaDwg1032MapEntry out[PAGE];
    assert(musa_dwg1032_object_map_iterator_init(
        &iterator,bytes,sizeof(bytes),N+1u));
    for (size_t start=0;start<N;start+=PAGE) {
        const size_t count=N-start<PAGE ? N-start : PAGE;
        assert(musa_dwg1032_object_map_iterator_page(
            &iterator,out,count,PAGE));
        for (size_t j=0;j<count;++j) {
            assert(out[j].handle==start+j+1u);
            assert(out[j].offset==start+j+1u);
        }
    }
    assert(iterator.cursor.bit_position==sizeof(bytes)*8u);
    /* A corrupt last record cannot publish even the first entry of
     * its page, despite 77 preceding pages being independently valid. */
    bytes[N*2u-1u]=0x40u;
    assert(musa_dwg1032_object_map_iterator_init(
        &iterator,bytes,sizeof(bytes),N+1u));
    for (size_t start=0;start+PAGE<N-PAGE;start+=PAGE)
        assert(musa_dwg1032_object_map_iterator_page(
            &iterator,out,PAGE,PAGE));
    /* Seek by complete pages until the final short page. */
    const size_t processed=(N/PAGE)*PAGE;
    while (iterator.handle<processed)
        assert(musa_dwg1032_object_map_iterator_page(
            &iterator,out,PAGE,PAGE));
    MusaDwg1032MapIterator saved=iterator;
    MusaDwg1032MapEntry snapshot[PAGE];
    memcpy(snapshot,out,sizeof(out));
    assert(!musa_dwg1032_object_map_iterator_page(
        &iterator,out,N-processed,PAGE));
    assert(same_iterator(&saved,&iterator));
    assert(memcmp(snapshot,out,sizeof(out))==0);
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
/* Complete verified payload -> candidate physical spans, with output rollback
 * on both delta corruption and duplicate physical object locations. */
static void decode_locations_atomic(void) {
    const uint8_t payload[]={1u,10u,2u,0x45u,3u,20u};
    MusaDwg1032ObjectLocation out[3]={
        {99u,99u,99u},{99u,99u,99u},{99u,99u,99u}};
    MusaDwg1032MapEntry scratch[3];
    size_t order[3];
    assert(musa_dwg1032_object_map_decode_locations(
        payload,sizeof(payload),3u,26u,out,3u,scratch,3u,order,3u));
    assert(out[0].handle==1u && out[0].offset==10u &&
           out[0].length==15u);
    assert(out[1].handle==3u && out[1].offset==5u &&
           out[1].length==5u);
    assert(out[2].handle==6u && out[2].offset==25u &&
           out[2].length==1u);
    MusaDwg1032ObjectLocation saved[3];
    memcpy(saved,out,sizeof(out));
    const uint8_t duplicate[]={1u,10u,2u,0u,3u,20u};
    assert(!musa_dwg1032_object_map_decode_locations(
        duplicate,sizeof(duplicate),3u,31u,
        out,3u,scratch,3u,order,3u));
    assert(memcmp(saved,out,sizeof(out))==0);
    const uint8_t truncated[]={1u,10u,2u,0x45u,3u};
    assert(!musa_dwg1032_object_map_decode_locations(
        truncated,sizeof(truncated),3u,26u,
        out,3u,scratch,3u,order,3u));
    assert(memcmp(saved,out,sizeof(out))==0);
    assert(!musa_dwg1032_object_map_decode_locations(
        payload,sizeof(payload),3u,26u,out,2u,scratch,3u,order,3u));
    assert(!musa_dwg1032_object_map_decode_locations(
        payload,sizeof(payload),3u,26u,out,3u,scratch,2u,order,3u));
    assert(!musa_dwg1032_object_map_decode_locations(
        payload,sizeof(payload),3u,26u,out,3u,scratch,3u,order,2u));
    assert(!musa_dwg1032_object_map_decode_locations(
        payload,sizeof(payload),3u,26u,out,3u,scratch,3u,
        (size_t *)(void *)out,3u));
    assert(!musa_dwg1032_object_map_decode_locations(
        payload,sizeof(payload),3u,26u,out,3u,
        (MusaDwg1032MapEntry *)(void *)payload,3u,order,3u));
    assert(memcmp(saved,out,sizeof(out))==0);
    assert(musa_dwg1032_object_map_decode_locations(
        NULL,0u,0u,0u,NULL,0u,NULL,0u,NULL,0u));
}
static void decode_locations_large(void) {
    enum { N=20000 };
    static uint8_t bytes[N*2u];
    static MusaDwg1032ObjectLocation out[N];
    static MusaDwg1032MapEntry scratch[N];
    static size_t order[N];
    for (size_t i=0;i<N;++i) {
        bytes[2u*i]=1u;
        bytes[2u*i+1u]=1u;
    }
    assert(musa_dwg1032_object_map_decode_locations(
        bytes,sizeof(bytes),N,N+1u,out,N,scratch,N,order,N));
    for (size_t i=0;i<N;++i)
        assert(out[i].handle==i+1u && out[i].offset==i+1u &&
               out[i].length==1u);
    bytes[2u*N-1u]=0x40u;
    assert(!musa_dwg1032_object_map_decode_locations(
        bytes,sizeof(bytes),N,N+1u,out,N,scratch,N,order,N));
    assert(out[N-1u].handle==N && out[N-1u].length==1u);
}
/* Exercise the full signed 64-bit range in a map walk. In particular,
 * -INT64_MIN must never be evaluated as a signed expression. */
static void iterator_extreme_negative_delta(void) {
    const uint8_t stream[]={
        1u, /* handle 1, positive INT64_MAX */
        0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0x00,
        1u,1u, /* handle 2, offset 2^63 */
        1u, /* handle 3, signed delta INT64_MIN */
        0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x41
    };
    MusaDwg1032MapIterator iterator;
    MusaDwg1032MapEntry entry={99u,99u};
    assert(musa_dwg1032_object_map_iterator_init(
        &iterator,stream,sizeof(stream),UINT64_MAX));
    assert(musa_dwg1032_object_map_iterator_next(&iterator,&entry));
    assert(entry.handle==1u && entry.offset==(uint64_t)INT64_MAX);
    assert(musa_dwg1032_object_map_iterator_next(&iterator,&entry));
    assert(entry.handle==2u && entry.offset==(UINT64_C(1)<<63u));
    assert(musa_dwg1032_object_map_iterator_next(&iterator,&entry));
    assert(entry.handle==3u && entry.offset==0u);
    assert(musa_dwg1032_object_map_iterator_finished(&iterator,3u));
    MusaDwg1032MapEntry out[3]={{0}};
    assert(musa_dwg1032_object_map_decode(
        stream,sizeof(stream),3u,UINT64_MAX,out,3u));
    assert(out[0].offset==(uint64_t)INT64_MAX &&
           out[1].offset==(UINT64_C(1)<<63u) && out[2].offset==0u);

    const uint8_t min_only[]={
        1u,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x41
    };
    assert(musa_dwg1032_object_map_iterator_init(
        &iterator,min_only,sizeof(min_only),UINT64_MAX));
    iterator.offset=(UINT64_C(1)<<63u)-1u;
    const MusaDwg1032MapIterator saved=iterator;
    const MusaDwg1032MapEntry last=entry;
    assert(!musa_dwg1032_object_map_iterator_next(&iterator,&entry));
    assert(same_iterator(&saved,&iterator) && same_entry(&last,&entry));
}
int main(void) {iterator_extreme_negative_delta();iterator_streaming();iterator_completion();iterator_paged_atomic();iterator_large_map();iterator_paged_large();small();stress();candidate_spans();candidate_stress();decode_locations_atomic();decode_locations_large();return 0;}
