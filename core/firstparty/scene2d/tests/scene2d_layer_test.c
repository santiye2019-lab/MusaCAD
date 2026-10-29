/* SPDX-License-Identifier: MIT */
#include "musa_scene2d_layer.h"
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <assert.h>
#include <stdlib.h>
#include <string.h>
int main(void) {
    MusaScene2dCadLayer cad={0,-7,0,0,0,0};
    MusaScene2dLayer layer={99,0x12345678u,9}, saved;
    assert(musa_scene2d_layer_from_cad(&cad,NULL,0,0,&layer));
    assert(layer.handle==0 && layer.rgba==0xff000000u && layer.visible==0);
    cad.signed_aci=7;
    assert(musa_scene2d_layer_from_cad(&cad,NULL,0,0xffffffu,&layer));
    assert(layer.rgba==0xffffffffu && layer.visible==1);
    cad.frozen=1;
    assert(musa_scene2d_layer_from_cad(&cad,NULL,0,0,&layer) && !layer.visible);
    cad.frozen=0; cad.viewport_frozen=1;
    assert(musa_scene2d_layer_from_cad(&cad,NULL,0,0,&layer) && !layer.visible);
    cad.viewport_frozen=0; cad.has_truecolor=1; cad.truecolor_rgb=0x123456u;
    cad.signed_aci=-42;
    assert(musa_scene2d_layer_from_cad(&cad,NULL,0,0,&layer));
    assert(layer.rgba==0xff123456u && !layer.visible);
    saved=layer;
    cad.signed_aci=0;
    assert(!musa_scene2d_layer_from_cad(&cad,NULL,0,0,&layer));
    assert(memcmp(&saved,&layer,sizeof(layer))==0);
    cad.signed_aci=-256;
    assert(!musa_scene2d_layer_from_cad(&cad,NULL,0,0,&layer));
    cad.signed_aci=INT16_MIN;
    assert(!musa_scene2d_layer_from_cad(&cad,NULL,0,0,&layer));
    cad.signed_aci=42; cad.has_truecolor=0;
    assert(!musa_scene2d_layer_from_cad(&cad,NULL,0,0,&layer));
    uint32_t palette[256]={0}; palette[42]=0xabcdefu;
    assert(musa_scene2d_layer_from_cad(&cad,palette,256,0,&layer));
    assert(layer.rgba==0xffabcdefu && layer.visible==1);
    cad.frozen=2;
    assert(!musa_scene2d_layer_from_cad(&cad,palette,256,0,&layer));
    cad.frozen=0; cad.viewport_frozen=2;
    assert(!musa_scene2d_layer_from_cad(&cad,palette,256,0,&layer));
    cad.viewport_frozen=0; cad.has_truecolor=2;
    assert(!musa_scene2d_layer_from_cad(&cad,palette,256,0,&layer));
    cad.has_truecolor=0;
    assert(!musa_scene2d_layer_from_cad(&cad,palette,256,0,
           (MusaScene2dLayer *)(void *)&palette[0]));
    const size_t count=20000;
    MusaScene2dCadLayer *inputs=calloc(count,sizeof(*inputs));
    MusaScene2dLayer *outputs=calloc(count,sizeof(*outputs));
    assert(inputs && outputs);
    for (size_t i=0;i<count;++i) {
        inputs[i].handle=(uint64_t)i;
        inputs[i].signed_aci=(i%3u==0u) ? -42 : 42;
        inputs[i].frozen=(uint8_t)(i%5u==0u);
        inputs[i].viewport_frozen=(uint8_t)(i%7u==0u);
    }
    assert(musa_scene2d_layers_from_cad(inputs,count,palette,256,0,outputs,count));
    for (size_t i=0;i<count;++i) {
        assert(outputs[i].handle==(uint64_t)i && outputs[i].rgba==0xffabcdefu);
        assert(outputs[i].visible==(uint8_t)(i%3u!=0u && i%5u!=0u && i%7u!=0u));
    }
    outputs[0].rgba=0xdeadbeefu;
    inputs[count-1].signed_aci=0;
    assert(!musa_scene2d_layers_from_cad(inputs,count,palette,256,0,outputs,count));
    assert(outputs[0].rgba==0xdeadbeefu);
    inputs[count-1].signed_aci=42;
    assert(!musa_scene2d_layers_from_cad(inputs,count,palette,256,0,outputs,count-1));
    assert(musa_scene2d_layers_from_cad(NULL,0,NULL,0,0,NULL,0));
    free(outputs); free(inputs);
    return 0;
}
