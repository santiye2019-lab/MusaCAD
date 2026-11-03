/* SPDX-License-Identifier: MIT
 * A Release build must execute the actual C assertions in regression tests.
 */
#include <assert.h>
#ifdef NDEBUG
#error NDEBUG would silently disable first-party DWG regression assertions
#endif
int main(void) {
    volatile int executed = 0;
    assert(++executed == 1);
    return executed == 1 ? 0 : 1;
}
