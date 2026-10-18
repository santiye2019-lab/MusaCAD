/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef MUSACAD_MC1032_MIGRATE_H
#define MUSACAD_MC1032_MIGRATE_H
#include "musacad_mc1032_v2.h"
#include <stddef.h>
#ifdef __cplusplus
extern "C" {
#endif
/* Strict lossless revision conversion. NEVER silently discard geometry.
 * v1 LINE/POINT in XY -> v2 LINE3D/POINT3D with Z=0, layer 0.
 * v1 CIRCLE/ARC currently reject; v2 nonzero Z, named layers reject
 * downgrade. Preserves original 64-bit handles and coordinates.
 * Both conversions stage complete results; output bytes are unchanged
 * on failure. No source file I/O, third-party CAD code, or Java parser.
 */
int musa_mc1032_upgrade_v1_to_v2(
     const unsigned char *source,size_t source_bytes,
     unsigned char *destination,size_t capacity,size_t *written);
int musa_mc1032_downgrade_v2_to_v1(
     const unsigned char *source,size_t source_bytes,
     unsigned char *destination,size_t capacity,size_t *written);
#ifdef __cplusplus
}
#endif
#endif
