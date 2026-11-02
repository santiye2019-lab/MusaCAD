/* SPDX-License-Identifier: MIT
 * First-party C11 validated viewport tables for repeated 2D page queries.
 */
#ifndef MUSA_DWG1032_VIEWPORT_PLAN_H
#define MUSA_DWG1032_VIEWPORT_PLAN_H
#include "musa_dwg1032_viewport_visibility.h"
#ifdef __cplusplus
extern "C" {
#endif
/* This is a post-decode plan, NOT a DWG viewport/section parser.
 * Both tables and their backing storage MUST remain immutable and alive
 * from successful init through the final page call. If viewport/layer
 * state changes, discard the plan and initialize a new one. */
typedef struct MusaDwg1032ViewportPlan {
    const MusaDwg1032LayerState *layers;
    size_t layer_count;
    const MusaDwg1032ViewportFreeze *overrides;
    size_t override_count;
} MusaDwg1032ViewportPlan;

/* Validate sorted unique nonzero layer handles, Boolean layer states,
 * sorted unique Boolean overrides, and override membership exactly once.
 * On failure plan is unchanged. No allocation; O(layers + overrides log
 * layers). The plan itself must not overlap either table. */
int musa_dwg1032_viewport_plan_init(
    MusaDwg1032ViewportPlan *plan,
    const MusaDwg1032LayerState *layers, size_t layer_count,
    const MusaDwg1032ViewportFreeze *overrides, size_t override_count);

/* Query one page of an already-decoded full entity array using a plan
 * returned by init. Full source arrays, plan and metadata must be disjoint
 * from the output page. A malformed handle/flag in the requested page
 * leaves the entire output page untouched. O(page_count log layers +
 * page_count log overrides), with NO repeated metadata table scan.
 * Caller must enforce the immutable-table lifetime contract above.
 * This mask is local evidence only: INSERT/owner visibility must still be
 * applied separately. Does not parse or authenticate AC1032 DWG bytes. */
int musa_dwg1032_viewport_plan_page(
    const MusaDwg1032ViewportPlan *plan,
    const uint64_t *entity_layer_handles,
    const uint8_t *entity_visible, size_t total_count,
    size_t first, size_t page_count, uint8_t *page_visible);
/* Single-lookup bounded page variant for large 2D drawings. Computes each
 * mask byte into caller scratch, then publishes the whole page only after
 * all handles and Boolean flags validate. Unlike plan_page, it performs
 * only one layer/override lookup per entity. The ENTIRE scratch capacity
 * must be disjoint from the full source arrays, metadata, plan and output.
 * Scratch may change on failure; page_visible never does. Requires an
 * immutable plan initialized by viewport_plan_init. No heap, no parsing,
 * no owner/INSERT authorization. O(page_count log layers + page_count log
 * overrides + page_count), with no repeated metadata scan. */
int musa_dwg1032_viewport_plan_page_scratch(
    const MusaDwg1032ViewportPlan *plan,
    const uint64_t *entity_layer_handles,
    const uint8_t *entity_visible, size_t total_count,
    size_t first, size_t page_count, uint8_t *page_visible,
    uint8_t *scratch, size_t scratch_capacity);
#ifdef __cplusplus
}
#endif
#endif
