/* SPDX-License-Identifier: MIT */
#include "musa_dwg1032_signature.h"
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <assert.h>
#include <stdint.h>
#include <stddef.h>

int main(void) {
    const uint8_t candidate[]={'A','C','1','0','3','2',0,0,0};
    const uint8_t other[]={'A','C','1','0','2','7',0};
    const uint8_t bad[]={'A','C','1','0','3','x',0};
    assert(musa_dwg1032_probe_signature(candidate,sizeof candidate)==
           MUSA_DWG1032_SIGNATURE_AC1032_CANDIDATE);
    assert(musa_dwg1032_probe_signature(candidate,6)==
           MUSA_DWG1032_SIGNATURE_AC1032_CANDIDATE);
    assert(musa_dwg1032_probe_signature(other,sizeof other)==
           MUSA_DWG1032_SIGNATURE_OTHER_AC_VERSION);
    assert(musa_dwg1032_probe_signature(bad,sizeof bad)==
           MUSA_DWG1032_SIGNATURE_INVALID);
    assert(musa_dwg1032_probe_signature(NULL,6)==
           MUSA_DWG1032_SIGNATURE_INVALID);
    assert(musa_dwg1032_probe_signature(candidate,SIZE_MAX)==
           MUSA_DWG1032_SIGNATURE_INVALID);
    for (size_t n=0;n<6;++n)
        assert(musa_dwg1032_probe_signature(candidate,n)==
               MUSA_DWG1032_SIGNATURE_INVALID);
    for (size_t i=0;i<6;++i) {
        uint8_t mutated[6]={'A','C','1','0','3','2'};
        mutated[i]=0xff;
        assert(musa_dwg1032_probe_signature(mutated,sizeof mutated)==
               MUSA_DWG1032_SIGNATURE_INVALID);
    }
    return 0;
}
