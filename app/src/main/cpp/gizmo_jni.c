/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Android transport only: 3D gizmo computation is solely in shared C11. */
#include <jni.h>
#include <math.h>
#include "musacad_gizmo.h"

JNIEXPORT jdoubleArray JNICALL
Java_com_musa_cad_CadGizmoMath_axisDirectionNative(
        JNIEnv *env,jclass type,jdouble dx,jdouble dy,jdouble dz,
        jdouble yaw,jdouble pitch){
    (void)type;
    const double axis[3]={dx,dy,dz};
    double xy[2];
    if(!musa_core_gizmo_axis_direction(axis,yaw,pitch,xy)){
        jclass err=(*env)->FindClass(env,"java/lang/IllegalArgumentException");
        if(err)(*env)->ThrowNew(env,err,"Invalid native 3D gizmo axis");
        return NULL;
    }
    jdoubleArray out=(*env)->NewDoubleArray(env,2);
    if(out)(*env)->SetDoubleArrayRegion(env,out,0,2,xy);
    return out;
}
JNIEXPORT jint JNICALL
Java_com_musa_cad_CadGizmoMath_pickNative(
        JNIEnv *env,jclass type,jdouble x,jdouble y,jobjectArray dirs,
        jdouble handle,jdouble tolerance){
    (void)type;
    if(!dirs||(*env)->GetArrayLength(env,dirs)!=3)return -1;
    double flat[6]={NAN,NAN,NAN,NAN,NAN,NAN};
    for(jsize i=0;i<3;i++){
        jobject element=(*env)->GetObjectArrayElement(env,dirs,i);
        if((*env)->ExceptionCheck(env))return -1;
        if(!element)continue;
        jdoubleArray row=(jdoubleArray)element;
        if((*env)->GetArrayLength(env,row)==2){
            (*env)->GetDoubleArrayRegion(env,row,0,2,flat+2*i);
            if((*env)->ExceptionCheck(env)){
                (*env)->DeleteLocalRef(env,element);return -1;
            }
        }
        (*env)->DeleteLocalRef(env,element);
    }
    return (jint)musa_core_gizmo_pick(x,y,flat,handle,tolerance);
}
JNIEXPORT jdouble JNICALL
Java_com_musa_cad_CadGizmoMath_drawingDistanceNative(
        JNIEnv *env,jclass type,jdouble dx,jdouble dy,jdouble ax,
        jdouble ay,jdouble pixels_per_unit){
    (void)env;(void)type;
    return (jdouble)musa_core_gizmo_drag_distance(dx,dy,ax,ay,pixels_per_unit);
}
