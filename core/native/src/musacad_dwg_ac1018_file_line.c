/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "musacad_dwg_ac1018_file_line.h"
#include "musacad_dwg_ac1018_scene.h"
#include <stddef.h>
#include <stdint.h>
/* Retain the original one-shot read-only API while routing every AC1018
 * geometry semantic through the SAME first-party validated scene engine.
 * Call scene_open once and reuse it for bulk reads to avoid repeated
 * decompression and handle-index reconstruction on large drawings. */
static int overlaps(const void *a,size_t na,const void *b,size_t nb){
    const uintptr_t x=(uintptr_t)a,y=(uintptr_t)b;
    return na&&nb&&((x<=y&&y-x<na)||(y<x&&x-y<nb));
}
int musa_dwg_ac1018_file_read_line(const uint8_t *file,size_t file_bytes,
    uint64_t handle,MusaDwgAc1018Line *out){
    if(!file||!out||!handle||
       overlaps(file,file_bytes,out,sizeof(*out)))return 0;
    MusaDwgAc1018Scene *session=NULL;
    if(!musa_dwg_ac1018_scene_open(file,file_bytes,&session))return 0;
    const int ok=musa_dwg_ac1018_scene_read_line(session,handle,out);
    musa_dwg_ac1018_scene_close(session);
    return ok;
}
