/* SPDX-License-Identifier: GPL-3.0-or-later */
/* JNI is a byte transport only: both revision converters live exclusively
 * in the portable MusaCAD C core. No external CAD SDK, no second Java codec.
 */
#include <jni.h>
#include "musacad_mc1032_migrate.h"
#include "musacad_mc1032.h"
#include <stdlib.h>
#include <string.h>
static void fail(JNIEnv *env,int code){
    const char *msg=code==MUSA_MC2_UNSUPPORTED?
        "MC1032 kayipsiz gecis reddedildi: hedef surum geometriyi desteklemiyor":
        code==MUSA_MC2_CORRUPT?
        "MC1032 gecersiz veya bozuk kaynak dosya":
        code==MUSA_MC2_LIMIT?
        "MC1032 gecis boyut siniri":
        "MC1032 bagimsiz cekirdekte gecis basarisiz";
    jclass ex=(*env)->FindClass(env,"java/io/IOException");
    if(ex){(*env)->ThrowNew(env,ex,msg);(*env)->DeleteLocalRef(env,ex);}
}
static jbyteArray convert(JNIEnv *env,jbyteArray src,int to_v2){
    if(!src){fail(env,MUSA_MC2_INVALID);return NULL;}
    const jsize n=(*env)->GetArrayLength(env,src);
    if(n<0||(size_t)n>MUSA_MC2_MAX_BYTES){fail(env,MUSA_MC2_LIMIT);return NULL;}
    jbyte *data=(*env)->GetByteArrayElements(env,src,NULL);
    if(!data)return NULL;
    size_t bytes=0;
    int (*fn)(const unsigned char *,size_t,unsigned char *,size_t,size_t *)=
        to_v2?musa_mc1032_upgrade_v1_to_v2:
              musa_mc1032_downgrade_v2_to_v1;
    int status=fn((const unsigned char *)data,(size_t)n,NULL,0,&bytes);
    if(status!=MUSA_MC2_OK){
        (*env)->ReleaseByteArrayElements(env,src,data,JNI_ABORT);
        fail(env,status);return NULL;
    }
    if(!bytes||bytes>MUSA_MC2_MAX_BYTES){
        (*env)->ReleaseByteArrayElements(env,src,data,JNI_ABORT);
        fail(env,MUSA_MC2_LIMIT);return NULL;
    }
    unsigned char *buffer=(unsigned char *)malloc(bytes);
    if(!buffer){
        (*env)->ReleaseByteArrayElements(env,src,data,JNI_ABORT);
        fail(env,MUSA_MC2_NO_MEMORY);return NULL;
    }
    status=fn((const unsigned char *)data,(size_t)n,buffer,bytes,&bytes);
    (*env)->ReleaseByteArrayElements(env,src,data,JNI_ABORT);
    if(status!=MUSA_MC2_OK){
        memset(buffer,0,bytes);free(buffer);fail(env,status);return NULL;
    }
    jbyteArray output=(*env)->NewByteArray(env,(jsize)bytes);
    if(output)(*env)->SetByteArrayRegion(env,output,0,(jsize)bytes,(const jbyte *)buffer);
    memset(buffer,0,bytes);free(buffer);
    return output;
}
JNIEXPORT jbyteArray JNICALL
Java_com_musa_cad_Mc1032Migration_upgradeNative(
    JNIEnv *env,jclass cls,jbyteArray source){
    (void)cls;return convert(env,source,1);
}
JNIEXPORT jbyteArray JNICALL
Java_com_musa_cad_Mc1032Migration_downgradeNative(
    JNIEnv *env,jclass cls,jbyteArray source){
    (void)cls;return convert(env,source,0);
}
