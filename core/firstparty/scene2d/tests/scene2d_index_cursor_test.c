/* SPDX-License-Identifier: MIT
 * Independent cursor-pagination regressions on synthetic prepared 2D bounds.
 * No real DWG file decoding or Android rendering is asserted.
 */
#include "musa_scene2d_index.h"
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

static void edge_and_rollback(void) {
    MusaScene2dPreparedBounds bounds[7]={
        {{0,0,1,1},1}, {{10,10,11,11},1}, {{0,0,1,1},0},
        {{20,20,21,21},1}, {{30,30,31,31},1},
        {{40,40,41,41},1}, {{50,50,51,51},1}
    };
    MusaScene2dIndexNode tree[16];
    size_t leaves=0;
    assert(musa_scene2d_index_build(bounds,7,tree,16,&leaves));
    assert(leaves==8u);
    const MusaScene2dBox view={0,0,100,100};
    const size_t expected[]={0,1,3,4,5,6};
    size_t page[3]={999,999,999},written=88;
    size_t next=0,offset=0;
    for (;;) {
        assert(musa_scene2d_index_query_from(tree,7,leaves,view,next,
                                              page,3,&written));
        for (size_t i=0;i<written;++i) {
            assert(offset<6u && page[i]==expected[offset++]);
            if (i) assert(page[i-1]<page[i]);
        }
        if (written<3u) break;
        next=page[written-1u]+1u;
    }
    assert(offset==6u);
    assert(musa_scene2d_index_query_from(tree,7,leaves,view,SIZE_MAX,
                                          NULL,0,&written));
    assert(written==0);
    assert(musa_scene2d_index_query_from(tree,7,leaves,view,7,
                                          page,3,&written));
    assert(written==0);
    assert(musa_scene2d_index_query_from(tree,7,leaves,
        (MusaScene2dBox){11,11,11,11},0,page,3,&written));
    assert(written==1u && page[0]==1u); /* Edge contact. */
    const size_t saved=written;
    page[0]=999;
    assert(!musa_scene2d_index_query_from(tree,7,leaves,
        (MusaScene2dBox){NAN,0,1,1},0,page,3,&written));
    assert(!musa_scene2d_index_query_from(tree,7,7,view,0,page,3,&written));
    assert(!musa_scene2d_index_query_from(tree,7,leaves,view,0,
        (size_t *)(void *)tree,3,&written));
    assert(!musa_scene2d_index_query_from(tree,7,leaves,view,0,
        page,3,&page[0]));
    assert(!musa_scene2d_index_query_from(tree,7,leaves,view,0,
        NULL,3,&written));
    assert(!musa_scene2d_index_query_from(tree,7,leaves,view,0,
        page,3,NULL));
    assert(written==saved && page[0]==999);
    assert(musa_scene2d_index_query_from(NULL,0,0,view,0,NULL,0,&written));
    assert(written==0);
    written=88;
    assert(!musa_scene2d_index_query_from(NULL,0,1,view,0,NULL,0,&written));
    assert(written==88);
}

static void stress_equivalence(void) {
    enum { COUNT=20000, TREE_CAP=65536 };
    MusaScene2dPreparedBounds *bounds=calloc(COUNT,sizeof(*bounds));
    MusaScene2dIndexNode *tree=calloc(TREE_CAP,sizeof(*tree));
    size_t *all=malloc(COUNT*sizeof(*all));
    assert(bounds && tree && all);
    for (size_t i=0;i<COUNT;++i) {
        const double x=(double)((i/100u)*10u);
        const double y=(double)((i%100u)*10u);
        bounds[i].box=(MusaScene2dBox){x,y,x+4.0,y+4.0};
        bounds[i].visible=(uint8_t)(i%13u!=0u);
    }
    size_t leaves=0;
    assert(musa_scene2d_index_build(bounds,COUNT,tree,TREE_CAP,&leaves));
    for (size_t t=0;t<80u;++t) {
        const double x=(double)((t*17u)%205u)*10.0;
        const double y=(double)((t*29u)%105u)*10.0;
        const MusaScene2dBox view=(t==0u)
            ? (MusaScene2dBox){-1,-1,100000,100000}
            : (MusaScene2dBox){x,y,x+40.0,y+40.0};
        size_t all_count=0;
        assert(musa_scene2d_index_query(tree,COUNT,leaves,view,
                                         all,COUNT,&all_count));
        size_t next=0,offset=0;
        for (;;) {
            size_t page[127],written=999;
            assert(musa_scene2d_index_query_from(tree,COUNT,leaves,
                                                  view,next,page,127,&written));
            for (size_t i=0;i<written;++i) {
                assert(offset<all_count && page[i]==all[offset]);
                ++offset;
            }
            if (written<127u) break;
            next=page[written-1u]+1u;
        }
        assert(offset==all_count);
        /* A cursor in the middle of the source must never repeat earlier
         * results; compare against the reference query's suffix. */
        const size_t start=COUNT/2u;
        size_t page[8],written=0,first=0;
        while (first<all_count && all[first]<start) ++first;
        assert(musa_scene2d_index_query_from(tree,COUNT,leaves,view,
                                              start,page,8,&written));
        const size_t remain=all_count-first;
        const size_t want=remain<8u?remain:8u;
        assert(written==want);
        for (size_t i=0;i<want;++i) assert(page[i]==all[first+i]);
    }
    free(all);free(tree);free(bounds);
}
int main(void) {edge_and_rollback();stress_equivalence();return 0;}
