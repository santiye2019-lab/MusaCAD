/* SPDX-License-Identifier: MIT
 * Synthetic map-to-record regressions; not a real DWG fixture.
 */
#include "musa_dwg1032_record_batch.h"
#include "musa_dwg1032_object_map.h"
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <assert.h>
#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

static void three_candidates(void) {
    uint8_t bytes[]={1,0,0x21,0xee, 1,0,0x32,0xdd,
                     1,0,0x43,0xcc};
    const MusaDwg1032MapEntry map[]={{1,8},{2,0},{3,4}};
    MusaDwg1032ObjectLocation spans[3];
    size_t order[3];
    assert(musa_dwg1032_object_map_candidate_spans(
        map,3,sizeof(bytes),spans,3,order,3));
    const MusaDwg1032ObjectIndex index={spans,3,sizeof(bytes)};
    const MusaDwg1032Cursor section={bytes,sizeof(bytes),0};
    MusaDwg1032ObjectLocation out[3]={{99,99,99},
                                       {99,99,99},{99,99,99}};
    assert(musa_dwg1032_record_batch_extract(
        &index,&section,1,out,3,order,3));
    const uint64_t offsets[]={10,2,6};
    const uint8_t values[]={0x43u,0x21u,0x32u};
    for (size_t i=0;i<3;++i) {
        assert(out[i].handle==i+1u && out[i].offset==offsets[i] &&
               out[i].length==1u);
        assert(bytes[(size_t)out[i].offset]==values[i]);
    }
    /* Prepared index: page reads avoid re-sorting or allocating 3 outputs. */
    MusaDwg1032ObjectLocation page[2]={{77,77,77},{88,88,88}};
    size_t written=111u, total=222u;
    assert(musa_dwg1032_record_batch_extract_page(
        &index,&section,1u,1u,page,2u,&written,&total));
    assert(written==2u && total==3u);
    assert(page[0].handle==2u && page[0].offset==2u &&
           page[1].handle==3u && page[1].offset==6u);
    MusaDwg1032ObjectLocation saved_page[2]={page[0],page[1]};
    written=111u; total=222u;
    bytes[4]=0u; /* Later record in requested page fails atomically. */
    assert(!musa_dwg1032_record_batch_extract_page(
        &index,&section,1u,1u,page,2u,&written,&total));
    assert(memcmp(page,saved_page,sizeof(page))==0);
    assert(written==111u && total==222u);
    bytes[4]=1u;
    assert(!musa_dwg1032_record_batch_extract_page(
        &index,&section,1u,0u,page,2u,&written,&total));
    assert(!musa_dwg1032_record_batch_extract_page(
        &index,&section,1u,1u,(MusaDwg1032ObjectLocation *)(void *)bytes,
        2u,&written,&total));
    assert(!musa_dwg1032_record_batch_extract_page(
        &index,&section,1u,1u,spans,2u,&written,&total));
    assert(!musa_dwg1032_record_batch_extract_page(
        &index,&section,1u,1u,page,2u,
        (size_t *)(void *)page,&total));
    assert(!musa_dwg1032_record_batch_extract_page(
        &index,&section,1u,1u,page,2u,&written,&written));
    assert(!musa_dwg1032_record_batch_extract_page(
        &index,&section,1u,1u,page,2u,
        (size_t *)(void *)bytes,&total));
    assert(memcmp(page,saved_page,sizeof(page))==0);
    assert(written==111u && total==222u);
    const MusaDwg1032Cursor bad_section={bytes,sizeof(bytes),1u};
    assert(!musa_dwg1032_record_batch_extract_page(
        &index,&bad_section,1u,1u,page,2u,&written,&total));
    assert(memcmp(page,saved_page,sizeof(page))==0);
    assert(written==111u && total==222u);
    assert(musa_dwg1032_record_batch_extract_page(
        &index,&section,3u,0u,NULL,0u,&written,&total));
    assert(written==0u && total==3u);
    assert(musa_dwg1032_record_batch_extract_page(
        &index,&section,SIZE_MAX,0u,NULL,0u,&written,&total));
    assert(written==0u && total==3u);
    const MusaDwg1032ObjectLocation saved[3]={out[0],out[1],out[2]};
    bytes[4]=0; /* Late malformed record; no earlier result published. */
    assert(!musa_dwg1032_record_batch_extract(
        &index,&section,1,out,3,order,3));
    assert(memcmp(saved,out,sizeof(out))==0);
    bytes[4]=1;
    /* Payload limits must reject oversized records without output changes. */
    assert(!musa_dwg1032_record_batch_extract(
        &index,&section,0,out,3,order,3));
    assert(memcmp(saved,out,sizeof(out))==0);
    /* Index entries are immutable inputs, never writable result storage. */
    assert(!musa_dwg1032_record_batch_extract(
        &index,&section,1,spans,3,order,3));
    assert(memcmp(saved,out,sizeof(out))==0);
    assert(!musa_dwg1032_record_batch_extract(
        &index,&section,0,out,3,order,3));
    assert(!musa_dwg1032_record_batch_extract(
        &index,&section,1,out,2,order,3));
    assert(!musa_dwg1032_record_batch_extract(
        &index,&section,1,out,3,order,2));
    assert(!musa_dwg1032_record_batch_extract(
        &index,&section,1,(MusaDwg1032ObjectLocation *)(void *)bytes,
        3,order,3));
    assert(!musa_dwg1032_record_batch_extract(
        &index,&section,1,out,3,(size_t *)(void *)bytes,3));
    assert(!musa_dwg1032_record_batch_extract(
        &index,&section,1,out,3,(size_t *)(void *)out,3));
    assert(memcmp(saved,out,sizeof(out))==0);
    MusaDwg1032ObjectLocation bad[3]={spans[0],spans[1],spans[2]};
    bad[2].offset=spans[1].offset; /* Intersecting physical candidates. */
    const MusaDwg1032ObjectIndex invalid={bad,3,sizeof(bytes)};
    assert(!musa_dwg1032_record_batch_extract(
        &invalid,&section,1,out,3,order,3));
    assert(memcmp(saved,out,sizeof(out))==0);
    const MusaDwg1032Cursor misaligned={bytes,sizeof(bytes),1};
    assert(!musa_dwg1032_record_batch_extract(
        &index,&misaligned,1,out,3,order,3));
    assert(memcmp(saved,out,sizeof(out))==0);
    const MusaDwg1032ObjectIndex empty={NULL,0,0};
    const MusaDwg1032Cursor zero={NULL,0,0};
    assert(musa_dwg1032_record_batch_extract(
        &empty,&zero,0,NULL,0,NULL,0));
}
static void large_pafta(void) {
    enum { N=20000, STRIDE=4 };
    uint8_t *bytes=(uint8_t *)malloc(N*STRIDE);
    MusaDwg1032ObjectLocation *spans=
        (MusaDwg1032ObjectLocation *)malloc(N*sizeof(*spans));
    MusaDwg1032ObjectLocation *out=
        (MusaDwg1032ObjectLocation *)malloc(N*sizeof(*out));
    size_t *order=(size_t *)malloc(N*sizeof(*order));
    assert(bytes && spans && out && order);
    for (size_t i=0;i<N;++i) {
        const size_t off=(N-1u-i)*STRIDE;
        bytes[off]=1;bytes[off+1u]=0;
        bytes[off+2u]=(uint8_t)i;bytes[off+3u]=0xffu;
        spans[i]=(MusaDwg1032ObjectLocation){i+1u,off,STRIDE};
    }
    const MusaDwg1032ObjectIndex index={spans,N,N*STRIDE};
    const MusaDwg1032Cursor section={bytes,N*STRIDE,0};
    assert(musa_dwg1032_record_batch_extract(
        &index,&section,1,out,N,order,N));
    for (size_t i=0;i<N;++i) {
        assert(out[i].handle==i+1u &&
               out[i].offset==(N-1u-i)*STRIDE+2u &&
               out[i].length==1u);
    }
    /* Compare 79 independently requested pages with the bulk extraction. */
    for (size_t start=0;start<N;start+=257u) {
        MusaDwg1032ObjectLocation page[257];
        size_t written=SIZE_MAX, total=SIZE_MAX;
        assert(musa_dwg1032_record_batch_extract_page(
            &index,&section,start,1u,page,257u,&written,&total));
        assert(total==N);
        assert(written==(N-start<257u ? N-start : 257u));
        assert(memcmp(page,out+start,written*sizeof(*page))==0);
    }
    MusaDwg1032ObjectLocation *snapshot=
        (MusaDwg1032ObjectLocation *)malloc(N*sizeof(*snapshot));
    assert(snapshot);
    memcpy(snapshot,out,N*sizeof(*out));
    bytes[0]=0; /* Final handle has the first physical record. */
    assert(!musa_dwg1032_record_batch_extract(
        &index,&section,1,out,N,order,N));
    assert(memcmp(snapshot,out,N*sizeof(*out))==0);
    bytes[0]=1;
    bytes[(N/2u)*STRIDE]=0; /* Middle-record corruption must also roll back. */
    assert(!musa_dwg1032_record_batch_extract(
        &index,&section,1,out,N,order,N));
    assert(memcmp(snapshot,out,N*sizeof(*out))==0);
    MusaDwg1032ObjectLocation page[20];
    for (size_t i=0;i<20u;++i)
        page[i]=(MusaDwg1032ObjectLocation){77u,88u,99u};
    MusaDwg1032ObjectLocation saved_page[20];
    memcpy(saved_page,page,sizeof(page));
    size_t written=777u,total=888u;
    assert(!musa_dwg1032_record_batch_extract_page(
        &index,&section,(N/2u)-10u,1u,page,20u,&written,&total));
    assert(memcmp(saved_page,page,sizeof(page))==0);
    assert(written==777u && total==888u);
    free(snapshot);
    free(order);free(out);free(spans);free(bytes);
}
int main(void) {three_candidates();large_pafta();return 0;}
