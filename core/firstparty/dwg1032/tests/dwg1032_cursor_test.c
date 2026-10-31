/* SPDX-License-Identifier: MIT */
#include "musa_dwg1032_cursor.h"
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <assert.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>

static void bit_fields(void) {
    const uint8_t data[]={0xb2,0x63,0x5a,0x81,0x23,0x45,0x67,0x89};
    MusaDwg1032Cursor c;
    uint32_t value=0;
    assert(musa_dwg1032_cursor_init(&c,data,sizeof data));
    assert(musa_dwg1032_cursor_read_bits(&c,3,&value) && value==5);
    assert(musa_dwg1032_cursor_read_bits(&c,7,&value) && value==0x49);
    assert(musa_dwg1032_cursor_read_bits(&c,6,&value) && value==0x23);
    assert(c.bit_position==16);
    assert(musa_dwg1032_cursor_read_bits(&c,32,&value) && value==0x5a812345u);
    assert(c.bit_position==48);
    assert(musa_dwg1032_cursor_skip_bits(&c,16) && c.bit_position==64);
    value=777;
    assert(!musa_dwg1032_cursor_read_bits(&c,1,&value));
    assert(value==777 && c.bit_position==64);
    assert(!musa_dwg1032_cursor_skip_bits(&c,SIZE_MAX));
    assert(c.bit_position==64);
    assert(!musa_dwg1032_cursor_read_bits(&c,0,&value));
    assert(!musa_dwg1032_cursor_read_bits(&c,33,&value));
    assert(!musa_dwg1032_cursor_read_bits(&c,1,NULL));
    assert(value==777 && c.bit_position==64);
}
static void byte_fields(void) {
    const uint8_t data[]={0x34,0x12,0xef,0xcd,0xab,0x89,0x55};
    MusaDwg1032Cursor c;
    uint16_t a=0;
    uint32_t b=0;
    assert(musa_dwg1032_cursor_init(&c,data,sizeof data));
    assert(musa_dwg1032_cursor_read_u16le(&c,&a) && a==0x1234u);
    assert(musa_dwg1032_cursor_read_u32le(&c,&b) && b==0x89abcdefu);
    assert(c.bit_position==48);
    a=99; b=99;
    assert(!musa_dwg1032_cursor_read_u16le(&c,&a));
    assert(!musa_dwg1032_cursor_read_u32le(&c,&b));
    assert(a==99 && b==99 && c.bit_position==48);
    assert(musa_dwg1032_cursor_read_bits(&c,1,&b));
    assert(!musa_dwg1032_cursor_read_u16le(&c,&a));
    assert(c.bit_position==49 && a==99);
}
static void slices_and_bad_state(void) {
    const uint8_t data[]={1,2,3,4,5,6};
    MusaDwg1032Cursor parent, child, unchanged;
    assert(musa_dwg1032_cursor_init(&parent,data,sizeof data));
    assert(musa_dwg1032_cursor_slice(&parent,2,3,&child));
    assert(child.bytes==data+2 && child.byte_count==3 &&
           child.bit_position==0 && parent.bit_position==0);
    uint32_t value=0;
    assert(musa_dwg1032_cursor_read_bits(&child,8,&value) && value==3);
    unchanged=child;
    assert(!musa_dwg1032_cursor_slice(&parent,4,3,&child));
    assert(unchanged.bytes==child.bytes &&
           unchanged.byte_count==child.byte_count &&
           unchanged.bit_position==child.bit_position);
    assert(!musa_dwg1032_cursor_slice(&parent,SIZE_MAX,1,&child));
    assert(!musa_dwg1032_cursor_slice(&parent,0,2,&parent));
    assert(!musa_dwg1032_cursor_init(&parent,data,SIZE_MAX));
    assert(!musa_dwg1032_cursor_init(&parent,NULL,1));
    assert(!musa_dwg1032_cursor_init(&parent,(const uint8_t *)&parent,
                                     sizeof parent));
    assert(musa_dwg1032_cursor_init(&parent,NULL,0));
    assert(musa_dwg1032_cursor_slice(&parent,0,0,&child));
    assert(child.bytes==NULL && child.byte_count==0);
    assert(!musa_dwg1032_cursor_read_bits(&child,1,&value));
    parent.bit_position=1;
    assert(!musa_dwg1032_cursor_skip_bits(&parent,0));
    assert(!musa_dwg1032_cursor_read_bits(&parent,1,&value));
    parent.bit_position=0;
    parent.byte_count=SIZE_MAX;
    assert(!musa_dwg1032_cursor_skip_bits(&parent,0));
}
static void exhaustive_small_buffers(void) {
    uint8_t data[4]={0xa5,0x5a,0x81,0x7e};
    for (size_t n=0;n<=sizeof data;++n) {
        for (size_t offset=0;offset<=n*8u;++offset) {
            MusaDwg1032Cursor c;
            assert(musa_dwg1032_cursor_init(&c,data,n));
            assert(musa_dwg1032_cursor_skip_bits(&c,offset));
            for (unsigned bits=1;bits<=32;++bits) {
                MusaDwg1032Cursor copy=c;
                uint32_t value=0xdeadbeefu;
                int ok=musa_dwg1032_cursor_read_bits(&copy,bits,&value);
                assert(ok==(bits<=n*8u-offset));
                if (ok) {
                    uint32_t expected=0;
                    for (unsigned i=0;i<bits;++i) {
                        size_t at=offset+i;
                        expected=(expected<<1u)|
                            ((data[at/8u]>>(7u-at%8u))&1u);
                    }
                    assert(value==expected && copy.bit_position==offset+bits);
                } else {
                    assert(value==0xdeadbeefu && copy.bit_position==offset);
                }
            }
        }
    }
}
static void raw_byte_reads(void) {
    const uint8_t data[]={0,0xff,0x81,0x42,0};
    MusaDwg1032Cursor c;
    uint8_t out[4]={9,9,9,9}, v=7;
    assert(musa_dwg1032_cursor_init(&c,data,sizeof data));
    assert(musa_dwg1032_cursor_read_u8(&c,&v) && v==0);
    assert(c.bit_position==8);
    assert(musa_dwg1032_cursor_read_bytes(&c,out,3));
    assert(out[0]==0xff && out[1]==0x81 && out[2]==0x42);
    assert(out[3]==9 && c.bit_position==32);
    assert(!musa_dwg1032_cursor_read_bytes(&c,out,2));
    assert(out[0]==0xff && c.bit_position==32);
    assert(musa_dwg1032_cursor_read_bytes(&c,NULL,0));
    assert(c.bit_position==32);
    assert(!musa_dwg1032_cursor_read_bytes(&c,NULL,1));
    assert(!musa_dwg1032_cursor_read_bytes(&c,out,SIZE_MAX));
    assert(!musa_dwg1032_cursor_read_bytes(&c,(uint8_t *)&c,1));
    assert(!musa_dwg1032_cursor_read_bytes(&c,(uint8_t *)data,1));
    assert(c.bit_position==32);
    assert(musa_dwg1032_cursor_read_u8(&c,&v) && v==0);
    assert(c.bit_position==40);
    assert(!musa_dwg1032_cursor_read_u8(&c,&v) && v==0);
    assert(musa_dwg1032_cursor_init(&c,data,sizeof data));
    assert(musa_dwg1032_cursor_skip_bits(&c,1));
    assert(!musa_dwg1032_cursor_read_u8(&c,&v) && c.bit_position==1);
    assert(!musa_dwg1032_cursor_read_bytes(&c,out,0));
    assert(musa_dwg1032_cursor_init(&c,NULL,0));
    assert(musa_dwg1032_cursor_read_bytes(&c,NULL,0));
    assert(!musa_dwg1032_cursor_read_u8(&c,&v));
}
int main(void) {
    bit_fields(); byte_fields(); raw_byte_reads(); slices_and_bad_state();
    exhaustive_small_buffers();
    return 0;
}
