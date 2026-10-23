/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "musacad_dwg_ac1032_preview_v6.h"
#include "musacad_bulge.h"
#include <float.h>
#include <math.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#define MAX_PREVIEW_RECORDS 1000000u
static int overlaps(const void *a,size_t na,const void *b,size_t nb){
    if(!na||!nb)return 0;
    uintptr_t x=(uintptr_t)a,y=(uintptr_t)b;
    return x<=y?y-x<na:x-y<nb;
}
static int valid(double x){
    return isfinite(x)&&x<=FLT_MAX&&x>=-FLT_MAX;
}
static int planar3(const double *xyz,double thickness,
                   const double *extrusion){
    return valid(xyz[0])&&valid(xyz[1])&&
       isfinite(xyz[2])&&fabs(xyz[2])<1e-10&&
       isfinite(thickness)&&fabs(thickness)<1e-10&&
       isfinite(extrusion[0])&&isfinite(extrusion[1])&&
       isfinite(extrusion[2])&&
       fabs(extrusion[0])<1e-10&&
       fabs(extrusion[1])<1e-10&&
       fabs(extrusion[2]-1.0)<1e-10;
}
static void bounds(double x,double y,double b[4],int *has){
    if(!*has){b[0]=b[2]=x;b[1]=b[3]=y;*has=1;return;}
    if(x<b[0])b[0]=x;if(x>b[2])b[2]=x;
    if(y<b[1])b[1]=y;if(y>b[3])b[3]=y;
}
/* Native v6 can preserve only source-assigned ACI 1..255 without a layer
 * graph. RGB, BYLAYER/BYBLOCK and transparency are unresolved style and
 * retain an explicitly provisional ACI 7, never fake exact colors. */
static float original_direct_aci(const MusaDwgAc1032EntityCommon *common,
                                 unsigned type,int colors,
                                 uint8_t *unresolved){
    uint16_t aci=7u;
    if(colors){
        if(common&&common->common_decoded&&
           !common->has_rgb&&!common->has_color_reference&&
           !common->has_transparency&&common->invisibility==0u&&
           common->color_index>=1u&&common->color_index<=255u)
            aci=common->color_index;
        else *unresolved=1u;
    }
    return (float)(type*256u+(unsigned)aci);
}
/* LWPOLYLINE is validated as ONE geometry before emission. No straight
 * approximation for source bulges, no arbitrary OCS->WCS assumption. */
