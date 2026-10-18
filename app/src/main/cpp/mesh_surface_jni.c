/* SPDX-License-Identifier: GPL-3.0-or-later */
/*
 * Android-only JNI transport. All CAD surface mathematics lives in the
 * portable musacad_core C library, not in this binding.
 */
#include <jni.h>
#include <stdint.h>
#include "musacad_core.h"

_Static_assert(sizeof(jint)==sizeof(int), "JNI and C mesh indices must be 32-bit");

/* Portable geometry measurement shared by both the legacy diagnostic
 * bridge and the authoritative Android application adapter. */
static jdoubleArray surface_measure(JNIEnv *env,jfloatArray points,
                                    jintArray triangles,int allow_empty){
    if(!points||!triangles)return NULL;
    jsize n=(*env)->GetArrayLength(env,points);
    jsize f=(*env)->GetArrayLength(env,triangles);
    if(n%3||f%3||n/3>1000000||f/3>4000000||
       (!allow_empty&&(n<=0||f<=0)) )return NULL;
    jfloat *xyz=NULL;
    jint *indices=NULL;
    if(n){
        xyz=(*env)->GetFloatArrayElements(env,points,NULL);
        if(!xyz)return NULL;
    }
    if(f){
        indices=(*env)->GetIntArrayElements(env,triangles,NULL);
        if(!indices){
            if(xyz)(*env)->ReleaseFloatArrayElements(env,points,xyz,JNI_ABORT);
            return NULL;
        }
    }
    MusaCoreMeshSurface result={0};
    int ok=musa_core_mesh_surface_f32((const float*)xyz,(size_t)(n/3),
                                     (const int*)indices,(size_t)(f/3),
                                     &result);
    if(indices)(*env)->ReleaseIntArrayElements(env,triangles,indices,JNI_ABORT);
    if(xyz)(*env)->ReleaseFloatArrayElements(env,points,xyz,JNI_ABORT);
    if(!ok)return NULL;
    jdouble values[6]={result.area,result.centroid_x,result.centroid_y,
                       result.centroid_z,(double)result.triangle_count,
                       (double)result.degenerate_count};
    jdoubleArray out=(*env)->NewDoubleArray(env,6);
    if(out)(*env)->SetDoubleArrayRegion(env,out,0,6,values);
    return out;
}
JNIEXPORT jdoubleArray JNICALL
Java_com_musa_cad_Cad3dSurfaceMetrics_measureNative(
        JNIEnv *env,jclass ignored,jfloatArray points,jintArray triangles){
    (void)ignored;
    return surface_measure(env,points,triangles,1);
}
JNIEXPORT jdoubleArray JNICALL
Java_com_musa_cad_Cad3dNativeMeshSurface_measureNative(
        JNIEnv *env,jclass ignored,jfloatArray points,jintArray triangles){
    (void)ignored;
    return surface_measure(env,points,triangles,0);
}
