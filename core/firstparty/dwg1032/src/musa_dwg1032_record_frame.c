/* SPDX-License-Identifier: MIT
 * Independently authored transactional MS-prefixed bounded frame reader.
 */
#include "musa_dwg1032_record_frame.h"
#include "musa_dwg1032_modular.h"
#include <stdint.h>
#include <stddef.h>

static int overlaps(const void *a, size_t an, const void *b, size_t bn) {
    if (!an || !bn) return 0;
    const uintptr_t x=(uintptr_t)a, y=(uintptr_t)b;
    return x<=y ? y-x<an : x-y<bn;
}

int musa_dwg1032_record_take_ms_frame(MusaDwg1032Cursor *candidate,
                                      size_t max_payload_bytes,
                                      MusaDwg1032Cursor *payload) {
    if (!candidate || !payload ||
        overlaps(candidate,sizeof(*candidate),payload,sizeof(*payload)) ||
        overlaps(candidate->bytes,candidate->byte_count,
                 payload,sizeof(*payload)))
        return 0;
    MusaDwg1032Cursor trial=*candidate;
    MusaDwg1032Cursor frame;
    uint32_t length=0;
    if (!musa_dwg1032_modular_short(&trial,&length) ||
        !length || (size_t)length>max_payload_bytes ||
        !musa_dwg1032_cursor_take_bytes(&trial,(size_t)length,&frame))
        return 0;
    *payload=frame;
    *candidate=trial;
    return 1;
}
