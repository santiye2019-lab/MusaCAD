/* SPDX-License-Identifier: MIT
 * Independent bounded-record text field offset and rollback regressions.
 */
#include "musa_scene2d_utf16.h"
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

static void rejected(const uint8_t *record, size_t bytes,
                     size_t offset, size_t units) {
    uint8_t out[24];
    memset(out,0xa5,sizeof out);
    size_t written=111u, consumed=222u;
    assert(!musa_scene2d_utf16le_counted_at_to_utf8(
        record,bytes,offset,units,out,sizeof out,&written,&consumed));
    assert(written==111u && consumed==222u);
    for (size_t i=0;i<sizeof out;++i) assert(out[i]==0xa5u);
    assert(!musa_scene2d_utf16le_counted_at_to_utf8(
        record,bytes,offset,units,NULL,0,&written,&consumed));
    assert(written==111u && consumed==222u);
}

static void offset_and_surrogate(void) {
    /* Three prefix bytes, three UTF-16 code units, then two opaque bytes.
     * The odd field offset is intentional: UTF-16LE alignment is relative
     * to the verified field, not the start of the enclosing object. */
    const uint8_t record[]={0xaau,0xbbu,0xccu,
        0x41u,0u,0x3du,0xd8u,0u,0xdeu,0x55u,0xffu};
    const uint8_t expected[]={0x41u,0xf0u,0x9fu,0x98u,0x80u};
    uint8_t out[24];
    memset(out,0xa5,sizeof out);
    size_t written=111u, consumed=222u;
    assert(musa_scene2d_utf16le_counted_at_to_utf8(
        record,sizeof record,3u,3u,NULL,0,&written,&consumed));
    assert(written==sizeof expected && consumed==9u);
    written=111u; consumed=222u;
    assert(musa_scene2d_utf16le_counted_at_to_utf8(
        record,sizeof record,3u,3u,out,sizeof out,&written,&consumed));
    assert(written==sizeof expected && consumed==9u &&
           !memcmp(out,expected,sizeof expected) && out[written]==0u &&
           out[written+1u]==0xa5u);
    written=111u; consumed=222u;
    memset(out,0xa5,sizeof out);
    assert(!musa_scene2d_utf16le_counted_at_to_utf8(
        record,sizeof record,3u,3u,out,sizeof expected,
        &written,&consumed));
    assert(written==111u && consumed==222u);
    for (size_t i=0;i<sizeof out;++i) assert(out[i]==0xa5u);
    rejected(record,sizeof record,3u,2u); /* lone high surrogate */
    rejected(record,sizeof record,12u,0u); /* beyond enclosing record */
    rejected(record,sizeof record,3u,5u); /* too many code units */
    rejected(record,SIZE_MAX,3u,3u);
    rejected(record,sizeof record,SIZE_MAX,0u);
    rejected(record,sizeof record,3u,SIZE_MAX);
    const uint8_t malformed[]={0x99u,0x00u,0xd8u,0x41u,0u};
    rejected(malformed,sizeof malformed,1u,2u);
    written=111u; consumed=222u;
    assert(!musa_scene2d_utf16le_counted_at_to_utf8(
        record,sizeof record,3u,3u,(uint8_t *)(void *)record,
        sizeof record,&written,&consumed));
    assert(!musa_scene2d_utf16le_counted_at_to_utf8(
        record,sizeof record,3u,3u,out,sizeof out,
        &written,&written));
    assert(!musa_scene2d_utf16le_counted_at_to_utf8(
        record,sizeof record,3u,3u,out,sizeof out,
        NULL,&consumed));
    assert(!musa_scene2d_utf16le_counted_at_to_utf8(
        record,sizeof record,3u,3u,out,sizeof out,
        &written,NULL));
    assert(written==111u && consumed==222u);
    for (size_t i=0;i<sizeof out;++i) assert(out[i]==0xa5u);
    /* An empty field at the end of a nonempty record is legitimate. */
    assert(musa_scene2d_utf16le_counted_at_to_utf8(
        record,sizeof record,sizeof record,0u,out,sizeof out,
        &written,&consumed));
    assert(written==0u && consumed==sizeof record && out[0]==0u);
    assert(musa_scene2d_utf16le_counted_at_to_utf8(
        NULL,0u,0u,0u,out,sizeof out,&written,&consumed));
    assert(written==0u && consumed==0u && out[0]==0u);
}

static void large_bounded_field(void) {
    const size_t prefix=17u, units=20000u;
    const size_t record_bytes=prefix+units*2u+1u;
    uint8_t *record=(uint8_t *)malloc(record_bytes);
    uint8_t *out=(uint8_t *)malloc(units*3u+1u);
    assert(record && out);
    memset(record,0xadu,prefix);
    for (size_t i=0;i<units;++i) {
        record[prefix+2u*i]=0xacu;
        record[prefix+2u*i+1u]=0x20u;
    }
    record[record_bytes-1u]=0xffu;
    size_t written=111u, consumed=222u;
    assert(musa_scene2d_utf16le_counted_at_to_utf8(
        record,record_bytes,prefix,units,NULL,0,&written,&consumed));
    assert(written==units*3u && consumed==prefix+units*2u);
    memset(out,0xa5u,units*3u+1u);
    assert(musa_scene2d_utf16le_counted_at_to_utf8(
        record,record_bytes,prefix,units,out,units*3u+1u,
        &written,&consumed));
    assert(written==units*3u && consumed==prefix+units*2u &&
           out[0]==0xe2u && out[1]==0x82u && out[2]==0xacu &&
           out[written-1u]==0xacu && out[written]==0u);
    free(out);
    free(record);
}

int main(void) {
    offset_and_surrogate();
    large_bounded_field();
    return 0;
}
