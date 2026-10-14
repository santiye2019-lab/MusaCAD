/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "config.h"
#include "native_color.h"
#include "scene_coord_safety.h"
#include "native_nurbs.h"
#include "native_fit_spline.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>
#ifdef MUSA_HOST
#include <stdio.h>
#endif
#include "dwg.h"
#include "dwg_api.h"
#include "bits.h"
#include "geom.h"
#include "native_scene.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#define MUSA_SCENE_VERSION 6.0f
/* v6 keeps the compact v5 geometry stream and adds packed UTF-8 single-line
   text primitives so labels can appear during native first paint. The 9M cap
   remains bounded for large-phone memory safety. */
#define MUSA_SCENE_MAX_FLOATS (9u*1024u*1024u)
#define MUSA_MAX_BLOCK_DEPTH 24

typedef struct { double a,b,c,d,tx,ty; } Affine2;
typedef struct { int aci; unsigned int rgb; int truecolor; } SceneColor;

static Affine2 identity2(void){Affine2 m={1,0,0,1,0,0};return m;}
static Affine2 multiply2(Affine2 p,Affine2 q){
    Affine2 r;
    r.a=p.a*q.a+p.c*q.b;r.b=p.b*q.a+p.d*q.b;
    r.c=p.a*q.c+p.c*q.d;r.d=p.b*q.c+p.d*q.d;
    r.tx=p.a*q.tx+p.c*q.ty+p.tx;r.ty=p.b*q.tx+p.d*q.ty+p.ty;
    return r;
}
static void map2(Affine2 m,double x,double y,double *ox,double *oy){
    *ox=m.a*x+m.c*y+m.tx;*oy=m.b*x+m.d*y+m.ty;
}
static int finite2(double x,double y){return isfinite(x)&&isfinite(y);}

static int reserve_scene(MusaNativeScene *s,size_t extra){
    if(!s||s->truncated)return 0;
    const size_t limit=s->max_words?s->max_words:MUSA_SCENE_MAX_FLOATS;
    if(extra>limit||s->count>limit-extra){s->truncated=1;return 0;}
    size_t need=s->count+extra;
    if(need<=s->capacity)return 1;
    size_t cap=s->capacity?s->capacity:4096;
    if(cap>limit)cap=limit;
    while(cap<need&&cap<limit){size_t next=cap*2;if(next<cap){cap=limit;break;}cap=next;}
    if(cap>limit)cap=limit;
    if(cap<need){s->truncated=1;return 0;}
    float *next=(float*)realloc(s->values,cap*sizeof(float));
    if(!next){s->truncated=1;return 0;}
    s->values=next;s->capacity=cap;return 1;
}
static int pushf(MusaNativeScene *s,float v){if(!reserve_scene(s,1))return 0;s->values[s->count++]=v;return 1;}
static size_t color_words(SceneColor color){return color.truecolor?2u:1u;}
static void push_code(MusaNativeScene *s,int type,SceneColor color){
    if(color.truecolor){
        pushf(s,(float)(-(type*256+1)));
        pushf(s,(float)(color.rgb&0x00ffffffu));
    }else pushf(s,(float)(type*256+(color.aci&255)));
}
static void add_bounds(MusaNativeScene *s,double x,double y){
    if(!finite2(x,y))return;
    if(!s->has_bounds){s->min_x=s->max_x=x;s->min_y=s->max_y=y;s->has_bounds=1;return;}
    if(x<s->min_x)s->min_x=x;if(x>s->max_x)s->max_x=x;
    if(y<s->min_y)s->min_y=y;if(y>s->max_y)s->max_y=y;
}
static int layer_invisible(Dwg_Object *obj){
    if(!obj||!obj->tio.entity)return 1;
    Dwg_Object_Entity *ent=obj->tio.entity;
    if(ent->invisible)return 1;
    if(!ent->layer||!ent->layer->obj)return 0;
    Dwg_Object *layer=ent->layer->obj;
    if(layer->fixedtype!=DWG_TYPE_LAYER||!layer->tio.object||!layer->tio.object->tio.LAYER)return 0;
    Dwg_Object_LAYER *l=layer->tio.object->tio.LAYER;
    return l->off||l->frozen;
}
static SceneColor scene_aci(int aci){
    SceneColor out;out.aci=(aci>=1&&aci<=255)?aci:7;out.rgb=0;out.truecolor=0;return out;
}
static SceneColor scene_true(unsigned int rgb){
    SceneColor out;out.aci=7;out.rgb=rgb&0x00ffffffu;out.truecolor=1;return out;
}
static Dwg_Object_LAYER *entity_layer(Dwg_Object *obj){
    if(!obj||!obj->tio.entity)return NULL;
    Dwg_Object_Ref *lr=obj->tio.entity->layer;
    if(!lr||!lr->obj||lr->obj->fixedtype!=DWG_TYPE_LAYER||!lr->obj->tio.object)return NULL;
    return lr->obj->tio.object->tio.LAYER;
}
static SceneColor color_value(Dwg_Color *value,SceneColor fallback){
    if(!value)return fallback;
    MusaNativeColorToken token=musa_native_color_token((unsigned int)value->method,(int)value->index,(unsigned int)value->rgb);
    if(token.kind==MUSA_COLOR_RGB)return scene_true(token.rgb);
    if(token.kind==MUSA_COLOR_ACI)return scene_aci(token.aci);
    return fallback;
}
static int layer_is_zero(Dwg_Object_LAYER *layer){return layer&&layer->name&&strcmp(layer->name,"0")==0;}
static SceneColor effective_layer_color(Dwg_Object *obj,SceneColor inheritedLayer,int insideBlock){
    Dwg_Object_LAYER *layer=entity_layer(obj);
    if(insideBlock&&layer_is_zero(layer))return inheritedLayer;
    return layer?color_value(&layer->color,scene_aci(7)):inheritedLayer;
}
static SceneColor entity_color(Dwg_Object *obj,SceneColor byBlock,SceneColor layerColor){
    if(!obj||!obj->tio.entity)return layerColor;
    Dwg_Color *entity=&obj->tio.entity->color;
    MusaNativeColorToken token=musa_native_color_token((unsigned int)entity->method,(int)entity->index,(unsigned int)entity->rgb);
    if(token.kind==MUSA_COLOR_RGB)return scene_true(token.rgb);
    if(token.kind==MUSA_COLOR_ACI)return scene_aci(token.aci);
    if(token.kind==MUSA_COLOR_INHERIT_BLOCK)return byBlock;
    return layerColor;
}
static int emit_line(MusaNativeScene *s,SceneColor color,Affine2 m,double x1,double y1,double x2,double y2){
    double ax,ay,bx,by;map2(m,x1,y1,&ax,&ay);map2(m,x2,y2,&bx,&by);
    if(!musa_scene_xy_safe(ax,ay)||!musa_scene_xy_safe(bx,by)){
        s->truncated=1;return 0;
    }
    if(!reserve_scene(s,4u+color_words(color)))return 0;
    push_code(s,1,color);pushf(s,(float)ax);pushf(s,(float)ay);pushf(s,(float)bx);pushf(s,(float)by);
    add_bounds(s,ax,ay);add_bounds(s,bx,by);s->primitives++;return 1;
}
static int emit_point(MusaNativeScene *s,SceneColor color,Affine2 m,double x,double y){
    double ax,ay;map2(m,x,y,&ax,&ay);
    if(!musa_scene_xy_safe(ax,ay)){s->truncated=1;return 0;}
    if(!reserve_scene(s,2u+color_words(color)))return 0;
    push_code(s,3,color);pushf(s,(float)ax);pushf(s,(float)ay);add_bounds(s,ax,ay);s->primitives++;return 1;
}
static int begin_poly(MusaNativeScene *s,SceneColor color,int closed,int n){
    if(n<2||!reserve_scene(s,1u+color_words(color)+(size_t)n*2u))return 0;
    push_code(s,2,color);pushf(s,(float)(closed?-n:n));return 1;
}
static void emit_poly_point(MusaNativeScene *s,Affine2 m,double x,double y){
    double ax,ay;map2(m,x,y,&ax,&ay);
    if(!musa_scene_xy_safe(ax,ay)){
        // Never invent a vertex at the world origin. The page will roll
        // back transactionally; full-scene preview stays visibly partial.
        s->truncated=1;return;
    }
    pushf(s,(float)ax);pushf(s,(float)ay);add_bounds(s,ax,ay);
}
static void finish_poly(MusaNativeScene *s){if(!s->truncated)s->primitives++;}
static double infinite_reach(Dwg_Data *dwg){
    if(dwg){
        double x0=dwg_model_x_min(dwg),y0=dwg_model_y_min(dwg),x1=dwg_model_x_max(dwg),y1=dwg_model_y_max(dwg);
        if(isfinite(x0)&&isfinite(y0)&&isfinite(x1)&&isfinite(y1)&&x1>x0&&y1>y0){
            double span=hypot(x1-x0,y1-y0);if(isfinite(span)&&span>1e-9)return fmax(span*4.0,1.0);
        }
    }
    return 1000000.0;
}
static void emit_infinite_line(MusaNativeScene *s,SceneColor color,Affine2 parent,Dwg_Data *dwg,
                               double x,double y,double dx,double dy,int ray){
    double len=hypot(dx,dy);if(!isfinite(len)||len<1e-12||!finite2(x,y))return;
    dx/=len;dy/=len;double reach=infinite_reach(dwg);
    if(ray)emit_line(s,color,parent,x,y,x+dx*reach,y+dy*reach);
    else emit_line(s,color,parent,x-dx*reach,y-dy*reach,x+dx*reach,y+dy*reach);
}
static double clampd(double v,double lo,double hi){return v<lo?lo:(v>hi?hi:v);}
static void emit_mline_element(MusaNativeScene *s,SceneColor color,Affine2 parent,
                               dwg_mline_vertex *a,dwg_mline_vertex *b,int line_index){
    if(!a||!b||!a->lines||line_index<0||line_index>=a->num_lines)return;
    dwg_mline_line *al=(dwg_mline_line*)a->lines,*bl=b->lines?(dwg_mline_line*)b->lines:NULL;
    dwg_mline_line *la=&al[line_index],*lb=(bl&&line_index<b->num_lines)?&bl[line_index]:NULL;
    double ao=(la->segparms&&la->num_segparms>0)?la->segparms[0]:0.0;
    double bo=(lb&&lb->segparms&&lb->num_segparms>0)?lb->segparms[0]:ao;
    double ax=a->vertex.x+a->miter_direction.x*ao,ay=a->vertex.y+a->miter_direction.y*ao;
    double bx=b->vertex.x+b->miter_direction.x*bo,by=b->vertex.y+b->miter_direction.y*bo;
    double dx=a->vertex_direction.x,dy=a->vertex_direction.y,len=hypot(dx,dy);
    if(!isfinite(len)||len<1e-12){dx=bx-ax;dy=by-ay;len=hypot(dx,dy);}
    if(!isfinite(len)||len<1e-12)return;dx/=len;dy/=len;
    double start_shift=(la->segparms&&la->num_segparms>1)?la->segparms[1]:0.0;
    double sx=ax+dx*start_shift,sy=ay+dy*start_shift;
    double total=(bx-sx)*dx+(by-sy)*dy;
    if(!isfinite(total)||total<=1e-9){
        dx=bx-sx;dy=by-sy;len=hypot(dx,dy);if(!isfinite(len)||len<1e-12)return;dx/=len;dy/=len;total=len;
    }
    if(!la->segparms||la->num_segparms<=2){emit_line(s,color,parent,sx,sy,sx+dx*total,sy+dy*total);return;}
    double cursor=0.0;
    for(int k=2;k<la->num_segparms;k+=2){
        double cut_start=clampd(la->segparms[k],0.0,total);
        if(cut_start>cursor+1e-9)emit_line(s,color,parent,sx+dx*cursor,sy+dy*cursor,sx+dx*cut_start,sy+dy*cut_start);
        if(k+1<la->num_segparms)cursor=clampd(la->segparms[k+1],cut_start,total);else{cursor=cut_start;break;}
    }
    if(cursor<total-1e-9)emit_line(s,color,parent,sx+dx*cursor,sy+dy*cursor,sx+dx*total,sy+dy*total);
}
static void emit_mline(MusaNativeScene *s,SceneColor color,Affine2 parent,dwg_ent_mline *e){
    if(!e||!e->verts||e->num_verts<2||e->num_lines<1||e->num_lines>128)return;
    int n=(int)e->num_verts,closed=(e->flags&2)!=0,segments=closed?n:n-1;
    for(int v=0;v<segments&&!s->truncated;v++){
        dwg_mline_vertex *a=&e->verts[v],*b=&e->verts[(v+1)%n];
        for(int line=0;line<(int)e->num_lines&&!s->truncated;line++)emit_mline_element(s,color,parent,a,b,line);
    }
}

