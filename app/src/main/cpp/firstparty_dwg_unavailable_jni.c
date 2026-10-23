/* SPDX-License-Identifier: GPL-3.0-or-later */
/*
 * First-party Android transition barrier.
 * No LibreDWG / ODA / external CAD SDK is loaded or called here.
 *
 * A PARTIAL original AC1032 primitive reader now exists as a
 * separate opt-in diagnostic preview. Full DWG document/owner/block
 * semantics required by normal file-open remain UNAVAILABLE; preserve
 * the IOException barrier and never replace full editable geometry with
 * an incomplete preview or fabricated DXF model.
 */
#include <jni.h>

static void unsupported(JNIEnv *env){
    jclass exception=(*env)->FindClass(env,"java/io/IOException");
    if(!exception)return;
    (*env)->ThrowNew(env,exception,
       "MusaCAD bagimsiz DWG cozumleyicisi henuz tamamlanmadi; "
       "AC1032 dosyasi donusturulmedi. Kaynak DWG degistirilmedi.");
    (*env)->DeleteLocalRef(env,exception);
}

JNIEXPORT jlong JNICALL
Java_com_musa_cad_NativeCadEngine_openNative(
    JNIEnv *env,jclass type,jstring path){
    (void)type;(void)path;
    unsupported(env);
    return (jlong)0;
}
JNIEXPORT jint JNICALL
Java_com_musa_cad_NativeCadEngine_exportDxfNative(
    JNIEnv *env,jclass type,jlong session,jstring path){
    (void)type;(void)session;(void)path;
    unsupported(env);
    return (jint)-1;
}
JNIEXPORT jlongArray JNICALL
Java_com_musa_cad_NativeCadEngine_statsNative(
    JNIEnv *env,jclass type,jlong session){
    (void)type;(void)session;
    unsupported(env);
    return NULL;
}
JNIEXPORT jfloatArray JNICALL
Java_com_musa_cad_NativeCadEngine_sceneNative(
    JNIEnv *env,jclass type,jlong session){
    (void)type;(void)session;
    unsupported(env);
    return NULL;
}
JNIEXPORT jlongArray JNICALL
Java_com_musa_cad_NativeCadEngine_modelIdentityPageNative(
    JNIEnv *env,jclass type,jlong session,jlong first,jint limit,
    jlongArray cursor){
    (void)type;(void)session;(void)first;(void)limit;(void)cursor;
    unsupported(env);
    return NULL;
}
JNIEXPORT jfloatArray JNICALL
Java_com_musa_cad_NativeCadEngine_scenePageNative(
    JNIEnv *env,jclass type,jlong session,jlong first,jint limit,
    jlongArray cursor){
    (void)type;(void)session;(void)first;(void)limit;(void)cursor;
    unsupported(env);
    return NULL;
}
JNIEXPORT jfloatArray JNICALL
Java_com_musa_cad_NativeCadEngine_sceneNestedPageNative(
    JNIEnv *env,jclass type,jlong session,jlong top,jlong leaf,
    jint limit,jlongArray cursor){
    (void)type;(void)session;(void)top;(void)leaf;(void)limit;(void)cursor;
    unsupported(env);
    return NULL;
}
JNIEXPORT void JNICALL
Java_com_musa_cad_NativeCadEngine_closeNative(
    JNIEnv *env,jclass type,jlong session){
    (void)env;(void)type;(void)session;
}
JNIEXPORT jint JNICALL
Java_com_musa_cad_NativeDwg_convertNative(
    JNIEnv *env,jclass type,jstring input,jstring output){
    (void)type;(void)input;(void)output;
    unsupported(env);
    return (jint)-1;
}
