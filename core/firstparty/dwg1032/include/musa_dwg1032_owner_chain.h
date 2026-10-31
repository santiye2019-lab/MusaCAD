/* SPDX-License-Identifier: MIT
 * First-party post-decode owner-chain integrity checks; no DWG parsing.
 */
#ifndef MUSA_DWG1032_OWNER_CHAIN_H
#define MUSA_DWG1032_OWNER_CHAIN_H
#include "musa_dwg1032_object_index.h"
#ifdef __cplusplus
extern "C" {
#endif
/* owner_handles[i] is the already decoded owner of entries[i]; zero means
 * root. All arrays are immutable for the duration of the call. Checks each
 * hop against the validated index and rejects missing owners, cycles, and
 * paths exceeding max_hops. No allocation; O(hops log count). Does not
 * infer whether a relationship is semantically valid for a DWG entity type.
 * root_handle and hops are unchanged on failure and must not alias inputs. */
int musa_dwg1032_owner_chain_root(const MusaDwg1032ObjectIndex *index,
                                   const uint64_t *owner_handles,
                                   size_t start_position, size_t max_hops,
                                   uint64_t *root_handle, size_t *hops);
#ifdef __cplusplus
}
#endif
#endif
