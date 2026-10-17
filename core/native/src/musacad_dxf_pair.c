/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "musacad_dxf_pair.h"
#include "musacad_dxf_group.h"

int musa_core_dxf_pair_check(const char *code_text,size_t code_bytes,
                             int value_present,size_t previous_pairs,
                             size_t maximum_pairs,int *parsed_code){
    if(!code_text||!parsed_code||!maximum_pairs)
        return MUSA_DXF_PAIR_INVALID_ARGUMENT;
    if(previous_pairs>=maximum_pairs)
        return MUSA_DXF_PAIR_EXCESS_LIMIT;
    if(!value_present)
        return MUSA_DXF_PAIR_MISSING_VALUE;
    int code;
    if(!musa_core_dxf_group_parse(code_text,code_bytes,&code))
        return MUSA_DXF_PAIR_INVALID_CODE;
    *parsed_code=code;
    return MUSA_DXF_PAIR_OK;
}
