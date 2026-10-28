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
    return 0;
}
