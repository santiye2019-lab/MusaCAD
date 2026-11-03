/* SPDX-License-Identifier: MIT */
#include "musa_dwg1032_owner_chain.h"
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <assert.h>
#include <stddef.h>
#include <stdint.h>
static void paths(void) {
    const MusaDwg1032ObjectLocation entries[]={
        {2u,0u,1u},{4u,1u,1u},{8u,2u,1u},{16u,3u,1u}
    };
    MusaDwg1032ObjectIndex index;
    assert(musa_dwg1032_object_index_init(&index,entries,4u,4u));
    uint64_t parents[]={0u,2u,4u,8u};
    uint64_t root=99u;
    size_t depth=99u;
    assert(musa_dwg1032_owner_chain_root(&index,parents,3u,3u,&root,&depth));
    assert(root==2u && depth==3u);
    root=99u;depth=99u;
    assert(!musa_dwg1032_owner_chain_root(&index,parents,3u,2u,&root,&depth));
    assert(root==99u && depth==99u);
    assert(musa_dwg1032_owner_chain_root(&index,parents,0u,0u,&root,&depth));
    assert(root==2u && depth==0u);
    parents[0]=16u;root=99u;depth=99u;
    assert(!musa_dwg1032_owner_chain_root(&index,parents,3u,10u,&root,&depth));
    assert(root==99u && depth==99u);
    parents[0]=0u;parents[1]=999u;
    assert(!musa_dwg1032_owner_chain_root(&index,parents,2u,10u,&root,&depth));
    assert(root==99u && depth==99u);
    parents[1]=2u;
    assert(!musa_dwg1032_owner_chain_root(&index,parents,4u,10u,&root,&depth));
    assert(!musa_dwg1032_owner_chain_root(&index,parents,0u,10u,&parents[0],&depth));
    assert(!musa_dwg1032_owner_chain_root(&index,parents,0u,10u,&root,(size_t *)&root));
    assert(!musa_dwg1032_owner_chain_root(&index,parents,0u,10u,NULL,&depth));
}
static void long_chain(void) {
    MusaDwg1032ObjectLocation entries[4096];
    uint64_t parents[4096];
    for (size_t i=0;i<4096u;++i) {
        entries[i]=(MusaDwg1032ObjectLocation){(uint64_t)i+1u,(uint64_t)i,1u};
        parents[i]=i?(uint64_t)i:0u;
    }
    MusaDwg1032ObjectIndex index;
    assert(musa_dwg1032_object_index_init(&index,entries,4096u,4096u));
    uint64_t root=0;
    size_t depth=0;
    assert(musa_dwg1032_owner_chain_root(&index,parents,4095u,4095u,&root,&depth));
    assert(root==1u && depth==4095u);
}
static void stale_index_and_alias(void) {
    MusaDwg1032ObjectLocation entries[]={{2u,0u,1u},{4u,1u,1u}};
    MusaDwg1032ObjectIndex index;
    assert(musa_dwg1032_object_index_init(&index,entries,2u,2u));
    const uint64_t owners[]={0u,2u};
    uint64_t root=123u;
    size_t hops=456u;
    assert(musa_dwg1032_owner_chain_root(
        &index,owners,1u,2u,&root,&hops));
    assert(root==2u && hops==1u);
    root=123u;hops=456u;
    entries[0].length=0u; /* Stale index: reject a malformed ancestor. */
    assert(!musa_dwg1032_owner_chain_root(
        &index,owners,1u,2u,&root,&hops));
    assert(root==123u && hops==456u);
    entries[0].length=1u;
    entries[0].offset=2u; /* Root span now extends beyond section. */
    assert(!musa_dwg1032_owner_chain_root(
        &index,owners,0u,2u,&root,&hops));
    assert(root==123u && hops==456u);
    entries[0].offset=0u;
    entries[0].handle=0u; /* A zero handle cannot become a valid root. */
    assert(!musa_dwg1032_owner_chain_root(
        &index,owners,0u,2u,&root,&hops));
    assert(root==123u && hops==456u);
    entries[0].handle=2u;
    /* Reject owner storage that aliases the index header itself. */
    assert(!musa_dwg1032_owner_chain_root(
        &index,(const uint64_t *)(const void *)&index,0u,2u,
        &root,&hops));
    assert(root==123u && hops==456u);
}
int main(void) {paths();long_chain();stale_index_and_alias();return 0;}
