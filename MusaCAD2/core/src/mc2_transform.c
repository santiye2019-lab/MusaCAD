#include "mc2_transform.h"
#include <math.h>
static int valid(const Mc2Affine2 *t) {
    return t && isfinite(t->a) && isfinite(t->b) && isfinite(t->c) &&
           isfinite(t->d) && isfinite(t->tx) && isfinite(t->ty);
}
Mc2Affine2 mc2_transform_identity(void) { return (Mc2Affine2){1,0,0,1,0,0}; }
int mc2_transform_point(const Mc2Affine2 *t,Mc2Point2 p,Mc2Point2 *out) {
    if (!out || !valid(t) || !isfinite(p.x) || !isfinite(p.y)) return 0;
    Mc2Point2 q = {t->a*p.x+t->c*p.y+t->tx,t->b*p.x+t->d*p.y+t->ty};
    if (!isfinite(q.x) || !isfinite(q.y)) return 0;
    *out=q; return 1;
}
int mc2_transform_multiply(const Mc2Affine2 *p,const Mc2Affine2 *c,Mc2Affine2 *out) {
    if (!out || !valid(p) || !valid(c)) return 0;
    Mc2Affine2 q={
        p->a*c->a+p->c*c->b,
        p->b*c->a+p->d*c->b,
        p->a*c->c+p->c*c->d,
        p->b*c->c+p->d*c->d,
        p->a*c->tx+p->c*c->ty+p->tx,
        p->b*c->tx+p->d*c->ty+p->ty
    };
    if (!valid(&q)) return 0;
    *out=q; return 1;
}
int mc2_transform_inverse(const Mc2Affine2 *t,Mc2Affine2 *out) {
    if (!out || !valid(t)) return 0;
    double det=t->a*t->d-t->b*t->c;
    if (!isfinite(det) || det==0.0) return 0;
    Mc2Affine2 q={t->d/det,-t->b/det,-t->c/det,t->a/det,
        (t->c*t->ty-t->d*t->tx)/det,
        (t->b*t->tx-t->a*t->ty)/det};
    if (!valid(&q)) return 0;
    *out=q; return 1;
}
