/* SPDX-License-Identifier: GPL-3.0-or-later */
/* JNI is ONLY a byte transport. All DWG signature decisions live in C core. */
#include <jni.h>
#include "musacad_dwg_signature.h"

JNIEXPORT jint JNICALL
Java_com_musa_cad_CadDwgFormat_identifyNative(
        JNIEnv *env,jclass clazz,jbyteArray header){
    (void)clazz;
    if(header==NULL)return (jint)MUSA_DWG_SHORT_HEADER;
    jsize length=(*env)->GetArrayLength(env,header);
    if(length<6)return (jint)MUSA_DWG_SHORT_HEADER;
    unsigned char six[6]={0};
    (*env)->GetByteArrayRegion(env,header,0,6,(jbyte*)six);
    if((*env)->ExceptionCheck(env))return (jint)MUSA_DWG_SHORT_HEADER;
    return (jint)musa_core_dwg_identify_signature(six,6);
}
