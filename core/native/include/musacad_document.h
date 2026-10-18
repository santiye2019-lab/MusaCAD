/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef MUSACAD_DOCUMENT_H
#define MUSACAD_DOCUMENT_H

/* MusaCAD-owned geometric document, no DWG/LibreDWG/ODA dependencies.
 * C11 ABI: Android JNI, Windows C ABI and iOS Swift/C use this SAME model.
 * v1 only supports LINE, CIRCLE, ARC, POINT in XY (not a DWG decoder).
 */
#include <stddef.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif

#define MUSA_DOCUMENT_ABI_VERSION 1u

typedef struct MusaDocument MusaDocument;
typedef enum {
    MUSA_DOCUMENT_LINE=1,
    MUSA_DOCUMENT_CIRCLE=2,
    MUSA_DOCUMENT_ARC=3,
    MUSA_DOCUMENT_POINT=4
} MusaDocumentKind;

typedef enum {
    MUSA_DOCUMENT_OK=0,
    MUSA_DOCUMENT_INVALID=-1,
    MUSA_DOCUMENT_LIMIT=-2,
    MUSA_DOCUMENT_NO_MEMORY=-3,
    MUSA_DOCUMENT_NOT_FOUND=-4,
    MUSA_DOCUMENT_OVERFLOW=-5
} MusaDocumentStatus;

typedef struct {
    uint64_t handle;   /* MusaCAD-owned, stable, never reused in one document */
    int kind;
    double x1,y1;     /* LINE start, or CIRCLE/ARC/POINT center */
    double x2,y2;     /* LINE end, otherwise zero */
    double radius;    /* CIRCLE/ARC only */
    double start_deg,end_deg; /* ARC only; WCS degrees */
} MusaDocumentEntity;

unsigned int musa_document_abi_version(void);
MusaDocument *musa_document_create(size_t max_entities);
void musa_document_destroy(MusaDocument *document);
size_t musa_document_count(const MusaDocument *document);

/* Every mutating operation checks inputs and commits atomically on success. */
int musa_document_add_line(MusaDocument *document,double x1,double y1,
                            double x2,double y2,uint64_t *out_handle);
int musa_document_add_circle(MusaDocument *document,double cx,double cy,
                              double radius,uint64_t *out_handle);
int musa_document_add_arc(MusaDocument *document,double cx,double cy,
                           double radius,double start_deg,double end_deg,
                           uint64_t *out_handle);
int musa_document_add_point(MusaDocument *document,double x,double y,
                             uint64_t *out_handle);
int musa_document_get(const MusaDocument *document,uint64_t handle,
                       MusaDocumentEntity *out_entity);
int musa_document_get_index(const MusaDocument *document,size_t index,
                             MusaDocumentEntity *out_entity);
int musa_document_translate(MusaDocument *document,uint64_t handle,
                             double dx,double dy);
int musa_document_delete(MusaDocument *document,uint64_t handle);
/* Import-only entry point for a separate first-party file decoder.
 * Allows original, unordered nonzero file handles. Rejects duplicates and
 * later imports after any public edit/add/delete; never recycles deleted IDs.
 * handle=0 requests a new private monotonic handle.
 */
int musa_document_load_entity(MusaDocument *document,
                              const MusaDocumentEntity *source,
                              uint64_t *actual_handle);

#ifdef __cplusplus
}
#endif
#endif
