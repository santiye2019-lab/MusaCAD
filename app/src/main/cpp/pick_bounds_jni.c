/* SPDX-License-Identifier: GPL-3.0-or-later */
/* JNI transports a source's primitive coordinates to the shared C math. */
#include <jni.h>
#include "musacad_pick_bounds.h"
JNIEXPORT jboolean JNICALL
Java_com_musa_cad_CadPickBounds_mightHitNative(JNIEnv *env,jclass unused,
       jfloatArray source,jint kind,jfloat x,jfloat y,jfloat tolerance){
    (void)unused;
    if(!source)return JNI_TRUE;
    const jsize n=(*env)->GetArrayLength(env,source);
    if(n<2)return JNI_TRUE;
    if(kind==3||kind<0||kind>3)return JNI_TRUE;
    jfloat *xy=(*env)->GetFloatArrayElements(env,source,NULL);
    if(!xy)return JNI_TRUE;
    int hit=musa_core_pick_bounds_f32((const float*)xy,(size_t)n,
                                      (int)kind,x,y,tolerance);
    (*env)->ReleaseFloatArrayElements(env,source,xy,JNI_ABORT);
    return hit?JNI_TRUE:JNI_FALSE;
}
