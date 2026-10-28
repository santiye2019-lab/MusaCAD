#include "mc2_bytes.h"
#include <assert.h>
#include <stdint.h>
#include <string.h>
int main(void) {
    const uint8_t bytes[] = {0x12,0x34,0x56,0x78,0x9a,0xbc,0xde};
    Mc2Reader r;
    uint8_t b = 0;
    uint16_t s = 0;
    uint32_t w = 0;
    assert(mc2_reader_init(&r, bytes, sizeof bytes));
    assert(mc2_read_u8(&r, &b) && b == 0x12);
    assert(mc2_read_u16le(&r, &s) && s == 0x5634);
    assert(mc2_read_u32le(&r, &w) && w == UINT32_C(0xdebc9a78));
    assert(mc2_reader_remaining(&r) == 0);
    assert(!mc2_read_u8(&r, &b) && r.offset == sizeof bytes);
    assert(!mc2_read_u16le(&r, &s) && r.offset == sizeof bytes);
    assert(!mc2_reader_init(&r, 0, 1));
    assert(mc2_reader_init(&r, 0, 0));
    assert(mc2_reader_remaining(&r) == 0);
    char version[7];
    assert(mc2_read_dwg_version((const uint8_t *)"AC1032more", 10, version));
    assert(strcmp(version, "AC1032") == 0);
    assert(!mc2_read_dwg_version((const uint8_t *)"ZZ1032", 6, version));
    assert(!mc2_read_dwg_version((const uint8_t *)"AC10x2", 6, version));
    assert(!mc2_read_dwg_version((const uint8_t *)"AC1032", 5, version));
    return 0;
}
