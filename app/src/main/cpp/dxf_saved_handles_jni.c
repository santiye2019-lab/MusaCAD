/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Android/JVM adapter; all DXF hex and uniqueness semantics live in C11. */
#include <jni.h>
#include <stdint.h>
#include <stddef.h>
#include "musacad_dxf_handle_set.h"

JNIEXPORT jlong JNICALL
Java_com_musa_cad_DxfSavedHandleSet_openNative(JNIEnv *env,jclass cls,jint maximum){
    (void)env;(void)cls;
    if(maximum<1||maximum>1500000)return (jlong)0;
    MusaDxfHandleSet *set=musa_core_dxf_handle_set_create((size_t)maximum);
    return (jlong)(intptr_t)set;
}
JNIEXPORT jint JNICALL
Java_com_musa_cad_DxfSavedHandleSet_insertNative(JNIEnv *env,jclass cls,
                                                    jlong context,jstring value){
    (void)cls;
    if(context==0||!value)return MUSA_DXF_HANDLE_INVALID;
    jsize length=(*env)->GetStringUTFLength(env,value);
    if(length<1||length>1048576)return MUSA_DXF_HANDLE_INVALID;
    const char *raw=(*env)->GetStringUTFChars(env,value,NULL);
    if(!raw)return MUSA_DXF_HANDLE_NO_MEMORY;
    int status=musa_core_dxf_handle_set_insert(
        (MusaDxfHandleSet *)(intptr_t)context,raw,(size_t)length);
    (*env)->ReleaseStringUTFChars(env,value,raw);
    return status;
}
JNIEXPORT void JNICALL
Java_com_musa_cad_DxfSavedHandleSet_closeNative(JNIEnv *env,jclass cls,
                                                  jlong context){
    (void)env;(void)cls;
    if(context!=0)
        musa_core_dxf_handle_set_destroy((MusaDxfHandleSet *)(intptr_t)context);
}
