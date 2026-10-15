/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Android-only binding: NO CAD algorithms duplicated outside musacad_core. */
#include <jni.h>
#include <stdint.h>
#include "musacad_core.h"

_Static_assert(sizeof(jint)==sizeof(int), "MusaCAD JNI requires 32-bit int indices");

JNIEXPORT jdoubleArray JNICALL
Java_com_musa_cad_Cad3dNativeMeshMass_measureNative(
        JNIEnv *env,jclass clazz,jfloatArray points,jintArray faces) {
    (void)clazz;
    if(!points||!faces)return NULL;
    const jsize p=(*env)->GetArrayLength(env,points);
    const jsize f=(*env)->GetArrayLength(env,faces);
    /* Limit data size before JNI may copy untrusted mobile CAD buffers. */
    if(p<=0||f<=0||p%3||f%3||p/3>1000000||f/3>50000)return NULL;
    jfloat *xyz=(*env)->GetFloatArrayElements(env,points,NULL);
    if(!xyz)return NULL;
    jint *indices=(*env)->GetIntArrayElements(env,faces,NULL);
    if(!indices){
        (*env)->ReleaseFloatArrayElements(env,points,xyz,JNI_ABORT);
        return NULL;
    }
    MusaCoreMeshMass result={0};
    /* Java's independently checked Cad3dMeshTopology is mandatory. This
     * method is private, reachable only after that Java preflight.
     */
    int ok=musa_core_mesh_mass_f32((const float*)xyz,(size_t)(p/3),
                                  (const int*)indices,(size_t)(f/3),1,&result);
    (*env)->ReleaseIntArrayElements(env,faces,indices,JNI_ABORT);
    (*env)->ReleaseFloatArrayElements(env,points,xyz,JNI_ABORT);
    if(!ok)return NULL;
    const jdouble values[4]={
        result.signed_volume,result.centroid_x,result.centroid_y,result.centroid_z
    };
    jdoubleArray out=(*env)->NewDoubleArray(env,4);
    if(out)(*env)->SetDoubleArrayRegion(env,out,0,4,values);
    return out;
}
