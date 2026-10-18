/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef MUSACAD_MC1032_H
#define MUSACAD_MC1032_H
#include "musacad_document.h"
#include <stddef.h>
#ifdef __cplusplus
extern "C" {
#endif
/* MC1032 = entirely MusaCAD-owned binary document format, NOT DWG/AC1032.
 * v1 supports first-party LINE/CIRCLE/ARC/POINT in WCS XY only.
 * Other geometry, blocks, XREF, colors, 3D and metadata must be rejected,
 * not dropped. Future revisions must have distinct versioned schemas.
 */
#define MUSA_MC1032_HEADER_BYTES 32u
#define MUSA_MC1032_RECORD_BYTES 72u
#define MUSA_MC1032_MAX_ENTITIES 4096u
#define MUSA_MC1032_MAX_BYTES (MUSA_MC1032_HEADER_BYTES + MUSA_MC1032_RECORD_BYTES*MUSA_MC1032_MAX_ENTITIES)
typedef enum {
    MUSA_MC1032_OK=0,
    MUSA_MC1032_INVALID=-1,
    MUSA_MC1032_UNSUPPORTED=-2,
    MUSA_MC1032_CORRUPT=-3,
    MUSA_MC1032_LIMIT=-4,
    MUSA_MC1032_NO_MEMORY=-5
} MusaMC1032Status;
/* Required serialized bytes or 0 for invalid document. */
size_t musa_mc1032_encoded_size(const MusaDocument *document);
/* Query: output=NULL, capacity=0; return required size in *written.
 * Write: capacity >= size; output overwritten ONLY after successful full
 * construction in a private buffer. All document memory remains untouched.
 */
int musa_mc1032_encode(const MusaDocument *document,
                       unsigned char *output,size_t capacity,size_t *written);
/* Decode into a NEW document, never expose a partially decoded document.
 * Every length/CRC/schema/entity/handle is checked before success.
 * On failure *out_document==NULL, all temporary allocations are freed.
 */
int musa_mc1032_decode(const unsigned char *input,size_t bytes,
                       MusaDocument **out_document);
#ifdef __cplusplus
}
#endif
#endif