static int begin_fill_poly(MusaNativeScene *s,int n){
    SceneColor color=scene_aci(7);
    if(n<3||!reserve_scene(s,1u+color_words(color)+(size_t)n*2u))return 0;
    push_code(s,5,color);pushf(s,(float)(-n));return 1;
}
static void emit_image_frame(MusaNativeScene *s,SceneColor color,Affine2 parent,dwg_ent_image *e){
    if(!e)return;BITCODE_BL n=(e->clipping&&e->clip_verts&&e->num_clip_verts>=3)?e->num_clip_verts:4;
    if(n<3||n>100000||!begin_poly(s,color,1,(int)n))return;
    if(e->clipping&&e->clip_verts&&e->num_clip_verts>=3){
        for(BITCODE_BL i=0;i<n;i++){
            double px=e->clip_verts[i].x+.5,py=e->clip_verts[i].y+.5;
            emit_poly_point(s,parent,e->pt0.x+e->uvec.x*px+e->vvec.x*py,e->pt0.y+e->uvec.y*px+e->vvec.y*py);
        }
    }else{
        double width=fabs(e->image_size.x),height=fabs(e->image_size.y);if(!isfinite(width)||width<1.0)width=1.0;if(!isfinite(height)||height<1.0)height=1.0;
        emit_poly_point(s,parent,e->pt0.x,e->pt0.y);
        emit_poly_point(s,parent,e->pt0.x+e->uvec.x*width,e->pt0.y+e->uvec.y*width);
        emit_poly_point(s,parent,e->pt0.x+e->uvec.x*width+e->vvec.x*height,e->pt0.y+e->uvec.y*width+e->vvec.y*height);
        emit_poly_point(s,parent,e->pt0.x+e->vvec.x*height,e->pt0.y+e->vvec.y*height);
    }
    finish_poly(s);
}
static void emit_ole_frame(MusaNativeScene *s,SceneColor color,Affine2 parent,dwg_ent_ole2frame *e){
    if(!e||!finite2(e->pt1.x,e->pt1.y)||!finite2(e->pt2.x,e->pt2.y))return;
    if(fabs(e->pt2.x-e->pt1.x)+fabs(e->pt2.y-e->pt1.y)<1e-9||!begin_poly(s,color,1,4))return;
    emit_poly_point(s,parent,e->pt1.x,e->pt1.y);emit_poly_point(s,parent,e->pt2.x,e->pt1.y);
    emit_poly_point(s,parent,e->pt2.x,e->pt2.y);emit_poly_point(s,parent,e->pt1.x,e->pt2.y);finish_poly(s);
}
static void emit_wipeout(MusaNativeScene *s,Affine2 parent,Dwg_Entity_WIPEOUT *e){
    if(!e)return;
    BITCODE_BL n=(e->clip_verts&&e->num_clip_verts>=3)?e->num_clip_verts:4;
    if(n<3||n>100000||!begin_fill_poly(s,(int)n))return;
    if(e->clip_verts&&e->num_clip_verts>=3){
        for(BITCODE_BL i=0;i<n;i++){
            double px=e->clip_verts[i].x+.5,py=e->clip_verts[i].y+.5;
            double x=e->pt0.x+e->uvec.x*px+e->vvec.x*py;
            double y=e->pt0.y+e->uvec.y*px+e->vvec.y*py;
            emit_poly_point(s,parent,x,y);
        }
    }else{
        double width=fabs(e->image_size.x);if(!isfinite(width)||width<1.0)width=1.0;
        double height=fabs(e->image_size.y);if(!isfinite(height)||height<1.0)height=1.0;
        double pts[8]={-.5,-.5,width-.5,-.5,width-.5,height-.5,-.5,height-.5};
        for(int i=0;i<4;i++){
            double px=pts[i*2]+.5,py=pts[i*2+1]+.5;
            double x=e->pt0.x+e->uvec.x*px+e->vvec.x*py;
            double y=e->pt0.y+e->uvec.y*px+e->vvec.y*py;
            emit_poly_point(s,parent,x,y);
        }
    }
    finish_poly(s);
}
static size_t bounded_text_len(const char *value){
    if(!value)return 0;size_t n=0;while(n<4096&&value[n])n++;return n;
}
static void emit_text_basis(MusaNativeScene *s,SceneColor color,double ax,double ay,double ux,double uy,double vx,double vy,
                            int halign,int valign,const char *value){
    size_t bytes=bounded_text_len(value);if(bytes==0||!finite2(ax,ay)||!finite2(ux,uy)||!finite2(vx,vy))return;
    size_t words=(bytes+2u)/3u;if(!reserve_scene(s,9u+color_words(color)+words))return;
    push_code(s,6,color);pushf(s,(float)ax);pushf(s,(float)ay);pushf(s,(float)ux);pushf(s,(float)uy);pushf(s,(float)vx);pushf(s,(float)vy);
    pushf(s,(float)halign);pushf(s,(float)valign);pushf(s,(float)bytes);
    const unsigned char *raw=(const unsigned char*)value;
    for(size_t i=0;i<bytes;i+=3){
        unsigned int packed=raw[i];
        if(i+1<bytes)packed|=((unsigned int)raw[i+1])<<8;
        if(i+2<bytes)packed|=((unsigned int)raw[i+2])<<16;
        pushf(s,(float)packed);
    }
    double glyphs=(double)bytes*.65;if(glyphs<1.0)glyphs=1.0;
    double shift=(halign==1||halign==4||halign==3||halign==5)?-.5*glyphs:(halign==2?-glyphs:0.0);
    double x0=ax+ux*shift,y0=ay+uy*shift,x1=ax+ux*(shift+glyphs),y1=ay+uy*(shift+glyphs);
    add_bounds(s,x0,y0);add_bounds(s,x1,y1);add_bounds(s,x0-vx,y0-vy);add_bounds(s,x1-vx,y1-vy);s->primitives++;
}
static void emit_text_ocs(MusaNativeScene *s,SceneColor color,Affine2 parent,BITCODE_BE extrusion,
                          BITCODE_2DPOINT ins,BITCODE_2DPOINT align,double rotation,double height,double width_factor,
                          int halign,int valign,const char *value){
    if(bounded_text_len(value)==0||!isfinite(height)||height<=1e-12)return;
    if(!isfinite(rotation))rotation=0;double wf=isfinite(width_factor)&&fabs(width_factor)>1e-9?fabs(width_factor):1.0;
    BITCODE_2DPOINT base=((halign!=0||valign!=0)&&finite2(align.x,align.y))?align:ins;
    double co=cos(rotation),si=sin(rotation);
    BITCODE_2DPOINT ix={base.x+co*height*wf,base.y+si*height*wf};
    BITCODE_2DPOINT iy={base.x+si*height,base.y-co*height};
    BITCODE_2DPOINT pb,px,py;transform_OCS_2d(&pb,base,extrusion);transform_OCS_2d(&px,ix,extrusion);transform_OCS_2d(&py,iy,extrusion);
    double ax,ay,bx,by,cx,cy;map2(parent,pb.x,pb.y,&ax,&ay);map2(parent,px.x,px.y,&bx,&by);map2(parent,py.x,py.y,&cx,&cy);
    emit_text_basis(s,color,ax,ay,bx-ax,by-ay,cx-ax,cy-ay,halign,valign,value);
}
static void emit_mtext(MusaNativeScene *s,SceneColor color,Affine2 parent,Dwg_Entity_MTEXT *e){
    if(!e||bounded_text_len(e->text)==0||!isfinite(e->text_height)||e->text_height<=1e-12)return;
    double dx=e->x_axis_dir.x,dy=e->x_axis_dir.y,len=hypot(dx,dy);if(!isfinite(len)||len<1e-12){dx=1.0;dy=0.0;len=1.0;}dx/=len;dy/=len;
    double ax,ay,px,py,qx,qy;map2(parent,e->ins_pt.x,e->ins_pt.y,&ax,&ay);
    map2(parent,e->ins_pt.x+dx*e->text_height,e->ins_pt.y+dy*e->text_height,&px,&py);
    map2(parent,e->ins_pt.x+dy*e->text_height,e->ins_pt.y-dx*e->text_height,&qx,&qy);
    int h=0,v=3;switch(e->attachment){
        case 2:h=1;v=3;break;case 3:h=2;v=3;break;
        case 4:h=0;v=2;break;case 5:h=1;v=2;break;case 6:h=2;v=2;break;
        case 7:h=0;v=1;break;case 8:h=1;v=1;break;case 9:h=2;v=1;break;
        default:h=0;v=3;break;
    }
    emit_text_basis(s,color,ax,ay,px-ax,py-ay,qx-ax,qy-ay,h,v,e->text);
}


