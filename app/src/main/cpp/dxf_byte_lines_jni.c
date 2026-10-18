/* SPDX-License-Identifier: GPL-3.0-or-later */
/* JNI transports byte buffers; no DXF framing policy is duplicated here. */
#include <jni.h>
#include <stdint.h>
#include <stddef.h>
#include "musacad_dxf_byte_lines.h"

static void io_error(JNIEnv *env,const char *message){
    jclass cls=(*env)->FindClass(env,"java/io/IOException");
    if(cls){(*env)->ThrowNew(env,cls,message);(*env)->DeleteLocalRef(env,cls);}
}
JNIEXPORT jlong JNICALL
Java_com_musa_cad_DxfPortableLines_openNative(JNIEnv *env,jclass cls,jint maximum){
    (void)env;(void)cls;
    if(maximum<1||maximum>1048576)return (jlong)0;
    return (jlong)(intptr_t)musa_core_dxf_byte_lines_create((size_t)maximum);
}
static jbyteArray make_line(JNIEnv *env,const unsigned char *bytes,size_t length){
    if(length>1048576u)return NULL;
    jbyteArray output=(*env)->NewByteArray(env,(jsize)length);
    if(!output)return NULL;
    if(length)(*env)->SetByteArrayRegion(env,output,0,(jsize)length,
                                         (const jbyte *)bytes);
    return output;
}
JNIEXPORT jbyteArray JNICALL
Java_com_musa_cad_DxfPortableLines_nextNative(JNIEnv *env,jclass cls,
    jlong context,jbyteArray input,jintArray offset,jint end){
    (void)cls;
    if(!context||!input||!offset||(*env)->GetArrayLength(env,offset)!=1){
        io_error(env,"Geçersiz ortak DXF satır durumu");
        return NULL;
    }
    jint pos=0;
    (*env)->GetIntArrayRegion(env,offset,0,1,&pos);
    if((*env)->ExceptionCheck(env))return NULL;
    jsize size=(*env)->GetArrayLength(env,input);
    if(pos<0||end<pos||end>size){
        io_error(env,"Geçersiz DXF satır aralığı");
        return NULL;
    }
    const jbyte *data=(*env)->GetByteArrayElements(env,input,NULL);
    if(!data)return NULL;
    size_t consumed=0,line_size=0;
    const unsigned char *line=NULL;
    int status=musa_core_dxf_byte_lines_step(
        (MusaDxfByteLines *)(intptr_t)context,
        (const unsigned char *)data+pos,(size_t)(end-pos),
        &consumed,&line,&line_size);
    (*env)->ReleaseByteArrayElements(env,input,(jbyte *)data,JNI_ABORT);
    if(status<0){
        io_error(env,status==MUSA_DXF_BYTE_TOO_LONG?
            "DXF grup satırı fazla uzun":
            status==MUSA_DXF_BYTE_NO_MEMORY?
            "DXF satır okuyucu belleği yetersiz":
            "Ortak DXF satır okuyucusu geçersiz");
        return NULL;
    }
    pos+=(jint)consumed;
    (*env)->SetIntArrayRegion(env,offset,0,1,&pos);
    if((*env)->ExceptionCheck(env))return NULL;
    return status==MUSA_DXF_BYTE_LINE?make_line(env,line,line_size):NULL;
}
JNIEXPORT jbyteArray JNICALL
Java_com_musa_cad_DxfPortableLines_finishNative(JNIEnv *env,jclass cls,
    jlong context){
    (void)cls;
    if(!context){
        io_error(env,"Ortak DXF satır okuyucusu yok");
        return NULL;
    }
    const unsigned char *line=NULL;
    size_t bytes=0;
    int status=musa_core_dxf_byte_lines_finish(
        (MusaDxfByteLines *)(intptr_t)context,&line,&bytes);
    if(status<0){
        io_error(env,"Ortak DXF satır okuyucusu bitiş hatası");
        return NULL;
    }
    return status==MUSA_DXF_BYTE_LINE?make_line(env,line,bytes):NULL;
}
JNIEXPORT void JNICALL
Java_com_musa_cad_DxfPortableLines_closeNative(JNIEnv *env,jclass cls,
    jlong context){
    (void)env;(void)cls;
    if(context)musa_core_dxf_byte_lines_destroy(
        (MusaDxfByteLines *)(intptr_t)context);
}
