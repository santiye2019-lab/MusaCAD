/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef MUSACAD_DWG_AC1015_CURVE_H
#define MUSACAD_DWG_AC1015_CURVE_H
#include "musacad_dwg_ac1015_object.h"
#ifdef __cplusplus
extern "C" {
#endif
/* Field-only parser. ARC type 17, CIRCLE type 18 in AC1015.
 * A verified entity-common-data end offset is a mandatory input.
 * Parsed scalar fields are NOT independently verified DWG geometry. */
typedef struct MusaDwgAc1015CurveFields {
    uint64_t indexed_handle;
    uint16_t type_code;
    double center[3],radius,thickness,extrusion[3];
    double start_angle,end_angle; /* ARC only, source radians */
    size_t first_field_bit,end_field_bit;
    int field_layout_decoded;
} MusaDwgAc1015CurveFields;
int musa_dwg_ac1015_decode_curve_fields(
    const MusaDwgAc1015ObjectRecord *record,
    size_t verified_common_data_end_bit,
    MusaDwgAc1015CurveFields *out);
#ifdef __cplusplus
}
#endif
#endif