static int lwpoly(const MusaDwgAc1032SceneIndex *s,uint64_t h,
                  MusaDwgAc1032LWVertex *vertices,float *dst,size_t cap,
                  size_t *w,size_t *primitive_count,double ext[4],int *has,
                  int colors,uint8_t *unresolved){
    MusaDwgAc1032NativeLWPolyline poly={0};
    if(!musa_dwg_ac1032_scene_index_read_lwpolyline(
         s,h,vertices,100000u,&poly)||
       !poly.geometry_decoded||poly.vertex_count<2u||
       poly.vertex_count>100000u||
       !isfinite(poly.elevation)||poly.elevation!=0.0||
       !isfinite(poly.thickness)||poly.thickness!=0.0||
       !isfinite(poly.constant_width)||poly.constant_width!=0.0||
       !isfinite(poly.extrusion[0])||poly.extrusion[0]!=0.0||
       !isfinite(poly.extrusion[1])||poly.extrusion[1]!=0.0||
       !isfinite(poly.extrusion[2])||poly.extrusion[2]!=1.0)
       return 0;
    if(colors&&poly.common.invisibility!=0u)return 0;
    const float poly_code=original_direct_aci(
        &poly.common,2u,colors,unresolved);
    const float line_code=original_direct_aci(
        &poly.common,1u,colors,unresolved);
    const float arc_code=original_direct_aci(
        &poly.common,4u,colors,unresolved);
    const size_t n=(size_t)poly.vertex_count;
    const size_t segments=n-(poly.closed?0u:1u);
    int has_bulge=0;
    for(size_t i=0u;i<n;i++){
        const MusaDwgAc1032LWVertex *a=&vertices[i];
        if(!valid(a->x)||!valid(a->y)||!isfinite(a->bulge)||
           !isfinite(a->start_width)||!isfinite(a->end_width)||
           a->start_width!=0.0||a->end_width!=0.0)return 0;
    }
    for(size_t i=0u;i<segments;i++){
        const MusaDwgAc1032LWVertex *a=&vertices[i],
                                   *z=&vertices[(i+1u)%n];
        if(a->bulge!=0.0){
            MusaCoreBulgeArc arc={0};
            if(!musa_core_bulge_to_arc(
                 a->x,a->y,z->x,z->y,a->bulge,&arc)||
               !valid(arc.cx)||!valid(arc.cy)||!valid(arc.radius)||
               !valid(arc.start)||!valid(arc.sweep)||
               !valid(arc.cx-arc.radius)||!valid(arc.cx+arc.radius)||
               !valid(arc.cy-arc.radius)||!valid(arc.cy+arc.radius))
                return 0;
            has_bulge=1;
        }
    }
    if(!has_bulge){
        if(n>(SIZE_MAX-2u)/2u||*w>SIZE_MAX-(2u+2u*n))
            return -1;
        const size_t need=2u+2u*n;
        if(dst){
            if(*w>cap||need>cap-*w)return -1;
            dst[*w]=poly_code;
            dst[*w+1u]=(float)(poly.closed?-(int)n:(int)n);
            for(size_t i=0;i<n;i++){
                dst[*w+2u+2u*i]=(float)vertices[i].x;
                dst[*w+3u+2u*i]=(float)vertices[i].y;
            }
        }
        for(size_t i=0;i<n;i++)
            bounds(vertices[i].x,vertices[i].y,ext,has);
        *w+=need;(*primitive_count)++;return 1;
    }
    /* Bulged paths use separate v6 straight LINE and parametric CURVE
     * segments, retaining source ordering including final CLOSED edge. */
    size_t required=0u;
    for(size_t i=0;i<segments;i++){
        const size_t extra=vertices[i].bulge==0.0?5u:9u;
        if(required>SIZE_MAX-extra)return -1;
        required+=extra;
    }
    if(*w>SIZE_MAX-required||*primitive_count>SIZE_MAX-segments)
        return -1;
    if(dst&&(*w>cap||required>cap-*w))return -1;
    size_t pos=*w;
    for(size_t i=0;i<segments;i++){
        const MusaDwgAc1032LWVertex *a=&vertices[i],
                                   *z=&vertices[(i+1u)%n];
        if(a->bulge==0.0){
            if(dst){
                dst[pos]=line_code;dst[pos+1u]=(float)a->x;
                dst[pos+2u]=(float)a->y;
                dst[pos+3u]=(float)z->x;
                dst[pos+4u]=(float)z->y;
            }
            bounds(a->x,a->y,ext,has);bounds(z->x,z->y,ext,has);
            pos+=5u;
        }else{
            MusaCoreBulgeArc arc={0};
            /* Same verified, deterministic arithmetic as validation. */
            if(!musa_core_bulge_to_arc(
                a->x,a->y,z->x,z->y,a->bulge,&arc))return -1;
            if(dst){
                dst[pos]=arc_code;dst[pos+1u]=(float)arc.cx;
                dst[pos+2u]=(float)arc.cy;
                dst[pos+3u]=(float)arc.radius;dst[pos+4u]=0.0f;
                dst[pos+5u]=0.0f;dst[pos+6u]=(float)arc.radius;
                dst[pos+7u]=(float)arc.start;
                dst[pos+8u]=(float)arc.sweep;
            }
            bounds(arc.cx-arc.radius,arc.cy-arc.radius,ext,has);
            bounds(arc.cx+arc.radius,arc.cy+arc.radius,ext,has);
            pos+=9u;
        }
    }
    *w=pos;*primitive_count+=segments;return 1;
}
/* Flatten a GENUINELY planar 3D POLYLINE only after resolving the
 * source's ordered hard-owner H references and reverse-owned children.
 * 3D mesh, spline-fit, elevation and model-space graph uncertainty remain
 * explicitly unrenderable: no guessed numeric H order or Z projection. */
