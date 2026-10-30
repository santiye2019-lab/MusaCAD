/* SPDX-License-Identifier: MIT
 * Independently authored caller-owned binary union-AABB index (C11).
 */
#include "musa_scene2d_index.h"
#include <math.h>
#include <stdint.h>
#include <stddef.h>

static int overlap(const void *a, size_t an, const void *b, size_t bn) {
    if (!an || !bn) return 0;
    const uintptr_t x=(uintptr_t)a, y=(uintptr_t)b;
    return x<=y ? y-x<an : x-y<bn;
}
static int ordered(MusaScene2dBox b) {
    return !isnan(b.min_x) && !isnan(b.min_y) &&
           !isnan(b.max_x) && !isnan(b.max_y) &&
           b.min_x<=b.max_x && b.min_y<=b.max_y;
}
static int finite_box(MusaScene2dBox b) {
    return isfinite(b.min_x) && isfinite(b.min_y) &&
           isfinite(b.max_x) && isfinite(b.max_y) &&
           b.min_x<=b.max_x && b.min_y<=b.max_y;
}
static MusaScene2dBox union_box(MusaScene2dBox a, MusaScene2dBox b) {
    MusaScene2dBox c={
        a.min_x<b.min_x ? a.min_x:b.min_x,
        a.min_y<b.min_y ? a.min_y:b.min_y,
        a.max_x>b.max_x ? a.max_x:b.max_x,
        a.max_y>b.max_y ? a.max_y:b.max_y
    };
    return c;
}
static int intersects(MusaScene2dBox a, MusaScene2dBox b) {
    return a.min_x<=b.max_x && a.max_x>=b.min_x &&
           a.min_y<=b.max_y && a.max_y>=b.min_y;
}
int musa_scene2d_index_build(const MusaScene2dPreparedBounds *prepared,
                             size_t count, MusaScene2dIndexNode *storage,
                             size_t capacity, size_t *leaf_count) {
    if (!leaf_count) return 0;
    if (!count) { *leaf_count=0; return 1; }
    if (!prepared || !storage ||
        count>SIZE_MAX/sizeof(*prepared)) return 0;
    size_t leaves=1;
    while (leaves<count) {
        if (leaves>SIZE_MAX/2u) return 0;
        leaves*=2u;
    }
    if (leaves>SIZE_MAX/2u ||
        2u*leaves>SIZE_MAX/sizeof(*storage) ||
        capacity<2u*leaves) return 0;
    const size_t pbytes=count*sizeof(*prepared);
    const size_t tbytes=2u*leaves*sizeof(*storage);
    if (overlap(prepared,pbytes,storage,tbytes) ||
        overlap(leaf_count,sizeof(*leaf_count),prepared,pbytes) ||
        overlap(leaf_count,sizeof(*leaf_count),storage,tbytes)) return 0;
    for (size_t i=0;i<count;++i)
        if (prepared[i].visible>1u ||
            (prepared[i].visible && !ordered(prepared[i].box)))
            return 0;
    storage[0]=(MusaScene2dIndexNode){{0,0,0,0},0};
    for (size_t i=0;i<leaves;++i) {
        MusaScene2dIndexNode node={{0,0,0,0},0};
        if (i<count && prepared[i].visible) {
            node.box=prepared[i].box;
            node.occupied=1;
        }
        storage[leaves+i]=node;
    }
    for (size_t i=leaves-1u;i>0;--i) {
        const MusaScene2dIndexNode a=storage[2u*i];
        const MusaScene2dIndexNode b=storage[2u*i+1u];
        MusaScene2dIndexNode node={{0,0,0,0},0};
        if (a.occupied && b.occupied) {
            node.box=union_box(a.box,b.box);
            node.occupied=1;
        } else if (a.occupied) node=a;
        else if (b.occupied) node=b;
        storage[i]=node;
    }
    *leaf_count=leaves;
    return 1;
}
static void visit(const MusaScene2dIndexNode *tree, size_t node,
                  size_t first, size_t span, size_t count,
                  MusaScene2dBox view, size_t *indices, size_t *hits) {
    if (!tree[node].occupied || !intersects(tree[node].box,view)) return;
    if (span==1u) {
        if (first<count) {
            if (indices) indices[*hits]=first;
            ++*hits;
        }
        return;
    }
    const size_t half=span/2u;
    visit(tree,node*2u,first,half,count,view,indices,hits);
    visit(tree,node*2u+1u,first+half,half,count,view,indices,hits);
}
int musa_scene2d_index_query(const MusaScene2dIndexNode *storage,
                             size_t count, size_t leaf_count,
                             MusaScene2dBox viewport, size_t *indices,
                             size_t capacity, size_t *written) {
    if (!written || !finite_box(viewport) ||
        capacity>SIZE_MAX/sizeof(*indices)) return 0;
    if (!count) {
        if (leaf_count) return 0;
        *written=0;
        return 1;
    }
    if (!storage || !leaf_count || (leaf_count&(leaf_count-1u)) ||
        count>leaf_count || leaf_count>SIZE_MAX/2u ||
        2u*leaf_count>SIZE_MAX/sizeof(*storage) ||
        (capacity && !indices)) return 0;
    const size_t tbytes=2u*leaf_count*sizeof(*storage);
    const size_t obytes=capacity*sizeof(*indices);
    if (overlap(storage,tbytes,indices,obytes) ||
        overlap(storage,tbytes,written,sizeof(*written)) ||
        overlap(indices,obytes,written,sizeof(*written))) return 0;
    size_t hits=0;
    visit(storage,1,0,leaf_count,count,viewport,NULL,&hits);
    if (hits>capacity) return 0;
    if (hits) {
        size_t second=0;
        visit(storage,1,0,leaf_count,count,viewport,indices,&second);
    }
    *written=hits;
    return 1;
}

