/* SPDX-License-Identifier: MIT
 * AC1032 six-byte signature preflight; not a DWG validity guarantee.
 */
#ifndef MUSA_DWG1032_SIGNATURE_H
#define MUSA_DWG1032_SIGNATURE_H
#include <stddef.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif

typedef enum MusaDwg1032Signature {
    MUSA_DWG1032_SIGNATURE_INVALID=0,
    MUSA_DWG1032_SIGNATURE_AC1032_CANDIDATE=1,
    MUSA_DWG1032_SIGNATURE_OTHER_AC_VERSION=2
} MusaDwg1032Signature;

/* Recognizes only the six ASCII version bytes at the start of a bounded
 * buffer. Does not verify sections, CRC, compression, entities or length.
 * A candidate MUST NOT be passed to production rendering as parsed DWG. */
MusaDwg1032Signature musa_dwg1032_probe_signature(const uint8_t *data,
                                                   size_t length);
#ifdef __cplusplus
}
#endif
#endif
