#include "mc2_scene.h"
#include <assert.h>
#include <stdlib.h>
int main(void) {
    Mc2SceneItem items[2] = {0};
    Mc2Layer layers[] = {{0,0xffffff,1,0}};
    Mc2EntityAppearance a;
    for (int i=0;i<2;++i) {
        items[i].entity.handle = (uint64_t)(i+1);
        items[i].entity.kind = MC2_ENTITY_LINE;
        items[i].entity.geometry.line.end.x = 1;
        items[i].color.mode = MC2_COLOR_BY_LAYER;
        items[i].own_visible = 1;
    }
    items[1].parent_handle = 1;
    assert(mc2_scene_resolve(items,2,1,layers,1,0,&a));
    assert(a.visible && a.effective_rgb == 0xffffff);
    items[0].own_visible = 0;
    assert(mc2_scene_resolve(items,2,1,layers,1,0,&a) && !a.visible);
    items[0].own_visible = 1;
    items[0].parent_handle = 2;
    assert(!mc2_scene_resolve(items,2,1,layers,1,0,&a));
    items[0].parent_handle = 0;
    items[1].parent_handle = 99;
    assert(!mc2_scene_resolve(items,2,1,layers,1,0,&a));
    items[1].parent_handle = 1;
    items[1].entity.handle = 1;
    assert(!mc2_scene_resolve(items,2,1,layers,1,0,&a));
    /* Ancestor layer visibility must propagate into nested inserts. */
    items[1].entity.handle=2;
    items[0].entity.layer_id=1;
    Mc2Layer extended[]={{0,0xffffff,1,0},{1,0x123456,0,0}};
    Mc2EntityAppearance output[2]={{0xdeadbeef,9},{0xdeadbeef,9}};
    assert(mc2_scene_resolve_all(items,2,extended,2,0,output));
    assert(!output[0].visible && !output[1].visible);
    extended[1].is_on=1;
    assert(mc2_scene_resolve_all(items,2,extended,2,0,output));
    assert(output[0].visible && output[1].visible);
    items[0].parent_handle=2;
    output[0].effective_rgb=0xdeadbeef;
    assert(!mc2_scene_resolve_all(items,2,extended,2,0,output));
    assert(output[0].effective_rgb==0xdeadbeef);
    items[0].parent_handle=0;
    /* Long ownership chains must be iterative and indexed, not recursive. */
    const size_t n=2048;
    Mc2SceneItem *large=(Mc2SceneItem *)calloc(n,sizeof(*large));
    Mc2EntityAppearance *appearances=(Mc2EntityAppearance *)calloc(n,sizeof(*appearances));
    assert(large && appearances);
    for (size_t i=0;i<n;++i) {
        large[i].entity.handle=(uint64_t)(i+1);
        large[i].entity.kind=MC2_ENTITY_LINE;
        large[i].entity.geometry.line.end.x=1;
        large[i].color.mode=MC2_COLOR_BY_LAYER;
        large[i].parent_handle=i?(uint64_t)i:0;
        large[i].own_visible=1;
    }
    assert(mc2_scene_resolve_all(large,n,layers,1,0,appearances));
    assert(appearances[n-1].visible);
    large[1023].own_visible=0;
    assert(mc2_scene_resolve_all(large,n,layers,1,0,appearances));
    assert(appearances[1000].visible && !appearances[n-1].visible);
    large[1023].parent_handle=(uint64_t)n;
    assert(!mc2_scene_resolve_all(large,n,layers,1,0,appearances));
    /* BYBLOCK must use the immediate parent insertion's resolved RGB,
     * not the scene's external fallback color. */
    Mc2SceneItem nested[3]={0};
    Mc2EntityAppearance nested_out[3]={{0}};
    for (size_t i=0;i<3;++i) {
        nested[i].entity.handle=(uint64_t)(i+100);
        nested[i].entity.kind=MC2_ENTITY_LINE;
        nested[i].entity.geometry.line.end.x=1;
        nested[i].own_visible=1;
        nested[i].parent_handle=i ? (uint64_t)(i+99) : 0;
        nested[i].color.mode=MC2_COLOR_BY_BLOCK;
    }
    nested[0].color=(Mc2Color){0x123456,MC2_COLOR_EXPLICIT};
    assert(mc2_scene_resolve_all(nested,3,layers,1,0xaabbcc,nested_out));
    assert(nested_out[0].effective_rgb==0x123456);
    assert(nested_out[1].effective_rgb==0x123456);
    assert(nested_out[2].effective_rgb==0x123456);
    nested[1].color=(Mc2Color){0,MC2_COLOR_BY_LAYER};
    assert(mc2_scene_resolve_all(nested,3,layers,1,0xaabbcc,nested_out));
    assert(nested_out[1].effective_rgb==0xffffff);
    assert(nested_out[2].effective_rgb==0xffffff);
    nested[0].color=(Mc2Color){0,MC2_COLOR_BY_BLOCK};
    nested[1].color=(Mc2Color){0,MC2_COLOR_BY_BLOCK};
    assert(mc2_scene_resolve_all(nested,3,layers,1,0xaabbcc,nested_out));
    assert(nested_out[2].effective_rgb==0xaabbcc);
    nested[0].own_visible=0;
    assert(mc2_scene_resolve_all(nested,3,layers,1,0xaabbcc,nested_out));
    assert(!nested_out[2].visible && nested_out[2].effective_rgb==0xaabbcc);
    /* Large, unsorted layer tables exercise indexed lookups and duplicate rejection. */
    const size_t layer_n=1024;
    Mc2Layer *many_layers=(Mc2Layer *)calloc(layer_n,sizeof(*many_layers));
    assert(many_layers);
    for (size_t i=0;i<layer_n;++i) {
        many_layers[i].id=(uint32_t)(layer_n-1-i);
        many_layers[i].color_rgb=(uint32_t)i;
        many_layers[i].is_on=1;
    }
    large[1023].parent_handle=1023;
    large[1023].own_visible=1;
    for (size_t i=0;i<n;++i) large[i].entity.layer_id=(uint32_t)(i%layer_n);
    assert(mc2_scene_resolve_all(large,n,many_layers,layer_n,0,appearances));
    assert(appearances[n-1].visible);
    assert(appearances[0].effective_rgb==1023);
    assert(appearances[1].effective_rgb==1022);
    many_layers[100].id=many_layers[101].id;
    appearances[0].effective_rgb=0xdeadbeef;
    assert(!mc2_scene_resolve_all(large,n,many_layers,layer_n,0,appearances));
    assert(appearances[0].effective_rgb==0xdeadbeef);
    many_layers[100].id=(uint32_t)(layer_n-101);
    many_layers[0].color_rgb=0x1000000;
    assert(!mc2_scene_resolve_all(large,n,many_layers,layer_n,0,appearances));
    assert(appearances[0].effective_rgb==0xdeadbeef);
    free(many_layers);
    free(appearances);
    free(large);
    return 0;
}
