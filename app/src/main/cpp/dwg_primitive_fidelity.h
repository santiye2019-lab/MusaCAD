/* SPDX-License-Identifier: GPL-3.0-or-later */
/*
 * Strict host-pilot primitive coordinate audit.
 * The converter's separate type-count guard remains authoritative for all
 * whitelisted entity kinds. This audit covers LINE, CIRCLE, ARC, POINT and
 * 3DFACE values, not layers, text, XREFs, block ownership or polyline vertices.
 * A non-finite, missing, excessive or changed primitive rejects publication.
 */
#ifndef MUSACAD_DWG_PRIMITIVE_FIDELITY_H
#define MUSACAD_DWG_PRIMITIVE_FIDELITY_H
#include <math.h>
#include <stdlib.h>
#include <string.h>

#define MUSA_MAX_FIDELITY_OBJECTS 250000u
#define MUSA_MAX_PRIMITIVE_FIELDS 16

typedef struct {
  int kind;
  unsigned int used;
  double value[MUSA_MAX_PRIMITIVE_FIELDS];
} MusaPrimitive;

typedef struct {
  MusaPrimitive *items;
  size_t size;
} MusaPrimitiveList;

static void musa_free_primitives(MusaPrimitiveList *list) {
  free(list->items);
  list->items = NULL;
  list->size = 0;
}

static int musa_capture_primitives(const Dwg_Data *dwg, MusaPrimitiveList *out) {
  memset(out, 0, sizeof(*out));
  if (!dwg || dwg->num_objects > MUSA_MAX_FIDELITY_OBJECTS) return -1;
  out->items = calloc(dwg->num_objects ? dwg->num_objects : 1,
                      sizeof(*out->items));
  if (!out->items) return -1;
#define MUSA_VAL(expr) do { \
    if (rec->used >= MUSA_MAX_PRIMITIVE_FIELDS || !isfinite((double)(expr))) \
      goto invalid; \
    rec->value[rec->used++] = (double)(expr); \
  } while (0)
#define MUSA_VEC3(vec) do { \
    MUSA_VAL((vec).x); MUSA_VAL((vec).y); MUSA_VAL((vec).z); \
  } while (0)
  for (BITCODE_BL i = 0; i < dwg->num_objects; ++i) {
    const Dwg_Object *obj = &dwg->object[i];
    int kind = obj->fixedtype;
    if (kind != DWG_TYPE_LINE && kind != DWG_TYPE_CIRCLE &&
        kind != DWG_TYPE_ARC && kind != DWG_TYPE_POINT &&
        kind != DWG_TYPE__3DFACE && kind != DWG_TYPE_VERTEX_2D) continue;
    if (!obj->tio.entity) goto invalid;
    MusaPrimitive *rec = &out->items[out->size++];
    rec->kind = kind;
    switch (kind) {
      case DWG_TYPE_LINE: {
        const Dwg_Entity_LINE *p = obj->tio.entity->tio.LINE;
        if (!p) goto invalid;
        MUSA_VEC3(p->start); MUSA_VEC3(p->end);
        MUSA_VAL(p->thickness); MUSA_VEC3(p->extrusion);
        break;
      }
      case DWG_TYPE_CIRCLE: {
        const Dwg_Entity_CIRCLE *p = obj->tio.entity->tio.CIRCLE;
        if (!p) goto invalid;
        MUSA_VEC3(p->center); MUSA_VAL(p->radius);
        MUSA_VAL(p->thickness); MUSA_VEC3(p->extrusion);
        break;
      }
      case DWG_TYPE_ARC: {
        const Dwg_Entity_ARC *p = obj->tio.entity->tio.ARC;
        if (!p) goto invalid;
        MUSA_VEC3(p->center); MUSA_VAL(p->radius);
        MUSA_VAL(p->start_angle); MUSA_VAL(p->end_angle);
        MUSA_VAL(p->thickness); MUSA_VEC3(p->extrusion);
        break;
      }
      case DWG_TYPE_POINT: {
        const Dwg_Entity_POINT *p = obj->tio.entity->tio.POINT;
        if (!p) goto invalid;
        MUSA_VAL(p->x); MUSA_VAL(p->y); MUSA_VAL(p->z);
        MUSA_VAL(p->x_ang); MUSA_VAL(p->thickness);
        MUSA_VEC3(p->extrusion);
        break;
      }
      case DWG_TYPE_VERTEX_2D: {
        const Dwg_Entity_VERTEX_2D *p = obj->tio.entity->tio.VERTEX_2D;
        if (!p) goto invalid;
        MUSA_VEC3(p->point);
        MUSA_VAL(p->start_width); MUSA_VAL(p->end_width);
        MUSA_VAL(p->bulge); MUSA_VAL(p->tangent_dir);
        MUSA_VAL(p->flag);
        break;
      }
      case DWG_TYPE__3DFACE: {
        const Dwg_Entity__3DFACE *p = obj->tio.entity->tio._3DFACE;
        if (!p) goto invalid;
        MUSA_VEC3(p->corner1); MUSA_VEC3(p->corner2);
        MUSA_VEC3(p->corner3); MUSA_VEC3(p->corner4);
        MUSA_VAL(p->invis_flags);
        break;
      }
      default: goto invalid;
    }
  }
#undef MUSA_VAL
#undef MUSA_VEC3
  return 0;
invalid:
#undef MUSA_VAL
#undef MUSA_VEC3
  musa_free_primitives(out);
  return -1;
}

static int musa_primitive_compare(const void *left, const void *right) {
  const MusaPrimitive *a = left, *b = right;
  if (a->kind != b->kind) return a->kind < b->kind ? -1 : 1;
  if (a->used != b->used) return a->used < b->used ? -1 : 1;
  for (unsigned i = 0; i < a->used; ++i) {
    if (a->value[i] < b->value[i]) return -1;
    if (a->value[i] > b->value[i]) return 1;
  }
  return 0;
}

/* Geometry-unit tolerance: max(1e-7, 1e-12 * magnitude), never NaN. */
static int musa_coordinates_near(double a, double b) {
  return isfinite(a) && isfinite(b) &&
         fabs(a - b) <= fmax(1e-7, 1e-12 * fmax(fabs(a), fabs(b)));
}

static int musa_same_primitives(MusaPrimitiveList *a, MusaPrimitiveList *b) {
  if (a->size != b->size) return 0;
  qsort(a->items, a->size, sizeof(*a->items), musa_primitive_compare);
  qsort(b->items, b->size, sizeof(*b->items), musa_primitive_compare);
  for (size_t i = 0; i < a->size; ++i) {
    if (a->items[i].kind != b->items[i].kind ||
        a->items[i].used != b->items[i].used) return 0;
    for (unsigned j = 0; j < a->items[i].used; ++j)
      if (!musa_coordinates_near(a->items[i].value[j], b->items[i].value[j]))
        return 0;
  }
  return 1;
}

static int musa_verify_primitive_coordinates(const char *input,
                                             const Dwg_Data *decoded) {
  Dwg_Data source = {0};
  MusaPrimitiveList before = {0}, after = {0};
  source.header.version = R_2000;
  int read_status = dxf_read_file(input, &source);
  int valid = read_status < DWG_ERR_CRITICAL &&
              musa_capture_primitives(&source, &before) == 0 &&
              musa_capture_primitives(decoded, &after) == 0 &&
              musa_same_primitives(&before, &after);
  musa_free_primitives(&before);
  musa_free_primitives(&after);
  dwg_free(&source);
  return valid;
}
#endif
