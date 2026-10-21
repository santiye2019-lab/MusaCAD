/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Android JNI byte transport to the SAME independent native C11
 * R2004/AC1018 indexed scene reader used in Linux/Windows/iOS core.
 * Deliberately read-only. Never open any file path, mutate a source
 * byte, load a third-party CAD decoder or publish a partial result. */
#include <jni.h>
#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include "musacad_dwg_signature.h"
#include "musacad_dwg_ac1018_scene.h"

#define MUSA_AC1018_INSPECT_MAX_BYTES (32u*1024u*1024u)
static void reject(JNIEnv *env,const char *why){
    if((*env)->ExceptionCheck(env))return;
    jclass cls=(*env)->FindClass(env,"java/io/IOException");
    if(cls){
        (*env)->ThrowNew(env,cls,why);
        (*env)->DeleteLocalRef(env,cls);
    }
}
JNIEXPORT jlongArray JNICALL
Java_com_musa_cad_NativeAc1018Inspection_inspectNative(
    JNIEnv *env,jclass clazz,jbyteArray input){
    (void)clazz;
    if(!input){
        reject(env,"DWG bytes are required");
        return NULL;
    }
    jsize len=(*env)->GetArrayLength(env,input);
    if(len<256||len>(jsize)MUSA_AC1018_INSPECT_MAX_BYTES){
        reject(env,"AC1018 file exceeds bounded native proof");
        return NULL;
    }
    uint8_t *bytes=(uint8_t*)malloc((size_t)len);
    if(!bytes){
        reject(env,"Insufficient memory for native DWG inspection");
        return NULL;
    }
    (*env)->GetByteArrayRegion(env,input,0,len,(jbyte*)bytes);
    if((*env)->ExceptionCheck(env)){
        free(bytes);
        return NULL;
    }
    if(musa_core_dwg_identify_signature(bytes,(size_t)len)!=MUSA_DWG_2004){
        free(bytes);
        reject(env,"Only actual AC1018/R2004 binary DWG is supported by this inspector");
        return NULL;
    }
    MusaDwgAc1018Scene *scene=NULL;
    const int opened=musa_dwg_ac1018_scene_open(bytes,(size_t)len,&scene);
    /* Scene is independent from input: no borrowed bytes after open. */
    free(bytes);
    if(!opened||!scene){
        reject(env,"Invalid or unsupported protected R2004 DWG scene");
        return NULL;
    }
    MusaDwgAc1018SceneInventory list={0};
    MusaDwgAc1018RecordRangeAudit ranges={0};
    const int ok=musa_dwg_ac1018_scene_inventory(scene,0,&list) &&
                 musa_dwg_ac1018_scene_audit_record_ranges(scene,&ranges) &&
                 list.all_indexed_record_crcs_verified &&
                 ranges.ranges_nonoverlapping &&
                 list.indexed_record_count==ranges.indexed_records &&
                 list.indexed_record_count>0 &&
                 list.decoded_lines+list.decoded_arcs+list.decoded_circles+
                 list.decoded_lwpolylines+list.undecodable_supported_records+
                 list.other_declared_records==list.indexed_record_count;
    if(!ok){
        musa_dwg_ac1018_scene_close(scene);
        reject(env,"R2004 object inventory or physical ranges cannot be certified");
        return NULL;
    }
    /* These are counts, not an editable model, display colors,
     * lossless preserved graph, or AC1032 reader. */
    jlong results[11]={
        (jlong)list.indexed_record_count,
        (jlong)list.decoded_lines,
        (jlong)list.decoded_arcs,
        (jlong)list.decoded_circles,
        (jlong)list.decoded_lwpolylines,
        (jlong)list.undecodable_supported_records,
        (jlong)list.other_declared_records,
        (jlong)ranges.covered_record_bytes,
        (jlong)ranges.unindexed_section_bytes,
        (jlong)list.all_indexed_record_crcs_verified,
        (jlong)ranges.ranges_nonoverlapping
    };
    musa_dwg_ac1018_scene_close(scene);
    jlongArray out=(*env)->NewLongArray(env,11);
    if(!out)return NULL;
    (*env)->SetLongArrayRegion(env,out,0,11,results);
    if((*env)->ExceptionCheck(env))return NULL;
    return out;
}
