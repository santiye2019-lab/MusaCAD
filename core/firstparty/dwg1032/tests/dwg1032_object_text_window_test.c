/* SPDX-License-Identifier: MIT
 * Synthetic RAW-page and UTF-16LE field tests; NOT authentic DWG fixtures.
 */
#include "musa_dwg1032_object_text_window.h"
#include "musa_dwg1032_object_spans.h"
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <assert.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>

enum { N=20000, PAYLOAD=2*N, FILE_BYTES=PAYLOAD+4, HALF=FILE_BYTES/2 };
static uint8_t file[FILE_BYTES], scratch[PAYLOAD];
static char utf8[3*N+1];

static void put_logical(size_t offset, uint8_t byte) {
    /* Two reversed physical pages, with the MS prefix split at the seam
     * when a smaller test chooses a different page directory. */
    assert(offset<FILE_BYTES);
    file[offset<HALF ? HALF+offset : offset-HALF]=byte;
}
static void large_cross_page_text(void) {
    MusaDwg1032FilePage pages[]={
        {0,HALF,HALF},{HALF,0,FILE_BYTES-HALF}
    };
    MusaDwg1032ObjectLocation entries[]={{11,0,FILE_BYTES}};
    MusaDwg1032ObjectIndex index={0};
    size_t order[2];
    /* Canonical extended MS length: 40000 = 0x1c40 + (1 << 15). */
    put_logical(0,0x40); put_logical(1,0x9c);
    put_logical(2,1); put_logical(3,0);
    for (size_t i=0;i<N;++i) {
        put_logical(4u+2u*i,0xacu);
        put_logical(5u+2u*i,0x20u);
    }
    assert(musa_dwg1032_file_pages_validate_window_directory(
        pages,2,FILE_BYTES,FILE_BYTES,order,2));
    assert(musa_dwg1032_object_index_init(
        &index,entries,1,FILE_BYTES));
    assert(musa_dwg1032_object_spans_prepare(&index,order,2));
    size_t written=99;
    memset(utf8,0x5a,sizeof(utf8));
    assert(musa_dwg1032_object_ms_utf16le_text(
        file,FILE_BYTES,pages,2,&index,11,0,PAYLOAD,PAYLOAD,
        scratch,sizeof(scratch),utf8,sizeof(utf8),&written));
    assert(written==3u*N && utf8[written]==0);
    for (size_t i=0;i<N;++i) {
        assert((unsigned char)utf8[3u*i]==0xe2u);
        assert((unsigned char)utf8[3u*i+1u]==0x82u);
        assert((unsigned char)utf8[3u*i+2u]==0xacu);
    }
    /* The ordinal fast path must produce byte-for-byte identical UTF-8
     * for a large cross-page field, without an extra handle lookup. */
    memset(utf8,0x5a,sizeof(utf8));
    written=91u;
    assert(musa_dwg1032_object_ms_utf16le_text_at_position(
        file,FILE_BYTES,pages,2,&index,0,0,PAYLOAD,PAYLOAD,
        scratch,sizeof(scratch),utf8,sizeof(utf8),&written));
    assert(written==3u*N && utf8[written]=='\0');
    for (size_t i=0;i<N;++i) {
        assert((unsigned char)utf8[3u*i]==0xe2u);
        assert((unsigned char)utf8[3u*i+1u]==0x82u);
        assert((unsigned char)utf8[3u*i+2u]==0xacu);
    }
    /* A position is an index ordinal, never a byte offset or handle. */
    written=123u;
    memset(utf8,0x5a,sizeof(utf8));
    assert(!musa_dwg1032_object_ms_utf16le_text_at_position(
        file,FILE_BYTES,pages,2,&index,1,0,PAYLOAD,PAYLOAD,
        scratch,sizeof(scratch),utf8,sizeof(utf8),&written));
    assert(!musa_dwg1032_object_ms_utf16le_text_at_position(
        file,FILE_BYTES,pages,2,&index,SIZE_MAX,0,PAYLOAD,PAYLOAD,
        scratch,sizeof(scratch),utf8,sizeof(utf8),&written));
    assert(written==123u && utf8[0]=='Z' && utf8[3u*N]=='Z');
    /* A malformed LAST scalar must not expose earlier valid geometry text. */
    put_logical(FILE_BYTES-2u,0);
    put_logical(FILE_BYTES-1u,0xd8);
    written=77;
    memset(utf8,0x5a,sizeof(utf8));
    assert(!musa_dwg1032_object_ms_utf16le_text(
        file,FILE_BYTES,pages,2,&index,11,0,PAYLOAD,PAYLOAD,
        scratch,sizeof(scratch),utf8,sizeof(utf8),&written));
    assert(written==77 && utf8[0]=='Z' &&
           utf8[3u*N/2u]=='Z' && utf8[3u*N]=='Z');
    /* The ordinal path must also fail atomically on a late surrogate,
     * including when its UTF-8 buffer could hold the entire valid text. */
    written=141u;
    memset(utf8,0x5a,sizeof(utf8));
    assert(!musa_dwg1032_object_ms_utf16le_text_at_position(
        file,FILE_BYTES,pages,2,&index,0,0,PAYLOAD,PAYLOAD,
        scratch,sizeof(scratch),utf8,sizeof(utf8),&written));
    assert(written==141u && utf8[0]=='Z' &&
           utf8[3u*N/2u]=='Z' && utf8[3u*N]=='Z');
    put_logical(FILE_BYTES-2u,0xac);
    put_logical(FILE_BYTES-1u,0x20);

    assert(!musa_dwg1032_object_ms_utf16le_text(
        file,FILE_BYTES,pages,2,&index,12,0,PAYLOAD,PAYLOAD,
        scratch,sizeof(scratch),utf8,sizeof(utf8),&written));
    assert(!musa_dwg1032_object_ms_utf16le_text(
        file,FILE_BYTES,pages,2,&index,11,0,PAYLOAD,PAYLOAD,
        scratch,PAYLOAD-1u,utf8,sizeof(utf8),&written));
    assert(!musa_dwg1032_object_ms_utf16le_text(
        file,FILE_BYTES,pages,2,&index,11,0,PAYLOAD-1u,PAYLOAD,
        scratch,sizeof(scratch),utf8,sizeof(utf8),&written));
    assert(!musa_dwg1032_object_ms_utf16le_text(
        file,FILE_BYTES,pages,2,&index,11,0,PAYLOAD,PAYLOAD,
        scratch,sizeof(scratch),utf8,3u*N,&written));
    assert(!musa_dwg1032_object_ms_utf16le_text(
        file,FILE_BYTES,pages,2,&index,11,0,PAYLOAD,PAYLOAD,
        scratch,sizeof(scratch),(char *)(void *)file,FILE_BYTES,&written));
    assert(!musa_dwg1032_object_ms_utf16le_text(
        file,FILE_BYTES,pages,2,&index,11,0,PAYLOAD,PAYLOAD,
        scratch,sizeof(scratch),utf8,sizeof(utf8),
        (size_t *)(void *)scratch));
    assert(!musa_dwg1032_object_ms_utf16le_text(
        file,FILE_BYTES,pages,2,&index,11,0,PAYLOAD,PAYLOAD,
        (uint8_t *)(void *)file,PAYLOAD,utf8,sizeof(utf8),&written));
    assert(written==77 && utf8[0]=='Z');
    pages[1].file_offset=FILE_BYTES;
    assert(!musa_dwg1032_object_ms_utf16le_text(
        file,FILE_BYTES,pages,2,&index,11,0,PAYLOAD,PAYLOAD,
        scratch,sizeof(scratch),utf8,sizeof(utf8),&written));
    assert(written==77 && utf8[0]=='Z');
}
int main(void) {
    large_cross_page_text();
    return 0;
}
