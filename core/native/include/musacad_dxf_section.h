/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef MUSACAD_DXF_SECTION_H
#define MUSACAD_DXF_SECTION_H
#include <stddef.h>
#ifdef __cplusplus
extern "C" {
#endif
/* A portable strict DXF SECTION/ENDSEC/EOF state machine.
 * The caller owns this four-int value: no heap allocation or OS dependency.
 * Recognizes only exact uppercase control records (as the prior verifier
 * did), but accepts section names case-insensitively in ASCII.
 * Does not claim complete DXF geometry/entity validation.
 */
typedef struct MusaDxfSectionState {
    int current_kind;  /* 0 outside, 1 HEADER, 2 ENTITIES, 3 BLOCKS,
                          4 TABLES, 5 OBJECTS, 6 other section */
    int awaiting_name;
    int seen_entities;
    int seen_eof;
} MusaDxfSectionState;

enum MusaDxfSectionFlags {
    MUSA_DXF_SECTION_RECORD_BOUNDARY=1,
    MUSA_DXF_SECTION_LINE_STARTED=2,
    MUSA_DXF_SECTION_HANDLE_AREA=4
};
enum MusaDxfSectionResult {
    MUSA_DXF_SECTION_OK=0,
    MUSA_DXF_SECTION_AFTER_EOF=-10,
    MUSA_DXF_SECTION_MISSING_NAME=-11,
    MUSA_DXF_SECTION_DUP_ENTITIES=-12,
    MUSA_DXF_SECTION_NESTED=-13,
    MUSA_DXF_SECTION_ENDSEC_MISMATCH=-14,
    MUSA_DXF_SECTION_EOF_MISMATCH=-15,
    MUSA_DXF_SECTION_OUTSIDE=-16,
    MUSA_DXF_SECTION_INCOMPLETE=-17,
    MUSA_DXF_SECTION_BAD_ARGUMENT=-18
};

/* Success modifies state and writes *flags. All errors leave both untouched. */
int musa_core_dxf_section_step(MusaDxfSectionState *state,int code,
                               const char *value,size_t value_len,int *flags);
int musa_core_dxf_section_finish(const MusaDxfSectionState *state);
#ifdef __cplusplus
}
#endif
#endif
