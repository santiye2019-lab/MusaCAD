/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Reuse original CRC-backed R2004 LAYER/BLOCK_HEADER nonentity fixture.
 * Unknown/non-geometric declared objects must be reported, never omitted. */
#define main existing_nonentity_fixture_test_entrypoint
#include "core_dwg_ac1018_scene_nonentity_test.c"
#undef main
#include "musacad_dwg_ac1018_inspection.h"
int main(void){
    uint8_t original[1120]={0};
    construct(original,0,0,49u,0);
    uint8_t unchanged[1120];memcpy(unchanged,original,sizeof(original));
    MusaDwgAc1018FileInspection result={0};
    assert(musa_dwg_ac1018_inspect_file(original,sizeof(original),&result));
    assert(result.indexed_records==2u&&
           result.other_declared_records==2u&&
           result.decoded_lines==0u&&result.decoded_arcs==0u&&
           result.decoded_circles==0u&&
           result.decoded_lwpolylines==0u&&
           result.undecodable_supported_records==0u&&
           result.all_record_crcs_verified&&
           result.record_ranges_nonoverlapping&&
           memcmp(original,unchanged,sizeof(original))==0);
    const MusaDwgAc1018FileInspection saved=result;
    original[288u]^=1u; /* corrupt object page, no fake counts */
    assert(!musa_dwg_ac1018_inspect_file(
                              original,sizeof(original),&result));
    assert(memcmp(&result,&saved,sizeof(result))==0);
    return 0;
}
