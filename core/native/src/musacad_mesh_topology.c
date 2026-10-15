/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "musacad_core.h"
#include <math.h>
#include <stdint.h>
#include <stdlib.h>

/* Shared 3D topology validation, independent of Java/Android SDK.
 * Uses O(F log F) sorted oriented edges and sorted unoriented faces.
 * This does not inspect self-intersections, geometric vertex welding,
 * enclosed shells or curved B-Rep surfaces.
 */
#define MUSA_TOPO_MAX_TRIANGLES 50000u
#define MUSA_TOPO_MAX_VERTICES 1000000u
typedef struct TopoEdge { int low,high,dir; } TopoEdge;
typedef struct TopoFace { int a,b,c; } TopoFace;

static int cmp_edge(const void *left,const void *right){
    const TopoEdge *a=(const TopoEdge*)left,*b=(const TopoEdge*)right;
    if(a->low!=b->low)return a->low<b->low?-1:1;
    if(a->high!=b->high)return a->high<b->high?-1:1;
    return a->dir<b->dir?-1:a->dir>b->dir?1:0;
}
static int cmp_face(const void *left,const void *right){
    const TopoFace *a=(const TopoFace*)left,*b=(const TopoFace*)right;
    if(a->a!=b->a)return a->a<b->a?-1:1;
    if(a->b!=b->b)return a->b<b->b?-1:1;
    return a->c<b->c?-1:a->c>b->c?1:0;
}
static void add_edge(TopoEdge *e,int a,int b){
    e->low=a<b?a:b;e->high=a<b?b:a;e->dir=a<b?1:-1;
}
static TopoFace sort_face(int a,int b,int c){
    if(a>b){int tmp=a;a=b;b=tmp;}
    if(b>c){int tmp=b;b=c;c=tmp;}
    if(a>b){int tmp=a;a=b;b=tmp;}
    TopoFace f={a,b,c};return f;
}
int musa_core_mesh_topology_f32(const float *xyz,size_t point_count,
                               const int *triangles,size_t face_count,
                               MusaCoreMeshTopology *out){
    if(!out||point_count>MUSA_TOPO_MAX_VERTICES||
       point_count>SIZE_MAX/3u||face_count>SIZE_MAX/3u||
       (face_count&&(!triangles||!xyz)))return 0;
    MusaCoreMeshTopology result={0};
    result.triangles=face_count;
    if(face_count>MUSA_TOPO_MAX_TRIANGLES){
        /* The Java method likewise reports NOT CHECKED, never a solid. */
        *out=result;
        return 1;
    }
    result.checked=1;
    if(face_count==0){*out=result;return 1;}
    TopoEdge *edges=(TopoEdge*)malloc(face_count*3u*sizeof(*edges));
    TopoFace *faces=(TopoFace*)malloc(face_count*sizeof(*faces));
    if(!edges||!faces){free(edges);free(faces);return 0;}
    size_t ne=0,nf=0;
    for(size_t k=0;k<face_count;k++){
        size_t i=k*3u;
        int a=triangles[i],b=triangles[i+1],c=triangles[i+2];
        if(a<0||b<0||c<0||(size_t)a>=point_count||
           (size_t)b>=point_count||(size_t)c>=point_count)
            goto invalid;
        const size_t ia=(size_t)a*3u,ib=(size_t)b*3u,ic=(size_t)c*3u;
        for(int axis=0;axis<3;axis++)
            if(!isfinite(xyz[ia+axis])||!isfinite(xyz[ib+axis])||
               !isfinite(xyz[ic+axis]))goto invalid;
        if(a==b||b==c||c==a){result.degenerate_triangles++;continue;}
        const double ux=(double)xyz[ib]-xyz[ia],uy=(double)xyz[ib+1]-xyz[ia+1],
                     uz=(double)xyz[ib+2]-xyz[ia+2];
        const double vx=(double)xyz[ic]-xyz[ia],vy=(double)xyz[ic+1]-xyz[ia+1],
                     vz=(double)xyz[ic+2]-xyz[ia+2];
        const double cx=uy*vz-uz*vy,cy=uz*vx-ux*vz,cz=ux*vy-uy*vx;
        if(!isfinite(cx)||!isfinite(cy)||!isfinite(cz))goto invalid;
        if(cx==0.&&cy==0.&&cz==0.){
            result.degenerate_triangles++;continue;
        }
        faces[nf++]=sort_face(a,b,c);
        add_edge(&edges[ne++],a,b);
        add_edge(&edges[ne++],b,c);
        add_edge(&edges[ne++],c,a);
    }
    if(nf>1){
        qsort(faces,nf,sizeof(*faces),cmp_face);
        for(size_t i=1;i<nf;i++)if(cmp_face(&faces[i-1],&faces[i])==0)
            result.duplicate_faces++;
    }
    if(ne){
        qsort(edges,ne,sizeof(*edges),cmp_edge);
        for(size_t i=0;i<ne;){
            size_t j=i+1;
            while(j<ne&&edges[i].low==edges[j].low&&
                  edges[i].high==edges[j].high)j++;
            size_t n=j-i;
            if(n==1)result.boundary_edges++;
            else if(n==2&&edges[i].dir==edges[i+1].dir)
                result.orientation_conflicts++;
            else if(n>2)result.non_manifold_edges++;
            i=j;
        }
    }
    result.closed_oriented_by_index=(face_count>0&&
        result.degenerate_triangles==0&&result.boundary_edges==0&&
        result.orientation_conflicts==0&&result.non_manifold_edges==0&&
        result.duplicate_faces==0);
    free(edges);free(faces);
    *out=result;return 1;
invalid:
    free(edges);free(faces);return 0;
}
