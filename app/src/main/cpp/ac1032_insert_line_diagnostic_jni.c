/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Android-only explicit diagnostic JNI for ONE original AC1032 INSERT.
 * The packet is intentionally incomplete: not model-space/viewport
 * visibility, nested block rendering, BYLAYER paint or normal file open.
 * Same native first-party C11 engine as Windows/iOS/Android. */
#include <jni.h>
#include "musacad_dwg_ac1032_insert_line_v6_diagnostic.h"
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define ORIGINAL_DWG_MAX_BYTES (64u*1024u*1024u)
#define INSERT_LINE_PACKET_CAP (7u+6u*MUSACAD_AC1032_OT49_CHILD_LIMIT)

static void raise_io(JNIEnv *env,const char *msg){
    if((*env)->ExceptionCheck(env))return;
    jclass cls=(*env)->FindClass(env,"java/io/IOException");
    if(!cls)return;
    (*env)->ThrowNew(env,cls,msg);
    (*env)->DeleteLocalRef(env,cls);
}

JNIEXPORT jfloatArray JNICALL
Java_com_musa_cad_FirstPartyDwgPreview_readInsertLineNative(
    JNIEnv *env,jclass cls,jstring input,jlong insert_handle,
    jint max_words,jlongArray audit){
    (void)cls;
    if(!input||!audit||insert_handle<=0||
       (*env)->GetArrayLength(env,audit)!=8||
       max_words<7||max_words>9*1024*1024){
        raise_io(env,"Gecersiz INSERT tanisal parametresi");
        return NULL;
    }
    const char *path=(*env)->GetStringUTFChars(env,input,NULL);
    if(!path)return NULL;
    FILE *stream=fopen(path,"rb");
    (*env)->ReleaseStringUTFChars(env,input,path);
    if(!stream){
        raise_io(env,"Orijinal DWG dosyasi okunamadi");
        return NULL;
    }
    long length=-1;
    int ok=fseek(stream,0,SEEK_END)==0;
    if(ok)length=ftell(stream);
    if(ok)ok=length>=256L&&length<=(long)ORIGINAL_DWG_MAX_BYTES&&
             fseek(stream,0,SEEK_SET)==0;
    uint8_t *source=NULL;
    if(ok)source=(uint8_t*)malloc((size_t)length);
    if(!source)ok=0;
    if(ok)ok=fread(source,1u,(size_t)length,stream)==(size_t)length;
    if(fclose(stream)!=0)ok=0;
    MusaDwgAc1032SceneIndex *scene=NULL;
    if(ok)ok=!memcmp(source,"AC1032",6u)&&
             musa_dwg_ac1032_scene_index_open(
                source,(size_t)length,&scene);
    free(source);
    if(!ok){
        musa_dwg_ac1032_scene_index_close(scene);
        raise_io(env,"Orijinal MC1032 kaynak DWG dogrulanamadi");
        return NULL;
    }
    MusaDwgAc1032InsertLineV6Diagnostic pre={0},post={0};
    float words[INSERT_LINE_PACKET_CAP]={0};
    ok=musa_dwg_ac1032_insert_line_v6_diagnostic(
           scene,(uint64_t)insert_handle,NULL,0u,&pre)&&
       pre.original_h4_h5_wcs_source_verified&&
       pre.packet_partial_by_design&&
       !pre.final_visibility_and_paint_accepted&&
       pre.required_float_words>=7u&&
       pre.required_float_words<=INSERT_LINE_PACKET_CAP&&
       pre.required_float_words<=(size_t)max_words&&
       pre.required_float_words<=INT_MAX;
    if(ok)ok=musa_dwg_ac1032_insert_line_v6_diagnostic(
           scene,(uint64_t)insert_handle,words,
           INSERT_LINE_PACKET_CAP,&post)&&
       !memcmp(&pre,&post,sizeof(pre))&&
       words[0]==6.0f&&
       words[1]==(float)pre.admitted_line_count&&
       words[6]==1.0f;
    musa_dwg_ac1032_scene_index_close(scene);
    if(!ok){
        raise_io(env,"Dogru H4/H5 INSERT LINE kaynagi veya paket butcesi bulunamadi");
        return NULL;
    }
    jfloatArray result=(*env)->NewFloatArray(
        env,(jsize)pre.required_float_words);
    if(!result)return NULL;
    (*env)->SetFloatArrayRegion(
        env,result,0,(jsize)pre.required_float_words,(const jfloat*)words);
    if((*env)->ExceptionCheck(env))return NULL;
    const jlong values[8]={
        insert_handle,
        (jlong)pre.original_source_line_count,
        (jlong)pre.admitted_line_count,
        (jlong)pre.omitted_source_line_count,
        (jlong)pre.required_float_words,
        (jlong)pre.original_h4_h5_wcs_source_verified,
        (jlong)pre.packet_partial_by_design,
        (jlong)pre.final_visibility_and_paint_accepted
    };
    (*env)->SetLongArrayRegion(env,audit,0,8,values);
    if((*env)->ExceptionCheck(env))return NULL;
    return result;
}
