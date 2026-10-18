/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "musacad_dwg_regions.h"
#include <limits.h>

static int bounded(uint64_t file_bytes,const MusaDwgRegion *r) {
    /* The subtract-before-compare pattern never overflows UINT64_MAX. */
    return r&&r->length>0u&&r->offset<=file_bytes&&
           r->length<=file_bytes-r->offset;
}

int musa_dwg_regions_validate(uint64_t bytes,
                              const MusaDwgRegion *regions,
                              size_t count) {
    if(count>MUSA_DWG_REGIONS_MAX||(!regions&&count))return 0;
    for(size_t i=0u;i<count;i++){
        if(!bounded(bytes,&regions[i]))return 0;
        const uint64_t end_i=regions[i].offset+regions[i].length;
        for(size_t j=0u;j<i;j++){
            /* Both sums are safe because each extent is <= bytes. */
            const uint64_t end_j=regions[j].offset+regions[j].length;
            if(regions[i].offset<end_j&&regions[j].offset<end_i)
                return 0;
        }
    }
    return 1;
}

int musa_dwg_region_readonly_span(const uint8_t *source,
                                 size_t source_bytes,
                                 const MusaDwgRegion *region,
                                 const uint8_t **out_start,
                                 size_t *out_length) {
    if(!source||!region||!out_start||!out_length||
       !bounded((uint64_t)source_bytes,region)||
       region->offset>(uint64_t)SIZE_MAX||
       region->length>(uint64_t)SIZE_MAX)return 0;
    /* Do not commit either output before every safety check passes. */
    const uint8_t *start=source+(size_t)region->offset;
    const size_t len=(size_t)region->length;
    *out_start=start;
    *out_length=len;
    return 1;
}
