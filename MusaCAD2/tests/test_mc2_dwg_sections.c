#include "mc2_dwg_sections.h"
#include <assert.h>
#include <stdint.h>
#include <string.h>
static void put32(uint8_t *p, uint32_t x) {
    p[0]=(uint8_t)x; p[1]=(uint8_t)(x>>8); p[2]=(uint8_t)(x>>16); p[3]=(uint8_t)(x>>24);
}
int main(void) {
    uint8_t f[128] = {0};
    Mc2LegacySectionTable table;
    memcpy(f,"AC1015",6);
    put32(f+0x15,2);
    f[0x19]=1; put32(f+0x1a,64); put32(f+0x1e,8);
    f[0x22]=2; put32(f+0x23,80); put32(f+0x27,16);
    assert(mc2_ac1015_sections(f,sizeof f,&table));
    assert(table.count==2 && table.entries[0].offset==64 && table.entries[1].length==16);
    put32(f+0x23,70);
    assert(!mc2_ac1015_sections(f,sizeof f,&table));
    put32(f+0x23,80);
    put32(f+0x27,100);
    assert(!mc2_ac1015_sections(f,sizeof f,&table));
    put32(f+0x27,16);
    memcpy(f,"AC1032",6);
    assert(!mc2_ac1015_sections(f,sizeof f,&table));
    memcpy(f,"AC1015",6);
    assert(!mc2_ac1015_sections(f,0x20,&table));
    assert(!mc2_ac1015_sections(f,sizeof f,0));
    return 0;
}
