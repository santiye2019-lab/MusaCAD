/* Standalone host regression for float-native DWG scene coordinates. */
#include <assert.h>
#include <float.h>
#include <math.h>
#include <stdio.h>
#include "../app/src/main/cpp/scene_coord_safety.h"

int main(void){
    assert(musa_scene_xy_safe(0.0,0.0));
    assert(musa_scene_xy_safe(-123456.125,765432.75));
    assert(musa_scene_xy_safe((double)FLT_MAX,-(double)FLT_MAX));
    assert(!musa_scene_float_safe(INFINITY));
    assert(!musa_scene_float_safe(-INFINITY));
    assert(!musa_scene_float_safe(NAN));
    assert(!musa_scene_float_safe(DBL_MAX));
    assert(!musa_scene_float_safe(-DBL_MAX));
    assert(!musa_scene_float_safe((double)FLT_MAX*2.0));
    assert(!musa_scene_xy_safe(NAN,12.0));
    assert(!musa_scene_xy_safe(10.0,-INFINITY));
    assert(!musa_scene_xy_safe((double)FLT_MAX*2.0,8.0));
    puts("Native DWG preview float-coordinate safety PASS");
    return 0;
}
