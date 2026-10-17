/* SPDX-License-Identifier: GPL-3.0-or-later */
/* One Android JNI transport invocation replaces Java group and section logic.
 * In/out four-int Java state is safe, owned and never a dangling C pointer.
 */
#include <jni.h>
#include "musacad_dxf_pair.h"
#include "musacad_dxf_section.h"

JNIEXPORT jint JNICALL
Java_com_musa_cad_DxfSectionGuard_consumeNative(
    JNIEnv *env,jclass clazz,jintArray fields,jstring group,jstring value,
    jint pair_count,jint max_pairs) {
    (void)clazz;
    if(!fields||!group||pair_count<0||max_pairs<=0||
       (*env)->GetArrayLength(env,fields)!=4)
        return MUSA_DXF_SECTION_BAD_ARGUMENT;
    const jsize grouplen=(*env)->GetStringUTFLength(env,group);
    if(grouplen<1||grouplen>1048576)
        return MUSA_DXF_PAIR_INVALID_CODE;
    const char *g=(*env)->GetStringUTFChars(env,group,NULL);
    if(!g)return MUSA_DXF_SECTION_BAD_ARGUMENT;
    int code=0;
    const int pair_status=musa_core_dxf_pair_check(
        g,(size_t)grouplen,value!=NULL,(size_t)pair_count,
        (size_t)max_pairs,&code);
    (*env)->ReleaseStringUTFChars(env,group,g);
    if(pair_status!=MUSA_DXF_PAIR_OK)return pair_status;

    const jsize vlen=(*env)->GetStringUTFLength(env,value);
    if(vlen<0||vlen>4194304)return MUSA_DXF_SECTION_BAD_ARGUMENT;
    const char *v=(*env)->GetStringUTFChars(env,value,NULL);
    if(!v)return MUSA_DXF_SECTION_BAD_ARGUMENT;
    jint raw[4];
    (*env)->GetIntArrayRegion(env,fields,0,4,raw);
    if((*env)->ExceptionCheck(env)){
        (*env)->ReleaseStringUTFChars(env,value,v);
        return MUSA_DXF_SECTION_BAD_ARGUMENT;
    }
    MusaDxfSectionState state={raw[0],raw[1],raw[2],raw[3]};
    int flags=0;
    const int status=musa_core_dxf_section_step(&state,code,v,(size_t)vlen,&flags);
    (*env)->ReleaseStringUTFChars(env,value,v);
    if(status!=MUSA_DXF_SECTION_OK)return status;

    const jint updated[4]={state.current_kind,state.awaiting_name,
                           state.seen_entities,state.seen_eof};
    (*env)->SetIntArrayRegion(env,fields,0,4,updated);
    if((*env)->ExceptionCheck(env))return MUSA_DXF_SECTION_BAD_ARGUMENT;
    return ((jint)flags<<11)|code;
}

JNIEXPORT jint JNICALL
Java_com_musa_cad_DxfSectionGuard_finishNative(
    JNIEnv *env,jclass clazz,jintArray fields){
    (void)clazz;
    if(!fields||(*env)->GetArrayLength(env,fields)!=4)
        return MUSA_DXF_SECTION_BAD_ARGUMENT;
    jint raw[4];
    (*env)->GetIntArrayRegion(env,fields,0,4,raw);
    if((*env)->ExceptionCheck(env))return MUSA_DXF_SECTION_BAD_ARGUMENT;
    const MusaDxfSectionState state={raw[0],raw[1],raw[2],raw[3]};
    return musa_core_dxf_section_finish(&state);
}
