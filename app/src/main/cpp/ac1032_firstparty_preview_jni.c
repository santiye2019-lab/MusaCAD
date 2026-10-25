/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Android opt-in diagnostic adapter to ONE original independent C11 AC1032
 * engine. Never references third_party/libredwg, never writes source files.
 * Existing production DWG open intentionally remains fail-closed until
 * model-space + graph + text + style acceptance reaches completeness. */
#include <jni.h>
#include "musacad_dwg_ac1032_preview_v6.h"
#include "musacad_dwg_ac1032_display_readiness.h"
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_ORIGINAL_BYTES (64u*1024u*1024u)
#define MAX_V6_FLOAT_WORDS (9u*1024u*1024u)

static void throw_io(JNIEnv *env,const char *message){
    jclass klass=(*env)->FindClass(env,"java/io/IOException");
    if(!klass)return;
    (*env)->ThrowNew(env,klass,message);
    (*env)->DeleteLocalRef(env,klass);
}
JNIEXPORT jfloatArray JNICALL
Java_com_musa_cad_FirstPartyDwgPreview_readOriginalNative(
    JNIEnv *env,jclass klass,jstring original,jint maxWords,
    jlongArray readinessOut){
    (void)klass;
    if(!original||!readinessOut||
       (*env)->GetArrayLength(env,readinessOut)!=20||
       maxWords<256||maxWords>(jint)MAX_V6_FLOAT_WORDS){
        throw_io(env,"Gecersiz AC1032 dosyasi veya bellek siniri");
        return NULL;
    }
    const char *path=(*env)->GetStringUTFChars(env,original,NULL);
    if(!path)return NULL;
    FILE *stream=fopen(path,"rb");
    (*env)->ReleaseStringUTFChars(env,original,path);
    if(!stream){
        throw_io(env,"Kaynak DWG okunamadi");return NULL;
    }
    if(fseek(stream,0,SEEK_END)!=0){
        fclose(stream);
        throw_io(env,"DWG dosya boyutu okunamadi");return NULL;
    }
    const long len=ftell(stream);
    if(len<256||len>(long)MAX_ORIGINAL_BYTES||
       fseek(stream,0,SEEK_SET)!=0){
        fclose(stream);
        throw_io(env,"DWG boyutu desteklenmiyor");return NULL;
    }
    const size_t n=(size_t)len;
    uint8_t *file=(uint8_t*)malloc(n);
    if(!file){
        fclose(stream);
        throw_io(env,"DWG kaynak bellegi ayrilamadi");return NULL;
    }
    int ok=fread(file,1u,n,stream)==n;
    if(fclose(stream)!=0)ok=0;
    MusaDwgAc1032SceneIndex *scene=NULL;
    if(ok)ok=musa_dwg_ac1032_scene_index_open(file,n,&scene);
    free(file);file=NULL;
    if(!ok){
        musa_dwg_ac1032_scene_index_close(scene);
        throw_io(env,"Orijinal AC1032 DWG dogrulanamadi");
        return NULL;
    }
    MusaAc1032DisplayReadiness ready={0};
    MusaAc1032PreviewV6Stats needed={0},copied={0};
    MusaAc1032PreviewV6SourceCensus census={0};
    /* All three views of this scene use the SAME CRC/H-verified original
     * source-index, not a guessed object count or file expansion. This
     * diagnostic counts v6 admitted source records separately from
     * rendered primitives. Not a full AutoCAD visual fidelity claim. */
    ok=musa_dwg_ac1032_scene_display_readiness(scene,&ready)&&
       musa_dwg_ac1032_preview_v6_planar_source_aci(
        scene,NULL,0u,&needed)&&
       musa_dwg_ac1032_preview_v6_source_census(scene,&census)&&
        census.total_indexed_records==needed.total_indexed_records&&
        census.emitted_v6_primitives==needed.rendered_primitives&&
        census.omitted_v6_records==needed.omitted_records&&
        census.candidate_source_records==
            census.packet_admitted_source_records+
            census.candidate_not_admitted_records&&
        census.candidate_source_records+
            census.other_indexed_records==census.total_indexed_records&&
        census.complete_dwg_display_accepted==0u&&
        needed.rendered_primitives>0u&&
        needed.required_float_words>=7u&&
        needed.required_float_words<=(size_t)maxWords&&
        needed.required_float_words<=MAX_V6_FLOAT_WORDS&&
        needed.required_float_words<=INT_MAX&&
        needed.required_float_words<=SIZE_MAX/sizeof(float);
    float *words=NULL;
    if(ok){
        words=(float*)malloc(needed.required_float_words*sizeof(float));
        if(!words)ok=0;
    }
    if(ok)ok=musa_dwg_ac1032_preview_v6_planar_source_aci(
         scene,words,needed.required_float_words,&copied)&&
         copied.required_float_words==needed.required_float_words&&
         copied.rendered_primitives==needed.rendered_primitives&&
         copied.omitted_records==needed.omitted_records&&
         copied.partial_geometry==needed.partial_geometry&&
         ready.indexed_records==needed.total_indexed_records&&
         !ready.display_accepted;
    musa_dwg_ac1032_scene_index_close(scene);
    if(!ok){
        free(words);
        throw_io(env,"DWG eksik veya bellek butcesine sigmayan geometri; onizleme yok");
        return NULL;
    }
    jfloatArray output=(*env)->NewFloatArray(
           env,(jsize)needed.required_float_words);
    if(output){
        (*env)->SetFloatArrayRegion(env,output,0,
               (jsize)needed.required_float_words,(const jfloat*)words);
        if(!(*env)->ExceptionCheck(env)){
            const jlong values[20]={
                (jlong)ready.indexed_records,
                (jlong)ready.known_planar_records,
                (jlong)ready.source_style_joined,
                (jlong)ready.source_direct_aci,
                (jlong)ready.source_bylayer,
                (jlong)ready.source_byblock,
                (jlong)ready.source_complex_or_invisible,
                (jlong)ready.known_style_rejected,
                (jlong)ready.other_indexed_records,
                (jlong)ready.narrow_layer_aci_available,
                (jlong)ready.display_accepted,
                (jlong)ready.source_direct_rgb24,
                (jlong)ready.source_entity_invisible,
                (jlong)census.candidate_source_records,
                (jlong)census.packet_admitted_source_records,
                (jlong)census.candidate_not_admitted_records,
                (jlong)census.other_indexed_records,
                (jlong)census.emitted_v6_primitives,
                (jlong)census.omitted_v6_records,
                (jlong)census.complete_dwg_display_accepted
            };
            (*env)->SetLongArrayRegion(env,readinessOut,0,20,values);
        }
    }
    free(words);
    return output;
}
