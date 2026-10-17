/* SPDX-License-Identifier: GPL-3.0-or-later */
/* JNI transports group-5 values, does NOT reimplement CAD identity semantics. */
#include <jni.h>
#include "musacad_dxf_handle.h"
#include <stdint.h>
#include <stddef.h>

static int parse(JNIEnv *env,jstring input,uint64_t *out){
    if(!input||!out)return 0;
    jsize bytes=(*env)->GetStringUTFLength(env,input);
    if(bytes<1||bytes>16)return 0;
    const char *raw=(*env)->GetStringUTFChars(env,input,NULL);
    if(!raw)return 0;
    int ok=musa_core_dxf_handle_parse(raw,(size_t)bytes,out);
    (*env)->ReleaseStringUTFChars(env,input,raw);
    return ok;
}
JNIEXPORT jstring JNICALL
Java_com_musa_cad_DxfSourceHandlePlan_canonicalNative(JNIEnv *env,jclass type,jstring input){
    (void)type;
    uint64_t value=0;
    char text[17]={0};
    if(!parse(env,input,&value)||!musa_core_dxf_handle_format(value,text))return NULL;
    return (*env)->NewStringUTF(env,text);
}
JNIEXPORT jstring JNICALL
Java_com_musa_cad_DxfSourceHandlePlan_maximumNative(JNIEnv *env,jclass type,
                                                     jstring previous,jstring candidate){
    (void)type;
    uint64_t a=0,b=0;
    char output[17]={0};
    if(!parse(env,previous,&a)||!parse(env,candidate,&b))return NULL;
    if(!musa_core_dxf_handle_format(a>b?a:b,output))return NULL;
    return (*env)->NewStringUTF(env,output);
}
JNIEXPORT jobjectArray JNICALL
Java_com_musa_cad_DxfSourceHandlePlan_allocateNative(JNIEnv *env,jclass type,
                                                      jstring maximum,jint count){
    (void)type;
    uint64_t highest=0,first=0,seed=0;
    if(!parse(env,maximum,&highest)||count<=0||
       !musa_core_dxf_handle_reserve(highest,(size_t)count,&first,&seed))
        return NULL;
    jclass stringClass=(*env)->FindClass(env,"java/lang/String");
    if(!stringClass)return NULL;
    jobjectArray values=(*env)->NewObjectArray(env,count+1,stringClass,NULL);
    (*env)->DeleteLocalRef(env,stringClass);
    if(!values)return NULL;
    char text[17]={0};
    for(jint i=0;i<=count;i++){
        uint64_t v=i==count?seed:first+(uint64_t)i;
        if(!musa_core_dxf_handle_format(v,text))return NULL;
        jstring item=(*env)->NewStringUTF(env,text);
        if(!item)return NULL;
        (*env)->SetObjectArrayElement(env,values,i,item);
        (*env)->DeleteLocalRef(env,item);
        if((*env)->ExceptionCheck(env))return NULL;
    }
    return values;
}
