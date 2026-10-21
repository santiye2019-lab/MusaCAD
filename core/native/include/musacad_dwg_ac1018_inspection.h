/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef MUSACAD_DWG_AC1018_INSPECTION_H
#define MUSACAD_DWG_AC1018_INSPECTION_H
#include <stddef.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
/* Portable, first-party R2004/AC1018 admission + indexed object evidence.
 * Same implementation on Android, Windows and iOS; JNI is only transport.
 * Unsupported versions and corrupt/incomplete native records fail closed.
 * Output unchanged on failure, original source buffer never mutated.
 *
 * This is a bounded object INVENTORY, not decoded full object/owner graph,
 * presentation attributes, an editable model, AC1032 support or DWG writer.
 */
typedef struct MusaDwgAc1018FileInspection {
    size_t indexed_records;
    size_t decoded_lines;
    size_t decoded_arcs;
    size_t decoded_circles;
    size_t decoded_lwpolylines;
    size_t undecodable_supported_records;
    size_t other_declared_records;
    size_t covered_record_bytes;
    size_t unindexed_section_bytes;
    uint8_t all_record_crcs_verified;
    uint8_t record_ranges_nonoverlapping;
} MusaDwgAc1018FileInspection;
int musa_dwg_ac1018_inspect_file(
    const uint8_t *original_file,size_t length,
    MusaDwgAc1018FileInspection *out);
#ifdef __cplusplus
}
#endif
#endif
