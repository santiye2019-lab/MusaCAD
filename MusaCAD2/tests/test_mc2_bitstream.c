#include "mc2_bitstream.h"
#include <assert.h>
#include <stdint.h>
int main(void) {
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
    return 0;
}
