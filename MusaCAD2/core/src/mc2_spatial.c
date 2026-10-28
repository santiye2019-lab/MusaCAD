#include "mc2_spatial.h"
#include <math.h>
static int valid_bounds(const Mc2Bounds2 *b) {
    return b && isfinite(b->min_x) && isfinite(b->min_y) &&
           isfinite(b->max_x) && isfinite(b->max_y) &&
           b->min_x<=b->max_x && b->min_y<=b->max_y;
}
int mc2_bounds_intersects(const Mc2Bounds2 *a,const Mc2Bounds2 *b) {
    if (!valid_bounds(a) || !valid_bounds(b)) return 0;
    return a->min_x<=b->max_x && b->min_x<=a->max_x &&
           a->min_y<=b->max_y && b->min_y<=a->max_y;
}
int mc2_transform_bounds(const Mc2Bounds2 *source,const Mc2Affine2 *t,Mc2Bounds2 *out) {
    if (!valid_bounds(source) || !out) return 0;
    Mc2Point2 corners[4]={
        {source->min_x,source->min_y},{source->min_x,source->max_y},
        {source->max_x,source->min_y},{source->max_x,source->max_y}
    };
    Mc2Point2 p;
    if (!mc2_transform_point(t,corners[0],&p)) return 0;
    Mc2Bounds2 result={p.x,p.y,p.x,p.y};
    for (int i=1;i<4;++i) {
        if (!mc2_transform_point(t,corners[i],&p)) return 0;
        if (p.x<result.min_x) result.min_x=p.x;
        if (p.x>result.max_x) result.max_x=p.x;
        if (p.y<result.min_y) result.min_y=p.y;
        if (p.y>result.max_y) result.max_y=p.y;
    }
    *out=result;
    return 1;
}
int mc2_entity_world_bounds(const Mc2Entity2 *entity,const Mc2Affine2 *world,Mc2Bounds2 *out) {
    Mc2Bounds2 local;
    if (!entity || !world || !out || !mc2_entity_bounds(entity,&local)) return 0;
    if (entity->kind == MC2_ENTITY_LINE) {
        Mc2Point2 a,b;
        if (!mc2_transform_point(world,entity->geometry.line.start,&a) ||
            !mc2_transform_point(world,entity->geometry.line.end,&b)) return 0;
        *out=(Mc2Bounds2){
            a.x<b.x?a.x:b.x, a.y<b.y?a.y:b.y,
            a.x>b.x?a.x:b.x, a.y>b.y?a.y:b.y
        };
        return 1;
    }
    if (entity->kind == MC2_ENTITY_CIRCLE) {
        /* Exact AABB for an affine-transformed circle: each axis is a sinusoid. */
        Mc2Point2 center=entity->geometry.circle.center;
        double radius=entity->geometry.circle.radius;
        Mc2Point2 transformed;
        if (!mc2_transform_point(world,center,&transformed)) return 0;
        double extent_x=radius*hypot(world->a,world->c);
        double extent_y=radius*hypot(world->b,world->d);
        Mc2Bounds2 bounds={transformed.x-extent_x,transformed.y-extent_y,
                          transformed.x+extent_x,transformed.y+extent_y};
        if (!valid_bounds(&bounds)) return 0;
        *out=bounds;
        return 1;
    }
    /* Partial arcs: conservative bounds from the transformed local AABB. */
    return mc2_transform_bounds(&local,world,out);
}
