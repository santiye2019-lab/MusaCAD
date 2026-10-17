/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef MUSACAD_DWG_SIGNATURE_H
#define MUSACAD_DWG_SIGNATURE_H
#include <stddef.h>
#ifdef __cplusplus
extern "C" {
#endif
/* Authoritative identification of a SIX-BYTE DWG file signature.
 * This does NOT validate any DWG entity, promise a binary writer, or assert
 * AutoCAD 2026 feature equivalence. Used identically on all three platforms.
 */
typedef enum MusaCoreDwgSignature {
    MUSA_DWG_SHORT_HEADER = -2,
    MUSA_DWG_NON_ASCII = -1,
    MUSA_DWG_UNKNOWN = 0,
    MUSA_DWG_R12 = 1,
    MUSA_DWG_R13 = 2,
    MUSA_DWG_R14 = 3,
    MUSA_DWG_2000 = 4,
    MUSA_DWG_2004 = 5,
    MUSA_DWG_2007 = 6,
    MUSA_DWG_2010 = 7,
    MUSA_DWG_2013 = 8,
    MUSA_DWG_2018_2026 = 9
} MusaCoreDwgSignature;

/* length may exceed 6: inspect ONLY the first 6 bytes, matching DWG format.
 * NULL and length<6 return SHORT_HEADER without ever dereferencing bytes.
 */
MusaCoreDwgSignature musa_core_dwg_identify_signature(
    const unsigned char *bytes,size_t length);
#ifdef __cplusplus
}
#endif
#endif
