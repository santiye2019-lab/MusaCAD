/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "musacad_screen_frustum.h"
#include <math.h>

/* Frustum clipping is expressed as homogeneous plane inequalities.
 * x_ndc = clip_x/clip_w, y_ndc = clip_y/clip_w. The six world planes are
 * linear combinations of rows of world_to_clip, no camera inversion.
 */
int musa_core_screen_frustum_build(
    const double m[16],double width,double height,
    double left,double top,double right,double bottom,
    int z01,MusaCoreScreenFrustum *out) {
    if(!m||!out||!isfinite(width)||!isfinite(height)||
       width<=0.||height<=0.||
       !isfinite(left)||!isfinite(top)||!isfinite(right)||
       !isfinite(bottom)||left<0.||top<0.||right>width||
       bottom>height||right<=left||bottom<=top||
       (z01!=0&&z01!=1))return 0;
    double ndcx0=2.*(left/width)-1.;
    double ndcx1=2.*(right/width)-1.;
    double ndcy0=1.-2.*(bottom/height);
    double ndcy1=1.-2.*(top/height);
    if(!isfinite(ndcx0)||!isfinite(ndcx1)||
       !isfinite(ndcy0)||!isfinite(ndcy1))return 0;
    double row[4][4];
    for(int r=0;r<4;r++)for(int c=0;c<4;c++){
        row[r][c]=m[c*4+r];
        if(!isfinite(row[r][c]))return 0;
    }
    MusaCoreScreenFrustum result={0};
    for(int k=0;k<4;k++){
        result.planes[0][k]=row[0][k]-ndcx0*row[3][k];
        result.planes[1][k]=ndcx1*row[3][k]-row[0][k];
        result.planes[2][k]=row[1][k]-ndcy0*row[3][k];
        result.planes[3][k]=ndcy1*row[3][k]-row[1][k];
        result.planes[4][k]=row[2][k]+(z01?0.:row[3][k]);
        result.planes[5][k]=row[3][k]-row[2][k];
    }
    for(int p=0;p<6;p++){
        double norm=0.;
        for(int k=0;k<4;k++){
            if(!isfinite(result.planes[p][k]))return 0;
            if(k<3)norm=hypot(norm,result.planes[p][k]);
        }
        if(norm==0.||!isfinite(norm))return 0;
        /* Plane normalization improves WCS classification consistency;
         * preserve the D coefficient as world-space signed distance.
         */
        for(int k=0;k<4;k++){
            result.planes[p][k]/=norm;
            if(!isfinite(result.planes[p][k]))return 0;
        }
    }
    *out=result;
    return 1;
}
int musa_core_screen_frustum_classify_aabb(
    const MusaCoreScreenFrustum *f,
    const double mn[3],const double mx[3]){
    if(!f||!mn||!mx)return -1;
    for(int a=0;a<3;a++){
        if(!isfinite(mn[a])||!isfinite(mx[a])||mn[a]>mx[a])return -1;
    }
    int completely_inside=1;
    for(int p=0;p<6;p++){
        const double *plane=f->planes[p];
        double high=plane[3],low=plane[3];
        double magnitude=fabs(plane[3])+1.;
        for(int a=0;a<3;a++){
            if(!isfinite(plane[a]))return -1;
            double extreme_high=plane[a]>=0.?mx[a]:mn[a];
            double extreme_low=plane[a]>=0.?mn[a]:mx[a];
            double ha=plane[a]*extreme_high;
            double lo=plane[a]*extreme_low;
            if(!isfinite(ha)||!isfinite(lo))return -1;
            high+=ha;low+=lo;
            magnitude+=fmax(fabs(ha),fabs(lo));
        }
        if(!isfinite(plane[3])||!isfinite(high)||
           !isfinite(low)||!isfinite(magnitude))return -1;
        /* Conservative exclusion margin prevents false negatives from
         * cancellation near high-coordinate CAD WCS boundaries.
         * Full containment uses a stricter margin, not a relaxed one.
         */
        double slack=1e-13*magnitude;
        if(!isfinite(slack))return -1;
        if(high< -slack)return 0;
        if(low<slack)completely_inside=0;
    }
    return completely_inside?2:1;
}
