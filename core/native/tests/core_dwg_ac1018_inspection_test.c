/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Reuse the real CRC-protected synthetic binary fixture generator already
 * independently tested for R2004 LINE/LWPOLYLINE source fidelity. */
#define main existing_scene_fixture_test_entrypoint
#include "core_dwg_ac1018_scene_test.c"
#undef main
#include "musacad_dwg_ac1018_inspection.h"
int main(void){
    uint8_t raw[1120]; construct(raw,0);
    uint8_t original[1120];memcpy(original,raw,sizeof(raw));
    MusaDwgAc1018FileInspection result={0};
    assert(musa_dwg_ac1018_inspect_file(raw,sizeof(raw),&result));
    assert(result.indexed_records==2u&&
           result.decoded_lines==1u&&result.decoded_lwpolylines==1u&&
           result.decoded_arcs==0u&&result.decoded_circles==0u&&
           result.other_declared_records==0u&&
           result.undecodable_supported_records==0u&&
           result.covered_record_bytes==
                       sizeof(poly_record)+sizeof(line_record)&&
           result.unindexed_section_bytes==0u&&
           result.all_record_crcs_verified&&
           result.record_ranges_nonoverlapping&&
           memcmp(raw,original,sizeof(raw))==0);
    const MusaDwgAc1018FileInspection saved=result;
    memcpy(raw,"AC1032",6u);
    assert(!musa_dwg_ac1018_inspect_file(raw,sizeof(raw),&result));
    assert(memcmp(&result,&saved,sizeof(result))==0);
    memcpy(raw,original,sizeof(raw));
    raw[288u]^=1u; /* original protected Objects page CRC */
    assert(!musa_dwg_ac1018_inspect_file(raw,sizeof(raw),&result));
    assert(memcmp(&result,&saved,sizeof(result))==0);
    memcpy(raw,original,sizeof(raw));
    raw[608u]^=1u; /* original protected Handles page CRC */
    assert(!musa_dwg_ac1018_inspect_file(raw,sizeof(raw),&result));
    assert(memcmp(&result,&saved,sizeof(result))==0);
    assert(!musa_dwg_ac1018_inspect_file(raw,sizeof(raw)-64u,&result));
    assert(!musa_dwg_ac1018_inspect_file(raw,sizeof(raw),
              (MusaDwgAc1018FileInspection*)(void*)(raw+32u)));
    assert(!musa_dwg_ac1018_inspect_file(NULL,sizeof(raw),&result));
    assert(!musa_dwg_ac1018_inspect_file(raw,sizeof(raw),NULL));
    assert(memcmp(&result,&saved,sizeof(result))==0);
    construct_bulged(raw);
    assert(musa_dwg_ac1018_inspect_file(raw,sizeof(raw),&result));
    assert(result.indexed_records==1u&&
           result.decoded_lwpolylines==1u&&
           result.decoded_lines==0u&&
           result.undecodable_supported_records==0u&&
           result.all_record_crcs_verified);
    return 0;
}
