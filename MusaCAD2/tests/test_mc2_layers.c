#include "mc2_layers.h"
#include <assert.h>
int main(void) {
    Mc2Entity2 entity = {0};
    entity.kind = MC2_ENTITY_LINE;
    entity.layer_id = 7;
    entity.geometry.line.start = (Mc2Point2){0,0};
    entity.geometry.line.end = (Mc2Point2){1,1};
    Mc2Layer layers[] = {{7,0x123456,1,0},{8,0xabcdef,1,0}};
    Mc2EntityAppearance result;
    assert(mc2_resolve_appearance(&entity,(Mc2Color){0,MC2_COLOR_BY_LAYER},layers,2,0xffffff,1,&result));
    assert(result.visible && result.effective_rgb == 0x123456);
    assert(mc2_resolve_appearance(&entity,(Mc2Color){0x00ff00,MC2_COLOR_EXPLICIT},layers,2,0xffffff,1,&result));
    assert(result.effective_rgb == 0x00ff00);
    assert(mc2_resolve_appearance(&entity,(Mc2Color){0,MC2_COLOR_BY_BLOCK},layers,2,0x112233,1,&result));
    assert(result.effective_rgb == 0x112233);
    layers[0].is_frozen = 1;
    assert(mc2_resolve_appearance(&entity,(Mc2Color){0,MC2_COLOR_BY_LAYER},layers,2,0,1,&result));
    assert(!result.visible);
    layers[0].is_frozen = 0;
    layers[0].is_on = 0;
    assert(mc2_resolve_appearance(&entity,(Mc2Color){0,MC2_COLOR_BY_LAYER},layers,2,0,1,&result));
    assert(!result.visible);
    layers[0].is_on = 1;
    assert(mc2_resolve_appearance(&entity,(Mc2Color){0,MC2_COLOR_BY_LAYER},layers,2,0,0,&result));
    assert(!result.visible);
    layers[1].id = 7;
    assert(!mc2_resolve_appearance(&entity,(Mc2Color){0,MC2_COLOR_BY_LAYER},layers,2,0,1,&result));
    layers[1].id = 8;
    entity.layer_id = 99;
    assert(!mc2_resolve_appearance(&entity,(Mc2Color){0,MC2_COLOR_BY_LAYER},layers,2,0,1,&result));
    return 0;
}
