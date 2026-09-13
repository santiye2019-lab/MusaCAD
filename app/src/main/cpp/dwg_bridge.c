/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include "dwg.h"
#include "bits.h"
#include "out_dxf.h"

/* Kept common to host smoke tests and Android JNI. Negative codes are fatal. */
static int convert_dwg(const char *input,const char *output) {
    Dwg_Data *dwg=calloc(1,sizeof(Dwg_Data));
    Bit_Chain data={0};
    if(!dwg)return -1;
    int read_error=dwg_read_file(input,dwg);
    if(read_error>=DWG_ERR_CRITICAL){dwg_free(dwg);free(dwg);return -2;}
    data.version=dwg->header.version;
    data.from_version=dwg->header.from_version;
    data.fh=fopen(output,"wb");
    if(!data.fh){dwg_free(dwg);free(dwg);return -3;}
    int write_error=dwg_write_dxf(&data,dwg);
    int io_error=ferror(data.fh);
    if(fclose(data.fh)!=0)io_error=1;
    dwg_free(dwg);free(dwg);
    if(io_error||write_error>=DWG_ERR_CRITICAL){remove(output);return -4;}
    return read_error|write_error;
}
#ifdef MUSA_HOST
int main(int argc,char **argv){
    if(argc!=3)return 2;
    int result=convert_dwg(argv[1],argv[2]);
    fprintf(stderr,"MusaCAD converter status: %d\n",result);
    return result<0?1:0;
}
#else
#include <jni.h>
#include <pthread.h>
static pthread_mutex_t engine_lock=PTHREAD_MUTEX_INITIALIZER;
JNIEXPORT jint JNICALL Java_com_musa_cad_NativeDwg_convertNative(JNIEnv *env,jclass cls,jstring input,jstring output){
    (void)cls;
    const char *in=(*env)->GetStringUTFChars(env,input,NULL);
    if(!in)return -1;
    const char *out=(*env)->GetStringUTFChars(env,output,NULL);
    if(!out){(*env)->ReleaseStringUTFChars(env,input,in);return -1;}
    pthread_mutex_lock(&engine_lock);
    int result=convert_dwg(in,out);
    pthread_mutex_unlock(&engine_lock);
    (*env)->ReleaseStringUTFChars(env,input,in);
    (*env)->ReleaseStringUTFChars(env,output,out);
    return result;
}
#endif
