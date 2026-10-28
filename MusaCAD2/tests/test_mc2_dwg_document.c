#include "mc2_dwg_document.h"
#include <assert.h>
#include <stdint.h>
#include <string.h>
int main(void) {
    uint8_t file[64] = {0};
    Mc2DwgDocument doc;
    const uint8_t *ptr = 0;
    size_t length = 0;
    memcpy(file, "AC1032", 6);
    assert(mc2_dwg_document_open(&doc, file, sizeof file));
    assert(doc.header.version == MC2_DWG_AC1032);
    assert(!doc.section_directory_verified);
    assert(doc.sections.count == 0);
    assert(!mc2_dwg_document_section(&doc, 1, &ptr, &length));
    memcpy(file, "invalid", 7);
    assert(!mc2_dwg_document_open(&doc, file, sizeof file));
    assert(!mc2_dwg_document_open(0, file, sizeof file));
    assert(!mc2_dwg_document_open(&doc, 0, sizeof file));
    return 0;
}
