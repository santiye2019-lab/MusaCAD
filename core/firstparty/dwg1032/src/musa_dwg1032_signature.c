/* SPDX-License-Identifier: MIT
 * Independent ACxxxx version-token recognition; no third-party parser.
 */
#include "musa_dwg1032_signature.h"
#include <stddef.h>
#include <stdint.h>

MusaDwg1032Signature musa_dwg1032_probe_signature(const uint8_t *data,
                                                   size_t length) {
    if (!data || length<6u || length>PTRDIFF_MAX ||
        data[0]!='A' || data[1]!='C') return MUSA_DWG1032_SIGNATURE_INVALID;
    for (size_t i=2u;i<6u;++i)
        if (data[i]<'0' || data[i]>'9') return MUSA_DWG1032_SIGNATURE_INVALID;
    if (data[2]=='1' && data[3]=='0' && data[4]=='3' && data[5]=='2')
        return MUSA_DWG1032_SIGNATURE_AC1032_CANDIDATE;
    return MUSA_DWG1032_SIGNATURE_OTHER_AC_VERSION;
}