static int emit_curve(MusaNativeScene *s,SceneColor color,Affine2 m,double cx,double cy,double ux,double uy,double vx,double vy,double start,double sweep){
    double wcx,wcy,pu_x,pu_y,pv_x,pv_y;map2(m,cx,cy,&wcx,&wcy);map2(m,cx+ux,cy+uy,&pu_x,&pu_y);map2(m,cx+vx,cy+vy,&pv_x,&pv_y);
    ux=pu_x-wcx;uy=pu_y-wcy;vx=pv_x-wcx;vy=pv_y-wcy;
    if(!musa_scene_xy_safe(wcx,wcy)||!musa_scene_xy_safe(ux,uy)||
       !musa_scene_xy_safe(vx,vy)||!musa_scene_float_safe(start)||
       !musa_scene_float_safe(sweep)){
        s->truncated=1;return 0;
    }
    if(fabs(sweep)<1e-12||!reserve_scene(s,8u+color_words(color)))return 0;
    push_code(s,4,color);pushf(s,(float)wcx);pushf(s,(float)wcy);pushf(s,(float)ux);pushf(s,(float)uy);pushf(s,(float)vx);pushf(s,(float)vy);pushf(s,(float)start);pushf(s,(float)sweep);
    double rx=hypot(ux,vx),ry=hypot(uy,vy);add_bounds(s,wcx-rx,wcy-ry);add_bounds(s,wcx+rx,wcy+ry);s->primitives++;return 1;
}
static void emit_arc_curve(MusaNativeScene *s,SceneColor color,Affine2 m,double cx,double cy,double ux,double uy,double vx,double vy,double start,double end,int full){
    double sweep=full?2.0*M_PI:end-start;while(sweep<=0)sweep+=2.0*M_PI;if(sweep>2.0*M_PI)sweep=2.0*M_PI;
    emit_curve(s,color,m,cx,cy,ux,uy,vx,vy,start,sweep);
}
static void emit_ellipse_curve(MusaNativeScene *s,SceneColor color,Affine2 m,Dwg_Entity_ELLIPSE *e){
    if(!e||!isfinite(e->center.x)||!isfinite(e->center.y)||!isfinite(e->sm_axis.x)||!isfinite(e->sm_axis.y)||!isfinite(e->axis_ratio))return;
    /* LibreDWG exposes ELLIPSE center and major-axis vector in drawing coordinates.
       Do not apply the circle/arc OCS transform a second time here. */
    double ratio=fabs(e->axis_ratio);
    double ux=e->sm_axis.x,uy=e->sm_axis.y,vx=-e->sm_axis.y*ratio,vy=e->sm_axis.x*ratio;
    double sweep=e->end_angle-e->start_angle;while(sweep<=0)sweep+=2.0*M_PI;if(sweep>2.0*M_PI)sweep=2.0*M_PI;
    emit_curve(s,color,m,e->center.x,e->center.y,ux,uy,vx,vy,e->start_angle,sweep);
}
static void emit_hatch_segment(MusaNativeScene *s,SceneColor color,Affine2 parent,BITCODE_BE extrusion,BITCODE_2RD a,BITCODE_2RD b,double bulge){
    double dx=b.x-a.x,dy=b.y-a.y,chord=hypot(dx,dy);
    if(chord<1e-12||!isfinite(bulge)||fabs(bulge)<1e-12){
        BITCODE_2DPOINT ia={a.x,a.y},ib={b.x,b.y},pa,pb;transform_OCS_2d(&pa,ia,extrusion);transform_OCS_2d(&pb,ib,extrusion);emit_line(s,color,parent,pa.x,pa.y,pb.x,pb.y);return;
    }
    double theta=4.0*atan(bulge),h=chord*(1.0-bulge*bulge)/(4.0*bulge);
    double nx=-dy/chord,ny=dx/chord,cx=(a.x+b.x)*0.5+nx*h,cy=(a.y+b.y)*0.5+ny*h;
    double radius=chord*(1.0+bulge*bulge)/(4.0*fabs(bulge)),start=atan2(a.y-cy,a.x-cx);
    BITCODE_2DPOINT ic={cx,cy},ix={cx+radius,cy},iy={cx,cy+radius},pc,px,py;
    transform_OCS_2d(&pc,ic,extrusion);transform_OCS_2d(&px,ix,extrusion);transform_OCS_2d(&py,iy,extrusion);
    emit_curve(s,color,parent,pc.x,pc.y,px.x-pc.x,px.y-pc.y,py.x-pc.x,py.y-pc.y,start,theta);
}

static double positive_sweep(double angle){
    while(angle<0)angle+=2.0*M_PI;
    while(angle>=2.0*M_PI)angle-=2.0*M_PI;
    return angle;
}
static void emit_hatch_path_segment(MusaNativeScene *s,SceneColor color,Affine2 parent,BITCODE_BE extrusion,Dwg_HATCH_PathSeg *seg){
    if(!seg||s->truncated)return;
    if(seg->curve_type==1){
        BITCODE_2DPOINT a={seg->first_endpoint.x,seg->first_endpoint.y},b={seg->second_endpoint.x,seg->second_endpoint.y},pa,pb;
        transform_OCS_2d(&pa,a,extrusion);transform_OCS_2d(&pb,b,extrusion);
        emit_line(s,color,parent,pa.x,pa.y,pb.x,pb.y);
    }else if(seg->curve_type==2&&isfinite(seg->radius)&&seg->radius>1e-12){
        BITCODE_2DPOINT ic={seg->center.x,seg->center.y},ix={seg->center.x+seg->radius,seg->center.y},iy={seg->center.x,seg->center.y+seg->radius},pc,px,py;
        transform_OCS_2d(&pc,ic,extrusion);transform_OCS_2d(&px,ix,extrusion);transform_OCS_2d(&py,iy,extrusion);
        double sweep=seg->is_ccw?positive_sweep(seg->end_angle-seg->start_angle):-positive_sweep(seg->start_angle-seg->end_angle);
        if(fabs(sweep)<1e-12)sweep=seg->is_ccw?2.0*M_PI:-2.0*M_PI;
        emit_curve(s,color,parent,pc.x,pc.y,px.x-pc.x,px.y-pc.y,py.x-pc.x,py.y-pc.y,seg->start_angle,sweep);
    }else if(seg->curve_type==3){
        double ux=seg->endpoint.x,uy=seg->endpoint.y,ratio=fabs(seg->minor_major_ratio);
        if(!finite2(ux,uy)||ratio<1e-12)return;
        double vx=-uy*ratio,vy=ux*ratio;
        BITCODE_2DPOINT ic={seg->center.x,seg->center.y},iu={seg->center.x+ux,seg->center.y+uy},iv={seg->center.x+vx,seg->center.y+vy},pc,pu,pv;
        transform_OCS_2d(&pc,ic,extrusion);transform_OCS_2d(&pu,iu,extrusion);transform_OCS_2d(&pv,iv,extrusion);
        double sweep=seg->is_ccw?positive_sweep(seg->end_angle-seg->start_angle):-positive_sweep(seg->start_angle-seg->end_angle);
        if(fabs(sweep)<1e-12)sweep=seg->is_ccw?2.0*M_PI:-2.0*M_PI;
        emit_curve(s,color,parent,pc.x,pc.y,pu.x-pc.x,pu.y-pc.y,pv.x-pc.x,pv.y-pc.y,seg->start_angle,sweep);
    }else if(seg->curve_type==4){
        BITCODE_BL n=seg->num_fitpts>=2&&seg->fitpts?seg->num_fitpts:seg->num_control_points;
        if(n<2)return;
        if(begin_poly(s,color,seg->is_periodic!=0,(int)n)){
            for(BITCODE_BL i=0;i<n;i++){
                BITCODE_2DPOINT in,p;
                if(seg->num_fitpts>=2&&seg->fitpts){in.x=seg->fitpts[i].x;in.y=seg->fitpts[i].y;}
                else {in.x=seg->control_points[i].point.x;in.y=seg->control_points[i].point.y;}
                transform_OCS_2d(&p,in,extrusion);emit_poly_point(s,parent,p.x,p.y);
            }
            finish_poly(s);
        }
    }
}
static void emit_arrow_head(MusaNativeScene *s,SceneColor color,Affine2 parent,double tipx,double tipy,double prevx,double prevy,double size){
    double dx=prevx-tipx,dy=prevy-tipy,len=hypot(dx,dy);if(len<1e-12||!isfinite(size)||size<=0)return;
    dx/=len;dy/=len;double nx=-dy,ny=dx;
    emit_line(s,color,parent,tipx,tipy,tipx+dx*size+nx*size*.36,tipy+dy*size+ny*size*.36);
    emit_line(s,color,parent,tipx,tipy,tipx+dx*size-nx*size*.36,tipy+dy*size-ny*size*.36);
}
static void emit_leader_points(MusaNativeScene *s,SceneColor color,Affine2 parent,BITCODE_3DPOINT *pts,BITCODE_BL n,double arrow_size){
    if(!pts||n<2)return;
    for(BITCODE_BL i=1;i<n&&!s->truncated;i++)emit_line(s,color,parent,pts[i-1].x,pts[i-1].y,pts[i].x,pts[i].y);
    if(!s->truncated)emit_arrow_head(s,color,parent,pts[n-1].x,pts[n-1].y,pts[n-2].x,pts[n-2].y,arrow_size);
}


