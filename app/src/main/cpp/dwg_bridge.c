/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include "dwg.h"
#include "bits.h"
#include "out_dxf.h"

/*
 * MusaCAD native engine session.
 *
 * Phase 1 keeps a parsed Dwg_Data object behind an opaque handle.  The stable
 * application path still exports DXF, but the same session API is the base for
 * direct viewport/entity queries in later phases without changing Java tools.
 */
typedef struct {
    Dwg_Data *dwg;
    int read_status;
} MusaCadSession;

typedef struct {
    long long object_count;
    long long entity_count;
    long long core_entity_count;
    long long unsupported_entity_count;
    long long line_count;
    long long circle_count;
    long long arc_count;
    long long lwpolyline_count;
    long long polyline2d_count;
    long long point_count;
    long long version;
} MusaCadStats;

static MusaCadSession *session_open(const char *input,int *error_out) {
    if(error_out)*error_out=-1;
    if(!input)return NULL;
    MusaCadSession *session=calloc(1,sizeof(MusaCadSession));
    if(!session)return NULL;
    session->dwg=calloc(1,sizeof(Dwg_Data));
    if(!session->dwg){free(session);return NULL;}
    session->read_status=dwg_read_file(input,session->dwg);
    if(session->read_status>=DWG_ERR_CRITICAL){
        if(error_out)*error_out=session->read_status;
        dwg_free(session->dwg);free(session->dwg);free(session);
        return NULL;
    }
    if(error_out)*error_out=session->read_status;
    return session;
}

static void session_close(MusaCadSession *session) {
    if(!session)return;
    if(session->dwg){dwg_free(session->dwg);free(session->dwg);}
    free(session);
}

static int session_export_dxf(MusaCadSession *session,const char *output) {
    if(!session||!session->dwg||!output)return -1;
    Bit_Chain data={0};
    data.version=session->dwg->header.version;
    data.from_version=session->dwg->header.from_version;
    data.fh=fopen(output,"wb");
    if(!data.fh)return -3;
    int write_error=dwg_write_dxf(&data,session->dwg);
    int io_error=ferror(data.fh);
    if(fclose(data.fh)!=0)io_error=1;
    if(io_error||write_error>=DWG_ERR_CRITICAL){remove(output);return -4;}
    return session->read_status|write_error;
}

static MusaCadStats session_stats(MusaCadSession *session) {
    MusaCadStats stats={0};
    if(!session||!session->dwg)return stats;
    Dwg_Data *dwg=session->dwg;
    stats.object_count=(long long)dwg->num_objects;
    stats.version=(long long)dwg->header.version;
    for(BITCODE_BL i=0;i<dwg->num_objects;i++){
        Dwg_Object *obj=&dwg->object[i];
        if(obj->supertype==DWG_SUPERTYPE_OBJECT)continue;
        stats.entity_count++;
        switch(obj->fixedtype){
            case DWG_TYPE_LINE: stats.line_count++; break;
            case DWG_TYPE_CIRCLE: stats.circle_count++; break;
            case DWG_TYPE_ARC: stats.arc_count++; break;
            case DWG_TYPE_LWPOLYLINE: stats.lwpolyline_count++; break;
            case DWG_TYPE_POLYLINE_2D: stats.polyline2d_count++; break;
            case DWG_TYPE_POINT: stats.point_count++; break;
            default: break;
        }
    }
    stats.core_entity_count=stats.line_count+stats.circle_count+stats.arc_count+
        stats.lwpolyline_count+stats.polyline2d_count+stats.point_count;
    stats.unsupported_entity_count=stats.entity_count-stats.core_entity_count;
    return stats;
}

/* Kept common to host smoke tests and the legacy Android bridge. */
static int convert_dwg(const char *input,const char *output) {
    int read_error=0;
    MusaCadSession *session=session_open(input,&read_error);
    if(!session)return -2;
    int result=session_export_dxf(session,output);
    session_close(session);
    return result;
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

static void throw_io(JNIEnv *env,const char *message){
    jclass type=(*env)->FindClass(env,"java/io/IOException");
    if(type)(*env)->ThrowNew(env,type,message);
}

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

JNIEXPORT jlong JNICALL Java_com_musa_cad_NativeCadEngine_openNative(JNIEnv *env,jclass cls,jstring input){
    (void)cls;
    const char *in=(*env)->GetStringUTFChars(env,input,NULL);
    if(!in)return 0;
    int error=0;
    pthread_mutex_lock(&engine_lock);
    MusaCadSession *session=session_open(in,&error);
    pthread_mutex_unlock(&engine_lock);
    (*env)->ReleaseStringUTFChars(env,input,in);
    if(!session){
        char message[96];
        snprintf(message,sizeof(message),"DWG native oturumu acilamadi (%d)",error);
        throw_io(env,message);
        return 0;
    }
    return (jlong)(intptr_t)session;
}

JNIEXPORT jint JNICALL Java_com_musa_cad_NativeCadEngine_exportDxfNative(JNIEnv *env,jclass cls,jlong handle,jstring output){
    (void)cls;
    MusaCadSession *session=(MusaCadSession*)(intptr_t)handle;
    if(!session||!output)return -1;
    const char *out=(*env)->GetStringUTFChars(env,output,NULL);
    if(!out)return -1;
    pthread_mutex_lock(&engine_lock);
    int result=session_export_dxf(session,out);
    pthread_mutex_unlock(&engine_lock);
    (*env)->ReleaseStringUTFChars(env,output,out);
    return result;
}

JNIEXPORT jlongArray JNICALL Java_com_musa_cad_NativeCadEngine_statsNative(JNIEnv *env,jclass cls,jlong handle){
    (void)cls;
    MusaCadSession *session=(MusaCadSession*)(intptr_t)handle;
    if(!session)return NULL;
    pthread_mutex_lock(&engine_lock);
    MusaCadStats s=session_stats(session);
    int read_status=session->read_status;
    pthread_mutex_unlock(&engine_lock);
    jlong values[12]={
        (jlong)read_status,(jlong)s.object_count,(jlong)s.entity_count,
        (jlong)s.core_entity_count,(jlong)s.unsupported_entity_count,
        (jlong)s.line_count,(jlong)s.circle_count,(jlong)s.arc_count,
        (jlong)s.lwpolyline_count,(jlong)s.polyline2d_count,
        (jlong)s.point_count,(jlong)s.version
    };
    jlongArray out=(*env)->NewLongArray(env,12);
    if(out)(*env)->SetLongArrayRegion(env,out,0,12,values);
    return out;
}

JNIEXPORT void JNICALL Java_com_musa_cad_NativeCadEngine_closeNative(JNIEnv *env,jclass cls,jlong handle){
    (void)env;(void)cls;
    MusaCadSession *session=(MusaCadSession*)(intptr_t)handle;
    if(!session)return;
    pthread_mutex_lock(&engine_lock);
    session_close(session);
    pthread_mutex_unlock(&engine_lock);
}
#endif
