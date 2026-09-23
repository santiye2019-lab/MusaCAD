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

#define MUSA_SCENE_VERSION 4.0f
/* v4 keeps the compact v3 ACI encoding and adds an optional exact 24-bit
   TrueColor word only for entities/layers that actually use group-420 color.
   The 9M cap remains bounded for large-phone memory safety. */
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
static SceneColor entity_color(Dwg_Object *obj){
    if(!obj||!obj->tio.entity)return scene_aci(7);
    Dwg_Color *entity=&obj->tio.entity->color;
    if((entity->flag&0x80)!=0)return scene_true((unsigned int)entity->rgb);
    int aci=(int)entity->index;
    if(aci>=1&&aci<=255)return scene_aci(aci);
    Dwg_Object_Ref *lr=obj->tio.entity->layer;
    if(lr&&lr->obj&&lr->obj->fixedtype==DWG_TYPE_LAYER&&lr->obj->tio.object&&lr->obj->tio.object->tio.LAYER){
        Dwg_Color *layer=&lr->obj->tio.object->tio.LAYER->color;
        if((layer->flag&0x80)!=0)return scene_true((unsigned int)layer->rgb);
        int layer_aci=(int)layer->index;if(layer_aci>=1&&layer_aci<=255)return scene_aci(layer_aci);
    }
    return scene_aci(7);
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
    SceneColor color=entity_color(obj);
    switch(obj->fixedtype){
        case DWG_TYPE_INSERT: emit_insert(obj,s,parent,depth); break;
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
        while(obj&&!scene->truncated){emit_object(obj,scene,identity,0);obj=get_next_owned_entity(model->obj,obj);}
    }else{
        Affine2 identity=identity2();
        for(BITCODE_BL i=0;i<dwg->num_objects&&!scene->truncated;i++)emit_object(&dwg->object[i],scene,identity,0);
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