static void emit_lw_segment(MusaNativeScene *s,SceneColor color,Affine2 parent,Dwg_Entity_LWPOLYLINE *e,dwg_point_2d a,dwg_point_2d b,double bulge){
    double dx=b.x-a.x,dy=b.y-a.y,chord=hypot(dx,dy);
    if(chord<1e-12||!isfinite(bulge)||fabs(bulge)<1e-12){
        BITCODE_2DPOINT ia={a.x,a.y},ib={b.x,b.y},pa,pb;transform_OCS_2d(&pa,ia,e->extrusion);transform_OCS_2d(&pb,ib,e->extrusion);emit_line(s,color,parent,pa.x,pa.y,pb.x,pb.y);return;
    }
    double theta=4.0*atan(bulge),h=chord*(1.0-bulge*bulge)/(4.0*bulge);
    double nx=-dy/chord,ny=dx/chord,cx=(a.x+b.x)*0.5+nx*h,cy=(a.y+b.y)*0.5+ny*h;
    double radius=chord*(1.0+bulge*bulge)/(4.0*fabs(bulge)),start=atan2(a.y-cy,a.x-cx);
    BITCODE_2DPOINT ic={cx,cy},ix={cx+radius,cy},iy={cx,cy+radius},pc,px,py;
    transform_OCS_2d(&pc,ic,e->extrusion);transform_OCS_2d(&px,ix,e->extrusion);transform_OCS_2d(&py,iy,e->extrusion);
    emit_curve(s,color,parent,pc.x,pc.y,px.x-pc.x,px.y-pc.y,py.x-pc.x,py.y-pc.y,start,theta);
}

static void emit_object(Dwg_Object *obj,MusaNativeScene *s,Affine2 parent,int depth,SceneColor byBlock,SceneColor inheritedLayer);

static void emit_insert(Dwg_Object *obj,MusaNativeScene *s,Affine2 parent,int depth,SceneColor insertColor,SceneColor insertLayer){
    if(depth>=MUSA_MAX_BLOCK_DEPTH||!obj||!obj->tio.entity||!obj->tio.entity->tio.INSERT)return;
    Dwg_Entity_INSERT *ins=obj->tio.entity->tio.INSERT;
    if(!ins->block_header)return;
    Dwg_Object *block=ins->block_header->obj;
    if(!block)block=dwg_ref_object_silent(obj->parent,ins->block_header);
    if(!block||block->fixedtype!=DWG_TYPE_BLOCK_HEADER||!block->tio.object||!block->tio.object->tio.BLOCK_HEADER)return;
    Dwg_Object_BLOCK_HEADER *hdr=block->tio.object->tio.BLOCK_HEADER;
    BITCODE_3DPOINT ip;
    transform_OCS(&ip,ins->ins_pt,ins->extrusion);
    double rotation=ins->rotation;
    double sx=isfinite(ins->scale.x)&&fabs(ins->scale.x)>1e-12?ins->scale.x:1.0;
    double sy=isfinite(ins->scale.y)&&fabs(ins->scale.y)>1e-12?ins->scale.y:1.0;
    /* Plan blocks with extrusion (0,0,-1) use an OCS whose X axis is reversed.
       Mirror X and reverse rotation so text/symbol blocks are not shown as mirror images. */
    if(fabs(ins->extrusion.x)<1e-9&&fabs(ins->extrusion.y)<1e-9&&ins->extrusion.z<-.999999){
        sx=-sx;rotation=-rotation;
    }
    double co=cos(rotation),si=sin(rotation);
    Affine2 local={co*sx,si*sx,-si*sy,co*sy,ip.x,ip.y};
    local.tx-=local.a*hdr->base_pt.x+local.c*hdr->base_pt.y;
    local.ty-=local.b*hdr->base_pt.x+local.d*hdr->base_pt.y;
    Affine2 combined=multiply2(parent,local);
    Dwg_Object *child=get_first_owned_entity(block);
    while(child&&!s->truncated){emit_object(child,s,combined,depth+1,insertColor,insertLayer);child=get_next_owned_entity(block,child);}
}
static void emit_minsert(Dwg_Object *obj,MusaNativeScene *s,Affine2 parent,int depth,SceneColor insertColor,SceneColor insertLayer){
    if(depth>=MUSA_MAX_BLOCK_DEPTH||!obj||!obj->tio.entity||!obj->tio.entity->tio.MINSERT)return;
    Dwg_Entity_MINSERT *ins=obj->tio.entity->tio.MINSERT;if(!ins->block_header)return;
    int cols=ins->num_cols>0?(int)ins->num_cols:1,rows=ins->num_rows>0?(int)ins->num_rows:1;
    if(cols<1||rows<1||(long long)cols*rows>100000LL)return;
    Dwg_Object *block=ins->block_header->obj;if(!block)block=dwg_ref_object_silent(obj->parent,ins->block_header);
    if(!block||block->fixedtype!=DWG_TYPE_BLOCK_HEADER||!block->tio.object||!block->tio.object->tio.BLOCK_HEADER)return;
    Dwg_Object_BLOCK_HEADER *hdr=block->tio.object->tio.BLOCK_HEADER;
    double rawRotation=isfinite(ins->rotation)?ins->rotation:0.0,spacingCo=cos(rawRotation),spacingSi=sin(rawRotation);
    double colSpacing=isfinite(ins->col_spacing)?ins->col_spacing:0.0,rowSpacing=isfinite(ins->row_spacing)?ins->row_spacing:0.0;
    double rotation=rawRotation,sx=isfinite(ins->scale.x)&&fabs(ins->scale.x)>1e-12?ins->scale.x:1.0;
    double sy=isfinite(ins->scale.y)&&fabs(ins->scale.y)>1e-12?ins->scale.y:1.0;
    if(fabs(ins->extrusion.x)<1e-9&&fabs(ins->extrusion.y)<1e-9&&ins->extrusion.z<-.999999){sx=-sx;rotation=-rotation;}
    double co=cos(rotation),si=sin(rotation);
    for(int row=0;row<rows&&!s->truncated;row++)for(int col=0;col<cols&&!s->truncated;col++){
        double dx=col*colSpacing,dy=row*rowSpacing;
        BITCODE_3DPOINT raw=ins->ins_pt,ip;raw.x+=spacingCo*dx-spacingSi*dy;raw.y+=spacingSi*dx+spacingCo*dy;
        transform_OCS(&ip,raw,ins->extrusion);
        Affine2 local={co*sx,si*sx,-si*sy,co*sy,ip.x,ip.y};
        local.tx-=local.a*hdr->base_pt.x+local.c*hdr->base_pt.y;
        local.ty-=local.b*hdr->base_pt.x+local.d*hdr->base_pt.y;
        Affine2 combined=multiply2(parent,local);
        Dwg_Object *child=get_first_owned_entity(block);
        while(child&&!s->truncated){emit_object(child,s,combined,depth+1,insertColor,insertLayer);child=get_next_owned_entity(block,child);}
    }
}
static void emit_dimension_block(Dwg_Object *obj,MusaNativeScene *s,Affine2 parent,int depth,SceneColor dimColor,SceneColor dimLayer){
    if(depth>=MUSA_MAX_BLOCK_DEPTH||!obj)return;
    dwg_ent_dim *dim=dwg_object_to_DIMENSION(obj);if(!dim||!dim->block)return;
    Dwg_Object *block=dim->block->obj;if(!block)block=dwg_ref_object_silent(obj->parent,dim->block);
    if(!block||block->fixedtype!=DWG_TYPE_BLOCK_HEADER||!block->tio.object||!block->tio.object->tio.BLOCK_HEADER)return;
    Affine2 local=identity2();local.tx=dim->clone_ins_pt.x;local.ty=dim->clone_ins_pt.y;
    Affine2 combined=multiply2(parent,local);
    Dwg_Object *child=get_first_owned_entity(block);
    while(child&&!s->truncated){emit_object(child,s,combined,depth+1,dimColor,dimLayer);child=get_next_owned_entity(block,child);}
}

/*
 * An INSERT/MINSERT/dimension expands to recursively owned entities. In
 * experimental nested paging, count only drawable LEAVES of that expansion.
 * This preserves line/polyline/color/transform as an atomic unit; a primitive
 * inside a leaf is never cut in half. The cursor is stable for the same DWG.
 */
static int musa_is_scene_group(int type){
    switch(type){
        case DWG_TYPE_INSERT: case DWG_TYPE_MINSERT:
        case DWG_TYPE_DIMENSION_ORDINATE: case DWG_TYPE_DIMENSION_LINEAR:
        case DWG_TYPE_DIMENSION_ALIGNED: case DWG_TYPE_DIMENSION_ANG3PT:
        case DWG_TYPE_DIMENSION_ANG2LN: case DWG_TYPE_DIMENSION_RADIUS:
        case DWG_TYPE_DIMENSION_DIAMETER: case DWG_TYPE_ARC_DIMENSION:
        case DWG_TYPE_LARGE_RADIAL_DIMENSION: return 1;
        default: return 0;
    }
}

