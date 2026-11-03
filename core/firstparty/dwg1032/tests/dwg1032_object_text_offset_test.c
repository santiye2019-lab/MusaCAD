/* SPDX-License-Identifier: MIT
 * Synthetic cross-page Unicode field offset, rollback and alias tests.
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

static void put(uint8_t *file, size_t logical, uint8_t value) {
    assert(logical<14u);
    file[logical<7u ? logical+7u : logical-7u]=value;
}
static void offset_surrogate_and_empty(void) {
    uint8_t file[14]={0};
    const uint8_t logical[]={
        12,0,0x11,0x22,'A',0,0x3d,0xd8,0,0xde,
        0x55,0x66,0x77,0x88
    };
    for (size_t i=0;i<sizeof(logical);++i) put(file,i,logical[i]);
    MusaDwg1032FilePage pages[]={{0,7,7},{7,0,7}};
    MusaDwg1032ObjectLocation entries[]={{19,0,14}};
    MusaDwg1032ObjectIndex index={0};
    size_t order[2];
    assert(musa_dwg1032_file_pages_validate_window_directory(
        pages,2,sizeof(file),sizeof(file),order,2));
    assert(musa_dwg1032_object_index_init(
        &index,entries,1,sizeof(file)));
    assert(musa_dwg1032_object_spans_prepare(&index,order,2));
    uint8_t scratch[8]={0};
    char output[16];
    size_t written=77;
    memset(output,0x5a,sizeof(output));
    assert(musa_dwg1032_object_ms_utf16le_text(
        file,sizeof(file),pages,2,&index,19,2,6,12,
        scratch,sizeof(scratch),output,sizeof(output),&written));
    const char expected[]="A\xf0\x9f\x98\x80";
    assert(written==sizeof(expected)-1u);
    assert(memcmp(output,expected,sizeof(expected))==0);
    assert(output[written+1u]=='Z');

    written=77;
    memset(output,0x5a,sizeof(output));
    put(file,9,0); /* Invalid low surrogate in second physical page. */
    assert(!musa_dwg1032_object_ms_utf16le_text(
        file,sizeof(file),pages,2,&index,19,2,6,12,
        scratch,sizeof(scratch),output,sizeof(output),&written));
    assert(written==77 && output[0]=='Z' && output[15]=='Z');
    put(file,9,0xde);
    assert(!musa_dwg1032_object_ms_utf16le_text(
        file,sizeof(file),pages,2,&index,19,8,6,12,
        scratch,sizeof(scratch),output,sizeof(output),&written));
    assert(!musa_dwg1032_object_ms_utf16le_text(
        file,sizeof(file),pages,2,&index,19,2,5,12,
        scratch,sizeof(scratch),output,sizeof(output),&written));
    assert(!musa_dwg1032_object_ms_utf16le_text(
        file,sizeof(file),pages,2,&index,19,2,6,11,
        scratch,sizeof(scratch),output,sizeof(output),&written));
    assert(!musa_dwg1032_object_ms_utf16le_text(
        file,sizeof(file),pages,2,&index,19,2,6,12,
        scratch,sizeof(scratch),output,5,&written));
    assert(!musa_dwg1032_object_ms_utf16le_text(
        file,sizeof(file),pages,2,&index,19,2,6,12,
        scratch,sizeof(scratch),output,sizeof(output),
        (size_t *)(void *)pages));
    assert(!musa_dwg1032_object_ms_utf16le_text(
        file,sizeof(file),pages,2,&index,19,2,6,12,
        scratch,sizeof(scratch),output,sizeof(output),
        (size_t *)(void *)&index));
    assert(!musa_dwg1032_object_ms_utf16le_text(
        file,sizeof(file),pages,2,&index,19,2,6,12,
        scratch,sizeof(scratch),(char *)(void *)scratch,
        sizeof(scratch),&written));
    assert(written==77 && output[0]=='Z');

    /* Empty authenticated field at the exact payload end: no scratch. */
    assert(musa_dwg1032_object_ms_utf16le_text(
        file,sizeof(file),pages,2,&index,19,12,0,12,
        NULL,0,output,1,&written));
    assert(written==0 && output[0]==0 && output[1]=='Z');
}
int main(void) {
    offset_surrogate_and_empty();
    return 0;
}
