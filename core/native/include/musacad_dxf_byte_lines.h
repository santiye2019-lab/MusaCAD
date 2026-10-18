/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef MUSACAD_DXF_BYTE_LINES_H
#define MUSACAD_DXF_BYTE_LINES_H
#include <stddef.h>
#ifdef __cplusplus
extern "C" {
#endif
/* Portable bounded byte-to-line framing for staged textual DXF validation.
 * This is NOT a complete entity/DWG parser. File I/O stays on the platform.
 * CR is stripped exactly as in the legacy staged-save verifier; LF terminates.
 * Returned line points to the decoder's reusable memory, valid until next step.
 */
typedef struct MusaDxfByteLines MusaDxfByteLines;
enum MusaDxfByteLineResult {
    MUSA_DXF_BYTE_MORE=0,
    MUSA_DXF_BYTE_LINE=1,
    MUSA_DXF_BYTE_EOF=2,
    MUSA_DXF_BYTE_TOO_LONG=-1,
    MUSA_DXF_BYTE_BAD_ARGUMENT=-2,
    MUSA_DXF_BYTE_NO_MEMORY=-3
};
MusaDxfByteLines *musa_core_dxf_byte_lines_create(size_t max_line_bytes);
int musa_core_dxf_byte_lines_step(MusaDxfByteLines *reader,
    const unsigned char *chunk,size_t chunk_len,size_t *consumed,
    const unsigned char **line,size_t *line_len);
int musa_core_dxf_byte_lines_finish(MusaDxfByteLines *reader,
    const unsigned char **line,size_t *line_len);
void musa_core_dxf_byte_lines_destroy(MusaDxfByteLines *reader);
#ifdef __cplusplus
}
#endif
#endif
