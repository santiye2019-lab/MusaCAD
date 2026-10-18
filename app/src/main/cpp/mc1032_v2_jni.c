/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Only platform/JNI transport. The SAME first-party C11 MusaCAD core
 * owns layers, geometry, handles, revisions, CRC, and binary serialization.
 */
#include <jni.h>
#include "musacad_mc1032_v2.h"
#include <stdint.h>
#include <string.h>
#include <stdlib.h>

static void error(JNIEnv *env,const char *msg){
    jclass e=(*env)->FindClass(env,"java/io/IOException");
    if(e){(*env)->ThrowNew(env,e,msg);(*env)->DeleteLocalRef(env,e);}
}
static void check(JNIEnv *env,int status){
    if(status==MUSA_MC2_CORRUPT)error(env,"MC1032 v2 dosya butunlugu veya kimlik hatasi");
    else if(status==MUSA_MC2_UNSUPPORTED)error(env,"MC1032 v2 bilinmeyen nesne/surum/alan");
    else if(status==MUSA_MC2_LIMIT)error(env,"MC1032 v2 bellek/nesne siniri");
    else error(env,"MC1032 v2 gecersiz dosya ya da katman");
}
JNIEXPORT jobjectArray JNICALL
Java_com_musa_cad_Mc1032V2Native_decodeNative(
       JNIEnv *env,jclass cls,jbyteArray source){
    (void)cls;
    if(!source){error(env,"MC1032 v2 dosya yok");return NULL;}
    jsize n=(*env)->GetArrayLength(env,source);
    if(n<0||(size_t)n>MUSA_MC2_MAX_BYTES){
        error(env,"MC1032 v2 dosya boyutu");return NULL;
    }
    jbyte *data=(*env)->GetByteArrayElements(env,source,NULL);
    if(!data)return NULL;
    MusaMC2Scene *scene=NULL;
    int status=musa_mc2_decode((const unsigned char *)data,(size_t)n,&scene);
    (*env)->ReleaseByteArrayElements(env,source,data,JNI_ABORT);
    if(status!=MUSA_MC2_OK){check(env,status);return NULL;}
    uint32_t entities=musa_mc2_scene_entity_count(scene);
    uint32_t layers=musa_mc2_scene_layer_count(scene);
    jlongArray words=(*env)->NewLongArray(env,(jsize)(entities*9u));
    jclass stringClass=(*env)->FindClass(env,"java/lang/String");
    jobjectArray names=stringClass?
       (*env)->NewObjectArray(env,(jsize)layers,stringClass,NULL):NULL;
    jintArray colors=(*env)->NewIntArray(env,(jsize)layers);
    jclass objectClass=(*env)->FindClass(env,"java/lang/Object");
    jobjectArray triple=objectClass?
       (*env)->NewObjectArray(env,3,objectClass,NULL):NULL;
    if(!words||!names||!colors||!triple)goto finish;
    for(uint32_t i=0;i<layers;i++){
        MusaMC2Layer layer={0};
        if(musa_mc2_scene_get_layer(scene,i,&layer)!=MUSA_MC2_OK){
            error(env,"MC1032 v2 katman aktarimi");goto finish;
        }
        jstring name=(*env)->NewStringUTF(env,layer.name);
        if(!name)goto finish;
        (*env)->SetObjectArrayElement(env,names,(jsize)i,name);
        (*env)->DeleteLocalRef(env,name);
        jint color=(jint)layer.rgba;
        (*env)->SetIntArrayRegion(env,colors,(jsize)i,1,&color);
        if((*env)->ExceptionCheck(env))goto finish;
    }
    for(uint32_t i=0;i<entities;i++){
        MusaMC2Entity e={0};
        if(musa_mc2_scene_get_index(scene,i,&e)!=MUSA_MC2_OK){
            error(env,"MC1032 v2 nesne aktarimi");goto finish;
        }
        jlong values[9]={0};
        uint64_t bits=e.handle;memcpy(&values[0],&bits,8);
        values[1]=(jlong)e.kind;values[2]=(jlong)e.layer;
        const double points[6]={e.x1,e.y1,e.z1,e.x2,e.y2,e.z2};
        for(unsigned j=0;j<6;j++){
            memcpy(&bits,&points[j],8);memcpy(&values[3+j],&bits,8);
        }
        (*env)->SetLongArrayRegion(env,words,(jsize)(i*9u),9,values);
        if((*env)->ExceptionCheck(env))goto finish;
    }
    (*env)->SetObjectArrayElement(env,triple,0,words);
    (*env)->SetObjectArrayElement(env,triple,1,names);
    (*env)->SetObjectArrayElement(env,triple,2,colors);
finish:
    musa_mc2_scene_destroy(scene);
    if((*env)->ExceptionCheck(env))return NULL;
    return triple;
}

