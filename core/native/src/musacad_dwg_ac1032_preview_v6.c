/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "musacad_dwg_ac1032_preview_v6.h"
#include "musacad_dwg_ac1032_text_v6_packet.h"
#include "musacad_dwg_ac1032_color_provenance.h"
#include "musacad_bulge.h"
#include <float.h>
#include <math.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#define MAX_PREVIEW_RECORDS 1000000u
/* This is an admission guard, NOT an MTEXT formatting decoder. Reject
 * unsupported formatting/paragraph syntax instead of painting literal
 * \\P, \\C, {...} as if they were the original CAD glyph layout.
 * Independently validate UTF-8 (including overlong/surrogate/out-of-range).
 * Never claim a valid plain string proves font or viewport equivalence. */
int musa_dwg_ac1032_mtext_v6_plain_source_preflight(
    const char *utf8,size_t bytes){
    if(!utf8||bytes==0u||bytes>2047u||utf8[bytes]!='\0')return 0;
    size_t i=0u;
    while(i<bytes){
        const unsigned char c=(unsigned char)utf8[i];
        if(c<0x80u){
            if(c<0x20u||c==0x7fu||c=='\\'||c=='{'||c=='}')
                return 0;
            i++;continue;
        }
        size_t count=0u;
        uint32_t scalar=0u,min_scalar=0u;
        if(c>=0xc2u&&c<=0xdfu){
            count=2u;scalar=(uint32_t)(c&0x1fu);min_scalar=0x80u;
        }else if(c>=0xe0u&&c<=0xefu){
            count=3u;scalar=(uint32_t)(c&0x0fu);min_scalar=0x800u;
        }else if(c>=0xf0u&&c<=0xf4u){
            count=4u;scalar=(uint32_t)(c&0x07u);min_scalar=0x10000u;
        }else return 0;
        if(count>bytes-i)return 0;
        for(size_t j=1u;j<count;j++){
            const unsigned char b=(unsigned char)utf8[i+j];
            if((b&0xc0u)!=0x80u)return 0;
            scalar=(scalar<<6u)|(uint32_t)(b&0x3fu);
        }
        if(scalar<min_scalar||scalar>0x10ffffu||
           (scalar>=0xd800u&&scalar<=0xdfffu)||
           scalar==0x2028u||scalar==0x2029u||
           /* Without Unicode bidi/shaping support, invisible direction
            * controls can silently reorder source-authenticated MTEXT. */
           (scalar>=0x202au&&scalar<=0x202eu)||
           (scalar>=0x2066u&&scalar<=0x2069u)||
           (scalar>=0x200bu&&scalar<=0x200fu)||
           scalar==0x061cu||scalar==0xfeffu||
           /* Nonprinting Unicode format controls are not faithful glyphs.
            * Reject rather than silently substitute or reorder them. */
           scalar==0x00adu||scalar==0x034fu||
           scalar==0x180eu||
           (scalar>=0x2060u&&scalar<=0x2064u)||
           (scalar>=0x206au&&scalar<=0x206fu)||
           (scalar>=0xfff9u&&scalar<=0xfffbu)||
           (scalar>=0xe0000u&&scalar<=0xe007fu))return 0;
        i+=count;
    }
    return 1;
}
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

/* Tight bounds of a finite circular arc, including both signed sweep
 * directions, the 0-degree wrap, all extrema and the full circle. */
