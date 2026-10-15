/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "musacad_camera_ray.h"
#include <math.h>

/* Invert the arbitrary 4x4 world-to-clip matrix via pivoted elimination.
 * Scaled partial pivoting handles different CAD axis/drawing units.
 * Matrix in/out are COLUMN-major, intermediate elimination is row-major.
 * Keep outputs pristine unless every component of the inverse is finite.
 */
static int inverse4(const double m[16],double result[16]){
    double a[4][8]={{0}};
    double rowmax[4]={0};
    for(int row=0;row<4;row++){
        for(int col=0;col<4;col++){
            double value=m[col*4+row];
            if(!isfinite(value))return 0;
            a[row][col]=value;
            if(fabs(value)>rowmax[row])rowmax[row]=fabs(value);
        }
        if(rowmax[row]==0.)return 0;
        a[row][4+row]=1.;
    }
    for(int k=0;k<4;k++){
        int pivot=k;
        double score=-1.;
        for(int row=k;row<4;row++){
            double ratio=fabs(a[row][k])/rowmax[row];
            if(ratio>score){score=ratio;pivot=row;}
        }
        if(!isfinite(score)||score<=1e-14)return 0;
        if(pivot!=k){
            for(int col=0;col<8;col++){
                double t=a[k][col];a[k][col]=a[pivot][col];a[pivot][col]=t;
            }
            double s=rowmax[k];rowmax[k]=rowmax[pivot];rowmax[pivot]=s;
        }
        double divisor=a[k][k];
        if(divisor==0.||!isfinite(divisor))return 0;
        for(int col=0;col<8;col++){
            a[k][col]/=divisor;
            if(!isfinite(a[k][col]))return 0;
        }
        for(int row=0;row<4;row++)if(row!=k){
            double factor=a[row][k];
            for(int col=0;col<8;col++){
                a[row][col]-=factor*a[k][col];
                if(!isfinite(a[row][col]))return 0;
            }
        }
    }
    for(int row=0;row<4;row++)
        for(int col=0;col<4;col++)
            result[col*4+row]=a[row][col+4];
    return 1;
}
static int unproject(const double inverse[16],double ndc_x,
                     double ndc_y,double ndc_z,double wcs[3]){
    const double p[4]={ndc_x,ndc_y,ndc_z,1.};
    double world[4]={0};
    for(int row=0;row<4;row++){
        for(int col=0;col<4;col++)
            world[row]+=inverse[col*4+row]*p[col];
        if(!isfinite(world[row]))return 0;
    }
    if(world[3]==0.)return 0;
    for(int row=0;row<3;row++){
        wcs[row]=world[row]/world[3];
        if(!isfinite(wcs[row]))return 0;
    }
    return 1;
}
int musa_core_camera_screen_ray(
    const double view_projection_colmajor[16],
    double width,double height,double screen_x,double screen_y,
    int depth_zero_to_one,double origin[3],double direction[3]){
    if(!view_projection_colmajor||!origin||!direction||
       !isfinite(width)||!isfinite(height)||
       !isfinite(screen_x)||!isfinite(screen_y)||
       width<=0.||height<=0.||
       screen_x<0.||screen_x>width||
       screen_y<0.||screen_y>height||
       (depth_zero_to_one!=0&&depth_zero_to_one!=1))return 0;

    const double x=2.*(screen_x/width)-1.;
    const double y=1.-2.*(screen_y/height);
    if(!isfinite(x)||!isfinite(y))return 0;

    double inv[16],start[3],end[3],dir[3];
    if(!inverse4(view_projection_colmajor,inv))return 0;
    if(!unproject(inv,x,y,depth_zero_to_one?0.:-1.,start)||
       !unproject(inv,x,y,1.,end))return 0;
    for(int k=0;k<3;k++)dir[k]=end[k]-start[k];
    double length=hypot(hypot(dir[0],dir[1]),dir[2]);
    if(!isfinite(length)||length==0.)return 0;
    for(int k=0;k<3;k++){
        dir[k]/=length;
        if(!isfinite(dir[k]))return 0;
    }
    for(int k=0;k<3;k++){
        origin[k]=start[k];
        direction[k]=dir[k];
    }
    return 1;
}
