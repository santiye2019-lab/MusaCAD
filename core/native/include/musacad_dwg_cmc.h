/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef MUSACAD_DWG_CMC_H
#define MUSACAD_DWG_CMC_H
#include <stddef.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
/* Independent DWG R2004+ CMC FIELD (layer/table color), unlike entity ENC.
 * DWG bit-coded BS index (ordinary CMC index=0), BL packed color/method,
 * RC color-string flags. This supported strict form accepts no optional
 * color-book/name strings; those live in a separate R2007+ string stream
 * and require independent validation before authentic color acceptance.
 *
 * This is a field-level decoder. Callers MUST supply the real independently
 * authenticated CMC start and an end before HANDLES; it does not find
 * colors by scanning numeric bytes, infer layer styles, or resolve BYLAYER.
 * Success means only the supplied bit-coded field is a supported CMC form;
 * not that the start offset is authentic. Fail-atomic on every error.
 */
typedef struct MusaDwgCmcField {
    uint16_t indexed_color;
    uint32_t packed_rgb;
    size_t start_bit;
    size_t end_bit;
    uint8_t color_name_flag;
    uint8_t book_name_flag;
    uint8_t supported_numeric_only;
} MusaDwgCmcField;
int musa_dwg_cmc_read_numeric_field(
    const uint8_t *data,size_t bytes,size_t start_bit,size_t end_limit_bit,
    MusaDwgCmcField *out);
#ifdef __cplusplus
}
#endif
#endif
