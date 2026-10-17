/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef MUSACAD_DXF_GROUP_H
#define MUSACAD_DXF_GROUP_H
#include <stddef.h>
#ifdef __cplusplus
extern "C" {
#endif
/* One platform-independent parser for the textual DXF group code field.
 * Only ASCII decimal values 0..1071, optional ASCII '+', surrounding ASCII
 * whitespace, and no overflow. A malformed/unsupported code returns 0.
 * This validates a code field; full DXF record semantics are separate.
 */
int musa_core_dxf_group_parse(const char *text,size_t length,int *out);
#ifdef __cplusplus
}
#endif
#endif
