/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "musacad_dwg_regions.h"
#include <assert.h>
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

int main(void){
    uint8_t original[64]={0};
    memcpy(original,"AC1032",6);
    for(size_t i=6u;i<sizeof(original);i++)
        original[i]=(uint8_t)(i*31u);
    uint8_t preserved[64];
    memcpy(preserved,original,sizeof(original));
    /* Adjacent, unsorted and separately typed candidates can coexist. */
    MusaDwgRegion good[]={{6u,10u,1u},{0u,6u,0u},{45u,19u,9u},
                          {16u,29u,7u}};
    assert(musa_dwg_regions_validate(sizeof(original),good,4u));
    const uint8_t *span=(const uint8_t*)0x1;
    size_t len=777u;
    assert(musa_dwg_region_readonly_span(original,sizeof(original),
           &good[1],&span,&len));
    assert(span==original&&len==6u&&memcmp(span,"AC1032",6)==0);
    assert(musa_dwg_region_readonly_span(original,sizeof(original),
           &good[2],&span,&len));
    assert(span==original+45&&len==19u);
    assert(memcmp(original,preserved,sizeof(original))==0);

    MusaDwgRegion crossing[]={{0u,6u,0u},{5u,10u,1u}};
    assert(!musa_dwg_regions_validate(sizeof(original),crossing,2u));
    MusaDwgRegion duplicate[]={{6u,8u,1u},{6u,8u,2u}};
    assert(!musa_dwg_regions_validate(sizeof(original),duplicate,2u));
    MusaDwgRegion empty[]={{6u,0u,0u}};
    assert(!musa_dwg_regions_validate(sizeof(original),empty,1u));
    MusaDwgRegion off[]={{65u,1u,1u}};
    assert(!musa_dwg_regions_validate(sizeof(original),off,1u));
    MusaDwgRegion long_region[]={{63u,2u,1u}};
    assert(!musa_dwg_regions_validate(sizeof(original),long_region,1u));
    MusaDwgRegion overflow[]={{UINT64_MAX,2u,1u}};
    assert(!musa_dwg_regions_validate(UINT64_MAX,overflow,1u));
    MusaDwgRegion exact_u64[]={{UINT64_MAX-6u,6u,1u}};
    assert(musa_dwg_regions_validate(UINT64_MAX,exact_u64,1u));
    MusaDwgRegion end_u64[]={{UINT64_MAX-2u,3u,1u}};
    assert(!musa_dwg_regions_validate(UINT64_MAX,end_u64,1u));

    assert(musa_dwg_regions_validate(0u,NULL,0u));
    assert(!musa_dwg_regions_validate(64u,NULL,1u));
    assert(!musa_dwg_regions_validate(64u,good,MUSA_DWG_REGIONS_MAX+1u));
    /* Negative acceptance must not alter any result pointer/length. */
    span=original+2;
    len=45u;
    assert(!musa_dwg_region_readonly_span(original,sizeof(original),
           &long_region[0],&span,&len));
    assert(span==original+2&&len==45u);
    assert(!musa_dwg_region_readonly_span(NULL,64u,good,&span,&len));
    assert(span==original+2&&len==45u);
    assert(!musa_dwg_region_readonly_span(original,64u,NULL,&span,&len));
    assert(span==original+2&&len==45u);
    assert(!musa_dwg_region_readonly_span(original,64u,&good[1],NULL,&len));
    assert(!musa_dwg_region_readonly_span(original,64u,&good[1],&span,NULL));

    /* Thousands of candidate pairs: no duplicates, overlaps or OOB.
     * Deliberately exercise the maximum accepted count. */
    MusaDwgRegion many[MUSA_DWG_REGIONS_MAX];
    for(size_t i=0u;i<MUSA_DWG_REGIONS_MAX;i++){
        many[i].offset=(uint64_t)i*3u;
        many[i].length=3u;
        many[i].section_kind=(uint32_t)(i&15u);
    }
    assert(musa_dwg_regions_validate(3072u,many,MUSA_DWG_REGIONS_MAX));
    many[512].offset=1535u;
    assert(!musa_dwg_regions_validate(3072u,many,MUSA_DWG_REGIONS_MAX));
    many[512].offset=1536u;
    assert(musa_dwg_regions_validate(3072u,many,MUSA_DWG_REGIONS_MAX));

    /* Offset and length mutation should NEVER pass out-of-file ranges. */
    uint32_t seed=0x7a8da013u;
    for(unsigned i=0;i<10000u;i++){
        seed=seed*1664525u+1013904223u;
        uint64_t at=(uint64_t)(seed&255u);
        seed=seed*1664525u+1013904223u;
        uint64_t size=(uint64_t)(seed&255u);
        MusaDwgRegion trial={at,size,(uint32_t)i};
        int expected=size>0u&&at<=64u&&size<=64u-at;
        assert(musa_dwg_regions_validate(64u,&trial,1u)==expected);
    }
    assert(memcmp(original,preserved,sizeof(original))==0);
    puts("PASS: original binary source, 64-bit DWG candidate ranges, overlaps, fuzz and rollback");
    return 0;
}
