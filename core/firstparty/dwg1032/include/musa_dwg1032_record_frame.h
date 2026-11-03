/* SPDX-License-Identifier: MIT
 * Bounded MS-length-prefixed record envelope over caller-verified bytes.
 * This is not an AC1032 object schema, section locator or decompressor.
 */
#ifndef MUSA_DWG1032_RECORD_FRAME_H
#define MUSA_DWG1032_RECORD_FRAME_H
#include <stddef.h>
#include "musa_dwg1032_cursor.h"
#ifdef __cplusplus
extern "C" {
#endif
/* Consume one nonempty MS-length-prefixed payload from a caller-verified
 * byte-aligned candidate region. The MS prefix is one or two 16-bit LE
 * words; length is bounded by max_payload_bytes and remaining input.
 * Candidate may contain trailing bytes: the inferred object-map span is
 * only an upper bound, not proof of an exact DWG record length.
 * On success payload is a borrowed bounded view and candidate advances.
 * On any failure both outputs remain unchanged; no allocation/copy.
 * Neither the prefix nor payload establishes a valid AC1032 object type,
 * handle, layer, owner, text, block or geometry. */
int musa_dwg1032_record_take_ms_frame(MusaDwg1032Cursor *candidate,
                                      size_t max_payload_bytes,
                                      MusaDwg1032Cursor *payload);
#ifdef __cplusplus
}
#endif
#endif
