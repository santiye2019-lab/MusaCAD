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
static void paged_queries_and_rollback(void) {
    MusaScene2dPreparedBounds p[6]={
        {{0,0,1,1},1}, {{10,10,11,11},1}, {{0,0,1,1},0},
        {{20,20,21,21},1}, {{30,30,31,31},1}, {{40,40,41,41},1}
    };
    MusaScene2dIndexNode tree[16];
    size_t leaves=0;
    assert(musa_scene2d_index_build(p,6,tree,16,&leaves));
    const MusaScene2dBox view={0,0,100,100};
    const size_t expected[]={0,1,3,4,5};
    for (size_t skip=0;skip<=7;++skip) {
        for (size_t cap=0;cap<=7;++cap) {
            size_t page[8],got=99,total=98;
            for (size_t i=0;i<8;++i) page[i]=777;
            assert(musa_scene2d_index_query_page(
                tree,6,leaves,view,skip,page,cap,&got,&total));
            const size_t remaining=skip<5 ? 5-skip:0;
            const size_t want=remaining<cap ? remaining:cap;
            assert(total==5 && got==want);
            for (size_t i=0;i<want;++i) assert(page[i]==expected[skip+i]);
            for (size_t i=want;i<8;++i) assert(page[i]==777);
        }
    }
    size_t got=99,total=98;
    assert(musa_scene2d_index_query_page(
        tree,6,leaves,view,0,NULL,0,&got,&total));
    assert(got==0 && total==5);
    assert(musa_scene2d_index_query_page(
        tree,6,leaves,view,SIZE_MAX,NULL,0,&got,&total));
    assert(got==0 && total==5);
    size_t page[8]={777};
    got=99;total=98;
    assert(!musa_scene2d_index_query_page(
        tree,6,leaves,(MusaScene2dBox){NAN,0,1,1},0,
        page,8,&got,&total));
    assert(got==99 && total==98 && page[0]==777);
    assert(!musa_scene2d_index_query_page(
        tree,6,7,view,0,page,8,&got,&total));
    assert(!musa_scene2d_index_query_page(
        tree,6,leaves,view,0,(size_t *)(void *)tree,8,&got,&total));
    assert(!musa_scene2d_index_query_page(
        tree,6,leaves,view,0,page,8,&got,&got));
    assert(!musa_scene2d_index_query_page(
        tree,6,leaves,view,0,page,8,&page[0],&total));
    assert(!musa_scene2d_index_query_page(
        tree,6,leaves,view,0,NULL,1,&got,&total));
    assert(got==99 && total==98 && page[0]==777);
    assert(musa_scene2d_index_query_page(
        NULL,0,0,view,0,NULL,0,&got,&total));
    assert(got==0 && total==0);
    got=99;total=98;
    assert(!musa_scene2d_index_query_page(
        NULL,0,1,view,0,NULL,0,&got,&total));
    assert(got==99 && total==98);
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
    /* One wide viewport: paginate nearly 20,000 hits without a large
     * per-query result allocation, preserving the full query's ordering. */
    const MusaScene2dBox all={-1,-1,100000,100000};
    size_t all_hits=0;
    assert(musa_scene2d_index_query(
        tree,count,leaves,all,fast,count,&all_hits));
    for (size_t skip=0;skip<all_hits;skip+=127u) {
        size_t page[127],got=0,total=0;
        assert(musa_scene2d_index_query_page(
            tree,count,leaves,all,skip,page,127,&got,&total));
        const size_t want=all_hits-skip<127u ? all_hits-skip:127u;
        assert(total==all_hits && got==want);
        for (size_t j=0;j<got;++j) assert(page[j]==fast[skip+j]);
    }
    free(fast);free(slow);free(tree);free(p);
}
int main(void) {
    atomic_and_edges();
    paged_queries_and_rollback();
    large_viewport_equivalence();
    return 0;
}
