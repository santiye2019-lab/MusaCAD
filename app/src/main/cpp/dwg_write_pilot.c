/* SPDX-License-Identifier: GPL-3.0-or-later */
/*
 * HOST-ONLY EXPERIMENT: DXF -> AutoCAD 2000 DWG.
 * Not linked into the Android app and not enabled in normal builds.
 * Deliberately refuses unsupported entity classes, does not overwrite files,
 * and verifies a fresh DWG decode before an exclusive publish.
 * This is NOT a general lossless DWG writer or ACIS/B-Rep editor.
 */
#include "config.h"
#include "dwg.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <limits.h>
#include <sys/stat.h>
#include <unistd.h>

#define GEOM_KINDS 7
#define MAX_INPUT_BYTES (64LL * 1024 * 1024)

typedef struct {
  unsigned long long counts[GEOM_KINDS];
  unsigned long long total;
} GeometrySignature;

static int geometry_signature(const Dwg_Data *dwg, GeometrySignature *sig) {
  memset(sig, 0, sizeof(*sig));
  for (BITCODE_BL i = 0; i < dwg->num_objects; ++i) {
    const Dwg_Object *obj = &dwg->object[i];
    int kind = -1;
    if (obj->supertype == DWG_SUPERTYPE_OBJECT) continue;
    switch (obj->fixedtype) {
      case DWG_TYPE_LINE: kind = 0; break;
      case DWG_TYPE_CIRCLE: kind = 1; break;
      case DWG_TYPE_ARC: kind = 2; break;
      case DWG_TYPE_LWPOLYLINE: kind = 3; break;
      case DWG_TYPE_POLYLINE_2D: kind = 4; break;
      case DWG_TYPE_POINT: kind = 5; break;
      case DWG_TYPE__3DFACE: kind = 6; break;
      /* Bookkeeping entities, never counted as drawing geometry. */
      case DWG_TYPE_BLOCK:
      case DWG_TYPE_ENDBLK:
      case DWG_TYPE_SEQEND: break;
      default: return -1; /* Not validated for this pilot: fail closed. */
    }
    if (kind >= 0) {
      ++sig->counts[kind];
      ++sig->total;
    }
  }
  return sig->total ? 0 : -1;
}

static int equal_geometry(const GeometrySignature *a, const GeometrySignature *b) {
  if (a->total != b->total) return 0;
  for (int i = 0; i < GEOM_KINDS; ++i)
    if (a->counts[i] != b->counts[i]) return 0;
  return 1;
}

static int pilot_write(const char *input, const char *output) {
  struct stat source_before, source_after, target;
  if (!input || !output || strcmp(input, output) == 0 ||
      stat(input, &source_before) != 0 || !S_ISREG(source_before.st_mode) ||
      source_before.st_size < 16 || source_before.st_size > MAX_INPUT_BYTES ||
      lstat(output, &target) == 0)
    return 2;
  Dwg_Data drawing = {0};
  drawing.header.version = R_2000;
  int read_status = dxf_read_file(input, &drawing);
  GeometrySignature expected;
  if (read_status >= DWG_ERR_CRITICAL ||
      geometry_signature(&drawing, &expected) != 0) {
    fprintf(stderr, "Rejecting incomplete/unsupported DXF: status=%d\n", read_status);
    dwg_free(&drawing);
    return 3;
  }
  /* LibreDWG pinned revision cannot write modern 2007+ binary versions. */
  drawing.header.version = R_2000;
  if (drawing.header.from_version == R_INVALID)
    drawing.header.from_version = R_2000;

  size_t n = strlen(output);
  if (n > 4000) { dwg_free(&drawing); return 4; }
  char *stage = malloc(n + 32);
  if (!stage) { dwg_free(&drawing); return 4; }
  sprintf(stage, "%s.musa-%ld-XXXXXX", output, (long)getpid());
  int fd = mkstemp(stage);
  if (fd < 0) { free(stage); dwg_free(&drawing); return 4; }
  close(fd);
  unlink(stage); /* Library requires a fresh path and opens it itself. */
  int write_status = dwg_write_file(stage, &drawing);
  dwg_free(&drawing);
  if (write_status >= DWG_ERR_CRITICAL ||
      stat(stage, &target) != 0 || target.st_size < 32) {
    fprintf(stderr, "DWG encoder rejected output: status=%d\n", write_status);
    unlink(stage); free(stage); return 5;
  }
  FILE *fp = fopen(stage, "rb");
  char magic[6] = {0};
  int valid_magic = fp && fread(magic, 1, 6, fp) == 6 &&
                    memcmp(magic, "AC1015", 6) == 0;
  if (fp) fclose(fp);
  Dwg_Data decoded = {0};
  int verify_status = valid_magic ? dwg_read_file(stage, &decoded) : DWG_ERR_CRITICAL;
  GeometrySignature actual;
  int valid = valid_magic && verify_status < DWG_ERR_CRITICAL &&
              geometry_signature(&decoded, &actual) == 0 &&
              equal_geometry(&expected, &actual);
  if (valid) valid = stat(input, &source_after) == 0 &&
                      source_before.st_size == source_after.st_size &&
                      source_before.st_mtime == source_after.st_mtime;
  dwg_free(&decoded);
  if (!valid) {
    fprintf(stderr, "Rejected DWG round-trip (read=%d, signatures/mtime).\n", verify_status);
    unlink(stage); free(stage); return 6;
  }
  /* link() is exclusive (EEXIST) and cannot overwrite the destination. */
  int published = link(stage, output) == 0;
  unlink(stage); free(stage);
  if (!published) return 7;
  fprintf(stderr, "Experimental R2000 DWG verified (geometry entities=%llu).\n", expected.total);
  return 0;
}

int main(int argc, char **argv) {
  if (argc != 3) {
    fprintf(stderr, "Usage: musacad_dxf2dwg_pilot INPUT.dxf NEW_OUTPUT.dwg\n");
    return 2;
  }
  return pilot_write(argv[1], argv[2]);
}