/* Walk only occupied/intersecting branches, retaining source-index order.
 * Stop as soon as the requested page has been emitted. */
static void visit_page(const MusaScene2dIndexNode *tree, size_t node,
                       size_t first, size_t span, size_t count,
                       MusaScene2dBox view, size_t skip,
                       size_t *indices, size_t capacity,
                       size_t *seen, size_t *emitted) {
    if (*emitted==capacity || !tree[node].occupied ||
        !intersects(tree[node].box,view)) return;
    if (span==1u) {
        if (first<count) {
            if (*seen>=skip) indices[(*emitted)++]=first;
            ++*seen;
        }
        return;
    }
    const size_t half=span/2u;
    visit_page(tree,node*2u,first,half,count,view,skip,
               indices,capacity,seen,emitted);
    visit_page(tree,node*2u+1u,first+half,half,count,view,skip,
               indices,capacity,seen,emitted);
}
int musa_scene2d_index_query_page(const MusaScene2dIndexNode *storage,
                                  size_t count, size_t leaf_count,
                                  MusaScene2dBox viewport, size_t skip,
                                  size_t *indices, size_t capacity,
                                  size_t *written, size_t *total_hits) {
    if (!written || !total_hits || !finite_box(viewport) ||
        capacity>SIZE_MAX/sizeof(*indices) ||
        (capacity && !indices) ||
        overlap(written,sizeof(*written),total_hits,sizeof(*total_hits)))
        return 0;
    const size_t obytes=capacity*sizeof(*indices);
    if (overlap(indices,obytes,written,sizeof(*written)) ||
        overlap(indices,obytes,total_hits,sizeof(*total_hits))) return 0;
    if (!count) {
        if (leaf_count) return 0;
        *written=0;
        *total_hits=0;
        return 1;
    }
    if (!storage || !leaf_count || (leaf_count&(leaf_count-1u)) ||
        count>leaf_count || leaf_count>SIZE_MAX/2u ||
        2u*leaf_count>SIZE_MAX/sizeof(*storage)) return 0;
    const size_t tbytes=2u*leaf_count*sizeof(*storage);
    if (overlap(storage,tbytes,indices,obytes) ||
        overlap(storage,tbytes,written,sizeof(*written)) ||
        overlap(storage,tbytes,total_hits,sizeof(*total_hits))) return 0;
    size_t total=0;
    visit(storage,1u,0u,leaf_count,count,viewport,NULL,&total);
    const size_t remaining=skip<total ? total-skip : 0u;
    const size_t page_count=remaining<capacity ? remaining:capacity;
    if (page_count) {
        size_t seen=0,emitted=0;
        visit_page(storage,1u,0u,leaf_count,count,viewport,skip,
                   indices,page_count,&seen,&emitted);
    }
    *written=page_count;
    *total_hits=total;
    return 1;
}
