#ifndef MC2_TRANSFORM_H
#define MC2_TRANSFORM_H
#include "mc2_geometry.h"
typedef struct { double a,b,c,d,tx,ty; } Mc2Affine2;
Mc2Affine2 mc2_transform_identity(void);
int mc2_transform_multiply(const Mc2Affine2 *parent,const Mc2Affine2 *child,Mc2Affine2 *out);
int mc2_transform_point(const Mc2Affine2 *transform,Mc2Point2 point,Mc2Point2 *out);
int mc2_transform_inverse(const Mc2Affine2 *transform,Mc2Affine2 *out);
#endif
