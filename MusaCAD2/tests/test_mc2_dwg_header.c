#include "mc2_dwg_header.h"
#include <assert.h>
#include <stdint.h>
#include <string.h>
int main(void) {
    static const char *known[] = {"AC1015","AC1018","AC1021","AC1024","AC1027","AC1032"};
    Mc2DwgHeader header;
    for (unsigned i = 0; i < 6; ++i) {
        assert(mc2_dwg_header_probe((const uint8_t *)known[i], 6, &header));
        assert(header.version == (Mc2DwgVersion)(i + 1));
        assert(header.file_length == 6);
        assert(strcmp(header.signature, known[i]) == 0);
    }
    header.version = MC2_DWG_AC1032;
    assert(!mc2_dwg_header_probe((const uint8_t *)"AC9999", 6, &header));
    assert(header.version == MC2_DWG_AC1032);
    assert(!mc2_dwg_header_probe((const uint8_t *)"AC1032", 5, &header));
    assert(!mc2_dwg_header_probe(0, 0, &header));
    assert(!mc2_dwg_header_probe((const uint8_t *)"AC1032", 6, 0));
    return 0;
}
