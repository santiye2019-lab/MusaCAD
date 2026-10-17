/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Android JNI string transport only: all DXF group semantics stay in C core. */
#include <jni.h>
#include "musacad_dxf_group.h"
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
