/* SPDX-License-Identifier: GPL-3.0-or-later */
/*
 * Android-only JNI transport. All CAD surface mathematics lives in the
 * portable musacad_core C library, not in this binding.
 */
#include <jni.h>
#include <stdint.h>
#include "musacad_core.h"

_Static_assert(sizeof(jint)==sizeof(int), "JNI and C mesh indices must be 32-bit");

JNIEXPORT jdoubleArray JNICALL
Java_com_musa_cad_Cad3dNativeMeshSurface_measureNative(
        JNIEnv *env,jclass clazz,jfloatArray points,jintArray triangles) {
    (void)clazz;
    if(!points||!triangles)return NULL;
    jsize n=(*env)->GetArrayLength(env,points);
    jsize f=(*env)->GetArrayLength(env,triangles);
    if(n<=0||f<=0||n%3||f%3||n/3>1000000||f/3>50000)
        return NULL;
    jfloat *xyz=(*env)->GetFloatArrayElements(env,points,NULL);
    if(!xyz)return NULL;
    jint *indices=(*env)->GetIntArrayElements(env,triangles,NULL);
    if(!indices){
        (*env)->ReleaseFloatArrayElements(env,points,xyz,JNI_ABORT);
        return NULL;
    }
    MusaCoreMeshSurface result={0};
    int ok=musa_core_mesh_surface_f32((const float*)xyz,(size_t)(n/3),
                                      (const int*)indices,(size_t)(f/3),
                                      &result);
    (*env)->ReleaseIntArrayElements(env,triangles,indices,JNI_ABORT);
    (*env)->ReleaseFloatArrayElements(env,points,xyz,JNI_ABORT);
    if(!ok)return NULL;
    jdouble data[6]={result.area,result.centroid_x,result.centroid_y,
                     result.centroid_z,(double)result.triangle_count,
                     (double)result.degenerate_count};
    jdoubleArray output=(*env)->NewDoubleArray(env,6);
    if(output)(*env)->SetDoubleArrayRegion(env,output,0,6,data);
    return output;
}
