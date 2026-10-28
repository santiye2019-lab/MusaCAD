#include "mc2_transform.h"
#include <assert.h>
#include <math.h>
int main(void) {
    Mc2Affine2 identity=mc2_transform_identity(), inverse, composed;
    Mc2Affine2 t={0,2,-3,0,10,20};
    Mc2Point2 p={4,5},q,r;
    assert(mc2_transform_point(&t,p,&q));
    assert(q.x==-5 && q.y==28);
    assert(mc2_transform_inverse(&t,&inverse));
    assert(mc2_transform_point(&inverse,q,&r));
    assert(fabs(r.x-p.x)<1e-12 && fabs(r.y-p.y)<1e-12);
    assert(mc2_transform_multiply(&t,&inverse,&composed));
    assert(fabs(composed.a-identity.a)<1e-12);
    assert(fabs(composed.d-identity.d)<1e-12);
    Mc2Affine2 singular={1,2,2,4,0,0};
    assert(!mc2_transform_inverse(&singular,&inverse));
    t.tx=NAN;
    assert(!mc2_transform_point(&t,p,&q));
    return 0;
}
