/* SPDX-License-Identifier: MIT */
#include "musa_dwg1032_section_assembly.h"
#include <assert.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>

static void basic(void) {
    const uint8_t a[]={1,2,3}, b[]={4,5};
    MusaDwg1032PageFragment f[]={{0,a,3},{3,b,2}};
    uint8_t out[]={9,9,9,9,9};
    const uint8_t expected[]={1,2,3,4,5};
    assert(musa_dwg1032_section_assemble(f,2,out,5));
    assert(!memcmp(out,expected,5));
    memset(out,9,5);
    f[1].logical_offset=4; /* gap */
    assert(!musa_dwg1032_section_assemble(f,2,out,5));
    for(size_t i=0;i<5;++i) assert(out[i]==9);
    f[1].logical_offset=2; /* overlap */
    assert(!musa_dwg1032_section_assemble(f,2,out,5));
    f[1].logical_offset=3;
    f[1].byte_count=3; /* truncated output */
    assert(!musa_dwg1032_section_assemble(f,2,out,5));
    f[1].byte_count=2;
    assert(!musa_dwg1032_section_assemble(f,2,out,6));
    assert(!musa_dwg1032_section_assemble(f,2,out,4));
    assert(!musa_dwg1032_section_assemble(f,2,out,0));
    assert(!musa_dwg1032_section_assemble(NULL,1,out,5));
    assert(!musa_dwg1032_section_assemble(f,2,NULL,5));
    assert(musa_dwg1032_section_assemble(NULL,0,NULL,0));
    assert(!musa_dwg1032_section_assemble(f,1,NULL,0));
    assert(!musa_dwg1032_section_assemble(f,2,(uint8_t*)f,sizeof(f)));
    f[0].data=out;
    assert(!musa_dwg1032_section_assemble(f,2,out,5));
    f[0].data=a;
    f[1].logical_offset=UINT64_MAX;
    assert(!musa_dwg1032_section_assemble(f,2,out,5));
}
static void stress(void) {
    const size_t n=20000;
    MusaDwg1032PageFragment *f=malloc(n*sizeof(*f));
    uint8_t *src=malloc(n), *dst=malloc(n);
    assert(f && src && dst);
    for(size_t i=0;i<n;++i) {
        src[i]=(uint8_t)(i*13u);
        dst[i]=0xAA;
        f[i].logical_offset=i;
        f[i].data=&src[i];
        f[i].byte_count=1;
    }
    assert(musa_dwg1032_section_assemble(f,n,dst,n));
    assert(!memcmp(src,dst,n));
    memset(dst,0xAA,n);
    f[n-1].logical_offset=n-2; /* late corruption: no partial writes */
    assert(!musa_dwg1032_section_assemble(f,n,dst,n));
    for(size_t i=0;i<n;++i) assert(dst[i]==0xAA);
    free(f);free(src);free(dst);
}
static void partitions(void) {
    enum { TOTAL=65536, MAX_PARTS=65536 };
    uint8_t *src=malloc(TOTAL), *dst=malloc(TOTAL);
    MusaDwg1032PageFragment *f=malloc(MAX_PARTS*sizeof(*f));
    assert(src && dst && f);
    for (size_t i=0;i<TOTAL;++i) src[i]=(uint8_t)((i*37u)^ (i>>5));
    for (size_t stride=1;stride<=1024;stride*=2) {
        size_t count=0, pos=0;
        while (pos<TOTAL) {
            size_t width=stride;
            if (width>TOTAL-pos) width=TOTAL-pos;
            assert(count<MAX_PARTS);
            f[count].logical_offset=pos;
            f[count].data=src+pos;
            f[count].byte_count=width;
            pos+=width;
            ++count;
        }
        memset(dst,0xCD,TOTAL);
        assert(musa_dwg1032_section_assemble(f,count,dst,TOTAL));
        assert(!memcmp(src,dst,TOTAL));
        if (count>1) {
            memset(dst,0xCD,TOTAL);
            f[count-1].data=dst+TOTAL-1; /* late alias */
            assert(!musa_dwg1032_section_assemble(f,count,dst,TOTAL));
            for (size_t i=0;i<TOTAL;++i) assert(dst[i]==0xCD);
        }
    }
    free(f);free(dst);free(src);
}
int main(void) {basic();stress();partitions();return 0;}
