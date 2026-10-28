#include "mc2_bitstream.h"
#include <assert.h>
#include <stdint.h>

/* Fixture encoder used only in tests: explicit bit patterns and byte order. */
static void put_test_bits(uint8_t *bytes, size_t *position, uint64_t value, unsigned count) {
    for (unsigned i=0; i<count; ++i) {
        unsigned shift=count-i-1u;
        if ((value >> shift) & 1u)
            bytes[*position / 8u] |= (uint8_t)(0x80u >> (*position % 8u));
        ++*position;
    }
}
static void test_dwg_bitcodes(void) {
    uint8_t fixture[64] = {0};
    size_t bits = 0;
    put_test_bits(fixture,&bits,0,2); /* BS raw +257 (LE) */
    put_test_bits(fixture,&bits,1,8);
    put_test_bits(fixture,&bits,1,8);
    put_test_bits(fixture,&bits,1,2); /* BS 127 */
    put_test_bits(fixture,&bits,127,8);
    put_test_bits(fixture,&bits,2,2); /* BS zero */
    put_test_bits(fixture,&bits,3,2); /* BS 256 */
    put_test_bits(fixture,&bits,0,2); /* BS -1 */
    put_test_bits(fixture,&bits,255,8);
    put_test_bits(fixture,&bits,255,8);
    put_test_bits(fixture,&bits,0,2); /* BL -2 */
    put_test_bits(fixture,&bits,254,8);
    put_test_bits(fixture,&bits,255,8);
    put_test_bits(fixture,&bits,255,8);
    put_test_bits(fixture,&bits,255,8);
    put_test_bits(fixture,&bits,1,2); /* BL 42 */
    put_test_bits(fixture,&bits,42,8);
    put_test_bits(fixture,&bits,2,2); /* BL zero */
    put_test_bits(fixture,&bits,1,2); /* BD 1.0 */
    put_test_bits(fixture,&bits,2,2); /* BD 0.0 */
    put_test_bits(fixture,&bits,0,2); /* BD raw 2.5 */
    const uint8_t raw_two_point_five[8] = {0,0,0,0,0,0,4,0x40};
    for (unsigned i=0;i<8;++i) put_test_bits(fixture,&bits,raw_two_point_five[i],8);
    Mc2BitReader r;
    int16_t s = -100;
    int32_t l = -100;
    double d = -100;
    assert(mc2_bitreader_init(&r,fixture,(bits+7u)/8u));
    assert(mc2_read_dwg_bitshort(&r,&s) && s==257);
    assert(mc2_read_dwg_bitshort(&r,&s) && s==127);
    assert(mc2_read_dwg_bitshort(&r,&s) && s==0);
    assert(mc2_read_dwg_bitshort(&r,&s) && s==256);
    assert(mc2_read_dwg_bitshort(&r,&s) && s==-1);
    assert(mc2_read_dwg_bitlong(&r,&l) && l==-2);
    assert(mc2_read_dwg_bitlong(&r,&l) && l==42);
    assert(mc2_read_dwg_bitlong(&r,&l) && l==0);
    assert(mc2_read_dwg_bitdouble(&r,&d) && d==1.0);
    assert(mc2_read_dwg_bitdouble(&r,&d) && d==0.0);
    assert(mc2_read_dwg_bitdouble(&r,&d) && d==2.5);
    assert(r.bit_position==bits);
    /* Invalid selector and truncated payload must not advance the cursor. */
    const uint8_t truncated[] = {0x00};
    assert(mc2_bitreader_init(&r,truncated,sizeof truncated));
    assert(!mc2_read_dwg_bitshort(&r,&s) && r.bit_position==0 && s==-1);
    assert(!mc2_read_dwg_bitlong(&r,&l) && r.bit_position==0 && l==0);
    assert(!mc2_read_dwg_bitdouble(&r,&d) && r.bit_position==0 && d==2.5);
    const uint8_t reserved[] = {0xc0};
    assert(mc2_bitreader_init(&r,reserved,sizeof reserved));
    assert(!mc2_read_dwg_bitlong(&r,&l) && r.bit_position==0);
    assert(!mc2_read_dwg_bitdouble(&r,&d) && r.bit_position==0);
    uint8_t invalid_double[9] = {0};
    size_t invalid_bits=0;
    put_test_bits(invalid_double,&invalid_bits,0,2);
    for (unsigned i=0;i<6;++i) put_test_bits(invalid_double,&invalid_bits,0,8);
    put_test_bits(invalid_double,&invalid_bits,0xf0,8);
    put_test_bits(invalid_double,&invalid_bits,0x7f,8); /* +infinity */
    assert(mc2_bitreader_init(&r,invalid_double,sizeof invalid_double));
    assert(!mc2_read_dwg_bitdouble(&r,&d) && r.bit_position==0 && d==2.5);
}