static void emit_object(Dwg_Object *obj,MusaNativeScene *s,Affine2 parent,int depth,SceneColor byBlock,SceneColor inheritedLayer){
    if(!obj||s->truncated||obj->supertype!=DWG_SUPERTYPE_ENTITY||layer_invisible(obj))return;
    const int nested_leaf=s->nested_paging&&depth>0&&!musa_is_scene_group(obj->fixedtype);
    if(s->collector_only&&nested_leaf){
        /* Capture a complete leaf INSTANCE, not merely the Dwg_Object pointer.
         * Different MINSERT rows or nested parents may reuse the SAME object
         * but have different transforms and by-block/layer color inheritance. */
        if(s->collector_count==65536u){s->truncated=1;return;}
        if(s->collector_count==s->collector_capacity){
            size_t cap=s->collector_capacity?s->collector_capacity*2u:256u;
            if(cap>65536u)cap=65536u;
            MusaSceneLeafContext *next=(MusaSceneLeafContext*)realloc(
                s->collector_entries,cap*sizeof(*next));
            if(!next){s->truncated=1;return;}
            s->collector_entries=next;s->collector_capacity=cap;
        }
        MusaSceneLeafContext *e=&s->collector_entries[s->collector_count++];
        e->object=obj;e->depth=depth;
        e->affine[0]=parent.a;e->affine[1]=parent.b;
        e->affine[2]=parent.c;e->affine[3]=parent.d;
        e->affine[4]=parent.tx;e->affine[5]=parent.ty;
        e->by_block_aci=byBlock.aci;e->by_block_truecolor=byBlock.truecolor;
        e->by_block_rgb=byBlock.rgb;
        e->layer_aci=inheritedLayer.aci;e->layer_truecolor=inheritedLayer.truecolor;
        e->layer_rgb=inheritedLayer.rgb;
        return;
    }
    if(nested_leaf&&s->nested_visited<s->nested_skip){
        ++s->nested_visited;
        return;
    }
    /* Each nested leaf is transactional even if it emits many primitives. */
    const size_t leaf_words=s->count;
    const int leaf_count=s->primitives,leaf_bounds=s->has_bounds;
    const double bx0=s->min_x,by0=s->min_y,bx1=s->max_x,by1=s->max_y;
    SceneColor layerColor=effective_layer_color(obj,inheritedLayer,depth>0);
    SceneColor color=entity_color(obj,byBlock,layerColor);
    switch(obj->fixedtype){
        case DWG_TYPE_INSERT: emit_insert(obj,s,parent,depth,color,layerColor); break;
        case DWG_TYPE_MINSERT: emit_minsert(obj,s,parent,depth,color,layerColor); break;
        case DWG_TYPE_DIMENSION_ORDINATE:
        case DWG_TYPE_DIMENSION_LINEAR:
        case DWG_TYPE_DIMENSION_ALIGNED:
        case DWG_TYPE_DIMENSION_ANG3PT:
        case DWG_TYPE_DIMENSION_ANG2LN:
        case DWG_TYPE_DIMENSION_RADIUS:
        case DWG_TYPE_DIMENSION_DIAMETER:
        case DWG_TYPE_ARC_DIMENSION:
        case DWG_TYPE_LARGE_RADIAL_DIMENSION:
            emit_dimension_block(obj,s,parent,depth,color,layerColor);break;
        case DWG_TYPE_LINE:{
            Dwg_Entity_LINE *e=obj->tio.entity->tio.LINE;BITCODE_3DPOINT a,b;
            if(!e)break;transform_OCS(&a,e->start,e->extrusion);transform_OCS(&b,e->end,e->extrusion);
            emit_line(s,color,parent,a.x,a.y,b.x,b.y);break;
        }
        case DWG_TYPE_CIRCLE:{
            Dwg_Entity_CIRCLE *e=obj->tio.entity->tio.CIRCLE;BITCODE_3DPOINT c,px,py,inx,iny;
            if(!e)break;inx=e->center;iny=e->center;inx.x+=e->radius;iny.y+=e->radius;transform_OCS(&c,e->center,e->extrusion);transform_OCS(&px,inx,e->extrusion);transform_OCS(&py,iny,e->extrusion);
            emit_arc_curve(s,color,parent,c.x,c.y,px.x-c.x,px.y-c.y,py.x-c.x,py.y-c.y,0,2*M_PI,1);break;
        }
        case DWG_TYPE_ARC:{
            Dwg_Entity_ARC *e=obj->tio.entity->tio.ARC;BITCODE_3DPOINT c,px,py,inx,iny;
            if(!e)break;inx=e->center;iny=e->center;inx.x+=e->radius;iny.y+=e->radius;transform_OCS(&c,e->center,e->extrusion);transform_OCS(&px,inx,e->extrusion);transform_OCS(&py,iny,e->extrusion);
            emit_arc_curve(s,color,parent,c.x,c.y,px.x-c.x,px.y-c.y,py.x-c.x,py.y-c.y,e->start_angle,e->end_angle,0);break;
        }
        case DWG_TYPE_POINT:{
            Dwg_Entity_POINT *e=obj->tio.entity->tio.POINT;
            if(!e)break; /* Corrupted AC1032/legacy records may lack payloads. */
            BITCODE_3DPOINT in={e->x,e->y,e->z},p;
            transform_OCS(&p,in,e->extrusion);emit_point(s,color,parent,p.x,p.y);break;
        }
        case DWG_TYPE_RAY:{
            dwg_ent_ray *e=dwg_object_to_RAY(obj);if(!e)break;
            emit_infinite_line(s,color,parent,obj->parent,e->point.x,e->point.y,e->vector.x,e->vector.y,1);break;
        }
        case DWG_TYPE_XLINE:{
            dwg_ent_xline *e=dwg_object_to_XLINE(obj);if(!e)break;
            emit_infinite_line(s,color,parent,obj->parent,e->point.x,e->point.y,e->vector.x,e->vector.y,0);break;
        }
        case DWG_TYPE_MLINE:{
            dwg_ent_mline *e=dwg_object_to_MLINE(obj);if(e)emit_mline(s,color,parent,e);break;
        }
        case DWG_TYPE_TEXT:{
            Dwg_Entity_TEXT *e=obj->tio.entity->tio.TEXT;if(!e)break;
            emit_text_ocs(s,color,parent,e->extrusion,e->ins_pt,e->alignment_pt,e->rotation,e->height,e->width_factor,e->horiz_alignment,e->vert_alignment,e->text_value);
            break;
        }
        case DWG_TYPE_ATTRIB:{
            Dwg_Entity_ATTRIB *e=obj->tio.entity->tio.ATTRIB;if(!e||(e->flags&1))break;
            emit_text_ocs(s,color,parent,e->extrusion,e->ins_pt,e->alignment_pt,e->rotation,e->height,e->width_factor,e->horiz_alignment,e->vert_alignment,e->text_value);
            break;
        }
        case DWG_TYPE_ATTDEF:{
            Dwg_Entity_ATTDEF *e=obj->tio.entity->tio.ATTDEF;if(!e||(e->flags&1))break;
            emit_text_ocs(s,color,parent,e->extrusion,e->ins_pt,e->alignment_pt,e->rotation,e->height,e->width_factor,e->horiz_alignment,e->vert_alignment,e->default_value);
            break;
        }
        case DWG_TYPE_MTEXT:{
            Dwg_Entity_MTEXT *e=obj->tio.entity->tio.MTEXT;if(e)emit_mtext(s,color,parent,e);
            break;
        }
        case DWG_TYPE_ELLIPSE:{
            Dwg_Entity_ELLIPSE *e=obj->tio.entity->tio.ELLIPSE;if(e)emit_ellipse_curve(s,color,parent,e);break;
        }
        case DWG_TYPE_LWPOLYLINE:{
            Dwg_Entity_LWPOLYLINE *e=obj->tio.entity->tio.LWPOLYLINE;if(!e)break;int error=0;BITCODE_RL n=dwg_ent_lwpline_get_numpoints(e,&error);if(error||n<2)break;
            dwg_point_2d *pts=dwg_ent_lwpline_get_points(e,&error);if(error||!pts)break;int closed=(e->flag&512)||(e->flag&1),has_bulge=0;
            if(e->bulges&&e->num_bulges==n)for(BITCODE_RL i=0;i<n;i++)if(isfinite(e->bulges[i])&&fabs(e->bulges[i])>1e-12){has_bulge=1;break;}
            if(has_bulge){
                BITCODE_RL segments=closed?n:n-1;for(BITCODE_RL i=0;i<segments&&!s->truncated;i++){BITCODE_RL j=(i+1)%n;emit_lw_segment(s,color,parent,e,pts[i],pts[j],e->bulges[i]);}
            }else if(begin_poly(s,color,closed,(int)n)){
                for(BITCODE_RL i=0;i<n&&!s->truncated;i++){BITCODE_2DPOINT in={pts[i].x,pts[i].y},p;transform_OCS_2d(&p,in,e->extrusion);emit_poly_point(s,parent,p.x,p.y);}finish_poly(s);
            }
            free(pts);break;
        }
        case DWG_TYPE_POLYLINE_2D:{
            Dwg_Entity_POLYLINE_2D *e=obj->tio.entity->tio.POLYLINE_2D;if(!e)break;int error=0;BITCODE_RL n=dwg_object_polyline_2d_get_numpoints(obj,&error);if(error||n<2)break;
            dwg_point_2d *pts=dwg_object_polyline_2d_get_points(obj,&error);if(error||!pts)break;
            if(begin_poly(s,color,(e->flag&1)!=0,(int)n)){for(BITCODE_RL i=0;i<n&&!s->truncated;i++){BITCODE_2DPOINT in={pts[i].x,pts[i].y},p;transform_OCS_2d(&p,in,e->extrusion);emit_poly_point(s,parent,p.x,p.y);}finish_poly(s);}free(pts);break;
        }
        case DWG_TYPE_POLYLINE_3D:{
            dwg_ent_polyline_3d *e=dwg_object_to_POLYLINE_3D(obj);if(!e)break;int error=0;BITCODE_BL n=dwg_object_polyline_3d_get_numpoints(obj,&error);if(error||n<2)break;
            dwg_point_3d *pts=dwg_object_polyline_3d_get_points(obj,&error);if(error||!pts)break;
            if(begin_poly(s,color,(e->flag&1)!=0,(int)n)){for(BITCODE_BL i=0;i<n&&!s->truncated;i++)emit_poly_point(s,parent,pts[i].x,pts[i].y);finish_poly(s);}free(pts);break;
        }
        case DWG_TYPE_SPLINE:{
            Dwg_Entity_SPLINE *e=obj->tio.entity->tio.SPLINE;
            if(!e)break;
            /* A spline is NOT the polyline joining fit/control points.
             * The old first paint silently displayed false geometry.
             * Render the real weighted B-spline from its knots, or mark
             * the page incomplete if DWG doesn't supply enough data.
             */
            size_t n=(size_t)e->num_ctrl_pts;
            int degree=(int)e->degree;
            /* LibreDWG's scenario 2 may carry fit points and tangents but
             * NO control polygon or knots. A degree-1 segment with exactly
             * two fit points is mathematically just a line, so it can be
             * rendered exactly. Higher-order fit-only data are NOT
             * uniquely reconstructible without a full fitting algorithm;
             * they remain visibly partial, never a fabricated polyline. */
            if(!e->ctrl_pts&&!e->knots&&degree==1&&!e->closed_b&&
               e->num_fit_pts==2&&e->fit_pts){
                emit_line(s,color,parent,e->fit_pts[0].x,e->fit_pts[0].y,
                          e->fit_pts[1].x,e->fit_pts[1].y);
                break;
            }
            /* Scenario-2 cubic SPLINE: LibreDWG stores ONLY fit points
             * without knots/controls. With zero endpoint tangents, the
             * standard CAD solution is a natural cubic interpolation in
             * chord-length parameter space. This evaluates the continuous
             * cubic, not the false piecewise LINE joining fit points.
             */
            if(e->scenario==2&&!e->ctrl_pts&&!e->knots&&degree==3&&
               !e->closed_b&&!e->periodic&&e->fit_pts&&
               e->num_fit_pts>=3&&
               e->num_fit_pts<=MUSA_CAD_FIT_MAX_POINTS&&
               musa_cad_fit_zero_tangent(e->beg_tan_vec.x,
                   e->beg_tan_vec.y,e->beg_tan_vec.z)&&
               musa_cad_fit_zero_tangent(e->end_tan_vec.x,
                   e->end_tan_vec.y,e->end_tan_vec.z)){
                size_t fit_count=(size_t)e->num_fit_pts;
                MusaCadFitPoint *fit=(MusaCadFitPoint*)calloc(
                    fit_count,sizeof(*fit));
                MusaCadNaturalKnot *samples=(MusaCadNaturalKnot*)calloc(
                    fit_count,sizeof(*samples));
                if(!fit||!samples){
                    s->truncated=1;
                    free(fit);free(samples);
                    break;
                }
                for(size_t i=0;i<fit_count;i++){
                    fit[i].x=e->fit_pts[i].x;
                    fit[i].y=e->fit_pts[i].y;
                    fit[i].z=e->fit_pts[i].z;
                }
                if(!musa_cad_fit_natural_build(fit,fit_count,samples)||
                   !begin_poly(s,color,0,
                       1+(int)(fit_count-1)*MUSA_CAD_FIT_SAMPLES_PER_SPAN)){
                    s->truncated=1;
                    free(fit);free(samples);
                    break;
                }
                for(size_t span=0;span+1<fit_count&&!s->truncated;span++){
                    for(int j=span==0?0:1;
                        j<=MUSA_CAD_FIT_SAMPLES_PER_SPAN&&!s->truncated;
                        j++){
                        double u=j==MUSA_CAD_FIT_SAMPLES_PER_SPAN
                            ?samples[span+1].t
                            :samples[span].t+
                              (samples[span+1].t-samples[span].t)*
                              ((double)j/MUSA_CAD_FIT_SAMPLES_PER_SPAN);
                        double x,y;
                        if(!musa_cad_fit_natural_eval(
                                samples,fit_count,u,&x,&y)){
                            s->truncated=1;break;
                        }
                        emit_poly_point(s,parent,x,y);
                    }
                }
                finish_poly(s);
                free(fit);free(samples);
                break;
            }
            if(n<2||n>MUSA_NURBS_MAX_CONTROL||!e->ctrl_pts||!e->knots||
               degree<1||degree>MUSA_NURBS_MAX_DEGREE||
               (size_t)e->num_knots!=n+(size_t)degree+1u){
#ifdef MUSA_HOST
                /* Count-only host diagnosis: no DWG names, text or coords.
                 * Never turn an incomplete native first paint into PASS. */
                fprintf(stderr,"MusaCAD spline rejected: scenario=%d degree=%d"
                        " control=%zu knots=%zu fit=%zu weighted=%d rational=%d"
                        " reason=missing_knots_or_fit_only\n",
                        (int)e->scenario,degree,n,(size_t)e->num_knots,
                        (size_t)e->num_fit_pts,(int)e->weighted,(int)e->rational);
#endif
                s->truncated=1;break;
            }
            MusaNurbsControl *points=(MusaNurbsControl*)calloc(n,sizeof(*points));
            if(!points){s->truncated=1;break;}
            for(size_t i=0;i<n;i++){
                points[i].x=e->ctrl_pts[i].x;
                points[i].y=e->ctrl_pts[i].y;
                /* LibreDWG dwg.spec stores w=0 when 'weighted' is false,
                 * even if the rational flag is set. The implicit DXF/NURBS
                 * weight in that case is exactly one. */
                points[i].w=musa_nurbs_weight(e->ctrl_pts[i].w,e->weighted);
            }
            int sampled=musa_nurbs_valid(points,n,e->knots,
                                         (size_t)e->num_knots,degree)
                ?musa_nurbs_preview_points(e->knots,n,degree):0;
            if(!sampled||!begin_poly(s,color,e->closed_b!=0,sampled)){
#ifdef MUSA_HOST
                fprintf(stderr,"MusaCAD spline rejected: scenario=%d degree=%d"
                        " control=%zu knots=%zu fit=%zu weighted=%d rational=%d"
                        " reason=bad_weights_knots_or_budget\n",
                        (int)e->scenario,degree,n,(size_t)e->num_knots,
                        (size_t)e->num_fit_pts,(int)e->weighted,(int)e->rational);
#endif
                s->truncated=1;free(points);break;
            }
            int first=1;
            for(size_t i=(size_t)degree;i<n&&!s->truncated;i++){
                if(e->knots[i+1]<=e->knots[i])continue;
                int start=first?0:1;
                for(int j=start;j<=MUSA_NURBS_SAMPLES_PER_KNOT_SPAN&&!s->truncated;j++){
                    double u=j==MUSA_NURBS_SAMPLES_PER_KNOT_SPAN
                        ?e->knots[i+1]
                        :e->knots[i]+(e->knots[i+1]-e->knots[i])*
                          ((double)j/MUSA_NURBS_SAMPLES_PER_KNOT_SPAN);
                    double x,y;
                    if(!musa_nurbs_eval(points,n,e->knots,degree,u,&x,&y)){
                        s->truncated=1;break;
                    }
                    emit_poly_point(s,parent,x,y);
                }
                first=0;
            }
            finish_poly(s);
            free(points);
            break;
        }
        case DWG_TYPE_SOLID:{
            Dwg_Entity_SOLID *e=obj->tio.entity->tio.SOLID;if(!e||!begin_poly(s,color,1,4))break;
            BITCODE_2DPOINT in,p;
            in.x=e->corner1.x;in.y=e->corner1.y;transform_OCS_2d(&p,in,e->extrusion);emit_poly_point(s,parent,p.x,p.y);
            in.x=e->corner2.x;in.y=e->corner2.y;transform_OCS_2d(&p,in,e->extrusion);emit_poly_point(s,parent,p.x,p.y);
            in.x=e->corner3.x;in.y=e->corner3.y;transform_OCS_2d(&p,in,e->extrusion);emit_poly_point(s,parent,p.x,p.y);
            in.x=e->corner4.x;in.y=e->corner4.y;transform_OCS_2d(&p,in,e->extrusion);emit_poly_point(s,parent,p.x,p.y);
            finish_poly(s);break;
        }
        case DWG_TYPE_TRACE:{
            Dwg_Entity_TRACE *e=obj->tio.entity->tio.TRACE;if(!e||!begin_poly(s,color,1,4))break;
            BITCODE_2DPOINT in,p;
            in.x=e->corner1.x;in.y=e->corner1.y;transform_OCS_2d(&p,in,e->extrusion);emit_poly_point(s,parent,p.x,p.y);
            in.x=e->corner2.x;in.y=e->corner2.y;transform_OCS_2d(&p,in,e->extrusion);emit_poly_point(s,parent,p.x,p.y);
            in.x=e->corner3.x;in.y=e->corner3.y;transform_OCS_2d(&p,in,e->extrusion);emit_poly_point(s,parent,p.x,p.y);
            in.x=e->corner4.x;in.y=e->corner4.y;transform_OCS_2d(&p,in,e->extrusion);emit_poly_point(s,parent,p.x,p.y);
            finish_poly(s);break;
        }
        case DWG_TYPE_LEADER:{
            Dwg_Entity_LEADER *e=obj->tio.entity->tio.LEADER;
            if(e&&e->points&&e->num_points>=2){
                double arrow=(isfinite(e->dimasz)&&e->dimasz>0)?e->dimasz:1.0;
                emit_leader_points(s,color,parent,e->points,e->num_points,arrow);
            }
            break;
        }
        case DWG_TYPE_MULTILEADER:{
            Dwg_Entity_MULTILEADER *e=obj->tio.entity->tio.MULTILEADER;
            if(!e)break;
            Dwg_MLEADER_AnnotContext *ctx=&e->ctx;
            // Corrupt modern DWGs can declare leaders without backing storage.
            // Fail closed rather than returning a falsely complete first paint.
            if(ctx->num_leaders>0&&!ctx->leaders){s->truncated=1;break;}
            double default_arrow=(isfinite(ctx->arrow_size)&&ctx->arrow_size>0)?ctx->arrow_size:((isfinite(e->arrow_size)&&e->arrow_size>0)?e->arrow_size:1.0);
            for(BITCODE_BL li=0;li<ctx->num_leaders&&!s->truncated;li++){
                Dwg_LEADER_Node *node=&ctx->leaders[li];
                if(node->num_lines>0&&!node->lines){s->truncated=1;break;}
                for(BITCODE_BL lj=0;lj<node->num_lines&&!s->truncated;lj++){
                    Dwg_LEADER_Line *line=&node->lines[lj];
                    double arrow=(isfinite(line->arrow_size)&&line->arrow_size>0)?line->arrow_size:default_arrow;
                    emit_leader_points(s,color,parent,line->points,line->num_points,arrow);
                }
                if(node->has_dogleg&&node->has_lastleaderlinepoint&&isfinite(node->dogleg_length)&&node->dogleg_length>0){
                    double x1=node->lastleaderlinepoint.x,y1=node->lastleaderlinepoint.y;
                    double x2=x1+node->dogleg_vector.x*node->dogleg_length,y2=y1+node->dogleg_vector.y*node->dogleg_length;
                    emit_line(s,color,parent,x1,y1,x2,y2);
                }
            }
            // Missing leader-line arrays invalidate the whole preview leaf.
            if(s->truncated)break;
            if(ctx->has_content_txt&&ctx->content.txt.type==2&&isfinite(ctx->content.txt.location.x)&&isfinite(ctx->content.txt.location.y)){
                for(BITCODE_BL li=0;li<ctx->num_leaders&&!s->truncated;li++){
                    Dwg_LEADER_Node *node=&ctx->leaders[li];
                    if(node->has_lastleaderlinepoint){
                        emit_line(s,color,parent,ctx->content.txt.location.x,ctx->content.txt.location.y,node->lastleaderlinepoint.x,node->lastleaderlinepoint.y);
                        break;
                    }
                }
                const char *label=ctx->content.txt.default_text;
                double height=(isfinite(ctx->text_height)&&ctx->text_height>1e-12)?ctx->text_height:1.0;
                double dx=ctx->content.txt.direction.x,dy=ctx->content.txt.direction.y,len=hypot(dx,dy);
                if(!isfinite(len)||len<1e-12){double rot=isfinite(ctx->content.txt.rotation)?ctx->content.txt.rotation:0.0;dx=cos(rot);dy=sin(rot);len=1.0;}
                dx/=len;dy/=len;
                double ax,ay,px,py,qx,qy;
                map2(parent,ctx->content.txt.location.x,ctx->content.txt.location.y,&ax,&ay);
                map2(parent,ctx->content.txt.location.x+dx*height,ctx->content.txt.location.y+dy*height,&px,&py);
                map2(parent,ctx->content.txt.location.x+dy*height,ctx->content.txt.location.y-dx*height,&qx,&qy);
                SceneColor textColor=color_value(&ctx->content.txt.color,color);
                int hAlign=e->text_alignment==1?1:e->text_alignment==2?2:0;
                emit_text_basis(s,textColor,ax,ay,px-ax,py-ay,qx-ax,qy-ay,hAlign,2,label);
            }
            break;
        }
        case DWG_TYPE_IMAGE:{
            dwg_ent_image *e=dwg_object_to_IMAGE(obj);if(e)emit_image_frame(s,color,parent,e);break;
        }
        case DWG_TYPE_OLE2FRAME:{
            dwg_ent_ole2frame *e=dwg_object_to_OLE2FRAME(obj);if(e)emit_ole_frame(s,color,parent,e);break;
        }
        case DWG_TYPE_WIPEOUT:{
            Dwg_Entity_WIPEOUT *e=obj->tio.entity->tio.WIPEOUT;
            if(e)emit_wipeout(s,parent,e);
            break;
        }
        case DWG_TYPE_HATCH:{
            Dwg_Entity_HATCH *e=obj->tio.entity->tio.HATCH;if(!e||!e->paths||e->num_paths<=0)break;
            for(BITCODE_BL pi=0;pi<e->num_paths&&!s->truncated;pi++){
                Dwg_HATCH_Path *path=&e->paths[pi];BITCODE_BL n=path->num_segs_or_paths;
                if(n>0&&((path->flag&2)?!path->polyline_paths:!path->segs)){
                    // Referenced boundary data is missing: never silently
                    // produce an incomplete HATCH as a complete CAD scene.
                    s->truncated=1;break;
                }
                if((path->flag&2)&&path->polyline_paths&&n>=2){
                    if(path->bulges_present){
                        BITCODE_BL segments=path->closed?n:n-1;
                        for(BITCODE_BL i=0;i<segments&&!s->truncated;i++){
                            BITCODE_BL j=(i+1)%n;
                            emit_hatch_segment(s,color,parent,e->extrusion,path->polyline_paths[i].point,path->polyline_paths[j].point,path->polyline_paths[i].bulge);
                        }
                    }else if(begin_poly(s,color,path->closed!=0,(int)n)){
                        for(BITCODE_BL i=0;i<n&&!s->truncated;i++){
                            BITCODE_2DPOINT in={path->polyline_paths[i].point.x,path->polyline_paths[i].point.y},p;
                            transform_OCS_2d(&p,in,e->extrusion);emit_poly_point(s,parent,p.x,p.y);
                        }
                        finish_poly(s);
                    }
                }else if(path->segs&&n>0){
                    for(BITCODE_BL i=0;i<n&&!s->truncated;i++)emit_hatch_path_segment(s,color,parent,e->extrusion,&path->segs[i]);
                }
            }
            break;
        }
        case DWG_TYPE__3DFACE:{
            Dwg_Entity__3DFACE *e=obj->tio.entity->tio._3DFACE;if(!e||!begin_poly(s,color,1,4))break;
            emit_poly_point(s,parent,e->corner1.x,e->corner1.y);emit_poly_point(s,parent,e->corner2.x,e->corner2.y);emit_poly_point(s,parent,e->corner3.x,e->corner3.y);emit_poly_point(s,parent,e->corner4.x,e->corner4.y);finish_poly(s);break;
        }
        default: break;
    }
    if(nested_leaf){
        if(s->truncated){
            /* Undo an incomplete leaf; parent recursion stops on truncated. */
            s->count=leaf_words;s->primitives=leaf_count;
            s->has_bounds=leaf_bounds;s->min_x=bx0;s->min_y=by0;
            s->max_x=bx1;s->max_y=by1;
        }else{
            ++s->nested_visited;
        }
    }
}

