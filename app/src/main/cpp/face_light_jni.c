/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Android JNI copies only the three selected source vertices; native core
 * owns all face geometry arithmetic and shape/finite validation. */
#include <jni.h>
#include "musacad_face_light.h"
JNIEXPORT jdouble JNICALL
Java_com_musa_cad_Cad3dFaceLighting_brightnessNative(
        JNIEnv *env,jclass ignored,jfloatArray points,jint a,jint b,jint c){
    (void)ignored;
    if(!points||a<0||b<0||c<0)return -1.0;
    const jsize length=(*env)->GetArrayLength(env,points);
    if(length%3||a>=length/3||b>=length/3||c>=length/3)return -1.0;
    jfloat triangle[9];
    const jint ids[3]={a,b,c};
    for(int i=0;i<3;i++){
        (*env)->GetFloatArrayRegion(env,points,ids[i]*3,3,triangle+3*i);
        if((*env)->ExceptionCheck(env))return -1.0;
    }
    return musa_core_face_lambert_f32(triangle,3,0,1,2);
}
