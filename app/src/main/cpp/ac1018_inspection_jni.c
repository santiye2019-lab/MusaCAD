/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Android-only byte transport. All CAD decode/admission/count semantics
 * live in the independently tested shared native C11 core. */
#include <jni.h>
#include <stdint.h>
#include <stdlib.h>
#include "musacad_dwg_ac1018_inspection.h"
#define MUSA_AC1018_INSPECT_MAX_BYTES (32u*1024u*1024u)
static void reject(JNIEnv *env,const char *why){
    if((*env)->ExceptionCheck(env))return;
    jclass cls=(*env)->FindClass(env,"java/io/IOException");
    if(cls){(*env)->ThrowNew(env,cls,why);(*env)->DeleteLocalRef(env,cls);}
}
JNIEXPORT jlongArray JNICALL
Java_com_musa_cad_NativeAc1018Inspection_inspectNative(
    JNIEnv *env,jclass clazz,jbyteArray input){
    (void)clazz;
    if(!input){reject(env,"DWG bytes are required");return NULL;}
    const jsize length=(*env)->GetArrayLength(env,input);
    if(length<256||length>(jsize)MUSA_AC1018_INSPECT_MAX_BYTES){
        reject(env,"AC1018 input size exceeds bounded diagnostic");return NULL;
    }
    uint8_t *bytes=(uint8_t*)malloc((size_t)length);
    if(!bytes){
        reject(env,"Insufficient memory for native DWG inspection");return NULL;
    }
    (*env)->GetByteArrayRegion(env,input,0,length,(jbyte*)bytes);
    if((*env)->ExceptionCheck(env)){free(bytes);return NULL;}
    MusaDwgAc1018FileInspection proof={0};
    const int accepted=musa_dwg_ac1018_inspect_file(
                                  bytes,(size_t)length,&proof);
    free(bytes);
    if(!accepted){
        reject(env,"Native reader cannot certify this R2004 DWG; other versions and unsupported files are not silently converted");
        return NULL;
    }
    const jlong values[11]={
      (jlong)proof.indexed_records,(jlong)proof.decoded_lines,
      (jlong)proof.decoded_arcs,(jlong)proof.decoded_circles,
      (jlong)proof.decoded_lwpolylines,
      (jlong)proof.undecodable_supported_records,
      (jlong)proof.other_declared_records,
      (jlong)proof.covered_record_bytes,
      (jlong)proof.unindexed_section_bytes,
      (jlong)proof.all_record_crcs_verified,
      (jlong)proof.record_ranges_nonoverlapping
    };
    jlongArray out=(*env)->NewLongArray(env,11);
    if(!out)return NULL;
    (*env)->SetLongArrayRegion(env,out,0,11,values);
    return (*env)->ExceptionCheck(env)?NULL:out;
}
