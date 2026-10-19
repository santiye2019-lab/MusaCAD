/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "musacad_dwg_ac1018_file_curve.h"
#include "musacad_dwg_ac1018_scene.h"
#include <stddef.h>
#include <stdint.h>
static int overlaps(const void *a,size_t na,const void *b,size_t nb){
    const uintptr_t x=(uintptr_t)a,y=(uintptr_t)b;
    return na&&nb&&((x<=y&&y-x<na)||(y<x&&x-y<nb));
}
int musa_dwg_ac1018_file_read_curve(const uint8_t *file,size_t file_bytes,
                                   uint64_t handle,MusaDwgAc1018Curve *out){
    if(!file||!out||!handle||
       overlaps(file,file_bytes,out,sizeof(*out)))return 0;
    MusaDwgAc1018Scene *scene=NULL;
    if(!musa_dwg_ac1018_scene_open(file,file_bytes,&scene))return 0;
    const int ok=musa_dwg_ac1018_scene_read_curve(scene,handle,out);
    musa_dwg_ac1018_scene_close(scene);
    return ok;
}