static int flat_polyline3d(const MusaDwgAc1032SceneIndex *s,uint64_t h,
                    MusaDwgAc1032Polyline3DOrderedVertex *vertices,
                    float *dst,size_t cap,size_t *w,
                    size_t *primitive_count,double ext[4],int *has,
                  int colors,uint8_t *unresolved){
    MusaDwgAc1032Polyline3DOrdered poly={0};
    if(!musa_dwg_ac1032_scene_index_read_polyline3d_vertices(
          s,h,vertices,100000u,&poly)||
       !poly.hard_owner_links_verified||
       !poly.reverse_vertex_owners_verified||
       !poly.seqend_type_verified||
       !poly.header.header_decoded||
       poly.header.spline_flags!=0u||
       poly.vertex_count<2u||poly.vertex_count>100000u||
       poly.header.closed>1u||
       poly.header.polyline_flags!=(uint8_t)poly.header.closed)
        return 0;
    if(colors&&poly.header.common.invisibility!=0u)return 0;
    const float poly_code=original_direct_aci(
         &poly.header.common,2u,colors,unresolved);
    const size_t count=poly.vertex_count;
    for(size_t i=0;i<count;i++){
        const MusaDwgAc1032Polyline3DOrderedVertex *v=&vertices[i];
        /* 0x20 is the standard 3D polyline vertex flag. Spline/mesh
         * control vertices and transformed/nonplanar Z are unsupported. */
        if((v->flags & (uint8_t)~32u)!=0u||
           !valid(v->point[0])||!valid(v->point[1])||
           !isfinite(v->point[2])||v->point[2]!=0.0)
            return 0;
    }
    if(count>(SIZE_MAX-2u)/2u||*w>SIZE_MAX-2u-2u*count||
       *primitive_count>=16777215u)
        return -1;
    const size_t needed=2u+count*2u;
    if(dst){
        if(*w>cap||needed>cap-*w)return -1;
        dst[*w]=poly_code;
        dst[*w+1u]=(float)(poly.header.closed?-(int)count:(int)count);
        for(size_t i=0;i<count;i++){
            dst[*w+2u+2u*i]=(float)vertices[i].point[0];
            dst[*w+3u+2u*i]=(float)vertices[i].point[1];
        }
    }
    for(size_t i=0;i<count;i++)
        bounds(vertices[i].point[0],vertices[i].point[1],ext,has);
    *w+=needed;
    (*primitive_count)++;
    return 1;
}
/* Probe/emit in the same immutable source-index order. All integrity
 * failures abort BEFORE touching output through a mandatory probe pass.
 * This transport deliberately leaves original color/layer unresolved.
 */
