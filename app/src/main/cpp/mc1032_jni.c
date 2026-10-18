/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Android adapter ONLY. Native first-party C core owns all MC1032 codec
 * semantics. This JNI transfers an exact [handle,kind,7x IEEE754 bits]
 * representation without any Java file-format or geometry implementation.
 */
#include <jni.h>
#include "musacad_mc1032.h"
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

_Static_assert(sizeof(jlong)==8,"MC1032 JNI needs 64-bit long");
static void err(JNIEnv *env,const char *message){
    jclass cls=(*env)->FindClass(env,"java/io/IOException");
    if(cls){
        (*env)->ThrowNew(env,cls,message);
        (*env)->DeleteLocalRef(env,cls);
    }
}
static void codec_error(JNIEnv *env,int code){
    switch(code){
      case MUSA_MC1032_UNSUPPORTED:
        err(env,"MC1032: desteklenmeyen nesne veya bicim surumu");break;
      case MUSA_MC1032_CORRUPT:
        err(env,"MC1032: checksum, nesne kimligi veya dosya butunlugu hatasi");break;
      case MUSA_MC1032_LIMIT:
        err(env,"MC1032: dosya veya nesne sayisi siniri asildi");break;
      default:err(env,"MC1032: gecersiz veya ayrilamayan bagimsiz CAD belgesi");break;
    }
}
JNIEXPORT jlongArray JNICALL
Java_com_musa_cad_Mc1032Native_decodeNative(
        JNIEnv *env,jclass type,jbyteArray source){
    (void)type;
    if(!source){err(env,"MC1032: dosya verisi yok");return NULL;}
    const jsize len=(*env)->GetArrayLength(env,source);
    if(len<32||len>(jsize)MUSA_MC1032_MAX_BYTES){
        err(env,"MC1032: gecersiz dosya boyutu");return NULL;
    }
    jbyte *raw=(*env)->GetByteArrayElements(env,source,NULL);
    if(!raw)return NULL;
    MusaDocument *doc=NULL;
    const int status=musa_mc1032_decode((const unsigned char *)raw,(size_t)len,&doc);
    (*env)->ReleaseByteArrayElements(env,source,raw,JNI_ABORT);
    if(status!=MUSA_MC1032_OK){codec_error(env,status);return NULL;}
    const size_t count=musa_document_count(doc);
    jlongArray output=(*env)->NewLongArray(env,(jsize)(count*9u));
    if(!output){musa_document_destroy(doc);return NULL;}
    for(size_t i=0;i<count;i++){
        MusaDocumentEntity e={0};
        if(musa_document_get_index(doc,i,&e)!=MUSA_DOCUMENT_OK){
            musa_document_destroy(doc);
            err(env,"MC1032: belgede nesne kaydi okunamadi");return NULL;
        }
        jlong fields[9]={0};
        const double values[7]={e.x1,e.y1,e.x2,e.y2,e.radius,e.start_deg,e.end_deg};
        uint64_t bits=e.handle;
        memcpy(fields,&bits,8u);
        fields[1]=(jlong)e.kind;
        for(int j=0;j<7;j++){
            memcpy(&bits,&values[j],8u);
            memcpy(&fields[j+2],&bits,8u);
        }
        (*env)->SetLongArrayRegion(env,output,(jsize)(i*9u),9,fields);
        if((*env)->ExceptionCheck(env)){musa_document_destroy(doc);return NULL;}
    }
    musa_document_destroy(doc);
    return output;
}
JNIEXPORT jbyteArray JNICALL
Java_com_musa_cad_Mc1032Native_encodeNative(
        JNIEnv *env,jclass type,jlongArray packed){
    (void)type;
    if(!packed){err(env,"MC1032: bos nesne dizisi");return NULL;}
    const jsize entries=(*env)->GetArrayLength(env,packed);
    if(entries<0||(entries%9)!=0||(entries/9)>(jsize)MUSA_MC1032_MAX_ENTITIES){
        err(env,"MC1032: gecersiz nesne alanlari");return NULL;
    }
    size_t count=(size_t)(entries/9);
    MusaDocument *doc=musa_document_create(count?count:1u);
    if(!doc){err(env,"MC1032: belge olusturulamadi");return NULL;}
    const jlong *raw=(*env)->GetLongArrayElements(env,packed,NULL);
    if(!raw){musa_document_destroy(doc);return NULL;}
    int status=MUSA_MC1032_OK;
    for(size_t i=0;i<count;i++){
        const jlong *f=raw+i*9u;
        MusaDocumentEntity e={0};
        memcpy(&e.handle,&f[0],8u);
        if(f[1]<1||f[1]>4){status=MUSA_MC1032_UNSUPPORTED;break;}
        e.kind=(int)f[1];
        double *values[7]={&e.x1,&e.y1,&e.x2,&e.y2,
                           &e.radius,&e.start_deg,&e.end_deg};
        for(int j=0;j<7;j++)memcpy(values[j],&f[2+j],8u);
        if(e.handle==0u){status=MUSA_MC1032_INVALID;break;}
        uint64_t actual=0u;
        const int result=musa_document_load_entity(doc,&e,&actual);
        if(result!=MUSA_DOCUMENT_OK||actual!=e.handle){
            status=result==MUSA_DOCUMENT_NO_MEMORY?
               MUSA_MC1032_NO_MEMORY:MUSA_MC1032_INVALID;break;
        }
    }
    (*env)->ReleaseLongArrayElements(env,packed,(jlong *)raw,JNI_ABORT);
    if(status!=MUSA_MC1032_OK){
        musa_document_destroy(doc);codec_error(env,status);return NULL;
    }
    size_t len=0;
    status=musa_mc1032_encode(doc,NULL,0u,&len);
    if(status!=MUSA_MC1032_OK||len>MUSA_MC1032_MAX_BYTES){
        musa_document_destroy(doc);codec_error(env,status);return NULL;
    }
    unsigned char *data=(unsigned char *)malloc(len);
    if(!data){musa_document_destroy(doc);codec_error(env,MUSA_MC1032_NO_MEMORY);return NULL;}
    status=musa_mc1032_encode(doc,data,len,&len);
    musa_document_destroy(doc);
    if(status!=MUSA_MC1032_OK){
        memset(data,0,len);free(data);codec_error(env,status);return NULL;
    }
    jbyteArray output=(*env)->NewByteArray(env,(jsize)len);
    if(output)(*env)->SetByteArrayRegion(env,output,0,(jsize)len,(jbyte *)data);
    memset(data,0,len);free(data);
    return output;
}