/*
 * Transactional top-level paging. A block INSERT can emit dozens of nested
 * elements, so a page boundary may occur only *between* top-level entities.
 * On overflow restore the previous primitive/bounds counters and retry that
 * same entity on the next page; if it cannot fit even in an empty page,
 * explicitly fail (never drop geometry or silently call the page complete).
 */
int musa_scene_build_page(Dwg_Data *dwg,MusaNativeScene *scene,
                          size_t first_index,size_t *next_index,
                          size_t max_words){
    if(!dwg||!scene||!next_index||max_words<256||
       max_words>MUSA_SCENE_MAX_FLOATS)return -1;
    memset(scene,0,sizeof(*scene));
    *next_index=first_index;
    scene->max_words=max_words;
    if(!reserve_scene(scene,7)){musa_scene_free(scene);return -2;}
    for(int h=0;h<7;h++)pushf(scene,0);
    Affine2 identity=identity2();
    SceneColor root=scene_aci(7);
    Dwg_Object_Ref *model=dwg_model_space_ref(dwg);
    Dwg_Object *owner=model?model->obj:NULL;
    Dwg_Object *obj=owner?get_first_owned_entity(owner):NULL;
    size_t at=0;
    int has_more=0;
    if(owner){
        while(obj){
            if(at>=first_index){
                const size_t before=scene->count;
                const int prior_count=scene->primitives;
                const int prior_bounds=scene->has_bounds;
                const double x0=scene->min_x,y0=scene->min_y;
                const double x1=scene->max_x,y1=scene->max_y;
                emit_object(obj,scene,identity,0,root,root);
                if(scene->truncated){
                    scene->count=before;
                    scene->primitives=prior_count;
                    scene->has_bounds=prior_bounds;
                    scene->min_x=x0;scene->min_y=y0;
                    scene->max_x=x1;scene->max_y=y1;
                    scene->truncated=0;
                    if(before==7){musa_scene_free(scene);return -4;}
                    has_more=1;
                    break;
                }
            }
            ++at;
            obj=get_next_owned_entity(owner,obj);
        }
    }else{
        for(BITCODE_BL i=0;i<dwg->num_objects;i++){
            if(at>=first_index){
                const size_t before=scene->count;
                const int prior_count=scene->primitives;
                const int prior_bounds=scene->has_bounds;
                const double x0=scene->min_x,y0=scene->min_y;
                const double x1=scene->max_x,y1=scene->max_y;
                emit_object(&dwg->object[i],scene,identity,0,root,root);
                if(scene->truncated){
                    scene->count=before;
                    scene->primitives=prior_count;
                    scene->has_bounds=prior_bounds;
                    scene->min_x=x0;scene->min_y=y0;
                    scene->max_x=x1;scene->max_y=y1;
                    scene->truncated=0;
                    if(before==7){musa_scene_free(scene);return -4;}
                    has_more=1;
                    break;
                }
            }
            ++at;
        }
    }
    *next_index=at;
    if(!scene->has_bounds||scene->primitives<=0){
        musa_scene_free(scene);
        return -3;
    }
    /* All pages share stable global extents to prevent fit/zoom shifts. */
    double ex0=dwg_model_x_min(dwg),ey0=dwg_model_y_min(dwg);
    double ex1=dwg_model_x_max(dwg),ey1=dwg_model_y_max(dwg);
    if(isfinite(ex0)&&isfinite(ey0)&&isfinite(ex1)&&isfinite(ey1)&&ex1>ex0&&ey1>ey0&&
       ex1-ex0<1e30&&ey1-ey0<1e30){
        scene->min_x=ex0;scene->min_y=ey0;
        scene->max_x=ex1;scene->max_y=ey1;
    }
    if(scene->max_x-scene->min_x<1e-9){scene->min_x-=.5;scene->max_x+=.5;}
    if(scene->max_y-scene->min_y<1e-9){scene->min_y-=.5;scene->max_y+=.5;}
    scene->values[0]=MUSA_SCENE_VERSION;
    scene->values[1]=(float)scene->primitives;
    scene->values[2]=(float)scene->min_x;scene->values[3]=(float)scene->min_y;
    scene->values[4]=(float)scene->max_x;scene->values[5]=(float)scene->max_y;
    scene->values[6]=0.0f; /* No partial entity is ever exposed on a page. */
    return has_more?1:0;
}

