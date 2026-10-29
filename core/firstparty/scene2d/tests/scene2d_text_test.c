/* SPDX-License-Identifier: MIT */
#include "musa_scene2d_text.h"
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <assert.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
int main(void) {
    size_t n=99;
    const uint8_t valid[]={0x41,0xc3,0xa7,0xe2,0x82,0xac,
                           0xf0,0x9f,0x98,0x80,0xcc,0x81};
    assert(musa_scene2d_utf8_count(valid,sizeof(valid),&n) && n==5);
    assert(musa_scene2d_utf8_count(NULL,0,&n) && n==0);
    const uint8_t invalid[][5]={
        {0x00,0,0,0,0}, {0xc0,0x80,0,0,0}, {0xed,0xa0,0x80,0,0},
        {0xf4,0x90,0x80,0x80,0}, {0xe2,0x28,0xa1,0,0},
        {0xf0,0x80,0x80,0x80,0}, {0x80,0,0,0,0}
    };
    const size_t invalid_sizes[]={1,2,3,4,3,4,1};
    for (size_t i=0;i<sizeof(invalid_sizes)/sizeof(invalid_sizes[0]);++i) {
        n=99;
        assert(!musa_scene2d_utf8_count(invalid[i],invalid_sizes[i],&n));
        assert(n==99);
    }
    n=99;
    assert(!musa_scene2d_utf8_count(valid,3,&n) && n==99);
    assert(!musa_scene2d_utf8_count(valid,sizeof(valid),NULL));
    uint8_t aliased[sizeof(size_t)+8]={65,66,67};
    assert(!musa_scene2d_utf8_count(aliased,sizeof(aliased),
                                   (size_t *)(void *)aliased));
    MusaScene2dTextGlyph g[3]={
        {{0,0,1,2},2,0,1},
        {{NAN,NAN,NAN,NAN},1,0,0},
        {{0,-1,2,1},2,0,1}
    };
    const double rotate[6]={0,1,-1,0,10,20};
    MusaScene2dBox box={99,98,97,96};
    assert(musa_scene2d_text_run_bounds(g,3,rotate,&box));
    assert(box.min_x<8 && box.min_x>7.999999999);
    assert(box.max_x>11 && box.max_x<11.000000001);
    assert(box.min_y<20 && box.min_y>19.999999999);
    assert(box.max_y>25 && box.max_y<25.000000001);
    const MusaScene2dBox saved=box;
    g[2].ink.min_x=NAN;
    assert(!musa_scene2d_text_run_bounds(g,3,rotate,&box));
    assert(memcmp(&saved,&box,sizeof(box))==0);
    g[2].ink.min_x=0;
    g[1].has_ink=2;
    assert(!musa_scene2d_text_run_bounds(g,3,rotate,&box));
    g[1].has_ink=0;
    g[1].advance_x=INFINITY;
    assert(!musa_scene2d_text_run_bounds(g,3,rotate,&box));
    g[1].advance_x=1;
    assert(!musa_scene2d_text_run_bounds(g,3,rotate,
                                         (MusaScene2dBox *)(void *)g));
    assert(!musa_scene2d_text_run_bounds(g,3,rotate,
                         (MusaScene2dBox *)(void *)(rotate+2)));
    const double huge[6]={1e308,0,0,1,0,0};
    assert(!musa_scene2d_text_run_bounds(g,3,huge,&box));
    assert(memcmp(&saved,&box,sizeof(box))==0);
    assert(!musa_scene2d_text_run_bounds(g,0,rotate,&box));
    MusaScene2dTextGlyph blank={ {NAN,NAN,NAN,NAN},1,0,0 };
    assert(!musa_scene2d_text_run_bounds(&blank,1,rotate,&box));
    const size_t count=20000;
    MusaScene2dTextGlyph *many=calloc(count,sizeof(*many));
    assert(many);
    for (size_t i=0;i<count;++i) {
        many[i].ink=(MusaScene2dBox){0,0,1,1};
        many[i].advance_x=1;
        many[i].has_ink=1;
    }
    const double identity[6]={1,0,0,1,0,0};
    assert(musa_scene2d_text_run_bounds(many,count,identity,&box));
    assert(box.min_x<0 && box.max_x>20000 &&
           box.min_y<0 && box.max_y>1);
    free(many);
    return 0;
}
