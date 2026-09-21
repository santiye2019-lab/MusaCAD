/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "config.h"
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

#define MUSA_SCENE_VERSION 1.0f
#define MUSA_SCENE_MAX_FLOATS (16u*1024u*1024u)
#define MUSA_CURVE_SEGMENTS 48
#define MUSA_MAX_BLOCK_DEPTH 24

typedef struct { double a,b,c,d,tx,ty; } Affine2;

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
static int entity_aci(Dwg_Object *obj){
    if(!obj||!obj->tio.entity)return 7;
    int aci=(int)obj->tio.entity->color.index;
    if(aci==0||aci==256||aci<0||aci>255){
        Dwg_Object_Ref *lr=obj->tio.entity->layer;
        if(lr&&lr->obj&&lr->obj->fixedtype==DWG_TYPE_LAYER&&lr->obj->tio.object&&lr->obj->tio.object->tio.LAYER){
            int layer_aci=(int)lr->obj->tio.object->tio.LAYER->color.index;
            if(layer_aci>=1&&layer_aci<=255)aci=layer_aci;
        }
    }
    return (aci>=1&&aci<=255)?aci:7;
}
static int emit_line(MusaNativeScene *s,int aci,Affine2 m,double x1,double y1,double x2,double y2){
    double ax,ay,bx,by;map2(m,x1,y1,&ax,&ay);map2(m,x2,y2,&bx,&by);
    if(!finite2(ax,ay)||!finite2(bx,by)||!reserve_scene(s,6))return 0;
    pushf(s,1);pushf(s,(float)aci);pushf(s,(float)ax);pushf(s,(float)ay);pushf(s,(float)bx);pushf(s,(float)by);
    add_bounds(s,ax,ay);add_bounds(s,bx,by);s->primitives++;return 1;
}
static int emit_point(MusaNativeScene *s,int aci,Affine2 m,double x,double y){
    double ax,ay;map2(m,x,y,&ax,&ay);if(!finite2(ax,ay)||!reserve_scene(s,4))return 0;
    pushf(s,3);pushf(s,(float)aci);pushf(s,(float)ax);pushf(s,(float)ay);add_bounds(s,ax,ay);s->primitives++;return 1;
}
static int begin_poly(MusaNativeScene *s,int aci,int closed,int n){
    if(n<2||!reserve_scene(s,4u+(size_t)n*2u))return 0;
    pushf(s,2);pushf(s,(float)aci);pushf(s,closed?1.0f:0.0f);pushf(s,(float)n);return 1;
}
static void emit_poly_point(MusaNativeScene *s,Affine2 m,double x,double y){
    double ax,ay;map2(m,x,y,&ax,&ay);
    if(!finite2(ax,ay)){ax=0;ay=0;}
    pushf(s,(float)ax);pushf(s,(float)ay);add_bounds(s,ax,ay);
}
static void finish_poly(MusaNativeScene *s){s->primitives++;}

static void emit_arc_poly(MusaNativeScene *s,int aci,Affine2 m,double cx,double cy,double r,double start,double end,int full){
    if(!isfinite(cx)||!isfinite(cy)||!isfinite(r)||r<=0)return;
    double sweep=full?2.0*M_PI:end-start;while(sweep<=0)sweep+=2.0*M_PI;if(sweep>2.0*M_PI)sweep=2.0*M_PI;
    int n=full?MUSA_CURVE_SEGMENTS+1:(int)ceil(MUSA_CURVE_SEGMENTS*sweep/(2.0*M_PI))+1;
    if(n<8)n=8;if(n>97)n=97;
    if(!begin_poly(s,aci,full,n))return;
    for(int i=0;i<n;i++){double t=start+sweep*(double)i/(double)(n-1);emit_poly_point(s,m,cx+r*cos(t),cy+r*sin(t));}
    finish_poly(s);
}
static void emit_ellipse_poly(MusaNativeScene *s,int aci,Affine2 m,Dwg_Entity_ELLIPSE *e){
    if(!e||!isfinite(e->center.x)||!isfinite(e->center.y)||!isfinite(e->sm_axis.x)||!isfinite(e->sm_axis.y)||!isfinite(e->axis_ratio))return;
    double start=e->start_angle,end=e->end_angle,sweep=end-start;while(sweep<=0)sweep+=2.0*M_PI;if(sweep>2.0*M_PI)sweep=2.0*M_PI;
    int full=fabs(sweep-2.0*M_PI)<1e-4;int n=full?MUSA_CURVE_SEGMENTS+1:(int)ceil(MUSA_CURVE_SEGMENTS*sweep/(2.0*M_PI))+1;if(n<8)n=8;
    if(!begin_poly(s,aci,full,n))return;
    double mx=e->sm_axis.x,my=e->sm_axis.y,ratio=fabs(e->axis_ratio);
    for(int i=0;i<n;i++){double t=start+sweep*(double)i/(double)(n-1);double x=e->center.x+mx*cos(t)-my*ratio*sin(t);double y=e->center.y+my*cos(t)+mx*ratio*sin(t);emit_poly_point(s,m,x,y);}
    finish_poly(s);
}

