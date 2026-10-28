#include "mc2_dwg_document.h"
#include <assert.h>
#include <stdint.h>
#include <string.h>
static void put32(uint8_t *p, uint32_t v) {
    p[0]=(uint8_t)v; p[1]=(uint8_t)(v>>8); p[2]=(uint8_t)(v>>16); p[3]=(uint8_t)(v>>24);
}
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
    memcpy(file, "AC1015", 6);
    put32(file+0x15,1);
    file[0x19]=7;
    put32(file+0x1a,48);
    put32(file+0x1e,8);
    file[48]=0xa5;
    assert(mc2_dwg_document_open(&doc,file,sizeof file));
    assert(doc.section_directory_verified && doc.sections.count==1);
    assert(mc2_dwg_document_section(&doc,7,&ptr,&length));
    assert(ptr==file+48 && length==8 && ptr[0]==0xa5);
    assert(!mc2_dwg_document_section(&doc,8,&ptr,&length));
    put32(file+0x1a,60);
    assert(!mc2_dwg_document_open(&doc,file,sizeof file));
    memcpy(file, "invalid", 7);
    assert(!mc2_dwg_document_open(&doc, file, sizeof file));
    assert(!mc2_dwg_document_open(0, file, sizeof file));
    assert(!mc2_dwg_document_open(&doc, 0, sizeof file));
    return 0;
}
