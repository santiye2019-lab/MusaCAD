/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Read-only, explicitly PARTIAL Android original-R2018 multi-INSERT LINE
 * diagnostic. One on-screen vector packet for ALL strictly authenticated
 * INSERT instances, not one manually entered handle. Never replace the
 * normal editable DWG open/scene and never resolve unknown layer visibility. */
#include <jni.h>
#include "musacad_dwg_ac1032_all_insert_line_v6_diagnostic.h"
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define ORIGINAL_DWG_MAX_BYTES (64u*1024u*1024u)
#define AUDIT_WORDS 11

static void fail_io(JNIEnv *env,const char *message){
    if((*env)->ExceptionCheck(env))return;
    jclass ex=(*env)->FindClass(env,"java/io/IOException");
    if(ex){
        (*env)->ThrowNew(env,ex,message);
        (*env)->DeleteLocalRef(env,ex);
    }
}
JNIEXPORT jfloatArray JNICALL
Java_com_musa_cad_FirstPartyDwgPreview_readAllInsertLinesNative(
    JNIEnv *env,jclass ignored,jstring input,jint max_words,jlongArray audit){
    (void)ignored;
    if(!input||!audit||(*env)->GetArrayLength(env,audit)!=AUDIT_WORDS||
       max_words<7||max_words>9*1024*1024){
        fail_io(env,"Coklu INSERT kaynagi parametresi gecersiz");
        return NULL;
    }
    const char *path=(*env)->GetStringUTFChars(env,input,NULL);
    if(!path)return NULL;
    FILE *file=fopen(path,"rb");
    (*env)->ReleaseStringUTFChars(env,input,path);
    if(!file){
        fail_io(env,"Kaynak DWG yalnizca okunur olarak acilamadi");
        return NULL;
    }
    long len=-1;
    int ok=fseek(file,0,SEEK_END)==0;
    if(ok)len=ftell(file);
    if(ok)ok=len>=256L&&len<=(long)ORIGINAL_DWG_MAX_BYTES&&
             fseek(file,0,SEEK_SET)==0;
    uint8_t *original=NULL;
    if(ok)original=(uint8_t*)malloc((size_t)len);
    if(!original)ok=0;
    if(ok)ok=fread(original,1u,(size_t)len,file)==(size_t)len;
    if(fclose(file)!=0)ok=0;
    MusaDwgAc1032SceneIndex *scene=NULL;
    if(ok)ok=!memcmp(original,"AC1032",6u)&&
             musa_dwg_ac1032_scene_index_open(
                 original,(size_t)len,&scene);
    free(original);
    if(!ok){
        musa_dwg_ac1032_scene_index_close(scene);
        fail_io(env,"Orijinal AC1032 indeks kaynagi dogrulanamadi");
        return NULL;
    }
    MusaDwgAc1032AllInsertLineV6Diagnostic pre={0},post={0};
    ok=musa_dwg_ac1032_all_insert_line_v6_diagnostic(
        scene,NULL,0u,&pre)&&
       pre.source_packet_is_partial&&
       !pre.final_visibility_and_paint_accepted&&
       pre.candidate_insert_records==
         pre.verified_insert_records+pre.unsupported_insert_records&&
       pre.verified_source_line_count==
         pre.admitted_line_count+pre.omitted_verified_line_count&&
       pre.required_float_words>=7u&&
       pre.required_float_words<=
         MUSACAD_AC1032_ALL_INSERT_LINE_MAX_FLOAT_WORDS&&
       pre.required_float_words<=(size_t)max_words&&
       pre.required_float_words<=INT_MAX;
    float *packet=NULL;
    if(ok)packet=(float*)malloc(pre.required_float_words*sizeof(float));
    if(!packet)ok=0;
    if(ok)ok=musa_dwg_ac1032_all_insert_line_v6_diagnostic(
        scene,packet,pre.required_float_words,&post)&&
       !memcmp(&pre,&post,sizeof(pre))&&
       packet[0]==6.0f&&packet[6]==1.0f&&
       packet[1]==(float)pre.admitted_line_count;
    musa_dwg_ac1032_scene_index_close(scene);
    if(!ok){
        free(packet);
        fail_io(env,"Coklu INSERT LINE parcali paket butcesi veya kaynagi dogrulanamadi");
        return NULL;
    }
    jfloatArray raw=(*env)->NewFloatArray(env,(jsize)pre.required_float_words);
    if(!raw){
        free(packet);
        return NULL;
    }
    (*env)->SetFloatArrayRegion(env,raw,0,
        (jsize)pre.required_float_words,(const jfloat*)packet);
    free(packet);
    if((*env)->ExceptionCheck(env))return NULL;
    const jlong evidence[AUDIT_WORDS]={
        (jlong)pre.total_indexed_records,
        (jlong)pre.candidate_insert_records,
        (jlong)pre.verified_insert_records,
        (jlong)pre.unsupported_insert_records,
        (jlong)pre.verified_source_line_count,
        (jlong)pre.admitted_line_count,
        (jlong)pre.omitted_verified_line_count,
        (jlong)pre.required_float_words,
        (jlong)pre.source_bounds_present,
        (jlong)pre.source_packet_is_partial,
        (jlong)pre.final_visibility_and_paint_accepted
    };
    (*env)->SetLongArrayRegion(env,audit,0,AUDIT_WORDS,evidence);
    if((*env)->ExceptionCheck(env))return NULL;
    return raw;
}
