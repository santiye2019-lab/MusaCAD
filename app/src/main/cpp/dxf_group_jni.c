/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Android JNI string transport only: all DXF group semantics stay in C core. */
#include <jni.h>
#include "musacad_dxf_group.h"
#include "musacad_dxf_pair.h"
JNIEXPORT jint JNICALL
Java_com_musa_cad_DxfGroupCode_parseNative(JNIEnv *env,jclass cls,jstring input){
    (void)cls;
    if(!input)return -1;
    const jsize length=(*env)->GetStringUTFLength(env,input);
    if(length<1||length>1048576)return -1;
    const char *bytes=(*env)->GetStringUTFChars(env,input,NULL);
    if(!bytes)return -1; /* Preserve any pending OOM exception. */
    int result=-1;
    int parsed=0;
    if(musa_core_dxf_group_parse(bytes,(size_t)length,&parsed))result=parsed;
    (*env)->ReleaseStringUTFChars(env,input,bytes);
    return result;
}

/* One common group-code + value-presence + bound check per DXF pair. */
JNIEXPORT jint JNICALL
Java_com_musa_cad_DxfGroupCode_parsePairNative(
        JNIEnv *env,jclass cls,jstring code,jboolean hasValue,
        jint previousPairs,jint maximumPairs){
    (void)cls;
    if(!code||previousPairs<0||maximumPairs<=0)
        return MUSA_DXF_PAIR_INVALID_ARGUMENT;
    jsize length=(*env)->GetStringUTFLength(env,code);
    if(length<=0||length>1048576)return MUSA_DXF_PAIR_INVALID_CODE;
    const char *raw=(*env)->GetStringUTFChars(env,code,NULL);
    if(!raw)return MUSA_DXF_PAIR_INVALID_ARGUMENT;
    int result=-1;
    const int status=musa_core_dxf_pair_check(
        raw,(size_t)length,hasValue==JNI_TRUE,
        (size_t)previousPairs,(size_t)maximumPairs,&result);
    (*env)->ReleaseStringUTFChars(env,code,raw);
    return status==MUSA_DXF_PAIR_OK?result:status;
}