static int process(const MusaDwgAc1032SceneIndex *s,
                   float *dst,size_t cap,
                   MusaAc1032PreviewV6Stats *stats,int curves,int polylines,
                   int colors){
    size_t n=musa_dwg_ac1032_scene_index_count(s);
    if(!n||n>MAX_PREVIEW_RECORDS)return 0;
    MusaAc1032PreviewV6Stats v={0};
    v.total_indexed_records=n;
    v.required_float_words=7u;
    v.provisional_aci_style=colors?0u:1u;
    MusaDwgAc1032LWVertex *lw=NULL;
    MusaDwgAc1032Polyline3DOrderedVertex *poly3d=NULL;
    double ext[4]={0};int has=0;
    size_t w=7u;uint64_t prev=0u;
    for(size_t i=0;i<n;i++){
        MusaDwgAc1032HandleEntry e={0};
        uint16_t t=0u;
        if(!musa_dwg_ac1032_scene_index_entry(s,i,&e)||
           !e.handle||e.handle<=prev||
           !musa_dwg_ac1032_scene_index_object_type(s,e.handle,&t))
            return 0;
        prev=e.handle;
        if(t==19u){
            MusaDwgAc1032NativeLine line={0};
            if(musa_dwg_ac1032_scene_index_read_line(s,e.handle,&line)&&
               (!colors||line.common.invisibility==0u)&&
               planar3(line.geometry.start,line.geometry.thickness,
                       line.geometry.extrusion)&&
               planar3(line.geometry.end,line.geometry.thickness,
                       line.geometry.extrusion)){
                if(w>SIZE_MAX-5u)return 0;
                if(dst){
                    if(w+5u>cap)return 0;
                    dst[w]=original_direct_aci(
                          &line.common,1u,colors,&v.provisional_aci_style);
                    dst[w+1u]=(float)line.geometry.start[0];
                    dst[w+2u]=(float)line.geometry.start[1];
                    dst[w+3u]=(float)line.geometry.end[0];
                    dst[w+4u]=(float)line.geometry.end[1];
                }
                if(colors)(void)original_direct_aci(
                    &line.common,1u,colors,&v.provisional_aci_style);
                w+=5u;
                bounds(line.geometry.start[0],line.geometry.start[1],ext,&has);
                bounds(line.geometry.end[0],line.geometry.end[1],ext,&has);
                v.rendered_primitives++;
            }else v.omitted_records++;
        }else if((t==17u||t==18u)&&curves){
            MusaDwgAc1032NativeCurve curve={0};
            int supported=musa_dwg_ac1032_scene_index_read_curve(
                 s,e.handle,&curve)&&
                 planar3(curve.center,curve.thickness,curve.extrusion)&&
                 (!colors||curve.common.invisibility==0u)&&
                 valid(curve.radius)&&curve.radius>0.0&&
                 valid(curve.center[0]-curve.radius)&&
                 valid(curve.center[0]+curve.radius)&&
                 valid(curve.center[1]-curve.radius)&&
                 valid(curve.center[1]+curve.radius);
            const double tau=6.283185307179586476925286766559;
            double start=0.0,sweep=tau;
            if(supported&&t==17u){
                start=curve.start_angle;
                sweep=curve.end_angle-start;
                if(!isfinite(start)||!isfinite(sweep)||
                   fabs(start)>8.0*tau||fabs(sweep)>8.0*tau)
                    supported=0;
                else if(sweep<=0.0)sweep+=tau;
                if(sweep<=0.0||sweep>tau+1e-10)supported=0;
            }
            if(supported&&!valid(sweep))supported=0;
            if(supported){
                if(w>SIZE_MAX-9u)return 0;
                if(dst){
                    if(w+9u>cap)return 0;
                    dst[w]=original_direct_aci(
                          &curve.common,4u,colors,&v.provisional_aci_style);
                    dst[w+1u]=(float)curve.center[0];
                    dst[w+2u]=(float)curve.center[1];
                    dst[w+3u]=(float)curve.radius;
                    dst[w+4u]=0.0f;
                    dst[w+5u]=0.0f;
                    dst[w+6u]=(float)curve.radius;
                    dst[w+7u]=(float)start;
                    dst[w+8u]=(float)sweep;
                }
                if(colors)(void)original_direct_aci(
                    &curve.common,4u,colors,&v.provisional_aci_style);
                w+=9u;
                /* Conservative complete-circle bounds avoid clipped arcs;
                 * an exact ARC-specific bounding box follows separately. */
                bounds(curve.center[0]-curve.radius,
                       curve.center[1]-curve.radius,ext,&has);
                bounds(curve.center[0]+curve.radius,
                       curve.center[1]+curve.radius,ext,&has);
                v.rendered_primitives++;
            }else v.omitted_records++;
        }else if(t==16u&&polylines){
            if(!poly3d){
                poly3d=(MusaDwgAc1032Polyline3DOrderedVertex*)
                       malloc(100000u*sizeof(*poly3d));
                if(!poly3d){free(lw);return 0;}
            }
            const int handled=flat_polyline3d(
                 s,e.handle,poly3d,dst,cap,&w,
                 &v.rendered_primitives,ext,&has,
                 colors,&v.provisional_aci_style);
            if(handled<0){free(poly3d);free(lw);return 0;}
            if(!handled)v.omitted_records++;
        }else if(t==77u&&polylines){
            if(!lw){
                lw=(MusaDwgAc1032LWVertex*)malloc(
                     100000u*sizeof(*lw));
                if(!lw){free(poly3d);return 0;}
            }
            const int handled=lwpoly(
                 s,e.handle,lw,dst,cap,&w,
                 &v.rendered_primitives,ext,&has,
                 colors,&v.provisional_aci_style);
            if(handled<0){free(poly3d);free(lw);return 0;}
            if(!handled)v.omitted_records++;
        }else if(t==27u){
            MusaDwgAc1032NativePoint point={0};
            if(musa_dwg_ac1032_scene_index_read_point(s,e.handle,&point)&&
               (!colors||point.common.invisibility==0u)&&
               planar3(point.point,point.thickness,point.extrusion)){
                if(w>SIZE_MAX-3u)return 0;
                if(dst){
                    if(w+3u>cap)return 0;
                    dst[w]=original_direct_aci(
                          &point.common,3u,colors,&v.provisional_aci_style);
                    dst[w+1u]=(float)point.point[0];
                    dst[w+2u]=(float)point.point[1];
                }
                if(colors)(void)original_direct_aci(
                    &point.common,3u,colors,&v.provisional_aci_style);
                w+=3u;
                bounds(point.point[0],point.point[1],ext,&has);
                v.rendered_primitives++;
            }else v.omitted_records++;
        }else v.omitted_records++;
    }
    if(v.rendered_primitives>16777215u||v.required_float_words>SIZE_MAX-w)
        return 0;
    v.required_float_words=w;
    v.partial_geometry=(uint8_t)(v.omitted_records>0u||
                                (colors&&v.provisional_aci_style));
    if(dst){
        if(w>cap)return 0;
        dst[0]=6.0f;dst[1]=(float)v.rendered_primitives;
        dst[2]=has?(float)ext[0]:0.0f;
        dst[3]=has?(float)ext[1]:0.0f;
        dst[4]=has?(float)ext[2]:0.0f;
        dst[5]=has?(float)ext[3]:0.0f;
        dst[6]=v.partial_geometry?1.0f:0.0f;
    }
    *stats=v;
    free(poly3d);
    free(lw);
    return 1;
}
static int preview(const MusaDwgAc1032SceneIndex *s,
                  float *words,size_t capacity,
                  MusaAc1032PreviewV6Stats *out,int curves,int polylines,
                  int colors){
    if(!s||!out||(!words&&capacity)||
       capacity>SIZE_MAX/sizeof(float)||
       (words&&overlaps(words,capacity*sizeof(float),out,sizeof(*out))))
        return 0;
    MusaAc1032PreviewV6Stats before={0};
    if(!process(s,NULL,0u,&before,curves,polylines,colors))return 0;
    if(!words){*out=before;return 1;}
    if(capacity<before.required_float_words)return 0;
    /* Build in scratch; malformed source or OOM cannot mutate the
     * caller's view buffer, even if record count changes unexpectedly. */
    float *scratch=(float*)malloc(before.required_float_words*sizeof(float));
    if(!scratch)return 0;
    MusaAc1032PreviewV6Stats after={0};
    int ok=process(s,scratch,before.required_float_words,&after,curves,polylines,colors)&&
           !memcmp(&before,&after,sizeof(before));
    if(ok){
        memcpy(words,scratch,before.required_float_words*sizeof(float));
        *out=after;
    }
    free(scratch);return ok;
}
int musa_dwg_ac1032_preview_v6_line_point(
    const MusaDwgAc1032SceneIndex *s,float *words,size_t capacity,
    MusaAc1032PreviewV6Stats *out){
    return preview(s,words,capacity,out,0,0,0);
}
int musa_dwg_ac1032_preview_v6_planar_curves(
    const MusaDwgAc1032SceneIndex *s,float *words,size_t capacity,
    MusaAc1032PreviewV6Stats *out){
    return preview(s,words,capacity,out,1,0,0);
}

