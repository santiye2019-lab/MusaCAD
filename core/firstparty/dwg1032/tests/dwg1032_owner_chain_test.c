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
int main(void) {paths();long_chain();return 0;}
