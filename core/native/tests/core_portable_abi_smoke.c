/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Run this exact shared C ABI smoke test on Linux, Windows, and macOS.
 * The same underlying C sources also build with Android NDK and iPhoneOS. */
#include "musacad_core.h"
#include "musacad_bits.h"
#include "musacad_pick_bounds.h"
#include "musacad_dwg_signature.h"
#include <assert.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
int main(void){
    assert(musa_core_abi_version()==MUSA_CORE_ABI_VERSION);
    uint8_t bytes[2]={0,0};
    MusaBits writer={0},reader={0};
    assert(musa_bits_writer(&writer,bytes,2));
    assert(musa_bits_write(&writer,6,41));
    assert(musa_bits_reader(&reader,bytes,2));
    uint64_t out=0;
    assert(musa_bits_read(&reader,6,&out)&&out==41);
    const float triangle[]={0,0,0,3,0,0,0,4,0};
    const int face[]={0,1,2};
    MusaCoreMeshSurface surface={0};
    assert(musa_core_mesh_surface_f32(triangle,3,face,1,&surface));
    assert(fabs(surface.area-6.)<1e-12);
    float circle[]={0,0,10,0};
    assert(!musa_core_pick_bounds_f32(circle,4,1,100,100,1));
    const unsigned char signature[]={'A','C','1','0','3','2'};
    assert(musa_core_dwg_identify_signature(signature,6)==MUSA_DWG_2018_2026);
    puts("PASS: identical CAD C ABI, bit I/O, 2D picking, 3D surface, DWG header recognition");
    return 0;
}
