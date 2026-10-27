/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Android read-only source ACI evidence; NEVER a visible-paint grant.
 * The original AC1032 bytes are read only, with no DXF fallback.
 * Rows: INSERT H, BLOCK_HEADER H, CIRCLE H, LAYER H, source ACI,
 * original packed LAYER values. Geometry remains the separate ACI6
 * NON-SOURCE diagnostic path. */
#include <jni.h>
#include "musacad_dwg_ac1032_circle_bylayer_inventory.h"
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define MUSA_CIRCLE_AUDIT_MAX_BYTES (64u*1024u*1024u)
static void circle_audit_refuse(JNIEnv *env,const char *message){
    if((*env)->ExceptionCheck(env))return;
    jclass e=(*env)->FindClass(env,"java/io/IOException");
    if(e){(*env)->ThrowNew(env,e,message);(*env)->DeleteLocalRef(env,e);}
}
JNIEXPORT jlongArray JNICALL
Java_com_musa_cad_FirstPartyDwgPreview_readCircleLayerAciEvidenceNative(
    JNIEnv *env,jclass cls,jstring pathArg,jint maxRows,jlongArray audit){
    (void)cls;
    if(!pathArg||!audit||(*env)->GetArrayLength(env,audit)!=10||
       maxRows<1||maxRows>(jint)MUSACAD_AC1032_CIRCLE_BYLAYER_INVENTORY_MAX){
        circle_audit_refuse(env,"CIRCLE kaynak renk raporu parametresi gecersiz");
        return NULL;
    }
    const char *path=(*env)->GetStringUTFChars(env,pathArg,NULL);
    if(!path)return NULL;
    FILE *f=fopen(path,"rb");
    (*env)->ReleaseStringUTFChars(env,pathArg,path);
    if(!f){
        circle_audit_refuse(env,"Orijinal DWG salt okunur acilamadi");
        return NULL;
    }
    int ok=fseek(f,0,SEEK_END)==0;
    long length=ok?ftell(f):-1L;
    if(length<256L||length>(long)MUSA_CIRCLE_AUDIT_MAX_BYTES||
       fseek(f,0,SEEK_SET)!=0)ok=0;
    uint8_t *bytes=ok?(uint8_t*)malloc((size_t)length):NULL;
    if(!bytes)ok=0;
    if(ok)ok=fread(bytes,1u,(size_t)length,f)==(size_t)length;
    if(fclose(f)!=0)ok=0;
    MusaDwgAc1032SceneIndex *scene=NULL;
    if(ok)ok=!memcmp(bytes,"AC1032",6u)&&
        musa_dwg_ac1032_scene_index_open(bytes,(size_t)length,&scene);
    free(bytes);
    MusaDwgAc1032CircleByLayerInventory inventory={0};
    if(ok)ok=musa_dwg_ac1032_circle_bylayer_inventory(scene,&inventory)&&
        inventory.original_source_inventory_only&&
        !inventory.final_visible_paint_accepted&&
        inventory.verified_source_color_count<=(size_t)maxRows&&
        inventory.verified_source_color_count<=
          MUSACAD_AC1032_CIRCLE_BYLAYER_INVENTORY_MAX&&
        inventory.verified_source_color_count+
          inventory.unresolved_circle_count==inventory.circle_candidate_count&&
        inventory.source_indexed_insert_count+
          inventory.unresolved_insert_count==inventory.insert_count;
    musa_dwg_ac1032_scene_index_close(scene);
    if(!ok){
        circle_audit_refuse(env,"Orijinal CIRCLE kaynak ACI kaniti dogrulanamadi");
        return NULL;
    }
    jlong values[6u*MUSACAD_AC1032_CIRCLE_BYLAYER_INVENTORY_MAX];
    for(size_t i=0;i<inventory.verified_source_color_count;i++){
        const MusaDwgAc1032CircleByLayerInventoryRow *r=&inventory.rows[i];
        if(!r->original_source_crc_and_wcs_verified||
           r->final_visible_paint_accepted||
           !r->insert_handle||!r->block_header_handle||
           !r->circle_handle||!r->layer_handle||
           r->insert_handle>INT64_MAX||r->block_header_handle>INT64_MAX||
           r->circle_handle>INT64_MAX||r->layer_handle>INT64_MAX||
           r->original_layer_aci<1u){
            circle_audit_refuse(env,"CIRCLE kaynak renk zinciri guvensiz");
            return NULL;
        }
        values[6u*i+0u]=(jlong)r->insert_handle;
        values[6u*i+1u]=(jlong)r->block_header_handle;
        values[6u*i+2u]=(jlong)r->circle_handle;
        values[6u*i+3u]=(jlong)r->layer_handle;
        values[6u*i+4u]=(jlong)r->original_layer_aci;
        values[6u*i+5u]=(jlong)r->original_layer_packed_values;
    }
    const jlong report[10]={
        (jlong)inventory.indexed_record_count,
        (jlong)inventory.insert_count,
        (jlong)inventory.source_indexed_insert_count,
        (jlong)inventory.unresolved_insert_count,
        (jlong)inventory.undecodable_indexed_count,
        (jlong)inventory.circle_candidate_count,
        (jlong)inventory.unresolved_circle_count,
        (jlong)inventory.verified_source_color_count,
        (jlong)inventory.original_source_inventory_only,
        (jlong)inventory.final_visible_paint_accepted
    };
    jlongArray result=(*env)->NewLongArray(
        env,(jsize)(6u*inventory.verified_source_color_count));
    if(!result)return NULL;
    (*env)->SetLongArrayRegion(env,result,0,
        (jsize)(6u*inventory.verified_source_color_count),values);
    if((*env)->ExceptionCheck(env))return NULL;
    (*env)->SetLongArrayRegion(env,audit,0,10,report);
    if((*env)->ExceptionCheck(env))return NULL;
    return result;
}
