/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef MUSACAD_DXF_HANDLE_H
#define MUSACAD_DXF_HANDLE_H
#include <stddef.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
/* CAD/DXF group 5, 105, and $HANDSEED identifiers are unsigned nonzero
 * hexadecimal values (1-16 digits). This is not a binary DWG handle decoder.
 * The same implementation is linked on Android, Windows and iOS.
 */
int musa_core_dxf_handle_parse(const char *hex,size_t length,uint64_t *out);
int musa_core_dxf_handle_format(uint64_t value,char out[17]);
/* Reserve n distinct new handles above ALL source handles and $HANDSEED;
 * seed is the following unused value. Reject wrap, zero, invalid count.
 */
int musa_core_dxf_handle_reserve(uint64_t maximum,size_t count,
                                 uint64_t *first,uint64_t *next_seed);
#ifdef __cplusplus
}
#endif
#endif
