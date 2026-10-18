/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef MUSACAD_DXF_WRITER_H
#define MUSACAD_DXF_WRITER_H
#include "musacad_dxf_document.h"
#include <stddef.h>
#ifdef __cplusplus
extern "C" {
#endif
/*
 * Complete no-loss writer for the first-party geometric DXF *subset*.
 * Only LINE/CIRCLE/ARC/POINT on layer 0 in WCS XY are representable.
 * No third-party CAD libraries; no binary DWG/AC1032 support is claimed.
 *
 * Query: output=NULL, capacity=0 -> OK, *written=required byte count.
 * Commit: capacity must be >= required+1; output NUL terminated, *written
 * excludes NUL. On failure output is completely untouched; nothing is ever
 * opened/written by this C API. Max data 16 MiB and <=4000 entities.
 */
int musa_dxf_document_write(const MusaDocument *doc,
                            char *output,size_t capacity,size_t *written);
#ifdef __cplusplus
}
#endif
#endif
