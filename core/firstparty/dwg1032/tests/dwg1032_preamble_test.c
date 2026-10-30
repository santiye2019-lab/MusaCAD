/* SPDX-License-Identifier: MIT
 * Synthetic fixed-preamble vectors; not real AC1032 fixture coverage.
 */
#include "musa_dwg1032_preamble.h"
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <assert.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
static void fixture(uint8_t file[0x180]) {
    memset(file,0,0x180u);
    memcpy(file,"AC1032",6u);
    file[0x0bu]=7u;
    file[0x0cu]=1u;
    file[0x11u]=33u;
    file[0x12u]=8u;
    file[0x13u]=0xe4u; file[0x14u]=0x04u;
    file[0x18u]=3u;
    file[0x0du]=0x20u; file[0x0eu]=1u;
    file[0x20u]=0x40u; file[0x21u]=1u;
    file[0x24u]=0x60u; file[0x25u]=1u;
    file[0x28u]=0x80u;
}
static void rejects(uint8_t file[0x180], size_t n,
                    MusaDwg1032Preamble *out) {
    const MusaDwg1032Preamble before=*out;
    assert(!musa_dwg1032_read_preamble(file,n,out));
    assert(memcmp(&before,out,sizeof(before))==0);
}
int main(void) {
    uint8_t file[0x180];
    fixture(file);
    MusaDwg1032Preamble out={0};
    assert(musa_dwg1032_read_preamble(file,sizeof(file),&out));
    assert(out.maintenance_version==7u && out.application_marker==1u &&
           out.writer_version==33u && out.writer_maintenance==8u &&
           out.codepage==1252u && out.security_flags==3u &&
           out.preview_offset==0x120u && out.summary_offset==0x140u &&
           out.vba_offset==0x160u);
    for (size_t n=0;n<0x100u;++n) rejects(file,n,&out);
    rejects(file,SIZE_MAX,&out);
    assert(!musa_dwg1032_read_preamble(NULL,sizeof(file),&out));
    assert(!musa_dwg1032_read_preamble(file,sizeof(file),NULL));
    assert(!musa_dwg1032_read_preamble(file,sizeof(file),
           (MusaDwg1032Preamble *)(void *)(file+0x40u)));
    const size_t mutated[]={0u,5u,6u,10u,12u,21u,23u,40u,43u,44u,127u};
    for (size_t i=0;i<sizeof(mutated)/sizeof(mutated[0]);++i) {
        fixture(file);
        file[mutated[i]]^=0x40u;
        rejects(file,sizeof(file),&out);
    }
    fixture(file);
    file[0x0cu]=3u;
    assert(musa_dwg1032_read_preamble(file,sizeof(file),&out));
    file[0x0cu]=0u;
    assert(musa_dwg1032_read_preamble(file,sizeof(file),&out));
    fixture(file);
    file[0x0eu]=0x02u; /* preview beyond file */
    rejects(file,sizeof(file),&out);
    fixture(file);
    file[0x20u]=0xffu;file[0x21u]=0xffu;file[0x22u]=0xffu;file[0x23u]=0xffu;
    rejects(file,sizeof(file),&out);
    fixture(file);
    file[0x24u]=0xffu;file[0x25u]=0xffu;
    rejects(file,sizeof(file),&out);
    fixture(file);
    file[0x0du]=0xffu;file[0x0eu]=0u; /* preview before header */
    rejects(file,sizeof(file),&out);
    fixture(file);
    file[0x0du]=0u;file[0x0eu]=0u;
    file[0x20u]=0u;file[0x21u]=0u;
    file[0x24u]=0u;file[0x25u]=0u;
    assert(musa_dwg1032_read_preamble(file,0x100u,&out));
    assert(out.preview_offset==0u && out.summary_offset==0u &&
           out.vba_offset==0u);
    return 0;
}