/*
 * Build a complete, bounded leaf-instance index for one expanded top-level
 * block. This invokes the SAME hierarchy, OCS transforms, layer inheritance
 * and source traversal as emit_object, but stops before geometry emission at
 * each leaf. A 65,537th instance fails closed and the caller replays normally.
 */
int musa_scene_collect_root_leaves(Dwg_Object *root,
                                    MusaSceneLeafContext **out,
                                    size_t *out_count){
    if(!out||!out_count)return 0;
    *out=NULL;*out_count=0;
    if(!root||!musa_is_scene_group(root->fixedtype))return 0;
    MusaNativeScene scan={0};
    scan.collector_only=1;
    scan.nested_paging=1;
    Affine2 identity=identity2();
    SceneColor rootColor=scene_aci(7);
    emit_object(root,&scan,identity,0,rootColor,rootColor);
    if(scan.truncated||!scan.collector_count){
        free(scan.collector_entries);
        return 0;
    }
    *out=scan.collector_entries;
    *out_count=scan.collector_count;
    return 1;
}

/*
 * EXPERIMENTAL: block-aware, bounded page with (model entity, nested leaf)
 * resume cursor. This deliberately replays the current block's earlier leaf
 * traversal when resuming; a future stack cursor will remove replay cost.
 * It can still reject a single huge HATCH/POLYLINE leaf with -4, safely.
 */
