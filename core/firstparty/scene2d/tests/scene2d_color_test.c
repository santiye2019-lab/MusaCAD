/* SPDX-License-Identifier: MIT
 * Standalone color adapter tests, including scene graph inheritance.
 */
#include "musa_scene2d_color.h"
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <assert.h>
#include <string.h>
static void graph_color_inheritance(void) {
    const MusaScene2dLayer layers[]={{0,0xffffffffu,1},
                                     {10,0xff112233u,1}};
    MusaScene2dNode nodes[]={
        {100,0,10,0,MUSA_SCENE2D_INSERT,1,MUSA_SCENE2D_BYLAYER,
         {1,0,0,1,0,0}},
        {200,100,0,0,MUSA_SCENE2D_ENTITY,1,MUSA_SCENE2D_BYBLOCK,
         {1,0,0,1,2,3}},
        {300,100,0,0,MUSA_SCENE2D_ENTITY,1,MUSA_SCENE2D_EXPLICIT,
         {1,0,0,1,4,5}}
    };
    uint32_t palette[256]={0};
    palette[42]=0x123456u;
    MusaScene2dCadColor c={42,0,0};
    MusaScene2dColorSelection selection={0,0};
    assert(musa_scene2d_cad_color_select(&c,palette,256,0,&selection));
    nodes[2].color_mode=selection.mode;
    nodes[2].rgba=selection.rgba;
    MusaScene2dResolved resolved[3];
    size_t written=99;
    assert(musa_scene2d_resolve(nodes,3,layers,2,resolved,3,&written));
    assert(written==3 && resolved[0].rgba==0xff112233u);
    assert(resolved[1].effective_layer==10 &&
           resolved[1].rgba==0xff112233u);
    assert(resolved[2].effective_layer==10 &&
           resolved[2].rgba==0xff123456u);
}
int main(void) {
    MusaScene2dColorSelection s={99,0x12345678u};
    MusaScene2dCadColor c={0,0,0};
    assert(musa_scene2d_cad_color_select(&c,NULL,0,0xffffffu,&s));
    assert(s.mode==MUSA_SCENE2D_BYBLOCK && s.rgba==0);
    c.aci=256;
    assert(musa_scene2d_cad_color_select(&c,NULL,0,0xffffffu,&s));
    assert(s.mode==MUSA_SCENE2D_BYLAYER && s.rgba==0);
    const uint32_t expected[7]={0,0xffff0000u,0xffffff00u,0xff00ff00u,
                                 0xff00ffffu,0xff0000ffu,0xffff00ffu};
    for (uint16_t i=1;i<=6;++i) {
        c.aci=i;
        assert(musa_scene2d_cad_color_select(&c,NULL,0,0,&s));
        assert(s.mode==MUSA_SCENE2D_EXPLICIT && s.rgba==expected[i]);
    }
    c.aci=7;
    assert(musa_scene2d_cad_color_select(&c,NULL,0,0,&s));
    assert(s.rgba==0xff000000u);
    assert(musa_scene2d_cad_color_select(&c,NULL,0,0xffffffu,&s));
    assert(s.rgba==0xffffffffu);
    c.aci=42;
    MusaScene2dColorSelection saved=s;
    assert(!musa_scene2d_cad_color_select(&c,NULL,0,0,&s));
    assert(memcmp(&s,&saved,sizeof s)==0);
    uint32_t palette[256]={0};
    palette[42]=0x123456u;
    palette[7]=0x010203u;
    assert(musa_scene2d_cad_color_select(&c,palette,256,0,&s));
    assert(s.rgba==0xff123456u);
    c.aci=7;
    assert(musa_scene2d_cad_color_select(&c,palette,256,0,&s));
    assert(s.rgba==0xff010203u);
    c.has_truecolor=1;
    c.truecolor_rgb=0x7f44ccu;
    assert(musa_scene2d_cad_color_select(&c,NULL,0,0,&s));
    assert(s.rgba==0xff7f44ccu);
    saved=s;
    c.truecolor_rgb=0x1000000u;
    assert(!musa_scene2d_cad_color_select(&c,NULL,0,0,&s));
    assert(memcmp(&s,&saved,sizeof s)==0);
    c.truecolor_rgb=0x7f44ccu;
    c.has_truecolor=2;
    assert(!musa_scene2d_cad_color_select(&c,NULL,0,0,&s));
    c.has_truecolor=0;
    c.aci=257;
    assert(!musa_scene2d_cad_color_select(&c,NULL,0,0,&s));
    c.aci=8;
    assert(!musa_scene2d_cad_color_select(&c,palette,255,0,&s));
    assert(!musa_scene2d_cad_color_select(&c,palette,256,0x1000000u,&s));
    palette[8]=0x1000000u;
    assert(!musa_scene2d_cad_color_select(&c,palette,256,0,&s));
    assert(memcmp(&s,&saved,sizeof s)==0);
    assert(!musa_scene2d_cad_color_select(&c,NULL,0,0,
        (MusaScene2dColorSelection *)(void *)&c));
    assert(!musa_scene2d_cad_color_select(&c,palette,256,0,
        (MusaScene2dColorSelection *)(void *)&palette[8]));
    graph_color_inheritance();
    return 0;
}
