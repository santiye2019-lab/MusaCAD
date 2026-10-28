#include "mc2_scene.h"
#include <assert.h>
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
    return 0;
}