static void emit_object(Dwg_Object *obj,MusaNativeScene *s,Affine2 parent,int depth);

static void emit_insert(Dwg_Object *obj,MusaNativeScene *s,Affine2 parent,int depth){
    if(depth>=MUSA_MAX_BLOCK_DEPTH||!obj||!obj->tio.entity||!obj->tio.entity->tio.INSERT)return;
    Dwg_Entity_INSERT *ins=obj->tio.entity->tio.INSERT;
    if(!ins->block_header)return;
    Dwg_Object *block=ins->block_header->obj;
    if(!block)block=dwg_ref_object_silent(obj->parent,ins->block_header);
    if(!block||block->fixedtype!=DWG_TYPE_BLOCK_HEADER||!block->tio.object||!block->tio.object->tio.BLOCK_HEADER)return;
    Dwg_Object_BLOCK_HEADER *hdr=block->tio.object->tio.BLOCK_HEADER;
    BITCODE_3DPOINT ip;
    transform_OCS(&ip,ins->ins_pt,ins->extrusion);
    double co=cos(ins->rotation),si=sin(ins->rotation);
    double sx=isfinite(ins->scale.x)&&fabs(ins->scale.x)>1e-12?ins->scale.x:1.0;
    double sy=isfinite(ins->scale.y)&&fabs(ins->scale.y)>1e-12?ins->scale.y:1.0;
    Affine2 local={co*sx,si*sx,-si*sy,co*sy,ip.x,ip.y};
    local.tx-=local.a*hdr->base_pt.x+local.c*hdr->base_pt.y;
    local.ty-=local.b*hdr->base_pt.x+local.d*hdr->base_pt.y;
    Affine2 combined=multiply2(parent,local);
    Dwg_Object *child=get_first_owned_entity(block);
    while(child&&!s->truncated){emit_object(child,s,combined,depth+1);child=get_next_owned_entity(block,child);}
}

