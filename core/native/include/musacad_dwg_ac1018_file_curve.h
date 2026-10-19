/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef MUSACAD_DWG_AC1018_FILE_CURVE_H
#define MUSACAD_DWG_AC1018_FILE_CURVE_H
#include <stddef.h>
#include <stdint.h>
#include "musacad_dwg_ac1018_curve.h"
#ifdef __cplusplus
extern "C" {
#endif
/* Read one ARC or CIRCLE by stable indexed handle directly from ORIGINAL
 * protected AC1018 DWG bytes. Checked physical pages and section CRCs,
 * a complete handles index and the record's own CRC and self handle.
 * Stateless convenience API; bulk readers should open one cached scene.
 * No DWG writes, target class graph, external CAD SDK or APK required. */
int musa_dwg_ac1018_file_read_curve(const uint8_t *file,size_t file_bytes,
                                   uint64_t handle,MusaDwgAc1018Curve *out);
#ifdef __cplusplus
}
#endif
#endif
