#include "mc2_dwg_document.h"
#include "mc2_dwg_sections.h"
#include "mc2_bitstream.h"
#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
/* Deterministic malformed-input regression, no external corpus required. */
static uint32_t state = UINT32_C(0x9e3779b9);
static uint32_t next_random(void) {
    state ^= state << 13;
    state ^= state >> 17;
    state ^= state << 5;
    return state;
}
int main(void) {
    uint8_t buffer[256];
    for (unsigned trial = 0; trial < 10000u; ++trial) {
        size_t size = (size_t)(next_random() % (sizeof buffer + 1u));
        for (size_t i = 0; i < size; ++i) buffer[i] = (uint8_t)next_random();
        if (size >= 6u && trial % 3u == 0u) memcpy(buffer,"AC1015",6);
        if (size >= 6u && trial % 3u == 1u) memcpy(buffer,"AC1032",6);
        Mc2DwgDocument doc;
        if (mc2_dwg_document_open(&doc,buffer,size)) {
            assert(doc.file == buffer && doc.file_length == size);
            if (doc.section_directory_verified) {
                assert(doc.header.version == MC2_DWG_AC1015);
                for (size_t j = 0; j < doc.sections.count; ++j) {
                    const Mc2SectionRange *range = &doc.sections.entries[j];
                    const uint8_t *slice = 0;
                    size_t length = 0;
                    assert(range->offset <= size);
                    assert(range->length <= size - range->offset);
                    assert(mc2_dwg_document_section(&doc,range->id,&slice,&length));
                    assert(slice == buffer + range->offset);
                    assert(length == range->length);
                }
            }
        }
        Mc2BitReader bits;
        assert(mc2_bitreader_init(&bits,buffer,size));
        uint64_t value = 0;
        const unsigned count = next_random() % 65u;
        const size_t previous = bits.bit_position;
        if (!mc2_read_bits_msb64(&bits,count,&value)) {
            assert(bits.bit_position == previous);
        } else {
            assert(bits.bit_position == count);
        }
    }
    return 0;
}
