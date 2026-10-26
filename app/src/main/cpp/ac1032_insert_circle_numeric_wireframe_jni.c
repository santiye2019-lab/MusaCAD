/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Experimental geometric evidence ONLY. The conspicuous ACI6 strokes
 * emitted below do NOT represent source DWG color, visibility or owner.
 * Never use this endpoint for normal editable DWG opening. */
#include <jni.h>
#include "musacad_dwg_ac1032_insert_circle_numeric_wireframe_v6.h"
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#define MAX_SOURCE_BYTES (64u*1024u*1024u)

static void refuse(JNIEnv *env,const char *message){
    if((*env)->ExceptionCheck(env))return;
    jclass e=(*env)->FindClass(env,"java/io/IOException");
    if(e){(*env)->ThrowNew(env,e,message);(*env)->DeleteLocalRef(env,e);}
}
JNIEXPORT jfloatArray JNICALL
Java_com_musa_cad_FirstPartyDwgPreview_readCircleGeometryEvidenceNative(
    JNIEnv *env,jclass cls,jstring pathArg,jint budget,jlongArray evidence){
    (void)cls;
    if(!pathArg||!evidence||(*env)->GetArrayLength(env,evidence)!=11||
       budget<7||budget>9*1024*1024){
        refuse(env,"Sayisal CIRCLE tani parametresi gecersiz");return NULL;
    }
    const char *path=(*env)->GetStringUTFChars(env,pathArg,NULL);
    if(!path)return NULL;
    FILE *f=fopen(path,"rb");
    (*env)->ReleaseStringUTFChars(env,pathArg,path);
    if(!f){refuse(env,"DWG kaynak dosyasi okunamadi");return NULL;}
    long n=-1;
    int ok=fseek(f,0,SEEK_END)==0;
    if(ok)n=ftell(f);
    if(ok)ok=n>=256&&n<=(long)MAX_SOURCE_BYTES&&fseek(f,0,SEEK_SET)==0;
    uint8_t *bytes=ok?(uint8_t*)malloc((size_t)n):NULL;
    if(!bytes)ok=0;
    if(ok)ok=fread(bytes,1u,(size_t)n,f)==(size_t)n;
    if(fclose(f)!=0)ok=0;
    MusaDwgAc1032SceneIndex *scene=NULL;
    if(ok)ok=!memcmp(bytes,"AC1032",6u)&&
       musa_dwg_ac1032_scene_index_open(bytes,(size_t)n,&scene);
    free(bytes);
    if(!ok){
        musa_dwg_ac1032_scene_index_close(scene);
        refuse(env,"R2018 CIRCLE kaynak H/CRC kaydi dogrulanamadi");
        return NULL;
    }
    MusaDwgAc1032CircleWireframeEvidence first={0},last={0};
    ok=musa_dwg_ac1032_circle_wireframe_v6_diagnostic(
         scene,NULL,0u,&first)&&
       first.source_only_partial&&first.paint_is_diagnostic_not_source&&
       !first.final_color_visibility_and_ownership_accepted&&
       first.required_float_words>=7u&&
       first.required_float_words<=MUSACAD_AC1032_CIRCLE_WIRE_WORDS&&
       first.required_float_words<=(size_t)budget&&
       first.required_float_words<=INT_MAX;
    float *packet=ok?(float*)malloc(first.required_float_words*sizeof(float)):NULL;
    if(!packet)ok=0;
    if(ok)ok=musa_dwg_ac1032_circle_wireframe_v6_diagnostic(
              scene,packet,first.required_float_words,&last)&&
           !memcmp(&first,&last,sizeof(first))&&
           packet[0]==6.0f&&packet[6]==1.0f&&
           packet[1]==(float)first.emitted_numeric_wireframes;
    musa_dwg_ac1032_scene_index_close(scene);
    if(!ok){
        free(packet);
        refuse(env,"Sayisal CIRCLE tel-kafes WCS geometri paketi dogrulanamadi");
        return NULL;
    }
    jfloatArray result=(*env)->NewFloatArray(
        env,(jsize)first.required_float_words);
    if(!result){free(packet);return NULL;}
    (*env)->SetFloatArrayRegion(env,result,0,
        (jsize)first.required_float_words,packet);
    free(packet);
    if((*env)->ExceptionCheck(env))return NULL;
    const jlong audit[11]={
        (jlong)first.indexed_records,
        (jlong)first.insert_candidates,
        (jlong)first.verified_inserts,
        (jlong)first.unsupported_inserts,
        (jlong)first.circle_candidates,
        (jlong)first.source_numeric_circles,
        (jlong)first.skipped_unverified_circles,
        (jlong)first.unresolved_h4_h5_circles,
        (jlong)first.emitted_numeric_wireframes,
        (jlong)first.required_float_words,
        (jlong)first.paint_is_diagnostic_not_source
    };
    (*env)->SetLongArrayRegion(env,evidence,0,11,audit);
    if((*env)->ExceptionCheck(env))return NULL;
    return result;
}
