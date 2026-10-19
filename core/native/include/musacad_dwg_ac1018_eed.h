/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef MUSACAD_DWG_AC1018_EED_H
#define MUSACAD_DWG_AC1018_EED_H
#include "musacad_dwg_ac1015_values.h"
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
/* Read-only R2004 EED boundary scanner, NOT EED semantic decoding.
 * Begins at BS size immediately after the self-handle. Repeats:
 * BS size, H APPID, size raw bytes. BS zero terminates.
 * Only observed APPID H code 5 is accepted; others fail closed.
 * Limits: 4096 bytes, 64 groups. Cursor + output remain unchanged
 * on failure. Caller is informed of all opaque EED payload bytes skipped.
 */
int musa_dwg_ac1018_eed_skip(MusaDwgAc1015Bits *bits,
                             uint32_t *raw_eed_bytes);
#ifdef __cplusplus
}
#endif
#endif
