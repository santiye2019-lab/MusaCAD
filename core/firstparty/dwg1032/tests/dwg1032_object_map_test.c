/* SPDX-License-Identifier: MIT */
#include "musa_dwg1032_object_map.h"
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <assert.h>
#include <stdint.h>
#include <stddef.h>
static void small(void) {
    const uint8_t payload[]={1,10,2,0x45,3,20};
    MusaDwg1032MapEntry out[3]={{99,99},{99,99},{99,99}};
    assert(musa_dwg1032_object_map_decode(payload,sizeof payload,3,26,out,3));
    assert(out[0].handle==1 && out[0].offset==10);
    assert(out[1].handle==3 && out[1].offset==5);
    assert(out[2].handle==6 && out[2].offset==25);
    const MusaDwg1032MapEntry saved=out[0];
    assert(!musa_dwg1032_object_map_decode(payload,sizeof payload-1,3,26,out,3));
    assert(!musa_dwg1032_object_map_decode(payload,sizeof payload,3,25,out,3));
    assert(!musa_dwg1032_object_map_decode(payload,sizeof payload,2,26,out,3));
    assert(!musa_dwg1032_object_map_decode(payload,sizeof payload,3,26,out,2));
    assert(out[0].handle==saved.handle && out[0].offset==saved.offset);
    const uint8_t bad_handle[]={0,10};
    assert(!musa_dwg1032_object_map_decode(bad_handle,2,1,100,out,3));
    const uint8_t bad_offset[]={1,0x41};
    assert(!musa_dwg1032_object_map_decode(bad_offset,2,1,100,out,3));
    const uint8_t noncanonical[]={0x81,0,10};
    assert(!musa_dwg1032_object_map_decode(noncanonical,3,1,100,out,3));
    assert(musa_dwg1032_object_map_decode(NULL,0,0,0,NULL,0));
    assert(!musa_dwg1032_object_map_decode(payload,sizeof payload,3,26,
        (MusaDwg1032MapEntry *)(void *)payload,3));
}
static void stress(void) {
    enum { N=10000 };
    static uint8_t bytes[N*2];
    static MusaDwg1032MapEntry out[N];
    for (size_t i=0;i<N;++i) {bytes[2*i]=1;bytes[2*i+1]=1;}
    assert(musa_dwg1032_object_map_decode(bytes,sizeof bytes,N,N+1,out,N));
    for (size_t i=0;i<N;++i) {
        assert(out[i].handle==i+1u && out[i].offset==i+1u);
    }
    bytes[N*2-1]=0x40;
    assert(!musa_dwg1032_object_map_decode(bytes,sizeof bytes,N,N+1,out,N));
    assert(out[N-1].handle==N && out[N-1].offset==N);
}
int main(void) {small();stress();return 0;}
