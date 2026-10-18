/* SPDX-License-Identifier: GPL-3.0-or-later */
/* JNI data transport only: topology and CAD volume math are all portable C. */
#include <jni.h>
#include <stdint.h>
#include "musacad_core.h"
_Static_assert(sizeof(jint)==sizeof(int),"MusaCAD JNI uses 32-bit face indices");
static jdoubleArray verified_mass(JNIEnv *env,jfloatArray points,jintArray faces){
    if(!points||!faces)return NULL;
    const jsize p=(*env)->GetArrayLength(env,points);
    const jsize f=(*env)->GetArrayLength(env,faces);
    if(p<=0||f<=0||p%3||f%3||p/3>1000000||f/3>50000)return NULL;
    jfloat *xyz=(*env)->GetFloatArrayElements(env,points,NULL);
    if(!xyz)return NULL;
    jint *ids=(*env)->GetIntArrayElements(env,faces,NULL);
    if(!ids){
        (*env)->ReleaseFloatArrayElements(env,points,xyz,JNI_ABORT);
        return NULL;
    }
    MusaCoreMeshMass out={0};
    int ok=musa_core_mesh_mass_verified_f32((const float*)xyz,(size_t)(p/3),
                                             (const int*)ids,(size_t)(f/3),&out);
    (*env)->ReleaseIntArrayElements(env,faces,ids,JNI_ABORT);
    (*env)->ReleaseFloatArrayElements(env,points,xyz,JNI_ABORT);
    if(!ok)return NULL;
    jdouble v[4]={out.signed_volume,out.centroid_x,out.centroid_y,out.centroid_z};
    jdoubleArray result=(*env)->NewDoubleArray(env,4);
    if(result)(*env)->SetDoubleArrayRegion(env,result,0,4,v);
    return result;
}
JNIEXPORT jdoubleArray JNICALL
Java_com_musa_cad_Cad3dSignedVolume_measureNative(
        JNIEnv *env,jclass cls,jfloatArray xyz,jintArray faces){
    (void)cls;return verified_mass(env,xyz,faces);
}
JNIEXPORT jdoubleArray JNICALL
Java_com_musa_cad_Cad3dNativeMeshMass_measureNative(
        JNIEnv *env,jclass cls,jfloatArray xyz,jintArray faces){
    (void)cls;return verified_mass(env,xyz,faces);
}
