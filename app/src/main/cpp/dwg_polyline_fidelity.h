/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Host-only fail-closed ordered 2D polyline vertex fidelity check. */
#ifndef MUSACAD_DWG_POLYLINE_FIDELITY_H
#define MUSACAD_DWG_POLYLINE_FIDELITY_H
#include "dwg_api.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>

#define MUSA_MAX_POLYLINE_VALUES 1000000u
#define MUSA_MAX_POLYLINE_VERTICES 100000u
typedef struct {
  int kind;
  size_t length;
  double *values;
} MusaPolyline;
typedef struct {
  MusaPolyline *lines;
  size_t length;
} MusaPolylines;

static void musa_polyline_free(MusaPolylines *p) {
  if (p->lines) {
    for (size_t i=0; i<p->length; ++i) free(p->lines[i].values);
  }
  free(p->lines);
  memset(p, 0, sizeof(*p));
}

/* Dwg and DXF decoders can reorder top-level entities. Preserve each
   polyline's ordered vertex path and compare complete paths as a multiset. */
static int musa_polyline_cmp(const void *lhs, const void *rhs) {
  const MusaPolyline *a=lhs, *b=rhs;
  if (a->kind != b->kind) return a->kind < b->kind ? -1 : 1;
  if (a->length != b->length) return a->length < b->length ? -1 : 1;
  for (size_t i=0; i<a->length; ++i) {
    if (a->values[i] < b->values[i]) return -1;
    if (a->values[i] > b->values[i]) return 1;
  }
  return 0;
}

static int musa_polyline_capture(const Dwg_Data *dwg, MusaPolylines *out) {
  memset(out, 0, sizeof(*out));
  if (!dwg || dwg->num_objects > MUSA_MAX_FIDELITY_OBJECTS) return -1;
  out->lines = calloc(dwg->num_objects ? dwg->num_objects : 1,
                      sizeof(*out->lines));
  if (!out->lines) return -1;
  size_t total_values = 0;
#define MUSA_ADD(v) do { \
    if (index >= poly->length || !isfinite((double)(v))) goto invalid; \
    poly->values[index++] = (double)(v); \
  } while (0)
