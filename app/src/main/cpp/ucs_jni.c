/* SPDX-License-Identifier: GPL-3.0-or-later */
/* JNI data marshalling only: all 3D UCS math runs in the common C core. */
#include <jni.h>
#include <string.h>
#include "musacad_ucs.h"
static void invalid(JNIEnv *env,const char *reason){
    if((*env)->ExceptionCheck(env))return;
    jclass err=(*env)->FindClass(env,"java/lang/IllegalArgumentException");
    if(err)(*env)->ThrowNew(env,err,reason);
}
static jdoubleArray publish(JNIEnv *env,const double *coords,jsize n){
    jdoubleArray ret=(*env)->NewDoubleArray(env,n);
    if(ret)(*env)->SetDoubleArrayRegion(env,ret,0,n,coords);
    return ret;
}
static int readFrame(JNIEnv *env,jdoubleArray packed,MusaCoreUcsFrame *out){
    if(!packed||!out||(*env)->GetArrayLength(env,packed)!=12)return 0;
    double values[12];
    (*env)->GetDoubleArrayRegion(env,packed,0,12,values);
    if((*env)->ExceptionCheck(env))return 0;
    memcpy(out->origin,values,3*sizeof(double));
    memcpy(out->axis,values+3,9*sizeof(double));
    return 1;
}
JNIEXPORT jdoubleArray JNICALL
Java_com_musa_cad_CadUcsFrame_createNative(JNIEnv *env,jclass type,jdoubleArray input){
    (void)type;
    if(!input||(*env)->GetArrayLength(env,input)!=9){
        invalid(env,"UCS requires 9 coordinates");return NULL;
    }
    double nine[9];
    (*env)->GetDoubleArrayRegion(env,input,0,9,nine);
    if((*env)->ExceptionCheck(env))return NULL;
    MusaCoreUcsFrame frame;
    if(!musa_core_ucs_create(nine,&frame)){
        invalid(env,"Invalid UCS origin or axes");return NULL;
    }
    double twelve[12];
    memcpy(twelve,frame.origin,3*sizeof(double));
    memcpy(twelve+3,frame.axis,9*sizeof(double));
    return publish(env,twelve,12);
}
static jdoubleArray convert(JNIEnv *env,jdoubleArray packed,
                            jdouble x,jdouble y,jdouble z,int to_world){
    MusaCoreUcsFrame frame;
    const double point[3]={x,y,z};
    double output[3];
    if(!readFrame(env,packed,&frame)||
       !(to_world?musa_core_ucs_to_world(&frame,point,output):
                  musa_core_ucs_to_local(&frame,point,output))){
        invalid(env,"Invalid native UCS transform");return NULL;
    }
    return publish(env,output,3);
}
JNIEXPORT jdoubleArray JNICALL
Java_com_musa_cad_CadUcsFrame_toWorldNative(
        JNIEnv *env,jclass type,jdoubleArray packed,jdouble x,jdouble y,jdouble z){
    (void)type;return convert(env,packed,x,y,z,1);
}
JNIEXPORT jdoubleArray JNICALL
Java_com_musa_cad_CadUcsFrame_toLocalNative(
        JNIEnv *env,jclass type,jdoubleArray packed,jdouble x,jdouble y,jdouble z){
    (void)type;return convert(env,packed,x,y,z,0);
}
JNIEXPORT jdoubleArray JNICALL
Java_com_musa_cad_CadUcsFrame_moveNative(
        JNIEnv *env,jclass type,jdoubleArray packed,jint axis,jdouble distance){
    (void)type;
    MusaCoreUcsFrame frame;
    double output[3];
    if(!readFrame(env,packed,&frame)||
       !musa_core_ucs_move(&frame,axis,distance,output)){
        invalid(env,"Invalid native UCS axis displacement");return NULL;
    }
    return publish(env,output,3);
}
