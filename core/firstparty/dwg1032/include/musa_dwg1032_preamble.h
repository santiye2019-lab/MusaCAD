/* SPDX-License-Identifier: MIT
 * Independently authored AC1032 fixed-file-preamble reader.
 * A successful preflight is NOT a validated or renderable DWG.
 */
#ifndef MUSA_DWG1032_PREAMBLE_H
#define MUSA_DWG1032_PREAMBLE_H
#include <stddef.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
typedef struct MusaDwg1032Preamble {
    uint8_t maintenance_version;
    uint8_t application_marker;
    uint8_t writer_version;
    uint8_t writer_maintenance;
    uint16_t codepage;
    uint32_t security_flags;
    uint32_t preview_offset;
    uint32_t summary_offset;
    uint32_t vba_offset;
} MusaDwg1032Preamble;
/* Strictly checks the 0x80-byte unencrypted fixed preamble of a bounded
 * AC1032 file, plus the minimum 0x100-byte initial header extent.
 * Checks reserved bytes, the known 0x80 header pointer and nonzero
 * preview/summary/VBA offsets against the supplied file length.
 * It deliberately does NOT decrypt/verify the 0x6c-byte header at 0x80,
 * authenticate CRCs, find sections/pages, decode objects or enable JNI.
 * Security flags are reported, NOT interpreted as an authorization to
 * read encrypted content. May reject nonstandard but recoverable files.
 * No heap allocation. On failure *out is unchanged; no input/output alias.
 */
int musa_dwg1032_read_preamble(const uint8_t *file, size_t file_bytes,
                               MusaDwg1032Preamble *out);
#ifdef __cplusplus
}
#endif
#endif
