/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef MUSACAD_DXF_DOCUMENT_H
#define MUSACAD_DXF_DOCUMENT_H
#include "musacad_document.h"
#include <stddef.h>
#ifdef __cplusplus
extern "C" {
#endif

/* Strict, first-party C11 DXF -> MusaCAD document loader; NO LibreDWG/ODA.
 * Deliberately narrow v1 contract: one ASCII ENTITIES section and EOF;
 * only LINE, POINT, CIRCLE and ARC in XY, default layer 0, no Z/proxies,
 * dimensions, blocks, styles, properties or references. Unexpected content
 * MUST fail closed, never be silently omitted.
 * The input is read-only and the caller receives a new document only on
 * complete validated success. 16MiB total, 4096 byte lines, 4000 entities.
 */
enum MusaDxfDocumentStatus {
    MUSA_DXF_DOCUMENT_OK=0,
    MUSA_DXF_DOCUMENT_MALFORMED=-1,
    MUSA_DXF_DOCUMENT_UNSUPPORTED=-2,
    MUSA_DXF_DOCUMENT_LIMIT=-3,
    MUSA_DXF_DOCUMENT_NO_MEMORY=-4
};
int musa_dxf_document_read(const unsigned char *text,size_t length,
                           size_t max_entities,MusaDocument **out_document);

#ifdef __cplusplus
}
#endif
#endif
