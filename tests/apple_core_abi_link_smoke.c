/* SPDX-License-Identifier: MIT
 * Apple target link smoke for the shared CAD engine.
 * Cross-link-only: never executed as an iOS/iPadOS app.
 */
#include "musacad_core.h"
#include "musacad_dwg_ac1024_line.h"

int main(void) {
    MusaDwgAc1024LineFields fields = {0};
    /* The calls are linked from the SAME archive as other platforms.
     * Null input must be refused without altering fields when executed. */
    if (musa_core_abi_version() != MUSA_CORE_ABI_VERSION) return 1;
    return musa_dwg_ac1024_line_fields_probe(0, 0u, 0u, &fields);
}
