/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef MUSACAD_DWG_AC1018_LZ_H
#define MUSACAD_DWG_AC1018_LZ_H
#include <stddef.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
/* Bounded, read-only first-party R2004 page LZ stream decoder.
 * A source page may include padding after the 0x11 terminal opcode:
 * consumed_bytes reports precisely the bytes used, not padded length.
 * Output and consumed_bytes remain unchanged on any invalid/truncated
 * stream. Decoder is NOT proof of DWG section checksum/locator integrity.
 * Maximum expanded stream length: 1 MiB. */
int musa_dwg_ac1018_lz_decode(const uint8_t *source, size_t compressed_bytes,
                              uint8_t *output, size_t decompressed_bytes,
                              size_t *consumed_bytes);
/* Read-only diagnostic of the same strict bounded R2004 LZ grammar.
 * Reports only phase, byte counts and final success; never exposes source
 * bytes, CAD object data or a partial decoded buffer. No new acceptance path.
 * Source and output remain unmodified. The diagnostic result must not alias
 * the source stream. */
typedef enum MusaDwgAc1018LzFailurePhase {
    MUSA_DWG_AC1018_LZ_PHASE_NONE = 0,
    MUSA_DWG_AC1018_LZ_PHASE_ALLOC = 1,
    MUSA_DWG_AC1018_LZ_PHASE_LITERAL_HEADER = 2,
    MUSA_DWG_AC1018_LZ_PHASE_LITERAL_COPY = 3,
    MUSA_DWG_AC1018_LZ_PHASE_OPCODE = 4,
    MUSA_DWG_AC1018_LZ_PHASE_MATCH_LENGTH = 5,
    MUSA_DWG_AC1018_LZ_PHASE_MATCH_OFFSET = 6,
    MUSA_DWG_AC1018_LZ_PHASE_BACKREFERENCE = 7,
    MUSA_DWG_AC1018_LZ_PHASE_TERMINATOR = 8,
    MUSA_DWG_AC1018_LZ_PHASE_STEP_LIMIT = 9,
    MUSA_DWG_AC1018_LZ_PHASE_OUTPUT_OVERRUN = 10
} MusaDwgAc1018LzFailurePhase;
typedef struct MusaDwgAc1018LzDiagnostic {
    size_t input_consumed;
    size_t output_produced;
    size_t overflow_match_bytes;   /* only when OUTPUT_OVERRUN */
    size_t overflow_available_bytes; /* only when OUTPUT_OVERRUN */
    MusaDwgAc1018LzFailurePhase failure_phase;
    uint8_t completed;
} MusaDwgAc1018LzDiagnostic;
int musa_dwg_ac1018_lz_diagnose(const uint8_t *source,
    size_t compressed_bytes,size_t decompressed_bytes,
    MusaDwgAc1018LzDiagnostic *out);

#ifdef __cplusplus
}
#endif
#endif