JNIEXPORT jbyteArray JNICALL
Java_com_musa_cad_Mc1032V2Native_encodeNative(
      JNIEnv *env,jclass cls,jobjectArray names,
      jintArray colors,jlongArray entities){
    (void)cls;
    if(!names||!colors||!entities){
        error(env,"MC1032 v2 katman veya nesne dizisi yok");return NULL;
    }
    jsize layers=(*env)->GetArrayLength(env,names);
    jsize color_count=(*env)->GetArrayLength(env,colors);
    jsize words=(*env)->GetArrayLength(env,entities);
    if(layers<1||layers>(jsize)MUSA_MC2_MAX_LAYERS||
       color_count!=layers||words<0||words%9!=0||
       words/9>(jsize)MUSA_MC2_MAX_ENTITIES){
        error(env,"MC1032 v2 dizi siniri");return NULL;
    }
    MusaMC2Scene *scene=musa_mc2_scene_create(words? (uint32_t)(words/9):1u);
    if(!scene){error(env,"MC1032 v2 bellek tahsisi");return NULL;}
    int status=MUSA_MC2_OK;
    for(jsize i=0;i<layers;i++){
        jstring n=(jstring)(*env)->GetObjectArrayElement(env,names,i);
        if(!n){status=MUSA_MC2_INVALID;break;}
        const char *name=(*env)->GetStringUTFChars(env,n,NULL);
        if(!name){(*env)->DeleteLocalRef(env,n);status=MUSA_MC2_NO_MEMORY;break;}
        jint rgba=0;
        (*env)->GetIntArrayRegion(env,colors,i,1,&rgba);
        if((*env)->ExceptionCheck(env)){
            (*env)->ReleaseStringUTFChars(env,n,name);
            (*env)->DeleteLocalRef(env,n);
            status=MUSA_MC2_INVALID;break;
        }
        if(i==0){
            if(strcmp(name,"0")||((uint32_t)rgba)!=0xffffffffu)
                status=MUSA_MC2_UNSUPPORTED;
        }else{
            uint32_t id=0;
            status=musa_mc2_scene_add_layer(scene,name,(uint32_t)rgba,&id);
            if(status==MUSA_MC2_OK&&id!=(uint32_t)i)
                status=MUSA_MC2_INVALID;
        }
        (*env)->ReleaseStringUTFChars(env,n,name);
        (*env)->DeleteLocalRef(env,n);
        if(status!=MUSA_MC2_OK)break;
    }
    for(jsize i=0;status==MUSA_MC2_OK&&i<words/9;i++){
        jlong values[9]={0};
        (*env)->GetLongArrayRegion(env,entities,i*9,9,values);
        if((*env)->ExceptionCheck(env)){status=MUSA_MC2_INVALID;break;}
        MusaMC2Entity e={0};
        memcpy(&e.handle,&values[0],8);
        if(values[1]<1||values[1]>2||
           values[2]<0||values[2]>=layers){
            status=MUSA_MC2_UNSUPPORTED;break;
        }
        e.kind=(uint32_t)values[1];e.layer=(uint32_t)values[2];
        double *positions[6]={&e.x1,&e.y1,&e.z1,&e.x2,&e.y2,&e.z2};
        for(unsigned k=0;k<6;k++)memcpy(positions[k],&values[3+k],8);
        uint64_t got=0;
        status=musa_mc2_scene_add_entity(scene,&e,&got);
        if(status==MUSA_MC2_OK&&got!=e.handle)
            status=MUSA_MC2_INVALID;
    }
    size_t length=0u;
    if(status==MUSA_MC2_OK)
        status=musa_mc2_encode(scene,NULL,0,&length);
    unsigned char *data=NULL;
    if(status==MUSA_MC2_OK){
        data=(unsigned char *)malloc(length);
        if(!data)status=MUSA_MC2_NO_MEMORY;
    }
    if(status==MUSA_MC2_OK)
        status=musa_mc2_encode(scene,data,length,&length);
    musa_mc2_scene_destroy(scene);
    if(status!=MUSA_MC2_OK){
        if(data){memset(data,0,length);free(data);}
        if(!(*env)->ExceptionCheck(env))check(env,status);
        return NULL;
    }
    jbyteArray result=(*env)->NewByteArray(env,(jsize)length);
    if(result)(*env)->SetByteArrayRegion(env,result,0,(jsize)length,
                                       (const jbyte *)data);
    memset(data,0,length);free(data);
    return result;
}
