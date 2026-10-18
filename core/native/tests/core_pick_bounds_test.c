/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "musacad_pick_bounds.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
int main(void){
    float line[]={-40.f,20.f,100.f,-20.f};
    assert(musa_core_pick_bounds_f32(line,4,0,0,0,0));
    assert(!musa_core_pick_bounds_f32(line,4,0,200,200,1));
    float c[]={0,0,10,0};
    assert(musa_core_pick_bounds_f32(c,4,1,10,0,.01f));
    assert(!musa_core_pick_bounds_f32(c,4,1,100,100,5));
    float arc[]={15,0,0,15,-15,0,0,0};
    assert(musa_core_pick_bounds_f32(arc,8,2,-15,0,1));
    assert(!musa_core_pick_bounds_f32(arc,8,2,100,100,1));
    assert(musa_core_pick_bounds_f32(line,4,3,100000,100000,0));
    assert(musa_core_pick_bounds_f32(NULL,4,0,10,10,0));
    assert(musa_core_pick_bounds_f32(line,4,0,NAN,0,0));
    assert(musa_core_pick_bounds_f32(line,4,0,0,0,-1));
    assert(musa_core_pick_bounds_f32(line,4,8,10000,10000,0));
    float bad[]={NAN,0,10,10};
    assert(musa_core_pick_bounds_f32(bad,4,0,10000,10000,0));
    float big[]={1000000.f,1000000.f,1000040.f,1000080.f};
    assert(musa_core_pick_bounds_f32(big,4,0,1000000.f,1000000.f,0));
    assert(!musa_core_pick_bounds_f32(big,4,0,0,0,1));
    puts("PASS: portable conservative 2D pre-pick, bounds, unknown data, large WCS");
    return 0;
}
