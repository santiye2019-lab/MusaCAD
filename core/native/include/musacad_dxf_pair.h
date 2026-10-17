/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef MUSACAD_DXF_PAIR_H
#define MUSACAD_DXF_PAIR_H
#include <stddef.h>
#ifdef __cplusplus
extern "C" {
#endif
/* Text DXF pairs consist of a group-code line and exactly one value line.
 * Values may be empty. Each call is bounded and side-effect-free. No Java,
 * Android, Windows or Apple-specific CAD rules.
 * C ABI status: success 0, missing value -1, invalid code -2,
 * exceeded pair cap -3, invalid input -4.
 * This is pair *framing*, not full SECTION/ENTITY or byte-stream parsing.
 */
enum MusaDxfPairStatus {
    MUSA_DXF_PAIR_OK=0,
    MUSA_DXF_PAIR_MISSING_VALUE=-1,
    MUSA_DXF_PAIR_INVALID_CODE=-2,
    MUSA_DXF_PAIR_EXCESS_LIMIT=-3,
    MUSA_DXF_PAIR_INVALID_ARGUMENT=-4
};
int musa_core_dxf_pair_check(const char *code_text,size_t code_bytes,
                             int value_present,size_t previous_pairs,
                             size_t maximum_pairs,int *parsed_code);
#ifdef __cplusplus
}
#endif
#endif