static void emit_object(Dwg_Object *obj,MusaNativeScene *s,Affine2 parent,int depth){
    if(!obj||s->truncated||obj->supertype!=DWG_SUPERTYPE_ENTITY||layer_invisible(obj))return;
    int aci=entity_aci(obj);
    switch(obj->fixedtype){
        case DWG_TYPE_INSERT: emit_insert(obj,s,parent,depth); break;
        case DWG_TYPE_LINE:{
            Dwg_Entity_LINE *e=obj->tio.entity->tio.LINE;BITCODE_3DPOINT a,b;
            if(!e)break;transform_OCS(&a,e->start,e->extrusion);transform_OCS(&b,e->end,e->extrusion);
            emit_line(s,aci,parent,a.x,a.y,b.x,b.y);break;
        }
        case DWG_TYPE_CIRCLE:{
            Dwg_Entity_CIRCLE *e=obj->tio.entity->tio.CIRCLE;BITCODE_3DPOINT c;
            if(!e)break;transform_OCS(&c,e->center,e->extrusion);emit_arc_poly(s,aci,parent,c.x,c.y,e->radius,0,2*M_PI,1);break;
        }
        case DWG_TYPE_ARC:{
            Dwg_Entity_ARC *e=obj->tio.entity->tio.ARC;BITCODE_3DPOINT c;
            if(!e)break;transform_OCS(&c,e->center,e->extrusion);emit_arc_poly(s,aci,parent,c.x,c.y,e->radius,e->start_angle,e->end_angle,0);break;
        }
        case DWG_TYPE_POINT:{
            Dwg_Entity_POINT *e=obj->tio.entity->tio.POINT;BITCODE_3DPOINT in={e->x,e->y,e->z},p;
            if(!e)break;transform_OCS(&p,in,e->extrusion);emit_point(s,aci,parent,p.x,p.y);break;
        }
        case DWG_TYPE_ELLIPSE:{
            Dwg_Entity_ELLIPSE *e=obj->tio.entity->tio.ELLIPSE;if(e)emit_ellipse_poly(s,aci,parent,e);break;
        }
        case DWG_TYPE_LWPOLYLINE:{
            Dwg_Entity_LWPOLYLINE *e=obj->tio.entity->tio.LWPOLYLINE;if(!e)break;int error=0;BITCODE_RL n=dwg_ent_lwpline_get_numpoints(e,&error);if(error||n<2)break;
            dwg_point_2d *pts=dwg_ent_lwpline_get_points(e,&error);if(error||!pts)break;int closed=(e->flag&512)||(e->flag&1);
            if(begin_poly(s,aci,closed,(int)n)){for(BITCODE_RL i=0;i<n;i++){BITCODE_2DPOINT in={pts[i].x,pts[i].y},p;transform_OCS_2d(&p,in,e->extrusion);emit_poly_point(s,parent,p.x,p.y);}finish_poly(s);}free(pts);break;
        }
        case DWG_TYPE_POLYLINE_2D:{
            Dwg_Entity_POLYLINE_2D *e=obj->tio.entity->tio.POLYLINE_2D;if(!e)break;int error=0;BITCODE_RL n=dwg_object_polyline_2d_get_numpoints(obj,&error);if(error||n<2)break;
            dwg_point_2d *pts=dwg_object_polyline_2d_get_points(obj,&error);if(error||!pts)break;
            if(begin_poly(s,aci,(e->flag&1)!=0,(int)n)){for(BITCODE_RL i=0;i<n;i++){BITCODE_2DPOINT in={pts[i].x,pts[i].y},p;transform_OCS_2d(&p,in,e->extrusion);emit_poly_point(s,parent,p.x,p.y);}finish_poly(s);}free(pts);break;
        }
        case DWG_TYPE_SPLINE:{
            Dwg_Entity_SPLINE *e=obj->tio.entity->tio.SPLINE;if(!e)break;
            if(e->num_fit_pts>=2&&e->fit_pts){int n=(int)e->num_fit_pts;if(begin_poly(s,aci,e->closed_b,n)){for(int i=0;i<n;i++)emit_poly_point(s,parent,e->fit_pts[i].x,e->fit_pts[i].y);finish_poly(s);}}
            else if(e->num_ctrl_pts>=2&&e->ctrl_pts){int n=(int)e->num_ctrl_pts;if(begin_poly(s,aci,e->closed_b,n)){for(int i=0;i<n;i++)emit_poly_point(s,parent,e->ctrl_pts[i].x,e->ctrl_pts[i].y);finish_poly(s);}}
            break;
        }
        case DWG_TYPE_SOLID:{
            Dwg_Entity_SOLID *e=obj->tio.entity->tio.SOLID;if(!e||!begin_poly(s,aci,1,4))break;
            BITCODE_2DPOINT in,p;in=e->corner1;transform_OCS_2d(&p,in,e->extrusion);emit_poly_point(s,parent,p.x,p.y);in=e->corner2;transform_OCS_2d(&p,in,e->extrusion);emit_poly_point(s,parent,p.x,p.y);in=e->corner3;transform_OCS_2d(&p,in,e->extrusion);emit_poly_point(s,parent,p.x,p.y);in=e->corner4;transform_OCS_2d(&p,in,e->extrusion);emit_poly_point(s,parent,p.x,p.y);finish_poly(s);break;
        }
        case DWG_TYPE__3DFACE:{
            Dwg_Entity__3DFACE *e=obj->tio.entity->tio._3DFACE;if(!e||!begin_poly(s,aci,1,4))break;
            emit_poly_point(s,parent,e->corner1.x,e->corner1.y);emit_poly_point(s,parent,e->corner2.x,e->corner2.y);emit_poly_point(s,parent,e->corner3.x,e->corner3.y);emit_poly_point(s,parent,e->corner4.x,e->corner4.y);finish_poly(s);break;
        }
        default: break;
    }
}

int musa_scene_build(Dwg_Data *dwg,MusaNativeScene *scene){
    if(!dwg||!scene)return -1;memset(scene,0,sizeof(*scene));
    if(!reserve_scene(scene,6))return -2;
    for(int i=0;i<6;i++)pushf(scene,0);
    Dwg_Object_Ref *model=dwg_model_space_ref(dwg);
    if(model&&model->obj){
        Dwg_Object *obj=get_first_owned_entity(model->obj);
        Affine2 identity=identity2();
        while(obj&&!scene->truncated){emit_object(obj,scene,identity,0);obj=get_next_owned_entity(model->obj,obj);}
    }else{
        Affine2 identity=identity2();
        for(BITCODE_BL i=0;i<dwg->num_objects&&!scene->truncated;i++)emit_object(&dwg->object[i],scene,identity,0);
    }
    if(!scene->has_bounds||scene->primitives<=0){musa_scene_free(scene);return -3;}
    if(scene->max_x-scene->min_x<1e-9){scene->min_x-=.5;scene->max_x+=.5;}
    if(scene->max_y-scene->min_y<1e-9){scene->min_y-=.5;scene->max_y+=.5;}
    scene->values[0]=MUSA_SCENE_VERSION;scene->values[1]=(float)scene->primitives;
    scene->values[2]=(float)scene->min_x;scene->values[3]=(float)scene->min_y;scene->values[4]=(float)scene->max_x;scene->values[5]=(float)scene->max_y;
    return scene->truncated?1:0;
}
void musa_scene_free(MusaNativeScene *scene){if(!scene)return;free(scene->values);memset(scene,0,sizeof(*scene));}