int musa_dwg_ac1032_preview_v6_arc_bounds(
    double cx,double cy,double r,double start,double sweep,double out[4]){
    const double tau=6.283185307179586476925286766559;
    if(!out||!valid(cx)||!valid(cy)||!valid(r)||r<=0.0||
       !isfinite(start)||!isfinite(sweep)||fabs(start)>8.0*tau||
       sweep==0.0||fabs(sweep)>tau+1e-12||
       !valid(cx-r)||!valid(cx+r)||!valid(cy-r)||!valid(cy+r))
        return 0;
    double b[4]={0};
    if(fabs(sweep)>=tau-1e-12){
        b[0]=cx-r;b[1]=cy-r;b[2]=cx+r;b[3]=cy+r;
    }else{
        const double s=fmod(start,tau),e=s+sweep;
        const double x0=cx+r*cos(s),y0=cy+r*sin(s);
        const double x1=cx+r*cos(e),y1=cy+r*sin(e);
        if(!valid(x0)||!valid(y0)||!valid(x1)||!valid(y1))
            return 0;
        b[0]=fmin(x0,x1);b[1]=fmin(y0,y1);
        b[2]=fmax(x0,x1);b[3]=fmax(y0,y1);
        for(unsigned k=0;k<4u;k++){
            const double a=(double)k*tau/4.0;
            double d=fmod(sweep>0.0?a-s:s-a,tau);
            if(d<0.0)d+=tau;
            if(d<=fabs(sweep)+1e-12){
                const double x=k==0u?cx+r:k==2u?cx-r:cx;
                const double y=k==1u?cy+r:k==3u?cy-r:cy;
                if(x<b[0])b[0]=x;
                if(y<b[1])b[1]=y;
                if(x>b[2])b[2]=x;
                if(y>b[3])b[3]=y;
            }
        }
    }
    if(!valid(b[0])||!valid(b[1])||!valid(b[2])||!valid(b[3]))
        return 0;
    memcpy(out,b,sizeof(b));return 1;
}
/* Outward-rounded float32 viewport bounds prevent edge clipping. */
static float outer_bound(double x,int lower){
    float f=(float)x;
    if(lower?(double)f>x:(double)f<x)
        f=nextafterf(f,lower?-INFINITY:INFINITY);
    return f;
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
/* v6 style grammar already consumes a NEGATIVE primitive code followed
 * by one exactly representable 24-bit RGB float. Do NOT encode packed
 * 32-bit ARGB in float: low-order color bits would be silently lost.
 * An unambiguous DWG ENC direct RGB with no color-handle or transparency
 * can retain the original source RGB even while full scene/layer
 * visibility remains unproved. Other forms stay provisional ACI7. */
typedef struct MusaPreviewV6SourceStyle {
    float code;
    float rgb;
    size_t extra_words;
} MusaPreviewV6SourceStyle;
static MusaPreviewV6SourceStyle source_style(
    const MusaDwgAc1032EntityCommon *common,
    unsigned kind,int colors,uint8_t *unresolved){
    MusaPreviewV6SourceStyle result={0};
    if(colors&&common&&common->common_decoded&&
       common->has_rgb&&!common->has_color_reference&&
       !common->has_transparency&&common->invisibility==0u&&
       common->rgb<=0xffffffu){
        result.code=-(float)(kind*256u+2u);
        result.rgb=(float)common->rgb; /* integer <= 2^24-1, exact in f32 */
        result.extra_words=1u;
        return result;
    }
    result.code=original_direct_aci(common,kind,colors,unresolved);
    return result;
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
    const MusaPreviewV6SourceStyle poly_style=source_style(
        &poly.common,2u,colors,unresolved);
    const MusaPreviewV6SourceStyle line_style=source_style(
        &poly.common,1u,colors,unresolved);
    const MusaPreviewV6SourceStyle arc_style=source_style(
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
        if(n>(SIZE_MAX-3u)/2u||
           2u+2u*n>SIZE_MAX-poly_style.extra_words||
           *w>SIZE_MAX-(2u+2u*n+poly_style.extra_words))
            return -1;
        const size_t need=2u+2u*n+poly_style.extra_words;
        if(dst){
            if(*w>cap||need>cap-*w)return -1;
            dst[*w]=poly_style.code;
            if(poly_style.extra_words)dst[*w+1u]=poly_style.rgb;
            const size_t at=*w+1u+poly_style.extra_words;
            dst[at]=(float)(poly.closed?-(int)n:(int)n);
            for(size_t i=0;i<n;i++){
                dst[at+1u+2u*i]=(float)vertices[i].x;
                dst[at+2u+2u*i]=(float)vertices[i].y;
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
        const size_t extra=vertices[i].bulge==0.0?
            5u+line_style.extra_words:9u+arc_style.extra_words;
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
                dst[pos]=line_style.code;
                if(line_style.extra_words)dst[pos+1u]=line_style.rgb;
                const size_t at=pos+line_style.extra_words;
                dst[at+1u]=(float)a->x;
                dst[at+2u]=(float)a->y;
                dst[at+3u]=(float)z->x;
                dst[at+4u]=(float)z->y;
            }
            bounds(a->x,a->y,ext,has);bounds(z->x,z->y,ext,has);
            pos+=5u+line_style.extra_words;
        }else{
            MusaCoreBulgeArc arc={0};
            /* Same verified, deterministic arithmetic as validation. */
            if(!musa_core_bulge_to_arc(
                a->x,a->y,z->x,z->y,a->bulge,&arc))return -1;
            if(dst){
                dst[pos]=arc_style.code;
                if(arc_style.extra_words)dst[pos+1u]=arc_style.rgb;
                const size_t at=pos+arc_style.extra_words;
                dst[at+1u]=(float)arc.cx;
                dst[at+2u]=(float)arc.cy;
                dst[at+3u]=(float)arc.radius;dst[at+4u]=0.0f;
                dst[at+5u]=0.0f;dst[at+6u]=(float)arc.radius;
                dst[at+7u]=(float)arc.start;
                dst[at+8u]=(float)arc.sweep;
            }
            double ab[4]={0};
            if(!musa_dwg_ac1032_preview_v6_arc_bounds(
                 arc.cx,arc.cy,arc.radius,arc.start,arc.sweep,ab))
                return -1;
            bounds(ab[0],ab[1],ext,has);
            bounds(ab[2],ab[3],ext,has);
            pos+=9u+arc_style.extra_words;
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
    const MusaPreviewV6SourceStyle poly_style=source_style(
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
    if(count>(SIZE_MAX-3u)/2u||
       2u+2u*count>SIZE_MAX-poly_style.extra_words||
       *w>SIZE_MAX-(2u+2u*count+poly_style.extra_words)||
       *primitive_count>=16777215u)
        return -1;
    const size_t needed=2u+count*2u+poly_style.extra_words;
    if(dst){
        if(*w>cap||needed>cap-*w)return -1;
        dst[*w]=poly_style.code;
        if(poly_style.extra_words)dst[*w+1u]=poly_style.rgb;
        const size_t at=*w+1u+poly_style.extra_words;
        dst[at]=(float)(poly.header.closed?-(int)count:(int)count);
        for(size_t i=0;i<count;i++){
            dst[at+1u+2u*i]=(float)vertices[i].point[0];
            dst[at+2u+2u*i]=(float)vertices[i].point[1];
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
                   int colors,int with_mtext,int with_text,
                   MusaAc1032PreviewV6SourceCensus *census){
    size_t n=musa_dwg_ac1032_scene_index_count(s);
    if(!n||n>MAX_PREVIEW_RECORDS)return 0;
    MusaAc1032PreviewV6Stats v={0};
    v.total_indexed_records=n;
    v.required_float_words=7u;
    MusaAc1032PreviewV6SourceCensus ledger={0};
    ledger.total_indexed_records=n;
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
        const int candidate=t==19u||t==27u||
              (curves&&(t==17u||t==18u||t==35u))||
              (polylines&&(t==16u||t==77u))||
              (with_mtext&&t==44u)||(with_text&&t==1u);
        const size_t prior_primitives=v.rendered_primitives;
        if(candidate)ledger.candidate_source_records++;
        else ledger.other_indexed_records++;
        if(t==19u){
            MusaDwgAc1032NativeLine line={0};
            if(musa_dwg_ac1032_scene_index_read_line(s,e.handle,&line)&&
               (!colors||line.common.invisibility==0u)&&
               planar3(line.geometry.start,line.geometry.thickness,
                       line.geometry.extrusion)&&
               planar3(line.geometry.end,line.geometry.thickness,
                       line.geometry.extrusion)){
                const MusaPreviewV6SourceStyle style=source_style(
                    &line.common,1u,colors,&v.provisional_aci_style);
                if(w>SIZE_MAX-5u-style.extra_words)return 0;
                if(dst){
                    if(5u+style.extra_words>cap-w)return 0;
                    dst[w]=style.code;
                    if(style.extra_words)dst[w+1u]=style.rgb;
                    const size_t at=w+style.extra_words;
                    dst[at+1u]=(float)line.geometry.start[0];
                    dst[at+2u]=(float)line.geometry.start[1];
                    dst[at+3u]=(float)line.geometry.end[0];
                    dst[at+4u]=(float)line.geometry.end[1];
                }
                w+=5u+style.extra_words;
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
            double ab[4]={0};
            if(supported&&(!valid(sweep)||
                !musa_dwg_ac1032_preview_v6_arc_bounds(
                   curve.center[0],curve.center[1],
                   curve.radius,start,sweep,ab)))supported=0;
            if(supported){
                const MusaPreviewV6SourceStyle style=source_style(
                    &curve.common,4u,colors,&v.provisional_aci_style);
                if(w>SIZE_MAX-9u-style.extra_words)return 0;
                if(dst){
                    if(9u+style.extra_words>cap-w)return 0;
                    dst[w]=style.code;
                    if(style.extra_words)dst[w+1u]=style.rgb;
                    const size_t at=w+style.extra_words;
                    dst[at+1u]=(float)curve.center[0];
                    dst[at+2u]=(float)curve.center[1];
                    dst[at+3u]=(float)curve.radius;
                    dst[at+4u]=0.0f;
                    dst[at+5u]=0.0f;
                    dst[at+6u]=(float)curve.radius;
                    dst[at+7u]=(float)start;
                    dst[at+8u]=(float)sweep;
                }
                w+=9u+style.extra_words;
                /* True ARC/whole CIRCLE bounds, no fake full-circle extents. */
                bounds(ab[0],ab[1],ext,&has);
                bounds(ab[2],ab[3],ext,&has);
                v.rendered_primitives++;
            }else v.omitted_records++;
        }else if(t==35u&&curves){
            /* Real AC1032 ELLIPSE: direct WCS semi-major vector, CCW
             * minor axis ratio, source eccentric-anomaly parameter arc.
             * Strictly planar only; no fake projection of a 3D ellipse.
             * v6 type 4 already supports arbitrary affine ellipse axes. */
            MusaDwgAc1032NativeEllipse ellipse={0};
            int supported=musa_dwg_ac1032_scene_index_read_ellipse(
                s,e.handle,&ellipse)&&
                (!colors||ellipse.common.invisibility==0u)&&
                valid(ellipse.center[0])&&valid(ellipse.center[1])&&
                ellipse.center[2]==0.0&&
                ellipse.semi_major[2]==0.0&&
                ellipse.extrusion[0]==0.0&&
                ellipse.extrusion[1]==0.0&&
                ellipse.extrusion[2]==1.0&&
                valid(ellipse.semi_major[0])&&
                valid(ellipse.semi_major[1])&&
                isfinite(ellipse.ratio)&&ellipse.ratio>0.0&&
                ellipse.ratio<=1.0;
            const double tau=6.283185307179586476925286766559;
            double ax=0.0,ay=0.0,bx=0.0,by=0.0;
            double start=0.0,sweep=0.0,extent_x=0.0,extent_y=0.0;
            if(supported){
                ax=ellipse.semi_major[0];ay=ellipse.semi_major[1];
                bx=-ay*ellipse.ratio;by=ax*ellipse.ratio;
                start=ellipse.start_angle;
                sweep=ellipse.end_angle-start;
                if(!isfinite(start)||!isfinite(sweep)||
                   fabs(start)>8.0*tau||fabs(sweep)>8.0*tau)
                    supported=0;
                else if(sweep<0.0)sweep+=tau;
                if(!valid(bx)||!valid(by)||sweep<=0.0||
                   sweep>tau+1e-10)supported=0;
                extent_x=hypot(ax,bx);extent_y=hypot(ay,by);
                if(!valid(extent_x)||!valid(extent_y)||
                   !valid(ellipse.center[0]-extent_x)||
                   !valid(ellipse.center[0]+extent_x)||
                   !valid(ellipse.center[1]-extent_y)||
                   !valid(ellipse.center[1]+extent_y))
                   supported=0;
            }
            if(supported){
                const MusaPreviewV6SourceStyle style=source_style(
                    &ellipse.common,4u,colors,&v.provisional_aci_style);
                if(w>SIZE_MAX-9u-style.extra_words)return 0;
                if(dst){
                    if(9u+style.extra_words>cap-w)return 0;
                    dst[w]=style.code;
                    if(style.extra_words)dst[w+1u]=style.rgb;
                    const size_t at=w+style.extra_words;
                    dst[at+1u]=(float)ellipse.center[0];
                    dst[at+2u]=(float)ellipse.center[1];
                    dst[at+3u]=(float)ax;
                    dst[at+4u]=(float)ay;
                    dst[at+5u]=(float)bx;
                    dst[at+6u]=(float)by;
                    dst[at+7u]=(float)start;
                    dst[at+8u]=(float)sweep;
                }
                w+=9u+style.extra_words;
                /* Whole rotated-ellipse bounds conservatively include
                 * partial elliptical arcs; never clip source geometry. */
                bounds(ellipse.center[0]-extent_x,
                       ellipse.center[1]-extent_y,ext,&has);
                bounds(ellipse.center[0]+extent_x,
                       ellipse.center[1]+extent_y,ext,&has);
                v.rendered_primitives++;
            }else v.omitted_records++;
        }else if(t==44u&&with_mtext){
            /* Genuine original source MTEXT: provisional 1-line Android
             * v6 text glyph proxy, NOT its authentic formatted paragraph
             * layout. Encoded UTF-8 bytes are identical to verified source.
             * Only strictly planar non-tilted WCS and drawing-direction
             * left-to-right are admitted. Unsupported text is omitted,
             * never replaced by guessed object coordinates.
             *
             * v6 type 6 packet:
             * [styleCode, optionalExactRGB24, x,y, ux,uy, vx,vy,
             *  hAlign,vAlign, utf8_bytes, packed UTF-8 words].
             * Only direct authenticated ENC ACI/RGB can be source-exact;
             * BYLAYER/BYBLOCK/alpha/unknown are provisional ACI7.
             * Canvas glyph metrics may differ from the original TEXTSTYLE.
             */
            MusaDwgAc1032MTextSource source={0};
            MusaDwgAc1032MTextPlacement place={0};
            MusaDwgAc1032EntityCommon common={0};
            MusaAc1032ColorProvenance provenance={0};
            int supported=
                musa_dwg_ac1032_scene_index_read_mtext_source(
                    s,e.handle,&source)&&
                musa_dwg_ac1032_scene_index_read_mtext_placement(
                    s,e.handle,&place)&&
                source.indexed_handle==e.handle&&
                place.indexed_handle==e.handle&&
                source.original_record_crc_verified&&
                place.original_record_crc_verified&&
                place.geometry_fields_decoded&&
                !source.geometry_style_and_glyphs_accepted&&
                !place.glyph_layout_and_display_accepted&&
                source.utf8_bytes>0u&&
                source.utf8_bytes<=sizeof(source.utf8)-1u&&
                source.utf8[source.utf8_bytes]=='\0'&&
                strlen(source.utf8)==source.utf8_bytes&&
                musa_dwg_ac1032_mtext_v6_plain_source_preflight(
                    source.utf8,source.utf8_bytes)&&
                place.insertion[2]==0.0&&
                place.x_axis[2]==0.0&&
                place.extrusion[0]==0.0&&
                place.extrusion[1]==0.0&&
                place.extrusion[2]==1.0&&
                place.drawing_direction==5u&&
                place.attachment>=1u&&place.attachment<=9u&&
                valid(place.insertion[0])&&
                valid(place.insertion[1])&&
                valid(place.text_height)&&
                place.text_height>0.0&&
                valid(place.rect_width)&&place.rect_width>0.0&&
                valid(place.rect_height)&&place.rect_height>=0.0&&
                isfinite(place.x_axis[0])&&
                isfinite(place.x_axis[1])&&
                fabs(hypot(place.x_axis[0],place.x_axis[1])-1.0)<1e-6;
            /* A type ID or unverified common ENC cannot determine color.
             * Require the independently CRC/H-verified OT44->OT51 graph
             * before admitting this original source's style evidence.
             * Entity invisibility is a HARD omission, not provisional
             * white text. Layer/owner/viewport visibility remains unknown
             * even when a direct entity color is source-authenticated. */
            if(supported&&colors){
                supported=musa_dwg_ac1032_scene_index_read_mtext_common(
                    s,e.handle,&common)&&
                    musa_dwg_ac1032_scene_planar_color_provenance(
                    s,e.handle,&provenance)&&
                    common.common_decoded&&
                    common.prefix.indexed_handle==e.handle&&
                    common.prefix.raw_object_type==44u&&
                    provenance.entity_and_layer_crc_verified&&
                    provenance.entity_handle==e.handle&&
                    provenance.raw_entity_type==44u&&
                    provenance.layer_handle!=0u&&
                    provenance.entity_aci==common.color_index&&
                    !provenance.rendering_accepted&&
                    common.invisibility==0u&&
                    !provenance.source_entity_invisible;
            }
            const double ux=place.x_axis[0]*place.text_height;
            const double uy=place.x_axis[1]*place.text_height;
            /* Android Canvas y+ points DOWN, while source world y+ UP.
             * Negate the local text v-axis before world->screen flip. */
            const double vx=place.x_axis[1]*place.text_height;
            const double vy=-place.x_axis[0]*place.text_height;
            const double x=place.insertion[0],y=place.insertion[1];
            const double width=place.rect_width;
            const double height=fmax(place.text_height,place.rect_height);
            if(supported&&(!valid(ux)||!valid(uy)||!valid(vx)||
                !valid(vy)||!valid(x+place.x_axis[0]*width)||
                !valid(y+place.x_axis[1]*width)||
                !valid(x-place.x_axis[1]*height)||
                !valid(y+place.x_axis[0]*height)))supported=0;
            if(supported){
                const MusaPreviewV6SourceStyle style=source_style(
                    &common,6u,colors,&v.provisional_aci_style);
                const size_t packed=(source.utf8_bytes+2u)/3u;
                if(packed>SIZE_MAX-10u-style.extra_words||
                   w>SIZE_MAX-(10u+packed+style.extra_words))return 0;
                const size_t needed=10u+packed+style.extra_words;
                if(dst){
                    if(w>cap||needed>cap-w)return 0;
                    dst[w]=style.code;
                    if(style.extra_words)dst[w+1u]=style.rgb;
                    const size_t at=w+style.extra_words;
                    dst[at+1u]=(float)x;dst[at+2u]=(float)y;
                    dst[at+3u]=(float)ux;dst[at+4u]=(float)uy;
                    dst[at+5u]=(float)vx;dst[at+6u]=(float)vy;
                    dst[at+7u]=(float)((place.attachment-1u)%3u);
                    dst[at+8u]=(float)(3u-(place.attachment-1u)/3u);
                    dst[at+9u]=(float)source.utf8_bytes;
                    for(size_t b=0u;b<packed;b++){
                        uint32_t packed_word=0u;
                        for(size_t k=0u;k<3u;k++){
                            const size_t off=b*3u+k;
                            if(off<source.utf8_bytes)
                                packed_word|=(uint32_t)
                                  (uint8_t)source.utf8[off]<<(8u*k);
                        }
                        dst[at+10u+b]=(float)packed_word;
                    }
                }
                w+=needed;
                /* World-space source rectangle is a conservative bound
                 * for possible wrapped text. Viewport glyph fidelity is
                 * NOT certified by this envelope. */
                for(unsigned i=0u;i<4u;i++){
                    const double a=(i&1u)?width:0.0;
                    const double b=(i&2u)?height:0.0;
                    bounds(x+place.x_axis[0]*a-place.x_axis[1]*b,
                           y+place.x_axis[1]*a+place.x_axis[0]*b,
                           ext,&has);
                }
                v.rendered_primitives++;
                v.provisional_aci_style=1u; /* unresolved actual glyphs */
            }else v.omitted_records++;
        }else if(t==1u&&with_text){
            /* Genuine OT1 TEXT, NEVER decoded as MTEXT. The separately
             * authenticated standalone type-6 source packet preserves TU
             * Unicode and WCS anchor/basis, but Canvas font metrics and
             * layer/owner/viewport visibility are NOT equivalent.
             * Any source-hidden/unsupported original is omitted. */
            MusaAc1032TextV6PacketStats text={0};
            MusaDwgAc1032TextBundle b={0};
            int supported=musa_dwg_ac1032_text_v6_packet(
                s,e.handle,NULL,0u,&text)&&
                musa_dwg_ac1032_scene_text_bundle(s,e.handle,&b);
            double x=0.0,y=0.0,h=0.0,reach=0.0;
            const double *u=b.world.baseline_unit;
            const double *vaxis=b.world.up_unit;
            if(supported){
                const double *a=(b.world.horizontal_alignment||
                    b.world.vertical_alignment)?
                    b.world.alignment:b.world.insertion;
                x=a[0];y=a[1];h=b.world.height;
                reach=h*(double)text.utf8_bytes*2.0;
                supported=valid(x)&&valid(y)&&valid(h)&&h>0.0&&
                    valid(reach)&&reach>0.0&&
                    text.source_crc_and_graph_verified&&
                    text.provisional_font_and_visibility&&
                    !text.final_display_accepted&&
                    text.source_handle==e.handle&&
                    text.source_layer_handle==b.graph.layer_handle;
                for(unsigned k=0u;supported&&k<4u;k++){
                    const double along=(k&1u)?reach:-reach;
                    const double across=(k&2u)?2.0*h:-2.0*h;
                    if(!valid(x+along*u[0]+across*vaxis[0])||
                       !valid(y+along*u[1]+across*vaxis[1]))
                        supported=0;
                }
            }
            if(supported){
                if(w>SIZE_MAX-text.float_words)return 0;
                if(dst){
                    MusaAc1032TextV6PacketStats copied={0};
                    if(w>cap||text.float_words>cap-w||
                       !musa_dwg_ac1032_text_v6_packet(
                         s,e.handle,dst+w,cap-w,&copied)||
                       memcmp(&copied,&text,sizeof(text)))return 0;
                }
                w+=text.float_words;
                for(unsigned k=0u;k<4u;k++){
                    const double along=(k&1u)?reach:-reach;
                    const double across=(k&2u)?2.0*h:-2.0*h;
                    bounds(x+along*u[0]+across*vaxis[0],
                           y+along*u[1]+across*vaxis[1],ext,&has);
                }
                v.rendered_primitives++;
                v.provisional_aci_style=1u;
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
                const MusaPreviewV6SourceStyle style=source_style(
                    &point.common,3u,colors,&v.provisional_aci_style);
                if(w>SIZE_MAX-3u-style.extra_words)return 0;
                if(dst){
                    if(3u+style.extra_words>cap-w)return 0;
                    dst[w]=style.code;
                    if(style.extra_words)dst[w+1u]=style.rgb;
                    const size_t at=w+style.extra_words;
                    dst[at+1u]=(float)point.point[0];
                    dst[at+2u]=(float)point.point[1];
                }
                w+=3u+style.extra_words;
                bounds(point.point[0],point.point[1],ext,&has);
                v.rendered_primitives++;
            }else v.omitted_records++;
        }else v.omitted_records++;
        /* The v6 primitive count can exceed source-record count when one
         * LWPOLYLINE expands to multiple bulged segments. Never label
         * all of those primitives as distinct decoded DWG objects. */
        if(candidate){
            if(v.rendered_primitives>prior_primitives)
                ledger.packet_admitted_source_records++;
            else ledger.candidate_not_admitted_records++;
        }
    }
    if(ledger.candidate_source_records+
           ledger.other_indexed_records!=ledger.total_indexed_records||
       ledger.packet_admitted_source_records+
           ledger.candidate_not_admitted_records!=
              ledger.candidate_source_records||
       ledger.candidate_not_admitted_records+
           ledger.other_indexed_records!=v.omitted_records||
       ledger.packet_admitted_source_records>v.rendered_primitives)
        return 0;
    ledger.emitted_v6_primitives=v.rendered_primitives;
    ledger.omitted_v6_records=v.omitted_records;
    ledger.complete_dwg_display_accepted=0u;
    if(v.rendered_primitives>16777215u||v.required_float_words>SIZE_MAX-w)
        return 0;
    v.required_float_words=w;
    v.partial_geometry=(uint8_t)(v.omitted_records>0u||
                                (colors&&v.provisional_aci_style));
    if(dst){
        if(w>cap)return 0;
        dst[0]=6.0f;dst[1]=(float)v.rendered_primitives;
        /* A zero-width source axis maps to one identical float32 value
         * (e.g. real POINT.dwg). Outward expansion is only necessary for
         * NON-DEGENERATE arcs/lines; otherwise zero-area geometry becomes
         * a fake nonzero rectangle. */
        const int zero_x=ext[0]==ext[2];
        const int zero_y=ext[1]==ext[3];
        dst[2]=has?(zero_x?(float)ext[0]:outer_bound(ext[0],1)):0.0f;
        dst[3]=has?(zero_y?(float)ext[1]:outer_bound(ext[1],1)):0.0f;
        dst[4]=has?(zero_x?(float)ext[2]:outer_bound(ext[2],0)):0.0f;
        dst[5]=has?(zero_y?(float)ext[3]:outer_bound(ext[3],0)):0.0f;
        dst[6]=v.partial_geometry?1.0f:0.0f;
    }
    *stats=v;
    if(census)*census=ledger;
    free(poly3d);
    free(lw);
    return 1;
}
static int preview(const MusaDwgAc1032SceneIndex *s,
                  float *words,size_t capacity,
                  MusaAc1032PreviewV6Stats *out,int curves,int polylines,
                  int colors,int with_mtext,int with_text){
    if(!s||!out||(!words&&capacity)||
       capacity>SIZE_MAX/sizeof(float)||
       (words&&overlaps(words,capacity*sizeof(float),out,sizeof(*out))))
        return 0;
    MusaAc1032PreviewV6Stats before={0};
    if(!process(s,NULL,0u,&before,curves,polylines,colors,with_mtext,with_text,NULL))return 0;
    if(!words){*out=before;return 1;}
    if(capacity<before.required_float_words)return 0;
    /* Build in scratch; malformed source or OOM cannot mutate the
     * caller's view buffer, even if record count changes unexpectedly. */
    float *scratch=(float*)malloc(before.required_float_words*sizeof(float));
    if(!scratch)return 0;
    MusaAc1032PreviewV6Stats after={0};
    int ok=process(s,scratch,before.required_float_words,&after,curves,polylines,colors,with_mtext,with_text,NULL)&&
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
    return preview(s,words,capacity,out,0,0,0,0,0);
}
int musa_dwg_ac1032_preview_v6_planar_curves(
    const MusaDwgAc1032SceneIndex *s,float *words,size_t capacity,
    MusaAc1032PreviewV6Stats *out){
    return preview(s,words,capacity,out,1,0,0,0,0);
}

int musa_dwg_ac1032_preview_v6_planar_polylines(
    const MusaDwgAc1032SceneIndex *s,float *words,size_t capacity,
    MusaAc1032PreviewV6Stats *out){
    return preview(s,words,capacity,out,1,1,0,0,0);
}

/* Diagnostic census over the EXACT preview admission path; caller ABI
 * unchanged for every previously published preview function. A candidate
 * is a source numeric type 19/27/17/18/16/77. 3D POLYLINE only counts
 * as packet-admitted if independently proven flat with verified child
 * owner graph. All original data stays read-only. */
int musa_dwg_ac1032_preview_v6_source_census(
    const MusaDwgAc1032SceneIndex *scene,
    MusaAc1032PreviewV6SourceCensus *out){
    if(!scene||!out||(const void*)scene==(const void*)out)
        return 0;
    MusaAc1032PreviewV6Stats stats={0};
    MusaAc1032PreviewV6SourceCensus local={0};
    if(!process(scene,NULL,0u,&stats,1,1,1,0,0,&local)||
       (stats.omitted_records>0u&&stats.partial_geometry==0u)||
       local.emitted_v6_primitives!=stats.rendered_primitives||
       local.omitted_v6_records!=stats.omitted_records||
       local.total_indexed_records!=stats.total_indexed_records||
       local.complete_dwg_display_accepted)return 0;
    *out=local;
    return 1;
}
/* Source-assigned direct ACI where verifiably present. No layer colors. */
int musa_dwg_ac1032_preview_v6_planar_source_aci(
    const MusaDwgAc1032SceneIndex *scene,float *words,size_t cap,
    MusaAc1032PreviewV6Stats *out){
    return preview(scene,words,cap,out,1,1,1,0,0);
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

/* Source-only selector for deterministic direct RGB v6 transport.
 * A 24bit RGB integer is exactly representable by float. Its negative
 * primitive code needs one extra word; provisional ACI7 needs none.
 * No layer/viewport visibility is certified by source RGB alone. */
int musa_dwg_ac1032_preview_v6_entity_source_style_code(
    const MusaDwgAc1032EntityCommon *source,uint8_t kind,
    float *code,float *rgb,uint8_t *extra,uint8_t *exact){
    if(!source||!code||!rgb||!extra||!exact||
       !source->common_decoded||kind<1u||(kind>4u&&kind!=6u)||
       overlaps(source,sizeof(*source),code,sizeof(*code))||
       overlaps(source,sizeof(*source),rgb,sizeof(*rgb))||
       overlaps(source,sizeof(*source),extra,sizeof(*extra))||
       overlaps(source,sizeof(*source),exact,sizeof(*exact))||
       overlaps(code,sizeof(*code),rgb,sizeof(*rgb))||
       overlaps(code,sizeof(*code),extra,sizeof(*extra))||
       overlaps(code,sizeof(*code),exact,sizeof(*exact))||
       overlaps(rgb,sizeof(*rgb),extra,sizeof(*extra))||
       overlaps(rgb,sizeof(*rgb),exact,sizeof(*exact))||
       overlaps(extra,sizeof(*extra),exact,sizeof(*exact)))return 0;
    uint8_t provisional=0u;
    const MusaPreviewV6SourceStyle style=
        source_style(source,kind,1,&provisional);
    *code=style.code;
    *rgb=style.rgb;
    *extra=(uint8_t)style.extra_words;
    *exact=(uint8_t)!provisional;
    return 1;
}

/* Opt-in genuine CRC/H source MTEXT to v6 TEXT primitive; source Unicode
 * bytes and insertion/axes survive, original font/paragraph layout does
 * not. No final display acceptance and no changes to old v6 callers. */
int musa_dwg_ac1032_preview_v6_planar_source_with_mtext(
    const MusaDwgAc1032SceneIndex *scene,float *words,size_t cap,
    MusaAc1032PreviewV6Stats *out){
    return preview(scene,words,cap,out,1,1,1,1,0);
}
int musa_dwg_ac1032_preview_v6_source_census_with_mtext(
    const MusaDwgAc1032SceneIndex *scene,
    MusaAc1032PreviewV6SourceCensus *out){
    if(!scene||!out||(const void*)scene==(const void*)out)return 0;
    MusaAc1032PreviewV6Stats stats={0};
    MusaAc1032PreviewV6SourceCensus local={0};
    if(!process(scene,NULL,0u,&stats,1,1,1,1,0,&local)||
       local.emitted_v6_primitives!=stats.rendered_primitives||
       local.omitted_v6_records!=stats.omitted_records||
       local.total_indexed_records!=stats.total_indexed_records||
       local.complete_dwg_display_accepted)return 0;
    *out=local;return 1;
}

/* Explicit opt-in original OT1 TEXT + OT44 MTEXT diagnostic preview.
 * No final font, model-space, viewport, BYLAYER/BYBLOCK or block ancestry. */
int musa_dwg_ac1032_preview_v6_planar_source_with_text(
    const MusaDwgAc1032SceneIndex *scene,float *words,size_t cap,
    MusaAc1032PreviewV6Stats *out){
    return preview(scene,words,cap,out,1,1,1,1,1);
}
int musa_dwg_ac1032_preview_v6_source_census_with_text(
    const MusaDwgAc1032SceneIndex *scene,
    MusaAc1032PreviewV6SourceCensus *out){
    if(!scene||!out||(const void*)scene==(const void*)out)return 0;
    MusaAc1032PreviewV6Stats stats={0};
    MusaAc1032PreviewV6SourceCensus local={0};
    if(!process(scene,NULL,0u,&stats,1,1,1,1,1,&local)||
       local.emitted_v6_primitives!=stats.rendered_primitives||
       local.omitted_v6_records!=stats.omitted_records||
       local.total_indexed_records!=stats.total_indexed_records||
       local.complete_dwg_display_accepted)return 0;
    *out=local;return 1;
}