static int musa_scene_build_page_nested_impl(Dwg_Data *dwg,MusaNativeScene *scene,
                                 size_t first_index,size_t first_leaf,
                                 size_t *next_index,size_t *next_leaf,
                                 size_t max_words,
                                 Dwg_Object *const *indexed,size_t indexed_count,
                                 size_t cached_root_index,Dwg_Object *cached_root,
                                 const MusaSceneLeafContext *leaves,size_t leaf_count){
    if(!dwg||!scene||!next_index||!next_leaf||
       max_words<256||max_words>MUSA_SCENE_MAX_FLOATS)return -1;
    memset(scene,0,sizeof(*scene));
    scene->max_words=max_words;
    *next_index=first_index;*next_leaf=first_leaf;
    if(!reserve_scene(scene,7)){musa_scene_free(scene);return -2;}
    for(int h=0;h<7;h++)pushf(scene,0);
    Affine2 identity=identity2();
    SceneColor root=scene_aci(7);
    const int use_index=indexed!=NULL&&indexed_count>0;
    Dwg_Object_Ref *model=dwg_model_space_ref(dwg);
    Dwg_Object *owner=model?model->obj:NULL;
    Dwg_Object *obj=(!use_index&&owner)?get_first_owned_entity(owner):NULL;
    /* Indexed pages jump directly to first_index, avoiding prefix rewalks. */
    size_t at=use_index?first_index:0;
    int more=0;
    BITCODE_BL limit=owner?0:dwg->num_objects;
    for(BITCODE_BL i=0;use_index?(at<indexed_count):(owner?obj!=NULL:i<limit);){
        Dwg_Object *current=use_index?indexed[at]:(owner?obj:&dwg->object[i]);
        if(!current){musa_scene_free(scene);return -1;}
        if(at>=first_index){
            const size_t before=scene->count;
            const int previous_count=scene->primitives,previous_bounds=scene->has_bounds;
            const double x0=scene->min_x,y0=scene->min_y;
            const double x1=scene->max_x,y1=scene->max_y;
            const size_t skip=(at==first_index)?first_leaf:0;
            scene->nested_paging=1;
            scene->nested_skip=skip;
            scene->nested_visited=0;
            if(use_index&&at==cached_root_index&&current==cached_root&&
               leaves&&leaf_count&&skip<leaf_count){
                /* Replay one FULL leaf context at a time, preserving exact
                 * nested transforms, MINSERT instance placement and colors.
                 * No pointer-only shortcut and no descendant ancestry loss. */
                scene->nested_paging=0;
                scene->nested_visited=skip;
                for(size_t leaf=skip;leaf<leaf_count;leaf++){
                    const MusaSceneLeafContext *ctx=&leaves[leaf];
                    const size_t words_before=scene->count;
                    const int primitives_before=scene->primitives;
                    const int bounds_before=scene->has_bounds;
                    const double minx=scene->min_x,miny=scene->min_y;
                    const double maxx=scene->max_x,maxy=scene->max_y;
                    Affine2 affine={ctx->affine[0],ctx->affine[1],ctx->affine[2],
                                    ctx->affine[3],ctx->affine[4],ctx->affine[5]};
                    SceneColor byblock={ctx->by_block_aci,ctx->by_block_rgb,
                                       ctx->by_block_truecolor};
                    SceneColor inherited={ctx->layer_aci,ctx->layer_rgb,
                                         ctx->layer_truecolor};
                    emit_object(ctx->object,scene,affine,ctx->depth,byblock,inherited);
                    if(scene->truncated){
                        /* A leaf can write multiple primitives. Roll back its
                         * entire record and retry the SAME leaf next page. */
                        scene->count=words_before;
                        scene->primitives=primitives_before;
                        scene->has_bounds=bounds_before;
                        scene->min_x=minx;scene->min_y=miny;
                        scene->max_x=maxx;scene->max_y=maxy;
                        break;
                    }
                    scene->nested_visited=leaf+1;
                }
            }else{
                emit_object(current,scene,identity,0,root,root);
            }
            scene->nested_paging=0;
            /* Do not accept a stale cursor pointing past the last leaf. */
            if(skip>0 && scene->nested_visited<skip){
                musa_scene_free(scene);
                return -5;
            }
            if(scene->truncated){
                const size_t visited=scene->nested_visited;
                const int kept_nested=visited>skip && scene->count>before;
                if(kept_nested){
                    /* Commit complete leaves of the current expanded block. */
                    scene->truncated=0;
                    *next_index=at;
                    *next_leaf=visited;
                    more=1;
                    break;
                }
                /* Nothing from the current model object may be published. */
                scene->count=before;
                scene->primitives=previous_count;
                scene->has_bounds=previous_bounds;
                scene->min_x=x0;scene->min_y=y0;
                scene->max_x=x1;scene->max_y=y1;
                scene->truncated=0;
                if(before==7){musa_scene_free(scene);return -4;}
                *next_index=at;
                *next_leaf=skip;
                more=1;
                break;
            }
        }
        ++at;
        if(!use_index){
            if(owner)obj=get_next_owned_entity(owner,obj);
            else ++i;
        }
        *next_index=at;
        *next_leaf=0;
    }
    if(!scene->has_bounds||scene->primitives<=0){
        musa_scene_free(scene);
        return -3;
    }
    double ex0=dwg_model_x_min(dwg),ey0=dwg_model_y_min(dwg);
    double ex1=dwg_model_x_max(dwg),ey1=dwg_model_y_max(dwg);
    if(isfinite(ex0)&&isfinite(ey0)&&isfinite(ex1)&&isfinite(ey1)&&
       ex1>ex0&&ey1>ey0&&ex1-ex0<1e30&&ey1-ey0<1e30){
        scene->min_x=ex0;scene->min_y=ey0;
        scene->max_x=ex1;scene->max_y=ey1;
    }
    if(scene->max_x-scene->min_x<1e-9){scene->min_x-=.5;scene->max_x+=.5;}
    if(scene->max_y-scene->min_y<1e-9){scene->min_y-=.5;scene->max_y+=.5;}
    scene->values[0]=MUSA_SCENE_VERSION;
    scene->values[1]=(float)scene->primitives;
    scene->values[2]=(float)scene->min_x;scene->values[3]=(float)scene->min_y;
    scene->values[4]=(float)scene->max_x;scene->values[5]=(float)scene->max_y;
    scene->values[6]=0;
    return more?1:0;
}

int musa_scene_build_page_nested(Dwg_Data *dwg,MusaNativeScene *scene,
                                 size_t first_index,size_t first_leaf,
                                 size_t *next_index,size_t *next_leaf,
                                 size_t max_words){
    return musa_scene_build_page_nested_impl(dwg,scene,first_index,first_leaf,
                                              next_index,next_leaf,max_words,NULL,0,
                                              SIZE_MAX,NULL,NULL,0);
}

int musa_scene_build_page_nested_indexed(Dwg_Data *dwg,MusaNativeScene *scene,
                                         size_t first_index,size_t first_leaf,
                                         size_t *next_index,size_t *next_leaf,
                                         size_t max_words,
                                         Dwg_Object *const *model_entities,
                                         size_t model_count){
    if(!model_entities||!model_count||first_index>=model_count)return -1;
    return musa_scene_build_page_nested_impl(dwg,scene,first_index,first_leaf,
                          next_index,next_leaf,max_words,model_entities,model_count,
                          SIZE_MAX,NULL,NULL,0);
}

int musa_scene_build_page_nested_cached(
       Dwg_Data *dwg,MusaNativeScene *scene,
       size_t first_index,size_t first_leaf,
       size_t *next_index,size_t *next_leaf,size_t max_words,
       Dwg_Object *const *model_entities,size_t model_count,
       size_t cached_root_index,Dwg_Object *cached_root,
       const MusaSceneLeafContext *leaves,size_t leaf_count){
    if(!model_entities||!model_count||first_index>=model_count)return -1;
    return musa_scene_build_page_nested_impl(dwg,scene,first_index,first_leaf,
                         next_index,next_leaf,max_words,model_entities,model_count,
                         cached_root_index,cached_root,leaves,leaf_count);
}

int musa_scene_build(Dwg_Data *dwg,MusaNativeScene *scene){
    if(!dwg||!scene)return -1;memset(scene,0,sizeof(*scene));
    if(!reserve_scene(scene,7))return -2;
    for(int i=0;i<7;i++)pushf(scene,0);
    Dwg_Object_Ref *model=dwg_model_space_ref(dwg);
    if(model&&model->obj){
        Dwg_Object *obj=get_first_owned_entity(model->obj);
        Affine2 identity=identity2();
        SceneColor root=scene_aci(7);
        while(obj&&!scene->truncated){emit_object(obj,scene,identity,0,root,root);obj=get_next_owned_entity(model->obj,obj);}
    }else{
        Affine2 identity=identity2();SceneColor root=scene_aci(7);
        for(BITCODE_BL i=0;i<dwg->num_objects&&!scene->truncated;i++)emit_object(&dwg->object[i],scene,identity,0,root,root);
    }
    if(!scene->has_bounds||scene->primitives<=0){musa_scene_free(scene);return -3;}
    /* Prefer DWG model extents so the compact native scene and the later
       complete DXF model use nearly identical normalization. */
    double ex0=dwg_model_x_min(dwg),ey0=dwg_model_y_min(dwg),ex1=dwg_model_x_max(dwg),ey1=dwg_model_y_max(dwg);
    if(isfinite(ex0)&&isfinite(ey0)&&isfinite(ex1)&&isfinite(ey1)&&ex1>ex0&&ey1>ey0&&
       ex1-ex0<1e30&&ey1-ey0<1e30){scene->min_x=ex0;scene->min_y=ey0;scene->max_x=ex1;scene->max_y=ey1;}
    if(scene->max_x-scene->min_x<1e-9){scene->min_x-=.5;scene->max_x+=.5;}
    if(scene->max_y-scene->min_y<1e-9){scene->min_y-=.5;scene->max_y+=.5;}
    scene->values[0]=MUSA_SCENE_VERSION;scene->values[1]=(float)scene->primitives;
    scene->values[2]=(float)scene->min_x;scene->values[3]=(float)scene->min_y;scene->values[4]=(float)scene->max_x;scene->values[5]=(float)scene->max_y;scene->values[6]=scene->truncated?1.0f:0.0f;
    return scene->truncated?1:0;
}
void musa_scene_free(MusaNativeScene *scene){if(!scene)return;free(scene->values);memset(scene,0,sizeof(*scene));}
