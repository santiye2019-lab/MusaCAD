/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Android transport ONLY. Acceptance semantics live in core/native. */
#include <jni.h>
#include "musacad_3d_admission.h"

JNIEXPORT jint JNICALL
Java_com_musa_cad_Cad3dOnlyOpenGuard_admissionNative(
        JNIEnv *env,jclass clazz,jint xyz,jint triangles,jint edges,
        jboolean proxy) {
    (void)env;
    (void)clazz;
    if(xyz<0 || triangles<0 || edges<0)
        return (jint)MUSA_CORE_3D_MALFORMED_ARRAYS;
    return (jint)musa_core_3d_only_admission(
            (size_t)xyz,(size_t)triangles,(size_t)edges,
            proxy==JNI_TRUE);
}