#define MUSA_VEC(v) do { MUSA_ADD((v).x); MUSA_ADD((v).y); MUSA_ADD((v).z); } while (0)
  for (BITCODE_BL i=0; i<dwg->num_objects; ++i) {
    const Dwg_Object *obj=&dwg->object[i];
    if (obj->fixedtype != DWG_TYPE_LWPOLYLINE &&
        obj->fixedtype != DWG_TYPE_POLYLINE_2D) continue;
    if (!obj->tio.entity) goto invalid;
    MusaPolyline *poly=&out->lines[out->length++];
    poly->kind=obj->fixedtype;
    size_t index=0;
    if (obj->fixedtype == DWG_TYPE_LWPOLYLINE) {
      const Dwg_Entity_LWPOLYLINE *p=obj->tio.entity->tio.LWPOLYLINE;
      if (!p || p->num_points < 2 || p->num_points > MUSA_MAX_POLYLINE_VERTICES ||
          (p->num_points && !p->points) ||
          (p->num_bulges && !p->bulges) || (p->num_widths && !p->widths) ||
          p->num_bulges > p->num_points || p->num_widths > p->num_points)
        goto invalid;
      /* Flag, width, elevation, thickness, extrusion, vertex data, optional
         per-vertex arc bulges and start/end widths must all agree. */
      size_t need=10u + (size_t)p->num_points*2u +
                  (size_t)p->num_bulges + (size_t)p->num_widths*2u;
      if (need > MUSA_MAX_POLYLINE_VALUES ||
          total_values > MUSA_MAX_POLYLINE_VALUES-need) goto invalid;
      poly->length=need;
      poly->values=calloc(need,sizeof(double));
      if (!poly->values) goto invalid;
      total_values+=need;
      MUSA_ADD(p->flag); MUSA_ADD(p->const_width);
      MUSA_ADD(p->elevation); MUSA_ADD(p->thickness); MUSA_VEC(p->extrusion);
      MUSA_ADD(p->num_points); MUSA_ADD(p->num_bulges);
      MUSA_ADD(p->num_widths);
      for (BITCODE_BL j=0; j<p->num_points; ++j) {
        MUSA_ADD(p->points[j].x); MUSA_ADD(p->points[j].y);
      }
      for (BITCODE_BL j=0; j<p->num_bulges; ++j) MUSA_ADD(p->bulges[j]);
      for (BITCODE_BL j=0; j<p->num_widths; ++j) {
        MUSA_ADD(p->widths[j].start); MUSA_ADD(p->widths[j].end);
      }
    } else {
      const Dwg_Entity_POLYLINE_2D *p=obj->tio.entity->tio.POLYLINE_2D;
      if (!p) goto invalid;
      int error=0;
      BITCODE_BL count=dwg_object_polyline_2d_get_numpoints(obj,&error);
      if (error || count < 2 || count > MUSA_MAX_POLYLINE_VERTICES) goto invalid;
      dwg_point_2d *points=dwg_object_polyline_2d_get_points(obj,&error);
      if (error || !points) { free(points); goto invalid; }
      size_t need=10u + (size_t)count*2u;
      if (need > MUSA_MAX_POLYLINE_VALUES ||
          total_values > MUSA_MAX_POLYLINE_VALUES-need) {
        free(points); goto invalid;
      }
      poly->length=need;
      poly->values=calloc(need,sizeof(double));
      if (!poly->values) { free(points); goto invalid; }
      total_values+=need;
      MUSA_ADD(p->flag); MUSA_ADD(p->curve_type);
      MUSA_ADD(p->start_width); MUSA_ADD(p->end_width);
      MUSA_ADD(p->thickness); MUSA_ADD(p->elevation); MUSA_VEC(p->extrusion);
      MUSA_ADD(count);
      for (BITCODE_BL j=0; j<count; ++j) {
        MUSA_ADD(points[j].x); MUSA_ADD(points[j].y);
      }
      free(points);
    }
    if (index != poly->length) goto invalid;
  }
#undef MUSA_ADD
#undef MUSA_VEC
  return 0;
invalid:
#undef MUSA_ADD
#undef MUSA_VEC
  musa_polyline_free(out);
  return -1;
}

static int musa_verify_polylines(const char *input,const Dwg_Data *decoded) {
  Dwg_Data original={0};
  MusaPolylines before={0},after={0};
  original.header.version=R_2000;
  int read_result=dxf_read_file(input,&original);
  int before_ok=read_result < DWG_ERR_CRITICAL &&
                musa_polyline_capture(&original,&before)==0;
  int after_ok=before_ok && musa_polyline_capture(decoded,&after)==0;
  int valid=before_ok && after_ok && before.length == after.length;
  if (!valid)
    fprintf(stderr, "R2000 polyline audit: read=%d before_ok=%d after_ok=%d before=%zu after=%zu\n",
            read_result, before_ok, after_ok, before.length, after.length);
  if (valid) {
    qsort(before.lines,before.length,sizeof(*before.lines),musa_polyline_cmp);
    qsort(after.lines,after.length,sizeof(*after.lines),musa_polyline_cmp);
    for (size_t i=0; i<before.length && valid; ++i) {
      const MusaPolyline *a=&before.lines[i], *b=&after.lines[i];
      if (a->kind!=b->kind || a->length!=b->length) {
        fprintf(stderr, "R2000 polyline audit: type or vertex-path length differs (line=%zu expected_kind=%d actual_kind=%d length=%zu/%zu)\n",
                i, a->kind, b->kind, a->length, b->length);
        valid=0; break;
      }
      for (size_t j=0; j<a->length; ++j) {
        if (!musa_coordinates_near(a->values[j],b->values[j])) {
          /* CI diagnostics identify a field, never disclose drawing coordinates. */
          fprintf(stderr, "R2000 polyline audit: field mismatch line=%zu kind=%d index=%zu\n",
                  i, a->kind, j);
          valid=0; break;
        }
      }
    }
  }
  musa_polyline_free(&before);
  musa_polyline_free(&after);
  dwg_free(&original);
  return valid;
}
#endif
