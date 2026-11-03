/* SPDX-License-Identifier: MIT
 * Synthetic, first-party bounded 2BD/3BD field regressions. These are
 * NOT independent authentic AC1032 entity acceptance fixtures.
 */
#include "musa_dwg1032_point_batch.h"
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <assert.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>

#define N 20000u
typedef struct Bits { uint8_t data[20000]; size_t bit_count; } Bits;
static Bits encoded;
static MusaDwg1032Point2 out2[N], scratch2[N];
static MusaDwg1032Point3 out3[N], scratch3[N];

static void append(Bits *b, uint64_t v, unsigned n) {
    for (unsigned i=0;i<n;++i) {
        const size_t at=b->bit_count++;
        assert(at<sizeof(b->data)*8u);
        if ((v>>(n-1u-i))&1u)
            b->data[at/8u]|=(uint8_t)(1u<<(7u-at%8u));
    }
}
static void little(Bits *b, uint64_t v, unsigned count) {
    for (unsigned i=0;i<count;++i) append(b,(v>>(8u*i))&255u,8u);
}
static void fill2(double x,double y) {
    for (size_t i=0;i<N;++i) { out2[i].x=x; out2[i].y=y; }
}
static void assert2(double x,double y) {
    for (size_t i=0;i<N;++i)
        assert(out2[i].x==x && out2[i].y==y);
}
static void batch_20k_and_reference(void) {
    memset(&encoded,0,sizeof(encoded));
    append(&encoded,5u,3u); /* Unaligned start. */
    for (size_t i=0;i<N;++i) {
        append(&encoded,i&1u?1u:2u,2u);
        append(&encoded,i&1u?2u:1u,2u);
    }
    MusaDwg1032Cursor c,reference;
    assert(musa_dwg1032_cursor_init(&c,encoded.data,
                                   (encoded.bit_count+7u)/8u));
    assert(musa_dwg1032_cursor_skip_bits(&c,3u));
    reference=c;
    assert(musa_dwg1032_points2bd_batch(&c,out2,N,scratch2,N));
    assert(c.bit_position==encoded.bit_count);
    for (size_t i=0;i<N;++i) {
        MusaDwg1032Point2 single={-8,-9};
        assert(musa_dwg1032_point2bd(&reference,&single));
        assert(out2[i].x==single.x && out2[i].y==single.y);
        assert(out2[i].x==(i&1u?1.0:0.0));
        assert(out2[i].y==(i&1u?0.0:1.0));
    }
    assert(reference.bit_position==c.bit_position);
    assert(!musa_dwg1032_points2bd_batch(&c,out2,1,scratch2,N));
}
static void rollback_after_late_invalid(void) {
    memset(&encoded,0,sizeof(encoded));
    for (size_t i=0;i<N;++i) {
        append(&encoded,1u,2u);
        append(&encoded,i==N-1u?3u:2u,2u);
    }
    fill2(-7.0,9.0);
    MusaDwg1032Cursor c;
    assert(musa_dwg1032_cursor_init(&c,encoded.data,
                                   (encoded.bit_count+7u)/8u));
    assert(!musa_dwg1032_points2bd_batch(&c,out2,N,scratch2,N));
    assert(c.bit_position==0u);
    assert2(-7.0,9.0);
    /* A truncated last point also must not publish any earlier geometry. */
    encoded.data[(encoded.bit_count-2u)/8u]&=0xffu;
    assert(musa_dwg1032_cursor_init(&c,encoded.data,
                                   (encoded.bit_count+7u)/8u-1u));
    assert(!musa_dwg1032_points2bd_batch(&c,out2,N,scratch2,N));
    assert(c.bit_position==0u);
    assert2(-7.0,9.0);
}
static void triples_and_rejections(void) {
    memset(&encoded,0,sizeof(encoded));
    append(&encoded,7u,3u);
    for (size_t i=0;i<128u;++i) {
        append(&encoded,0u,2u);
        little(&encoded,UINT64_C(0x400c000000000000),8u); /* 3.5 */
        append(&encoded,1u,2u);
        append(&encoded,2u,2u);
    }
    MusaDwg1032Cursor c;
    assert(musa_dwg1032_cursor_init(&c,encoded.data,
                                   (encoded.bit_count+7u)/8u));
    assert(musa_dwg1032_cursor_skip_bits(&c,3u));
    assert(musa_dwg1032_points3bd_batch(&c,out3,128u,scratch3,N));
    assert(c.bit_position==encoded.bit_count);
    for (size_t i=0;i<128u;++i)
        assert(out3[i].x==3.5 && out3[i].y==1.0 && out3[i].z==0.0);

    assert(musa_dwg1032_cursor_init(&c,encoded.data,
                                   (encoded.bit_count+7u)/8u));
    assert(musa_dwg1032_cursor_skip_bits(&c,3u));
    for (size_t i=0;i<128u;++i) {
        out3[i].x=-1.0;out3[i].y=-2.0;out3[i].z=-3.0;
    }
    assert(!musa_dwg1032_points3bd_batch(&c,out3,129u,scratch3,N));
    assert(c.bit_position==3u);
    for (size_t i=0;i<128u;++i)
        assert(out3[i].x==-1.0 && out3[i].y==-2.0 && out3[i].z==-3.0);
    assert(!musa_dwg1032_points3bd_batch(&c,out3,128u,out3,N));
    assert(!musa_dwg1032_points3bd_batch(&c,out3,128u,scratch3,127u));
    assert(!musa_dwg1032_points3bd_batch(&c,out3,0u,scratch3,N));
    assert(!musa_dwg1032_points3bd_batch(&c,out3,SIZE_MAX,scratch3,N));
    assert(!musa_dwg1032_points3bd_batch(&c,out3,128u,
            (MusaDwg1032Point3 *)(void *)encoded.data,N));
    assert(c.bit_position==3u);
}
int main(void) {
    batch_20k_and_reference();
    rollback_after_late_invalid();
    triples_and_rejections();
    return 0;
}
