/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "config.h"
#include "native_color.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>
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
    if(extra>MUSA_SCENE_MAX_FLOATS||s->count>MUSA_SCENE_MAX_FLOATS-extra){s->truncated=1;return 0;}
    size_t need=s->count+extra;
    if(need<=s->capacity)return 1;
    size_t cap=s->capacity?s->capacity:4096;
    while(cap<need&&cap<MUSA_SCENE_MAX_FLOATS)cap*=2;
    if(cap>MUSA_SCENE_MAX_FLOATS)cap=MUSA_SCENE_MAX_FLOATS;
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
    if(!finite2(ax,ay)||!finite2(bx,by)||!reserve_scene(s,4u+color_words(color)))return 0;
    push_code(s,1,color);pushf(s,(float)ax);pushf(s,(float)ay);pushf(s,(float)bx);pushf(s,(float)by);
    add_bounds(s,ax,ay);add_bounds(s,bx,by);s->primitives++;return 1;
}
static int emit_point(MusaNativeScene *s,SceneColor color,Affine2 m,double x,double y){
    double ax,ay;map2(m,x,y,&ax,&ay);if(!finite2(ax,ay)||!reserve_scene(s,2u+color_words(color)))return 0;
    push_code(s,3,color);pushf(s,(float)ax);pushf(s,(float)ay);add_bounds(s,ax,ay);s->primitives++;return 1;
}
static int begin_poly(MusaNativeScene *s,SceneColor color,int closed,int n){
    if(n<2||!reserve_scene(s,1u+color_words(color)+(size_t)n*2u))return 0;
    push_code(s,2,color);pushf(s,(float)(closed?-n:n));return 1;
}
static void emit_poly_point(MusaNativeScene *s,Affine2 m,double x,double y){
    double ax,ay;map2(m,x,y,&ax,&ay);
    if(!finite2(ax,ay)){ax=0;ay=0;}
    pushf(s,(float)ax);pushf(s,(float)ay);add_bounds(s,ax,ay);
}
static void finish_poly(MusaNativeScene *s){s->primitives++;}

static int begin_fill_poly(MusaNativeScene *s,int n){
    SceneColor color=scene_aci(7);
    if(n<3||!reserve_scene(s,1u+color_words(color)+(size_t)n*2u))return 0;
    push_code(s,5,color);pushf(s,(float)(-n));return 1;
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
    if(!finite2(wcx,wcy)||!finite2(ux,uy)||!finite2(vx,vy)||!isfinite(start)||!isfinite(sweep)||fabs(sweep)<1e-12||!reserve_scene(s,8u+color_words(color)))return 0;
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

static void emit_object(Dwg_Object *obj,MusaNativeScene *s,Affine2 parent,int depth,SceneColor byBlock,SceneColor inheritedLayer){
    if(!obj||s->truncated||obj->supertype!=DWG_SUPERTYPE_ENTITY||layer_invisible(obj))return;
    SceneColor layerColor=effective_layer_color(obj,inheritedLayer,depth>0);
    SceneColor color=entity_color(obj,byBlock,layerColor);
    switch(obj->fixedtype){
        case DWG_TYPE_INSERT: emit_insert(obj,s,parent,depth,color,layerColor); break;
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
            Dwg_Entity_POINT *e=obj->tio.entity->tio.POINT;BITCODE_3DPOINT in={e->x,e->y,e->z},p;
            if(!e)break;transform_OCS(&p,in,e->extrusion);emit_point(s,color,parent,p.x,p.y);break;
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
                for(BITCODE_RL i=0;i<n;i++){BITCODE_2DPOINT in={pts[i].x,pts[i].y},p;transform_OCS_2d(&p,in,e->extrusion);emit_poly_point(s,parent,p.x,p.y);}finish_poly(s);
            }
            free(pts);break;
        }
        case DWG_TYPE_POLYLINE_2D:{
            Dwg_Entity_POLYLINE_2D *e=obj->tio.entity->tio.POLYLINE_2D;if(!e)break;int error=0;BITCODE_RL n=dwg_object_polyline_2d_get_numpoints(obj,&error);if(error||n<2)break;
            dwg_point_2d *pts=dwg_object_polyline_2d_get_points(obj,&error);if(error||!pts)break;
            if(begin_poly(s,color,(e->flag&1)!=0,(int)n)){for(BITCODE_RL i=0;i<n;i++){BITCODE_2DPOINT in={pts[i].x,pts[i].y},p;transform_OCS_2d(&p,in,e->extrusion);emit_poly_point(s,parent,p.x,p.y);}finish_poly(s);}free(pts);break;
        }
        case DWG_TYPE_SPLINE:{
            Dwg_Entity_SPLINE *e=obj->tio.entity->tio.SPLINE;if(!e)break;
            if(e->num_fit_pts>=2&&e->fit_pts){int n=(int)e->num_fit_pts;if(begin_poly(s,color,e->closed_b,n)){for(int i=0;i<n;i++)emit_poly_point(s,parent,e->fit_pts[i].x,e->fit_pts[i].y);finish_poly(s);}}
            else if(e->num_ctrl_pts>=2&&e->ctrl_pts){int n=(int)e->num_ctrl_pts;if(begin_poly(s,color,e->closed_b,n)){for(int i=0;i<n;i++)emit_poly_point(s,parent,e->ctrl_pts[i].x,e->ctrl_pts[i].y);finish_poly(s);}}
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
            double default_arrow=(isfinite(ctx->arrow_size)&&ctx->arrow_size>0)?ctx->arrow_size:((isfinite(e->arrow_size)&&e->arrow_size>0)?e->arrow_size:1.0);
            for(BITCODE_BL li=0;li<ctx->num_leaders&&!s->truncated;li++){
                Dwg_LEADER_Node *node=&ctx->leaders[li];
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
        case DWG_TYPE_WIPEOUT:{
            Dwg_Entity_WIPEOUT *e=obj->tio.entity->tio.WIPEOUT;
            if(e)emit_wipeout(s,parent,e);
            break;
        }
        case DWG_TYPE_HATCH:{
            Dwg_Entity_HATCH *e=obj->tio.entity->tio.HATCH;if(!e||!e->paths||e->num_paths<=0)break;
            for(BITCODE_BL pi=0;pi<e->num_paths&&!s->truncated;pi++){
                Dwg_HATCH_Path *path=&e->paths[pi];BITCODE_BL n=path->num_segs_or_paths;
                if((path->flag&2)&&path->polyline_paths&&n>=2){
                    if(path->bulges_present){
                        BITCODE_BL segments=path->closed?n:n-1;
                        for(BITCODE_BL i=0;i<segments&&!s->truncated;i++){
                            BITCODE_BL j=(i+1)%n;
                            emit_hatch_segment(s,color,parent,e->extrusion,path->polyline_paths[i].point,path->polyline_paths[j].point,path->polyline_paths[i].bulge);
                        }
                    }else if(begin_poly(s,color,path->closed!=0,(int)n)){
                        for(BITCODE_BL i=0;i<n;i++){
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
