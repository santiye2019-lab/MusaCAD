#include "mc2_ownership.h"
#include <assert.h>
int main(void) {
    Mc2OwnerNode nodes[] = {{10,0,1},{20,10,1},{30,20,1},{40,10,0}};
    int visible = -1;
    assert(mc2_ownership_validate(nodes,4));
    assert(mc2_ownership_visibility(nodes,4,30,&visible) && visible);
    nodes[0].visible = 0;
    assert(mc2_ownership_visibility(nodes,4,30,&visible) && !visible);
    nodes[0].visible = 1;
    assert(mc2_ownership_visibility(nodes,4,40,&visible) && !visible);
    nodes[0].parent_handle = 30;
    assert(!mc2_ownership_validate(nodes,4));
    assert(!mc2_ownership_visibility(nodes,4,30,&visible));
    nodes[0].parent_handle = 0;
    nodes[2].parent_handle = 99;
    assert(!mc2_ownership_validate(nodes,4));
    nodes[2].parent_handle = 20;
    nodes[2].handle = 20;
    assert(!mc2_ownership_validate(nodes,4));
    assert(!mc2_ownership_visibility(nodes,4,20,&visible));
    return 0;
}
