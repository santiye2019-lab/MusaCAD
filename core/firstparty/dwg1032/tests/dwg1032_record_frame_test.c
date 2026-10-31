/* SPDX-License-Identifier: MIT */
#include "musa_dwg1032_record_frame.h"
#include "musa_dwg1032_object_map.h"
#include "musa_dwg1032_object_spans.h"
#ifdef NDEBUG
#undef NDEBUG /* Assertions remain active in Release CI. */
#endif
#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

static void basic_and_rollback(void) {
    const uint8_t bytes[]={2,0,0x11,0x22,1,0,0x33,0xff};
    MusaDwg1032Cursor parent={bytes,sizeof(bytes),0};
    MusaDwg1032Cursor payload={NULL,77u,55u};
    assert(musa_dwg1032_record_take_ms_frame(&parent,2,&payload));
    assert(payload.bytes==bytes+2 && payload.byte_count==2 &&
           payload.bit_position==0 && parent.bit_position==32);
    assert(musa_dwg1032_record_take_ms_frame(&parent,2,&payload));
    assert(payload.bytes==bytes+6 && payload.byte_count==1 &&
           parent.bit_position==56);
    const size_t at=parent.bit_position;
    const MusaDwg1032Cursor saved=payload;
    assert(!musa_dwg1032_record_take_ms_frame(&parent,2,&payload));
    assert(parent.bit_position==at && payload.bytes==saved.bytes &&
           payload.byte_count==saved.byte_count &&
           payload.bit_position==saved.bit_position);
    parent.bit_position=0;
    assert(!musa_dwg1032_record_take_ms_frame(&parent,1,&payload));
    assert(parent.bit_position==0 && payload.bytes==saved.bytes);
    parent.bit_position=1;
    assert(!musa_dwg1032_record_take_ms_frame(&parent,2,&payload));
    assert(parent.bit_position==1 && payload.bytes==saved.bytes);
    parent.bit_position=0;
    assert(!musa_dwg1032_record_take_ms_frame(&parent,2,&parent));
    assert(parent.bit_position==0);
    assert(!musa_dwg1032_record_take_ms_frame(
        &parent,2,(MusaDwg1032Cursor *)(void *)bytes));
    assert(parent.bit_position==0);
    assert(!musa_dwg1032_record_take_ms_frame(NULL,2,&payload));
    assert(!musa_dwg1032_record_take_ms_frame(&parent,2,NULL));
    const uint8_t empty[]={0,0};
    parent=(MusaDwg1032Cursor){empty,sizeof(empty),0};
    assert(!musa_dwg1032_record_take_ms_frame(&parent,100,&payload));
    assert(parent.bit_position==0 && payload.bytes==saved.bytes);
    const uint8_t truncated[]={4,0,0xaa,0xbb};
    parent=(MusaDwg1032Cursor){truncated,sizeof(truncated),0};
    assert(!musa_dwg1032_record_take_ms_frame(&parent,100,&payload));
    assert(parent.bit_position==0 && payload.bytes==saved.bytes);
    const uint8_t malformed[]={0,0x80,0,0};
    parent=(MusaDwg1032Cursor){malformed,sizeof(malformed),0};
    assert(!musa_dwg1032_record_take_ms_frame(&parent,100,&payload));
    assert(parent.bit_position==0 && payload.bytes==saved.bytes);
}

static void two_word_prefix(void) {
    const size_t length=32768u;
    uint8_t *bytes=(uint8_t *)malloc(length+4u);
    assert(bytes);
    bytes[0]=0;bytes[1]=0x80;bytes[2]=1;bytes[3]=0;
    memset(bytes+4,0x5a,length);
    MusaDwg1032Cursor parent={bytes,length+4u,0};
    MusaDwg1032Cursor payload={NULL,8u,9u};
    assert(!musa_dwg1032_record_take_ms_frame(&parent,length-1u,&payload));
    assert(parent.bit_position==0 && payload.byte_count==8u);
    assert(musa_dwg1032_record_take_ms_frame(&parent,length,&payload));
    assert(payload.bytes==bytes+4 && payload.byte_count==length);
    assert(parent.bit_position==(length+4u)*8u);
    assert(payload.bytes[0]==0x5a && payload.bytes[length-1u]==0x5a);
    free(bytes);
}

static void large_pafta_record_stream(void) {
    enum { COUNT=20000, RECORD_BYTES=3 };
    uint8_t *bytes=(uint8_t *)malloc(COUNT*RECORD_BYTES);
    assert(bytes);
    for (size_t i=0;i<COUNT;++i) {
        bytes[i*3u]=1;bytes[i*3u+1u]=0;
        bytes[i*3u+2u]=(uint8_t)((i*37u)^(i>>5u));
    }
    MusaDwg1032Cursor parent={bytes,COUNT*RECORD_BYTES,0};
    MusaDwg1032Cursor payload={NULL,0,0};
    for (size_t i=0;i<COUNT;++i) {
        assert(musa_dwg1032_record_take_ms_frame(&parent,1,&payload));
        assert(payload.byte_count==1 &&
               payload.bytes[0]==(uint8_t)((i*37u)^(i>>5u)));
    }
    assert(parent.bit_position==COUNT*RECORD_BYTES*8u);
    assert(!musa_dwg1032_record_take_ms_frame(&parent,1,&payload));
    assert(parent.bit_position==COUNT*RECORD_BYTES*8u);
    free(bytes);
}

/* Synthetic integration: map offset -> bounded candidate -> MS envelope.
 * Candidate region may contain padding, so it is NOT an exact record. */
static void candidate_map_pipeline(void) {
    const uint8_t section_bytes[]={1,0,0xa1,1,0,0xb2,1,0,0xc3,0xfe};
    const MusaDwg1032MapEntry map[]={{1,6},{2,0},{3,3}};
    MusaDwg1032ObjectLocation locations[3];
    size_t order[3];
    assert(musa_dwg1032_object_map_candidate_spans(
        map,3,sizeof(section_bytes),locations,3,order,3));
    const MusaDwg1032ObjectIndex index={
        locations,3,sizeof(section_bytes)};
    const MusaDwg1032Cursor section={
        section_bytes,sizeof(section_bytes),0};
    const uint8_t expected[]={0xc3,0xa1,0xb2};
    for (size_t i=0;i<3;++i) {
        MusaDwg1032Cursor candidate;
        MusaDwg1032Cursor record;
        assert(musa_dwg1032_object_span_cursor(
            &index,&section,i,&candidate));
        assert(musa_dwg1032_record_take_ms_frame(
            &candidate,1,&record));
        assert(record.byte_count==1 && record.bytes[0]==expected[i]);
        assert(candidate.bit_position==24u);
        if (i==0) assert(candidate.byte_count==4u); /* padding */
        else assert(candidate.byte_count==3u);
    }
}
int main(void) {
    basic_and_rollback();
    two_word_prefix();
    large_pafta_record_stream();
    candidate_map_pipeline();
    return 0;
}
