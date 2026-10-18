/* SPDX-License-Identifier: GPL-3.0-or-later */
/* JNI transfers arrays and result fields; all indexed CAD topology logic
 * belongs to the same C11 source used on Windows and iOS. */
#include <jni.h>
#include <stdint.h>
#include "musacad_core.h"
_Static_assert(sizeof(jint)==sizeof(int),"CAD indexed vertices require 32-bit int");
JNIEXPORT jintArray JNICALL
Java_com_musa_cad_Cad3dMeshTopology_inspectNative(
        JNIEnv *env,jclass ignored,jfloatArray points,jintArray indices){
    (void)ignored;
    if(!points||!indices)return NULL;
    const jsize n=(*env)->GetArrayLength(env,points);
    const jsize f=(*env)->GetArrayLength(env,indices);
    if(n%3||f%3||n/3>1000000)return NULL;
    const size_t point_count=(size_t)(n/3);
    const size_t face_count=(size_t)(f/3);
    MusaCoreMeshTopology r={0};
    int ok=0;
    /* Huge meshes must be labelled 'not checked'. Do not copy their faces
     * or claim they are solids; the native core determines this status. */
    if(face_count>50000u||face_count==0u){
        ok=musa_core_mesh_topology_f32(NULL,point_count,NULL,face_count,&r);
    }else{
        jfloat *xyz=(*env)->GetFloatArrayElements(env,points,NULL);
        if(!xyz)return NULL;
        jint *faces=(*env)->GetIntArrayElements(env,indices,NULL);
        if(!faces){
            (*env)->ReleaseFloatArrayElements(env,points,xyz,JNI_ABORT);
            return NULL;
        }
        ok=musa_core_mesh_topology_f32((const float*)xyz,point_count,
                                       (const int*)faces,face_count,&r);
        (*env)->ReleaseIntArrayElements(env,indices,faces,JNI_ABORT);
        (*env)->ReleaseFloatArrayElements(env,points,xyz,JNI_ABORT);
    }
    if(!ok)return NULL;
    const jint values[8]={(jint)r.checked,(jint)r.closed_oriented_by_index,
        (jint)r.triangles,(jint)r.boundary_edges,(jint)r.orientation_conflicts,
        (jint)r.non_manifold_edges,(jint)r.degenerate_triangles,
        (jint)r.duplicate_faces};
    jintArray out=(*env)->NewIntArray(env,8);
    if(out)(*env)->SetIntArrayRegion(env,out,0,8,values);
    return out;
}
