#include "mc2_dwg_document.h"
#include <stddef.h>
#include <stdint.h>
int mc2_dwg_document_open(Mc2DwgDocument *out, const uint8_t *file, size_t length) {
    Mc2DwgHeader header;
    Mc2DwgDocument result;
    if (!out || !mc2_dwg_header_probe(file, length, &header)) return 0;
    result.header = header;
    result.file = file;
    result.file_length = length;
    result.section_directory_verified = 0;
    if (!mc2_section_map_init(&result.sections, length)) return 0;
    *out = result;
    return 1;
}
int mc2_dwg_document_section(const Mc2DwgDocument *doc, uint32_t id,
                             const uint8_t **data, size_t *length) {
    if (!doc || !doc->section_directory_verified) return 0;
    return mc2_section_map_slice(&doc->sections, id, doc->file,
                                 doc->file_length, data, length);
}
