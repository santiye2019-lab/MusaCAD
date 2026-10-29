/* SPDX-License-Identifier: MIT */
#include "musa_dwg1032_cursor.h"
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <assert.h>
#include <stddef.h>
#include <stdint.h>

int main(void) {
    const uint8_t data[]={1,2,3,4,5,6};
    MusaDwg1032Cursor parent, child;
    uint32_t v=0;
    assert(musa_dwg1032_cursor_init(&parent,data,sizeof data));
    assert(musa_dwg1032_cursor_take_bytes(&parent,2,&child));
    assert(parent.bit_position==16 && child.byte_count==2);
    assert(child.bytes==data && child.bit_position==0);
    assert(musa_dwg1032_cursor_read_bits(&child,8,&v) && v==1);
    assert(parent.bit_position==16);
    assert(musa_dwg1032_cursor_take_bytes(&parent,3,&child));
    assert(parent.bit_position==40 && child.bytes==data+2);
    assert(child.byte_count==3 && child.bit_position==0);
    assert(musa_dwg1032_cursor_read_bits(&child,8,&v) && v==3);
    MusaDwg1032Cursor saved_child=child;
    assert(!musa_dwg1032_cursor_take_bytes(&parent,2,&child));
    assert(parent.bit_position==40 && child.bytes==saved_child.bytes);
    assert(child.byte_count==saved_child.byte_count && child.bit_position==saved_child.bit_position);
    assert(!musa_dwg1032_cursor_take_bytes(&parent,SIZE_MAX,&child));
    assert(parent.bit_position==40);
    assert(musa_dwg1032_cursor_take_bytes(&parent,1,&child));
    assert(parent.bit_position==48 && child.bytes==data+5);
    assert(musa_dwg1032_cursor_take_bytes(&parent,0,&child));
    assert(parent.bit_position==48 && child.byte_count==0);
    assert(!musa_dwg1032_cursor_take_bytes(&parent,1,&child));
    assert(musa_dwg1032_cursor_init(&parent,data,sizeof data));
    assert(musa_dwg1032_cursor_skip_bits(&parent,1));
    saved_child=child;
    assert(!musa_dwg1032_cursor_take_bytes(&parent,1,&child));
    assert(parent.bit_position==1 && child.bytes==saved_child.bytes);
    assert(!musa_dwg1032_cursor_take_bytes(&parent,1,&parent));
    assert(parent.bit_position==1);
    assert(musa_dwg1032_cursor_init(&parent,data,sizeof data));
    assert(!musa_dwg1032_cursor_take_bytes(&parent,1,&parent));
    assert(parent.bit_position==0);
    union { MusaDwg1032Cursor aligned; uint8_t bytes[sizeof(MusaDwg1032Cursor)]; } storage={0};
    assert(musa_dwg1032_cursor_init(&parent,storage.bytes,sizeof storage.bytes));
    assert(!musa_dwg1032_cursor_take_bytes(&parent,1,&storage.aligned));
    assert(parent.bit_position==0);
    assert(musa_dwg1032_cursor_init(&parent,NULL,0));
    assert(musa_dwg1032_cursor_take_bytes(&parent,0,&child));
    assert(child.bytes==NULL && child.byte_count==0);
    assert(!musa_dwg1032_cursor_take_bytes(&parent,1,&child));
    assert(!musa_dwg1032_cursor_take_bytes(NULL,0,&child));
    assert(!musa_dwg1032_cursor_take_bytes(&parent,0,NULL));
    parent.bit_position=1;
    assert(!musa_dwg1032_cursor_take_bytes(&parent,0,&child));
    return 0;
}
