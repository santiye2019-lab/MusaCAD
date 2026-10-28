#ifndef MC2_DWG_DOCUMENT_H
#define MC2_DWG_DOCUMENT_H
#include "mc2_dwg_header.h"
#include "mc2_section_map.h"
#include <stddef.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
typedef struct {
    Mc2DwgHeader header;
    Mc2SectionMap sections;
    const uint8_t *file;
    size_t file_length;
    int section_directory_verified;
} Mc2DwgDocument;
/* Opens a signature-verified, read-only file view.
 * No AC1032 section directory is assumed or fabricated. */
int mc2_dwg_document_open(Mc2DwgDocument *out, const uint8_t *file, size_t length);
/* Only a format-specific authenticated parser may populate the map.
 * This API deliberately fails closed until one exists. */
int mc2_dwg_document_section(const Mc2DwgDocument *doc, uint32_t id,
                             const uint8_t **data, size_t *length);
#ifdef __cplusplus
}
#endif
#endif
