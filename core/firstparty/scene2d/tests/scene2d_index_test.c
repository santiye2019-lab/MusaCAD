/* SPDX-License-Identifier: MIT */
#include "musa_scene2d_index.h"
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

static void atomic_and_edges(void) {
    MusaScene2dPreparedBounds p[5]={
        {{0,0,1,1},1}, {{10,10,20,20},1}, {{0,0,0,0},0},
        {{20,20,30,30},1}, {{-INFINITY,0,-100,2},1}
    };
    MusaScene2dIndexNode tree[16];
    size_t leaves=91, written=99;
    memset(tree,0xa5,sizeof tree);
    MusaScene2dIndexNode saved[16];
    memcpy(saved,tree,sizeof tree);
    assert(!musa_scene2d_index_build(p,5,tree,15,&leaves));
    assert(leaves==91 && memcmp(tree,saved,sizeof tree)==0);
    p[1].box.min_x=NAN;
    assert(!musa_scene2d_index_build(p,5,tree,16,&leaves));
    assert(leaves==91 && memcmp(tree,saved,sizeof tree)==0);
    p[1].box.min_x=10;
    assert(musa_scene2d_index_build(p,5,tree,16,&leaves));
    assert(leaves==8 && tree[1].occupied);
    size_t indices[5]={91,92,93,94,95};
    assert(musa_scene2d_index_query(tree,5,leaves,
           (MusaScene2dBox){20,20,20,20},indices,5,&written));
    assert(written==2 && indices[0]==1 && indices[1]==3);
    written=99;indices[0]=91;
    assert(!musa_scene2d_index_query(tree,5,leaves,
           (MusaScene2dBox){0,0,100,100},indices,2,&written));
    assert(written==99 && indices[0]==91);
    assert(!musa_scene2d_index_query(tree,5,7,
           (MusaScene2dBox){0,0,1,1},indices,5,&written));
    assert(!musa_scene2d_index_query(tree,5,leaves,
           (MusaScene2dBox){NAN,0,1,1},indices,5,&written));
    assert(!musa_scene2d_index_query(tree,5,leaves,
           (MusaScene2dBox){0,0,1,1},(size_t *)(void *)tree,5,&written));
    assert(!musa_scene2d_index_build(p,5,
           (MusaScene2dIndexNode *)(void *)p,16,&leaves));
    assert(musa_scene2d_index_build(NULL,0,NULL,0,&leaves));
    assert(leaves==0);
    assert(musa_scene2d_index_query(NULL,0,0,
           (MusaScene2dBox){0,0,1,1},NULL,0,&written));
    assert(written==0);
}
static void large_viewport_equivalence(void) {
    const size_t count=20000, leaves_capacity=32768;
    MusaScene2dPreparedBounds *p=calloc(count,sizeof *p);
    MusaScene2dIndexNode *tree=calloc(2*leaves_capacity,sizeof *tree);
    size_t *slow=calloc(count,sizeof *slow), *fast=calloc(count,sizeof *fast);
    assert(p && tree && slow && fast);
    for (size_t i=0;i<count;++i) {
        /* Nearby source indices occupy nearby regions; a common CAD
         * export order where tree pruning saves work. */
        p[i].box=(MusaScene2dBox){
            (double)((i/100)*20), (double)((i%100)*20),
            (double)((i/100)*20+5), (double)((i%100)*20+5)};
        p[i].visible=(i%19u)!=0u;
    }
    size_t leaves=0;
    assert(musa_scene2d_index_build(p,count,tree,2*leaves_capacity,&leaves));
    assert(leaves==leaves_capacity);
    for (size_t t=0;t<500;++t) {
        const double x=(double)((t*17u)%220u)*20.0;
        const double y=(double)((t*31u)%110u)*20.0;
        const MusaScene2dBox view={x,y,x+35,y+35};
        size_t nslow=0,nfast=0;
        assert(musa_scene2d_collect_prepared(p,count,view,slow,count,&nslow));
        assert(musa_scene2d_index_query(tree,count,leaves,view,fast,count,&nfast));
        assert(nslow==nfast);
        for (size_t k=0;k<nslow;++k) assert(slow[k]==fast[k]);
    }
    free(fast);free(slow);free(tree);free(p);
}
int main(void) {
    atomic_and_edges();
    large_viewport_equivalence();
    return 0;
}
