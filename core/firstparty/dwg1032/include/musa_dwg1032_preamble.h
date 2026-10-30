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
/* Streaming-friendly entry point: read exactly the first 0x100 bytes of
 * the file into a caller-owned buffer, then supply the actual total file
 * size separately. Allows preflight of multi-gigabyte files without
 * allocating or mapping the complete DWG. Only offsets stored in the
 * unencrypted fixed preamble are checked; this is NOT a renderability test.
 * Rejects overlapping input/output, and leaves *out unchanged on failure.
 * prefix_bytes MUST be exactly 0x100; file_bytes may exceed SIZE_MAX on
 * 32-bit hosts because the whole file is never dereferenced. */
int musa_dwg1032_read_preamble_prefix(const uint8_t *prefix,
                                      size_t prefix_bytes,
                                      uint64_t file_bytes,
                                      MusaDwg1032Preamble *out);
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
