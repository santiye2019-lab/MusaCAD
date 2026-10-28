#include "mc2_dwg_sections.h"
#include "mc2_dwg_header.h"
#include <stdint.h>
#include <string.h>
/* AC1015 section count is a 32-bit LE value at byte offset 0x15.
 * Entries begin at 0x19: section number (1), offset (4), size (4).
 * This implementation intentionally accepts only AC1015 and is not
 * an AC1032 section parser. */
static uint32_t u32le(const uint8_t *p) {
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) |
           ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}
int mc2_ac1015_sections(const uint8_t *file, size_t length,
                        Mc2LegacySectionTable *out) {
    Mc2DwgHeader header;
    Mc2LegacySectionTable temp = {0};
    if (!out || !mc2_dwg_header_probe(file, length, &header) ||
        header.version != MC2_DWG_AC1015 || length < 0x19u) return 0;
    const uint32_t count = u32le(file + 0x15u);
    if (count > 64u || (size_t)count > (length - 0x19u) / 9u) return 0;
    temp.count = count;
    for (size_t i = 0; i < count; ++i) {
        const uint8_t *p = file + 0x19u + i * 9u;
        const uint32_t offset = u32le(p + 1u);
        const uint32_t size = u32le(p + 5u);
        if ((size_t)offset > length || (size_t)size > length - (size_t)offset) return 0;
        for (size_t j = 0; j < i; ++j) {
            const uint64_t a = offset, b = a + size;
            const uint64_t c = temp.entries[j].offset;
            const uint64_t d = c + temp.entries[j].length;
            if (size && temp.entries[j].length && a < d && c < b) return 0;
            if (p[0] == temp.entries[j].section_id) return 0;
        }
        temp.entries[i].section_id = p[0];
        temp.entries[i].offset = offset;
        temp.entries[i].length = size;
    }
    *out = temp;
    return 1;
}