static void test_dwg_handles(void) {
    const uint8_t references[] = {
        0x52,0x05,0xe7, /* hard pointer: absolute 0x5e7 */
        0x60,          /* base + 1 */
        0x80,          /* base - 1 */
        0xa1,0x05,     /* base + 5 */
        0xc1,0x05,     /* base - 5 */
        0x00           /* self handle code, zero length */
    };
    Mc2BitReader r;
    Mc2DwgHandleRef h = {99,999};
    assert(mc2_bitreader_init(&r,references,sizeof references));
    assert(mc2_read_dwg_handle(&r,100,&h) && h.code==5 && h.handle==0x5e7);
    assert(mc2_read_dwg_handle(&r,100,&h) && h.code==6 && h.handle==101);
    assert(mc2_read_dwg_handle(&r,100,&h) && h.code==8 && h.handle==99);
    assert(mc2_read_dwg_handle(&r,100,&h) && h.code==10 && h.handle==105);
    assert(mc2_read_dwg_handle(&r,100,&h) && h.code==12 && h.handle==95);
    assert(mc2_read_dwg_handle(&r,100,&h) && h.code==0 && h.handle==0);
    assert(r.bit_position==sizeof references*8u);
    const uint8_t invalid_code[] = {0xf0};
    assert(mc2_bitreader_init(&r,invalid_code,sizeof invalid_code));
    assert(!mc2_read_dwg_handle(&r,100,&h) && r.bit_position==0 && h.code==0);
    const uint8_t invalid_count[] = {0x59};
    assert(mc2_bitreader_init(&r,invalid_count,sizeof invalid_count));
    assert(!mc2_read_dwg_handle(&r,100,&h) && r.bit_position==0);
    const uint8_t truncated[] = {0x52,0x05};
    assert(mc2_bitreader_init(&r,truncated,sizeof truncated));
    assert(!mc2_read_dwg_handle(&r,100,&h) && r.bit_position==0);
    const uint8_t relative_bad[] = {0x61};
    assert(mc2_bitreader_init(&r,relative_bad,sizeof relative_bad));
    assert(!mc2_read_dwg_handle(&r,100,&h) && r.bit_position==0);
    const uint8_t plus_one[] = {0x60};
    assert(mc2_bitreader_init(&r,plus_one,sizeof plus_one));
    assert(!mc2_read_dwg_handle(&r,UINT64_MAX,&h) && r.bit_position==0);
    const uint8_t minus_one[] = {0x80};
    assert(mc2_bitreader_init(&r,minus_one,sizeof minus_one));
    assert(!mc2_read_dwg_handle(&r,0,&h) && r.bit_position==0);
    const uint8_t plus_five[] = {0xa1,0x05};
    assert(mc2_bitreader_init(&r,plus_five,sizeof plus_five));
    assert(!mc2_read_dwg_handle(&r,UINT64_MAX-4,&h) && r.bit_position==0);
    const uint8_t minus_five[] = {0xc1,0x05};
    assert(mc2_bitreader_init(&r,minus_five,sizeof minus_five));
    assert(!mc2_read_dwg_handle(&r,4,&h) && r.bit_position==0);
}

int main(void) {
    test_dwg_handles();
    test_dwg_bitcodes();
    const uint8_t data[] = {0xb2,0x5c,0xff};
    Mc2BitReader r;
    uint32_t x = 0;
    assert(mc2_bitreader_init(&r,data,sizeof data));
    assert(mc2_read_bits_msb(&r,3,&x) && x==5);
    assert(mc2_read_bits_msb(&r,5,&x) && x==18);
    assert(mc2_read_bits_msb(&r,8,&x) && x==0x5c);
    assert(mc2_bits_remaining(&r)==8);
    assert(!mc2_read_bits_msb(&r,9,&x) && r.bit_position==16);
    assert(mc2_skip_bits(&r,4));
    assert(mc2_align_byte(&r) && r.bit_position==24);
    assert(!mc2_skip_bits(&r,1));
    assert(!mc2_read_bits_msb(&r,33,&x));
    assert(mc2_bitreader_init(&r,0,0));
    assert(mc2_read_bits_msb(&r,0,&x) && x==0);
    assert(!mc2_read_bits_msb(&r,1,&x));
    assert(!mc2_bitreader_init(&r,0,1));
    const uint8_t wide[] = {0x12,0x34,0x56,0x78,0x9a,0xbc,0xde,0xf0};
    uint64_t large = 0;
    assert(mc2_bitreader_init(&r,wide,sizeof wide));
    assert(mc2_read_bits_msb64(&r,64,&large) && large == UINT64_C(0x123456789abcdef0));
    assert(!mc2_read_bits_msb64(&r,1,&large) && r.bit_position == 64);
    assert(mc2_seek_bits(&r,4));
    assert(mc2_read_bits_msb64(&r,12,&large) && large == 0x234);
    assert(!mc2_seek_bits(&r,65) && r.bit_position == 16);
    assert(!mc2_read_bits_msb64(&r,65,&large));
    const uint8_t little[] = {0x96,0x31,0xff,0xff,0xff,0xff,0xff,0xff};
    assert(mc2_bitreader_init(&r,little,sizeof little));
    assert(mc2_read_bits_lsb64(&r,4,&large) && large==6);
    assert(mc2_read_bits_lsb64(&r,8,&large) && large==0x19);
    assert(mc2_seek_bits(&r,0));
    assert(mc2_read_bits_lsb64(&r,64,&large) && large==UINT64_C(0xffffffffffff3196));
    assert(!mc2_read_bits_lsb64(&r,1,&large) && r.bit_position==64);
    assert(!mc2_read_bits_lsb64(&r,65,&large) && r.bit_position==64);
    assert(mc2_seek_bits(&r,7));
    assert(mc2_read_bits_lsb64(&r,2,&large) && large==3);
    return 0;
}
