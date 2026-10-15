/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "musacad_core.h"
#include "musacad_bounds3.h"
#include <math.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

/*
 * Immutable object-local triangle BVH.
 * Triangle/vertex buffers are OWNED by the handle, so a CAD document
 * unloading its source mesh cannot cause use-after-free in later picks.
 * Splits are spatial and keep recursion balanced, not a linear scan.
 */
#define BVH_MAX_POINTS 1000000u
#define BVH_MAX_FACES 250000u
#define BVH_LEAF_FACES 8u

typedef struct BVHBox {double mn[3],mx[3];} BVHBox;
typedef struct BVHNode {
    BVHBox box;
    size_t begin,count,left,right;
} BVHNode;
struct MusaCoreMeshBVH {
    float *xyz;int *faces;size_t points,triangles;
    BVHBox *tri_boxes;
    size_t *order;
    BVHNode *nodes;
    size_t nodes_used,nodes_capacity;
};
void musa_core_mesh_bvh_destroy(MusaCoreMeshBVH *tree) {
    if(!tree)return;
    free(tree->xyz);free(tree->faces);free(tree->tri_boxes);
    free(tree->order);free(tree->nodes);free(tree);
}
static double center(const BVHBox *box,int a) {
    return box->mn[a]+(box->mx[a]-box->mn[a])*0.5;
}
static int build_node(MusaCoreMeshBVH *b,size_t first,size_t count,
                      size_t *node_index) {
    if(!count||b->nodes_used>=b->nodes_capacity)return 0;
    size_t index=b->nodes_used++;
    BVHNode *node=&b->nodes[index];
    node->begin=first;node->count=count;
    node->left=SIZE_MAX;node->right=SIZE_MAX;
    const BVHBox *bb=&b->tri_boxes[b->order[first]];
    for(int a=0;a<3;a++){
        node->box.mn[a]=bb->mn[a];node->box.mx[a]=bb->mx[a];
    }
    double centroid_min[3],centroid_max[3];
    for(int a=0;a<3;a++)
        centroid_min[a]=centroid_max[a]=center(bb,a);
    for(size_t i=first+1;i<first+count;i++){
        const BVHBox *box=&b->tri_boxes[b->order[i]];
        for(int a=0;a<3;a++){
            if(box->mn[a]<node->box.mn[a])node->box.mn[a]=box->mn[a];
            if(box->mx[a]>node->box.mx[a])node->box.mx[a]=box->mx[a];
            double c=center(box,a);
            if(c<centroid_min[a])centroid_min[a]=c;
            if(c>centroid_max[a])centroid_max[a]=c;
        }
    }
    if(count<=BVH_LEAF_FACES){*node_index=index;return 1;}
    int axis=0;
    for(int a=1;a<3;a++)
        if(centroid_max[a]-centroid_min[a]>
           centroid_max[axis]-centroid_min[axis])axis=a;
    double split=centroid_min[axis]+(centroid_max[axis]-centroid_min[axis])*0.5;
    size_t lo=first,hi=first+count;
    while(lo<hi){
        size_t tri=b->order[lo];
        if(center(&b->tri_boxes[tri],axis)<split)lo++;
        else{
            size_t temp=b->order[lo];
            b->order[lo]=b->order[--hi];
            b->order[hi]=temp;
        }
    }
    size_t left_count=lo-first;
    /* Prevent pathological O(N) recursion for clustered outlier meshes.
     * Both groups cover every original face regardless of split quality.
     */
    if(left_count<count/8||left_count>count-count/8)
        left_count=count/2;
    if(!left_count||left_count>=count)return 0;
    size_t left=SIZE_MAX,right=SIZE_MAX;
    if(!build_node(b,first,left_count,&left)||
       !build_node(b,first+left_count,count-left_count,&right))return 0;
    node->left=left;node->right=right;
    *node_index=index;return 1;
}
int musa_core_mesh_bvh_build_f32(const float *xyz,size_t point_count,
                                const int *faces,size_t face_count,
                                MusaCoreMeshBVH **out) {
    if(!out||point_count>BVH_MAX_POINTS||face_count>BVH_MAX_FACES||
       point_count>SIZE_MAX/3u||face_count>SIZE_MAX/3u||
       (face_count&&(!xyz||!faces)))return 0;
    MusaCoreMeshBVH *b=(MusaCoreMeshBVH*)calloc(1,sizeof(*b));
    if(!b)return 0;
    b->points=point_count;b->triangles=face_count;
    if(face_count){
        b->xyz=(float*)malloc(point_count*3u*sizeof(float));
        b->faces=(int*)malloc(face_count*3u*sizeof(int));
        b->tri_boxes=(BVHBox*)malloc(face_count*sizeof(BVHBox));
        b->order=(size_t*)malloc(face_count*sizeof(size_t));
        b->nodes_capacity=face_count*2u;
        b->nodes=(BVHNode*)calloc(b->nodes_capacity,sizeof(BVHNode));
        if(!b->xyz||!b->faces||!b->tri_boxes||!b->order||!b->nodes)
            goto invalid;
        memcpy(b->xyz,xyz,point_count*3u*sizeof(float));
        memcpy(b->faces,faces,face_count*3u*sizeof(int));
        for(size_t i=0;i<face_count;i++){
            b->order[i]=i;
            BVHBox *box=&b->tri_boxes[i];
            for(int vertex=0;vertex<3;vertex++){
                int ix=faces[i*3u+(size_t)vertex];
                if(ix<0||(size_t)ix>=point_count)goto invalid;
                for(int a=0;a<3;a++){
                    double v=xyz[(size_t)ix*3u+(size_t)a];
                    if(!isfinite(v))goto invalid;
                    if(vertex==0){box->mn[a]=v;box->mx[a]=v;}
                    else{
                        if(v<box->mn[a])box->mn[a]=v;
                        if(v>box->mx[a])box->mx[a]=v;
                    }
                }
            }
        }
        size_t root=SIZE_MAX;
        if(!build_node(b,0,face_count,&root)||root!=0)goto invalid;
    }
    *out=b;return 1;
invalid:
    musa_core_mesh_bvh_destroy(b);return 0;
}
static int cast_node(const MusaCoreMeshBVH *b,size_t node_index,
                     const double origin[3],const double d[3],
                     double max_distance,MusaCoreMeshRayHit *best,
                     int *found,MusaCoreMeshBVHStats *stats) {
    if(node_index>=b->nodes_used)return -1;
    stats->visited_nodes++;
    const BVHNode *n=&b->nodes[node_index];
    double limit=*found?fmin(best->distance,max_distance):max_distance;
    int maybe=musa_core_bounds3_ray_maybe(n->box.mn,n->box.mx,
                                           origin,d,limit);
    if(maybe==0){stats->rejected_nodes++;return 0;}
    if(maybe<0)return -1;
    if(n->left==SIZE_MAX){
        for(size_t i=n->begin;i<n->begin+n->count;i++){
            size_t tri=b->order[i];
            stats->tested_faces++;
            MusaCoreMeshRayHit hit={0};
            int rc=musa_core_mesh_ray_pick_f32(
                b->xyz,b->points,b->faces+tri*3u,1,origin,d,
                *found?fmin(max_distance,best->distance):max_distance,&hit);
            if(rc<0)return -1;
            if(rc==1&&(!*found||hit.distance<best->distance||
                        (hit.distance==best->distance&&
                         tri<best->triangle_index))){
                hit.triangle_index=tri;*best=hit;*found=1;
            }
        }
        return 0;
    }
    if(cast_node(b,n->left,origin,d,max_distance,best,found,stats)<0)return -1;
    if(cast_node(b,n->right,origin,d,max_distance,best,found,stats)<0)return -1;
    return 0;
}
int musa_core_mesh_bvh_pick_stats(const MusaCoreMeshBVH *b,
                                 const double origin[3],const double direction[3],
                                 double max_distance,MusaCoreMeshRayHit *out,
                                 MusaCoreMeshBVHStats *stats) {
    if(!b||!out||!origin||!direction||
       !isfinite(max_distance)||max_distance<0.)return -1;
    for(int a=0;a<3;a++)
        if(!isfinite(origin[a])||!isfinite(direction[a]))return -1;
    double mag=fmax(fabs(direction[0]),
               fmax(fabs(direction[1]),fabs(direction[2])));
    if(mag==0.)return -1;
    double d[3]={direction[0]/mag,direction[1]/mag,direction[2]/mag};
    double norm=hypot(hypot(d[0],d[1]),d[2]);
    if(!isfinite(norm)||norm==0.)return -1;
    for(int a=0;a<3;a++)d[a]/=norm;
    MusaCoreMeshBVHStats local={0};
    if(b->triangles==0){
        if(stats)*stats=local;
        return 0;
    }
    MusaCoreMeshRayHit best={0};int found=0;
    if(cast_node(b,0,origin,d,max_distance,&best,&found,&local)<0)return -1;
    if(stats)*stats=local;
    if(!found)return 0;
    *out=best;return 1;
}
int musa_core_mesh_bvh_pick(const MusaCoreMeshBVH *tree,
                           const double origin[3],const double direction[3],
                           double max_distance,MusaCoreMeshRayHit *out) {
    return musa_core_mesh_bvh_pick_stats(tree,origin,direction,
                                         max_distance,out,NULL);
}
