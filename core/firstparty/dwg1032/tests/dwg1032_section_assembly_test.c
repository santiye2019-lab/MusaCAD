/* SPDX-License-Identifier: MIT */
#include "musa_dwg1032_section_assembly.h"
#ifdef NDEBUG
#undef NDEBUG /* Keep checks active in CMake Release and MSVC /DNDEBUG builds. */
#endif
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
    /* Source and destination may be adjacent, but must not overlap. */
    uint8_t adjacent[]={1,2,3,4,5,9,9,9,9,9};
    MusaDwg1032PageFragment contiguous[]={{0,adjacent,5}};
    assert(musa_dwg1032_section_assemble(contiguous,1,adjacent+5,5));
    assert(!memcmp(adjacent,adjacent+5,5));
    /* A source range crossing into destination must fail without writes. */
    memset(adjacent+5,9,5);
    contiguous[0].data=adjacent+1;
    assert(!musa_dwg1032_section_assemble(contiguous,1,adjacent+5,5));
    for (size_t i=5;i<10;++i) assert(adjacent[i]==9);
    /* Empty assembly cannot silently accept a nonempty output. */
    assert(!musa_dwg1032_section_assemble(NULL,0,out,5));
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
static void file_backed_pages(void) {
    const uint8_t physical[]={7,8,9,1,2,3,4,5,6};
    const uint8_t expected[]={1,2,3,4,5,6,7,8,9};
    MusaDwg1032FilePage pages[]={{0,3,3},{3,6,3},{6,0,3}};
    uint8_t out[9];
    memset(out,0xAA,sizeof(out));
    assert(musa_dwg1032_section_assemble_file_pages(
        physical,sizeof(physical),pages,3,out,sizeof(out)));
    assert(!memcmp(out,expected,sizeof(out)));
    memset(out,0xAA,sizeof(out));
    pages[2].file_offset=7; /* late truncated physical page */
    assert(!musa_dwg1032_section_assemble_file_pages(
        physical,sizeof(physical),pages,3,out,sizeof(out)));
    for (size_t i=0;i<sizeof(out);++i) assert(out[i]==0xAA);
    pages[2].file_offset=0;
    pages[1].logical_offset=4; /* gap */
    assert(!musa_dwg1032_section_assemble_file_pages(
        physical,sizeof(physical),pages,3,out,sizeof(out)));
    pages[1].logical_offset=2; /* logical overlap */
    assert(!musa_dwg1032_section_assemble_file_pages(
        physical,sizeof(physical),pages,3,out,sizeof(out)));
    pages[1].logical_offset=3;
    assert(!musa_dwg1032_section_assemble_file_pages(
        physical,sizeof(physical),pages,3,(uint8_t *)(void *)physical,9));
    assert(!musa_dwg1032_section_assemble_file_pages(
        physical,sizeof(physical),
        (const MusaDwg1032FilePage *)(const void *)out,1,out,sizeof(out)));
    assert(!musa_dwg1032_section_assemble_file_pages(
        NULL,sizeof(physical),pages,3,out,sizeof(out)));
    assert(!musa_dwg1032_section_assemble_file_pages(
        physical,sizeof(physical),NULL,3,out,sizeof(out)));
    assert(!musa_dwg1032_section_assemble_file_pages(
        physical,sizeof(physical),pages,3,NULL,sizeof(out)));
    assert(!musa_dwg1032_section_assemble_file_pages(
        physical,sizeof(physical),pages,3,out,0));
    assert(musa_dwg1032_section_assemble_file_pages(
        NULL,0,NULL,0,NULL,0));
    for (size_t i=0;i<sizeof(out);++i) assert(out[i]==0xAA);
}
static void file_backed_large_reverse_physical_order(void) {
    enum { COUNT=16384 };
    uint8_t *src=malloc(COUNT), *dst=malloc(COUNT);
    MusaDwg1032FilePage *pages=malloc(COUNT*sizeof(*pages));
    assert(src && dst && pages);
    for (size_t i=0;i<COUNT;++i) {
        src[i]=(uint8_t)((i*31u)^(i>>4));
        pages[i]=(MusaDwg1032FilePage){(uint64_t)i,
                                      (uint64_t)(COUNT-1u-i),1u};
    }
    memset(dst,0xCC,COUNT);
    assert(musa_dwg1032_section_assemble_file_pages(
        src,COUNT,pages,COUNT,dst,COUNT));
    for (size_t i=0;i<COUNT;++i) assert(dst[i]==src[COUNT-1u-i]);
    memset(dst,0xCC,COUNT);
    pages[COUNT-1u].file_offset=COUNT; /* late invalid page, no partial writes */
    assert(!musa_dwg1032_section_assemble_file_pages(
        src,COUNT,pages,COUNT,dst,COUNT));
    for (size_t i=0;i<COUNT;++i) assert(dst[i]==0xCC);
    free(pages);free(dst);free(src);
}
static void physical_directory_validation(void) {
    MusaDwg1032FilePage pages[]={{0,6,2},{2,0,2},{4,3,2}};
    size_t order[3]={99,99,99};
    assert(musa_dwg1032_file_pages_validate_physical(pages,3,8,order,3));
    assert(order[0]==1 && order[1]==2 && order[2]==0);
    pages[2].file_offset=1; /* Partial physical overlap. */
    assert(!musa_dwg1032_file_pages_validate_physical(pages,3,8,order,3));
    pages[2].file_offset=0; /* Duplicate physical start. */
    assert(!musa_dwg1032_file_pages_validate_physical(pages,3,8,order,3));
    pages[2].file_offset=7; /* Out of file. */
    assert(!musa_dwg1032_file_pages_validate_physical(pages,3,8,order,3));
    pages[2].file_offset=3;
    pages[2].byte_count=0;
    assert(!musa_dwg1032_file_pages_validate_physical(pages,3,8,order,3));
    pages[2].byte_count=2;
    pages[0].file_offset=UINT64_MAX;
    assert(!musa_dwg1032_file_pages_validate_physical(pages,3,8,order,3));
    pages[0].file_offset=6;
    assert(!musa_dwg1032_file_pages_validate_physical(pages,3,8,order,2));
    assert(!musa_dwg1032_file_pages_validate_physical(
        pages,3,8,(size_t *)(void *)pages,3));
    assert(!musa_dwg1032_file_pages_validate_physical(NULL,3,8,order,3));
    assert(!musa_dwg1032_file_pages_validate_physical(pages,3,8,NULL,3));
    assert(musa_dwg1032_file_pages_validate_physical(NULL,0,0,NULL,0));
    /* A physically adjacent range is legal; physical gaps are legal too. */
    pages[2].file_offset=2;
    assert(musa_dwg1032_file_pages_validate_physical(pages,3,8,order,3));
    pages[2].file_offset=3;
}
static void physical_directory_stress(void) {
    enum { COUNT=16384 };
    MusaDwg1032FilePage *pages=malloc(COUNT*sizeof(*pages));
    size_t *order=malloc(COUNT*sizeof(*order));
    assert(pages && order);
    for (size_t i=0;i<COUNT;++i)
        pages[i]=(MusaDwg1032FilePage){
            (uint64_t)i,(uint64_t)(COUNT-1u-i),1u};
    assert(musa_dwg1032_file_pages_validate_physical(
        pages,COUNT,COUNT,order,COUNT));
    for (size_t i=0;i<COUNT;++i) assert(order[i]==COUNT-1u-i);
    pages[COUNT-1u].file_offset=pages[COUNT-2u].file_offset;
    assert(!musa_dwg1032_file_pages_validate_physical(
        pages,COUNT,COUNT,order,COUNT));
    free(order);
    free(pages);
}
static void strict_file_page_assembly(void) {
    const uint8_t file[]={7,8,9,1,2,3};
    MusaDwg1032FilePage pages[]={{0,3,3},{3,0,3}};
    size_t order[2]={99u,99u};
    uint8_t out[6]={0xAA,0xAA,0xAA,0xAA,0xAA,0xAA};
    const uint8_t expected[]={1,2,3,7,8,9};
    assert(musa_dwg1032_section_assemble_checked_file_pages(
        file,6,pages,2,out,6,order,2));
    assert(!memcmp(out,expected,6));
    assert(order[0]==1u && order[1]==0u);
    memset(out,0xAA,6);
    pages[1].file_offset=2; /* Partial physical overlap, logical order valid. */
    assert(!musa_dwg1032_section_assemble_checked_file_pages(
        file,6,pages,2,out,6,order,2));
    pages[1].file_offset=3; /* Exact duplicate physical span. */
    assert(!musa_dwg1032_section_assemble_checked_file_pages(
        file,6,pages,2,out,6,order,2));
    /* The permissive API intentionally accepts physical reuse. */
    assert(musa_dwg1032_section_assemble_file_pages(
        file,6,pages,2,out,6));
    memset(out,0xAA,6);
    pages[1].file_offset=0;
    pages[1].logical_offset=4; /* Physical valid, logical gap. */
    assert(!musa_dwg1032_section_assemble_checked_file_pages(
        file,6,pages,2,out,6,order,2));
    pages[1].logical_offset=3;
    assert(!musa_dwg1032_section_assemble_checked_file_pages(
        file,6,pages,2,out,6,order,1));
    assert(!musa_dwg1032_section_assemble_checked_file_pages(
        file,6,pages,2,out,6,(size_t *)(void *)pages,2));
    assert(!musa_dwg1032_section_assemble_checked_file_pages(
        file,6,pages,2,out,6,(size_t *)(void *)out,2));
    assert(!musa_dwg1032_section_assemble_checked_file_pages(
        file,6,pages,2,out,6,(size_t *)(void *)file,2));
    assert(!musa_dwg1032_section_assemble_checked_file_pages(
        file,6,pages,2,out,6,NULL,2));
    for (size_t i=0;i<6;++i) assert(out[i]==0xAA);
    assert(musa_dwg1032_section_assemble_checked_file_pages(
        NULL,0,NULL,0,NULL,0,NULL,0));
}
int main(void) {
    basic();stress();partitions();
    file_backed_pages();file_backed_large_reverse_physical_order();
    physical_directory_validation();physical_directory_stress();
    strict_file_page_assembly();
    return 0;
}
