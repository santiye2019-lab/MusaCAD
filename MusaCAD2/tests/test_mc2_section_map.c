#include "mc2_section_map.h"
#include <assert.h>
#include <stdint.h>
#include <stddef.h>
int main(void) {
    Mc2SectionMap map;
    uint8_t bytes[32] = {0};
    const uint8_t *slice = 0;
    size_t n = 0;
    Mc2SectionRange found;
    assert(mc2_section_map_init(&map,sizeof bytes));
    assert(mc2_section_map_add(&map,1,4,8));
    assert(mc2_section_map_add(&map,2,12,4));
    assert(!mc2_section_map_add(&map,1,20,2));
    assert(!mc2_section_map_add(&map,3,10,4));
    assert(!mc2_section_map_add(&map,3,31,2));
    assert(map.count==2);
    assert(mc2_section_map_find(&map,2,&found));
    assert(found.offset==12 && found.length==4);
    assert(mc2_section_map_slice(&map,1,bytes,sizeof bytes,&slice,&n));
    assert(slice==bytes+4 && n==8);
    assert(!mc2_section_map_slice(&map,1,bytes,16-1,&slice,&n));
    assert(!mc2_section_map_slice(&map,99,bytes,sizeof bytes,&slice,&n));
    assert(mc2_section_map_add(&map,4,32,0));
    assert(!mc2_section_map_slice(&map,1,bytes,sizeof bytes+1,&slice,&n));
    map.count=129;
    assert(!mc2_section_map_find(&map,1,&found));
    assert(!mc2_section_map_slice(&map,1,bytes,sizeof bytes,&slice,&n));
    assert(!mc2_section_map_add(&map,5,0,1));
    return 0;
}
