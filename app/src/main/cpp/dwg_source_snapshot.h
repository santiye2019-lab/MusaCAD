/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef MUSACAD_DWG_SOURCE_SNAPSHOT_H
#define MUSACAD_DWG_SOURCE_SNAPSHOT_H
/*
 * Exact source-byte guard for the HOST-ONLY R2000 pilot.
 *
 * This pilot already caps DXF inputs at 64 MiB. Retaining an exact bounded
 * snapshot lets us reject same-size/same-mtime concurrent changes that the
 * previous size+mtime-only guard silently accepted. No hash collision and
 * no third-party crypto runtime dependency. Do not use this memory-heavy
 * approach for unlimited-size DWG files or Android production paths.
 */
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>

typedef struct {
  unsigned char *bytes;
  size_t length;
  dev_t device;
  ino_t inode;
} MusaDxfSourceSnapshot;

static void musa_dxf_source_release(MusaDxfSourceSnapshot *snap) {
  if (!snap) return;
  free(snap->bytes);
  memset(snap, 0, sizeof(*snap));
}

static int musa_dxf_source_capture(
    const char *path, size_t max_bytes, MusaDxfSourceSnapshot *snap) {
  struct stat st;
  if (!path || !snap || stat(path, &st) != 0 ||
      !S_ISREG(st.st_mode) || st.st_size <= 0 ||
      (uint64_t)st.st_size > (uint64_t)max_bytes)
    return 0;
  MusaDxfSourceSnapshot value = {0};
  value.length = (size_t)st.st_size;
  value.device = st.st_dev;
  value.inode = st.st_ino;
  value.bytes = (unsigned char *)malloc(value.length);
  if (!value.bytes) return 0;
  FILE *stream = fopen(path, "rb");
  if (!stream) { musa_dxf_source_release(&value); return 0; }
  size_t got = fread(value.bytes, 1, value.length, stream);
  int no_trailing_bytes = fgetc(stream) == EOF && !ferror(stream);
  int closed = fclose(stream) == 0;
  struct stat after;
  int unchanged = stat(path, &after) == 0 &&
      after.st_dev == value.device && after.st_ino == value.inode &&
      after.st_size == st.st_size && after.st_mtime == st.st_mtime;
  if (got != value.length || !no_trailing_bytes || !closed || !unchanged) {
    musa_dxf_source_release(&value);
    return 0;
  }
  *snap = value;
  return 1;
}

/* Constant-extra-memory reread; exact byte comparison, not size/mtime alone. */
static int musa_dxf_source_matches(
    const char *path, const MusaDxfSourceSnapshot *snap) {
  struct stat st;
  if (!path || !snap || !snap->bytes || !snap->length ||
      stat(path, &st) != 0 || !S_ISREG(st.st_mode) ||
      st.st_dev != snap->device || st.st_ino != snap->inode ||
      st.st_size < 0 || (uint64_t)st.st_size != (uint64_t)snap->length)
    return 0;
  FILE *stream = fopen(path, "rb");
  if (!stream) return 0;
  unsigned char block[8192];
  size_t offset = 0;
  int matches = 1;
  while (offset < snap->length) {
    size_t want = snap->length - offset;
    if (want > sizeof(block)) want = sizeof(block);
    if (fread(block, 1, want, stream) != want ||
        memcmp(block, snap->bytes + offset, want) != 0) {
      matches = 0;
      break;
    }
    offset += want;
  }
  if (matches && (fgetc(stream) != EOF || ferror(stream))) matches = 0;
  if (fclose(stream) != 0) matches = 0;
  struct stat after;
  if (!matches || stat(path, &after) != 0 ||
      after.st_dev != snap->device || after.st_ino != snap->inode ||
      (uint64_t)after.st_size != (uint64_t)snap->length)
    return 0;
  return 1;
}
#endif
