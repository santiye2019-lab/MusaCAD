#include "mc2_ownership.h"
#include <assert.h>
#include <stdlib.h>
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
    /* Deep, reverse-ordered ownership chain; validation must be indexed. */
    const size_t n=4096;
    Mc2OwnerNode *deep=(Mc2OwnerNode *)malloc(n*sizeof(*deep));
    assert(deep);
    for (size_t i=0;i<n;++i) {
        deep[i].handle=(uint64_t)(n-i);
        deep[i].parent_handle=(i+1==n)?0:(uint64_t)(n-i-1);
        deep[i].visible=1;
    }
    assert(mc2_ownership_validate(deep,n));
    deep[n-1].parent_handle=(uint64_t)n;
    assert(!mc2_ownership_validate(deep,n)); /* cycle */
    deep[n-1].parent_handle=0;
    deep[0].parent_handle=(uint64_t)(n+1);
    assert(!mc2_ownership_validate(deep,n)); /* missing parent */
    deep[0].parent_handle=(uint64_t)(n-1);
    deep[0].handle=deep[1].handle;
    assert(!mc2_ownership_validate(deep,n)); /* duplicate */
    deep[0].handle=(uint64_t)n;
    deep[0].handle=0;
    assert(!mc2_ownership_validate(deep,n)); /* invalid handle */
    free(deep);
    assert(mc2_ownership_validate(0,0));
    assert(!mc2_ownership_validate(0,1));
    return 0;
}
