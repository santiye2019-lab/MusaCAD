/* SPDX-License-Identifier: MIT
 * Synthetic map-to-record regressions; not a real DWG fixture.
 */
#include "musa_dwg1032_record_batch.h"
#include "musa_dwg1032_object_map.h"
#include "musa_dwg1032_object_spans.h"
#include "musa_dwg1032_section_assembly.h"
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
/* Handle-based viewport lookup is independent of physical record order.
 * Inputs are synthetic MS envelopes, not authenticated DWG entities. */
static void handle_lookup(void) {
    uint8_t bytes[]={1,0,0x21,0xee, 1,0,0x32,0xdd,
                     1,0,0x43,0xcc};
    const MusaDwg1032ObjectLocation spans[]={
        {2u,8u,4u},{8u,0u,4u},{99u,4u,4u}
    };
    const MusaDwg1032ObjectIndex index={spans,3u,sizeof(bytes)};
    const MusaDwg1032Cursor section={bytes,sizeof(bytes),0u};
    size_t order[3];
    assert(musa_dwg1032_object_spans_prepare(&index,order,3u));
    MusaDwg1032ObjectLocation out={777u,888u,999u};
    assert(musa_dwg1032_record_batch_extract_handle(
        &index,&section,8u,1u,&out));
    assert(out.handle==8u && out.offset==2u && out.length==1u);
    assert(musa_dwg1032_record_batch_extract_handle(
        &index,&section,2u,1u,&out));
    assert(out.handle==2u && out.offset==10u && out.length==1u);
    assert(musa_dwg1032_record_batch_extract_handle(
        &index,&section,99u,1u,&out));
    assert(out.handle==99u && out.offset==6u && out.length==1u);
    const MusaDwg1032ObjectLocation saved=out;
    assert(!musa_dwg1032_record_batch_extract_handle(
        &index,&section,0u,1u,&out));
    assert(!musa_dwg1032_record_batch_extract_handle(
        &index,&section,100u,1u,&out));
    assert(!musa_dwg1032_record_batch_extract_handle(
        &index,&section,99u,0u,&out));
    bytes[4]=0u; /* A malformed target must not publish partial output. */
    assert(!musa_dwg1032_record_batch_extract_handle(
        &index,&section,99u,1u,&out));
    /* A bad unrelated record must not block a valid visible handle. */
    assert(musa_dwg1032_record_batch_extract_handle(
        &index,&section,2u,1u,&out));
    assert(out.handle==2u && out.offset==10u);
    out=saved;
    bytes[4]=1u;
    const MusaDwg1032Cursor misaligned={bytes,sizeof(bytes),1u};
    assert(!musa_dwg1032_record_batch_extract_handle(
        &index,&misaligned,99u,1u,&out));
    assert(!musa_dwg1032_record_batch_extract_handle(
        &index,&section,99u,1u,(MusaDwg1032ObjectLocation *)(void *)bytes));
    assert(!musa_dwg1032_record_batch_extract_handle(
        &index,&section,99u,1u,(MusaDwg1032ObjectLocation *)(void *)spans));
    assert(!musa_dwg1032_record_batch_extract_handle(
        &index,&section,99u,1u,NULL));
    assert(memcmp(&out,&saved,sizeof(out))==0);
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
    /* Sparse viewport requests on a 20k-object index use O(log n)
     * handle lookup, without copying the full result array. */
    MusaDwg1032ObjectLocation picked={999u,999u,999u};
    for (size_t i=0;i<N;i+=97u) {
        assert(musa_dwg1032_record_batch_extract_handle(
            &index,&section,i+1u,1u,&picked));
        assert(picked.handle==out[i].handle &&
               picked.offset==out[i].offset &&
               picked.length==out[i].length);
    }
    const MusaDwg1032ObjectLocation saved_pick=picked;
    assert(!musa_dwg1032_record_batch_extract_handle(
        &index,&section,N+1u,1u,&picked));
    assert(memcmp(&picked,&saved_pick,sizeof(picked))==0);
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

/* Integrated, synthetic map-to-framed-record proof. Inputs represent
 * independently verified/decompressed sections, not a real DWG fixture. */
static void verified_map_pipeline(void) {
    uint8_t map[]={1u,10u,2u,0x45u,3u,20u};
    uint8_t bytes[30]={0};
    const size_t starts[]={10u,5u,25u};
    for (size_t i=0;i<3u;++i) {
        bytes[starts[i]]=1u;
        bytes[starts[i]+1u]=0u;
        bytes[starts[i]+2u]=(uint8_t)(0x41u+i);
    }
    const MusaDwg1032Cursor section={bytes,sizeof(bytes),0u};
    MusaDwg1032MapEntry entries[3];
    MusaDwg1032ObjectLocation candidates[3];
    MusaDwg1032ObjectLocation out[3]={{99u,99u,99u},
                                       {99u,99u,99u},{99u,99u,99u}};
    size_t order[3];
    assert(musa_dwg1032_record_batch_from_verified_map(
        map,sizeof(map),3u,&section,1u,entries,3u,candidates,3u,
        out,3u,order,3u));
    const uint64_t handles[]={1u,3u,6u};
    const uint64_t offsets[]={12u,7u,27u};
    for (size_t i=0;i<3u;++i)
        assert(out[i].handle==handles[i] &&
               out[i].offset==offsets[i] && out[i].length==1u);
    MusaDwg1032ObjectLocation saved[3];
    memcpy(saved,out,sizeof(saved));
    bytes[25]=0u; /* Last candidate malformed; no partial publication. */
    assert(!musa_dwg1032_record_batch_from_verified_map(
        map,sizeof(map),3u,&section,1u,entries,3u,candidates,3u,
        out,3u,order,3u));
    assert(memcmp(out,saved,sizeof(saved))==0);
    bytes[25]=1u;
    map[5]=0x40u; /* Noncanonical signed negative zero. */
    assert(!musa_dwg1032_record_batch_from_verified_map(
        map,sizeof(map),3u,&section,1u,entries,3u,candidates,3u,
        out,3u,order,3u));
    assert(memcmp(out,saved,sizeof(saved))==0);
    map[5]=20u;
    assert(!musa_dwg1032_record_batch_from_verified_map(
        map,sizeof(map),3u,&section,1u,entries,3u,candidates,3u,
        out,2u,order,3u));
    assert(!musa_dwg1032_record_batch_from_verified_map(
        map,sizeof(map),3u,&section,1u,
        (MusaDwg1032MapEntry *)(void *)map,3u,candidates,3u,
        out,3u,order,3u));
    assert(!musa_dwg1032_record_batch_from_verified_map(
        map,sizeof(map),3u,&section,1u,entries,3u,candidates,3u,
        (MusaDwg1032ObjectLocation *)(void *)entries,3u,order,3u));
    assert(!musa_dwg1032_record_batch_from_verified_map(
        map,sizeof(map),3u,&section,1u,entries,3u,candidates,3u,
        out,3u,(size_t *)(void *)bytes,3u));
    assert(memcmp(out,saved,sizeof(saved))==0);
    const MusaDwg1032Cursor empty={NULL,0u,0u};
    assert(musa_dwg1032_record_batch_from_verified_map(
        NULL,0u,0u,&empty,0u,NULL,0u,NULL,0u,NULL,0u,NULL,0u));
}
static void verified_map_large_pafta(void) {
    enum { N=20000, STRIDE=4 };
    uint8_t *map=(uint8_t *)malloc(2u*N);
    uint8_t *bytes=(uint8_t *)malloc(STRIDE*N);
    MusaDwg1032MapEntry *entries=
        (MusaDwg1032MapEntry *)malloc(N*sizeof(*entries));
    MusaDwg1032ObjectLocation *candidates=
        (MusaDwg1032ObjectLocation *)malloc(N*sizeof(*candidates));
    MusaDwg1032ObjectLocation *out=
        (MusaDwg1032ObjectLocation *)malloc(N*sizeof(*out));
    size_t *order=(size_t *)malloc(N*sizeof(*order));
    assert(map && bytes && entries && candidates && out && order);
    for (size_t i=0;i<N;++i) {
        map[2u*i]=1u;
        map[2u*i+1u]=(uint8_t)(i==0u ? 0u : STRIDE);
        bytes[STRIDE*i]=1u;
        bytes[STRIDE*i+1u]=0u;
        bytes[STRIDE*i+2u]=(uint8_t)i;
        bytes[STRIDE*i+3u]=0xeeu;
    }
    const MusaDwg1032Cursor section={bytes,STRIDE*N,0u};
    assert(musa_dwg1032_record_batch_from_verified_map(
        map,2u*N,N,&section,1u,entries,N,candidates,N,out,N,order,N));
    for (size_t i=0;i<N;++i)
        assert(out[i].handle==i+1u && out[i].offset==STRIDE*i+2u &&
               out[i].length==1u);
    const MusaDwg1032ObjectLocation first=out[0], last=out[N-1u];
    bytes[STRIDE*(N-1u)]=0u;
    assert(!musa_dwg1032_record_batch_from_verified_map(
        map,2u*N,N,&section,1u,entries,N,candidates,N,out,N,order,N));
    assert(memcmp(out,&first,sizeof(first))==0);
    assert(memcmp(out+N-1u,&last,sizeof(last))==0);
    free(order);free(out);free(candidates);free(entries);free(bytes);free(map);
}


/* Reverse physical page order is independent of logical record order.
 * Still synthetic: page metadata and the map are caller-verified inputs. */
static void verified_file_pages_to_records(void) {
    const uint8_t map[]={1u,10u,2u,0x45u,3u,20u};
    uint8_t logical[30]={0};
    const size_t starts[]={10u,5u,25u};
    for (size_t i=0;i<3u;++i) {
        logical[starts[i]]=1u;
        logical[starts[i]+1u]=0u;
        logical[starts[i]+2u]=(uint8_t)(0x61u+i);
    }
    uint8_t physical[30], assembled[30];
    memcpy(physical,logical+20u,10u);
    memcpy(physical+10u,logical,10u);
    memcpy(physical+20u,logical+10u,10u);
    MusaDwg1032FilePage pages[]={
        {0u,10u,10u},{10u,20u,10u},{20u,0u,10u}
    };
    size_t physical_order[3];
    memset(assembled,0xa5,sizeof(assembled));
    assert(musa_dwg1032_section_assemble_checked_file_pages(
        physical,sizeof(physical),pages,3u,assembled,sizeof(assembled),
        physical_order,3u));
    assert(memcmp(assembled,logical,sizeof(logical))==0);
    const MusaDwg1032Cursor section={assembled,sizeof(assembled),0u};
    MusaDwg1032MapEntry entries[3];
    MusaDwg1032ObjectLocation candidates[3],payloads[3];
    size_t order[3];
    assert(musa_dwg1032_record_batch_from_verified_map(
        map,sizeof(map),3u,&section,1u,entries,3u,candidates,3u,
        payloads,3u,order,3u));
    assert(payloads[0].offset==12u && payloads[1].offset==7u &&
           payloads[2].offset==27u);
    memset(assembled,0xa5,sizeof(assembled));
    pages[2].file_offset=25u; /* Last logical page is physically truncated. */
    assert(!musa_dwg1032_section_assemble_checked_file_pages(
        physical,sizeof(physical),pages,3u,assembled,sizeof(assembled),
        physical_order,3u));
    for (size_t i=0;i<sizeof(assembled);++i)
        assert(assembled[i]==0xa5u);
}

int main(void) {three_candidates();handle_lookup();large_pafta();verified_map_pipeline();verified_map_large_pafta();verified_file_pages_to_records();return 0;}
