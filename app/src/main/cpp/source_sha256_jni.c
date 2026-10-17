/* SPDX-License-Identifier: GPL-3.0-or-later */
/* JNI is bounded byte transport. Hashing happens only in portable C core. */
#include <jni.h>
#include <stdint.h>
#include <stddef.h>
#include "musacad_source_sha256.h"

static MusaSourceSha256 *ctx(jlong handle){
    return (MusaSourceSha256 *)(intptr_t)handle;
}
JNIEXPORT jlong JNICALL
Java_com_musa_cad_CadSaveSourceIntegrity_createDigestNative(JNIEnv *env,jclass type){
    (void)env;(void)type;
    return (jlong)(intptr_t)musa_core_source_sha256_create();
}
JNIEXPORT jboolean JNICALL
Java_com_musa_cad_CadSaveSourceIntegrity_updateDigestNative(
    JNIEnv *env,jclass type,jlong handle,jbyteArray input,jint count){
    (void)type;
    if(!ctx(handle)||!input||count<0||count>65536 ||
       (*env)->GetArrayLength(env,input)<count)return JNI_FALSE;
    unsigned char bytes[65536];
    (*env)->GetByteArrayRegion(env,input,0,count,(jbyte *)bytes);
    if((*env)->ExceptionCheck(env))return JNI_FALSE;
    const int ok=musa_core_source_sha256_update(ctx(handle),bytes,(size_t)count);
    /* Do not retain source bytes in a stack buffer after return. */
    volatile unsigned char *wipe=(volatile unsigned char *)bytes;
    for(size_t i=0;i<(size_t)count;i++)wipe[i]=0;
    return ok?JNI_TRUE:JNI_FALSE;
}
JNIEXPORT jbyteArray JNICALL
Java_com_musa_cad_CadSaveSourceIntegrity_finishDigestNative(
    JNIEnv *env,jclass type,jlong handle){
    (void)type;
    unsigned char digest[32];
    if(!musa_core_source_sha256_finish(ctx(handle),digest))return NULL;
    jbyteArray result=(*env)->NewByteArray(env,32);
    if(result)(*env)->SetByteArrayRegion(env,result,0,32,(jbyte *)digest);
    volatile unsigned char *wipe=(volatile unsigned char *)digest;
    for(size_t i=0;i<32;i++)wipe[i]=0;
    return result;
}
JNIEXPORT jboolean JNICALL
Java_com_musa_cad_CadSaveSourceIntegrity_equalDigestNative(
    JNIEnv *env,jclass type,jbyteArray left,jbyteArray right){
    (void)type;
    if(!left||!right||(*env)->GetArrayLength(env,left)!=32||
       (*env)->GetArrayLength(env,right)!=32)return JNI_FALSE;
    unsigned char a[32],b[32];
    (*env)->GetByteArrayRegion(env,left,0,32,(jbyte *)a);
    if((*env)->ExceptionCheck(env))return JNI_FALSE;
    (*env)->GetByteArrayRegion(env,right,0,32,(jbyte *)b);
    if((*env)->ExceptionCheck(env))return JNI_FALSE;
    return musa_core_source_sha256_equal(a,b,32)?JNI_TRUE:JNI_FALSE;
}
JNIEXPORT void JNICALL
Java_com_musa_cad_CadSaveSourceIntegrity_destroyDigestNative(
    JNIEnv *env,jclass type,jlong handle){
    (void)env;(void)type;
    musa_core_source_sha256_destroy(ctx(handle));
}