int musa_dwg_ac1032_preview_v6_planar_polylines(
    const MusaDwgAc1032SceneIndex *s,float *words,size_t capacity,
    MusaAc1032PreviewV6Stats *out){
    return preview(s,words,capacity,out,1,1,0);
}

/* Source-assigned direct ACI where verifiably present. No layer colors. */
int musa_dwg_ac1032_preview_v6_planar_source_aci(
    const MusaDwgAc1032SceneIndex *scene,float *words,size_t cap,
    MusaAc1032PreviewV6Stats *out){
    return preview(scene,words,cap,out,1,1,1);
}

int musa_dwg_ac1032_preview_v6_entity_aci_code(
    const MusaDwgAc1032EntityCommon *source,
    uint8_t kind,float *code,uint8_t *exact){
    if(!source||!code||!exact||!source->common_decoded||
       kind<1u||kind>4u||overlaps(code,sizeof(*code),exact,sizeof(*exact))||
       overlaps(source,sizeof(*source),code,sizeof(*code))||
       overlaps(source,sizeof(*source),exact,sizeof(*exact)))return 0;
    uint8_t unresolved=0u;
    const float value=original_direct_aci(source,kind,1,&unresolved);
    *code=value;
    *exact=(uint8_t)!unresolved;
    return 1;
}
