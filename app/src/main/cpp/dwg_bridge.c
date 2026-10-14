/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include "dwg.h"
#include "dwg_api.h"
#include "bits.h"
#include "out_dxf.h"
#include "native_scene.h"

typedef struct {
    Dwg_Data *dwg;
    int read_status;
    /* Optional bounded model-space index; never owns Dwg_Object storage. */
    Dwg_Object **model_entities;
    size_t model_count;
    int model_index_state; /* 0 uninitialized, 1 ready, -1 fail/unsupported */
    /* At most one bounded root expansion at a time; never owns Dwg_Object. */
    MusaSceneLeafContext *root_leaf_contexts;
    size_t root_leaf_count,root_leaf_top;
    Dwg_Object *root_leaf_object;
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

/*
 * Build the pointer index once per live DWG session, without copying geometry.
 * The maximum is a strict allocation guard. If unavailable, pages use the
 * previous linear owner traversal without loss of data. Always called under
 * engine_lock on Android; host probing is single-threaded.
 */
#define MUSA_MAX_MODEL_INDEX_ENTITIES 250000u
static int session_ensure_model_index(MusaCadSession *session){
    if(!session||!session->dwg)return 0;
    if(session->model_index_state!=0)return session->model_index_state==1;
    Dwg_Object_Ref *model=dwg_model_space_ref(session->dwg);
    Dwg_Object *owner=model?model->obj:NULL;
    if(!owner){session->model_index_state=-1;return 0;}
    size_t cap=1024,count=0;
    Dwg_Object **entries=malloc(cap*sizeof(*entries));
    if(!entries){session->model_index_state=-1;return 0;}
    Dwg_Object *obj=get_first_owned_entity(owner);
    while(obj){
        if(count>=MUSA_MAX_MODEL_INDEX_ENTITIES){
            free(entries);
            session->model_index_state=-1;
            return 0;
        }
        if(count==cap){
            size_t grown=cap*2;
            if(grown>MUSA_MAX_MODEL_INDEX_ENTITIES)
                grown=MUSA_MAX_MODEL_INDEX_ENTITIES;
            Dwg_Object **resized=realloc(entries,grown*sizeof(*entries));
            if(!resized){free(entries);session->model_index_state=-1;return 0;}
            entries=resized;cap=grown;
        }
        entries[count++]=obj;
        obj=get_next_owned_entity(owner,obj);
    }
    if(!count){free(entries);session->model_index_state=-1;return 0;}
    session->model_entities=entries;
    session->model_count=count;
    session->model_index_state=1;
    return 1;
}

/*
 * Materialize up to 65,536 exact LEAF INSTANCES from the same live DWG.
 * Their affine, inherited color and depth completely define each primitive
 * emission. Failed/too-large caches automatically use ordinary nested replay.
 */
static int session_ensure_root_leaf_contexts(MusaCadSession *session,
                                              size_t top,size_t leaf){
    if(!session||session->model_index_state!=1||
       top>=session->model_count||!leaf)return 0;
    Dwg_Object *root=session->model_entities[top];
    if(session->root_leaf_object==root&&session->root_leaf_top==top)
        return leaf<session->root_leaf_count;
    free(session->root_leaf_contexts);
    session->root_leaf_contexts=NULL;
    session->root_leaf_count=0;
    session->root_leaf_object=NULL;
    MusaSceneLeafContext *contexts=NULL;
    size_t count=0;
    if(!musa_scene_collect_root_leaves(root,&contexts,&count)||leaf>=count){
        free(contexts);
        return 0;
    }
    session->root_leaf_contexts=contexts;
    session->root_leaf_count=count;
    session->root_leaf_object=root;
    session->root_leaf_top=top;
    return 1;
}

static void session_close(MusaCadSession *session) {
    if(!session)return;
    free(session->model_entities);
    free(session->root_leaf_contexts);
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

static int convert_dwg(const char *input,const char *output) {
    int read_error=0;
    MusaCadSession *session=session_open(input,&read_error);
    if(!session)return -2;
    int result=session_export_dxf(session,output);
    session_close(session);
    return result;
}

#ifdef MUSA_HOST
/*
 * Non-production paged-scene acceptance probe. All drawing bytes remain local.
 * The legacy scene is built only for baseline count comparison on this test.
 */
static int host_paged_probe(const char *input,const char *budget){
    char *end=NULL;
    unsigned long long n=strtoull(budget,&end,10);
    if(!budget[0]||!end||*end||n<256||n>(9u*1024u*1024u))return 2;
    int read_error=0;
    MusaCadSession *session=session_open(input,&read_error);
    if(!session){fprintf(stderr,"Paged probe open failed: %d\n",read_error);return 1;}
    MusaNativeScene full={0};
    int full_status=musa_scene_build(session->dwg,&full);
    const int full_count=full.primitives;
    size_t cursor=0,total=0,pages=0;
    int error=0;
    while(pages<100000){
        MusaNativeScene page={0};
        size_t next=cursor;
        int status=musa_scene_build_page(session->dwg,&page,cursor,&next,(size_t)n);
        if(status<0||next<=cursor||page.count>(size_t)n||page.truncated){
            fprintf(stderr,"Paged probe rejected page=%zu cursor=%zu status=%d next=%zu floats=%zu\n",
                    pages,cursor,status,next,page.count);
            musa_scene_free(&page);error=1;break;
        }
        total+=(size_t)page.primitives;
        fprintf(stderr,"Paged scene: page=%zu start=%zu next=%zu primitives=%d floats=%zu more=%d\n",
                pages,cursor,next,page.primitives,page.count,status==1);
        musa_scene_free(&page);
        ++pages;cursor=next;
        if(status==0)break;
    }
    if(pages>=100000||full_status<0||(full_status==0&&total!=(size_t)full_count)||
       (full_status==1&&total<(size_t)full_count)){
        fprintf(stderr,"Paged probe count verification failed: full_status=%d full=%d pages=%zu total=%zu\n",
                full_status,full_count,pages,total);
        error=1;
    }
    if(!error)fprintf(stderr,"Paged scene verified: pages=%zu primitives=%zu legacy_count=%d legacy_status=%d\n",
                      pages,total,full_count,full_status);
    musa_scene_free(&full);
    session_close(session);
    return error?1:0;
}

/*
 * Opt-in CI/private acceptance for nested INSERT continuation. The legacy
 * capped scene provides a lower-bound count when it was itself truncated.
 */
static int host_nested_page_probe(const char *input,const char *budget,int indexed_mode,int context_mode){
    char *end=NULL;
    unsigned long long n=strtoull(budget,&end,10);
    if(!budget[0]||!end||*end||n<256||n>(9u*1024u*1024u))return 2;
    int read_error=0;
    MusaCadSession *session=session_open(input,&read_error);
    if(!session){fprintf(stderr,"Nested probe open failed: %d\n",read_error);return 1;}
    if(indexed_mode&&!session_ensure_model_index(session)){
        fprintf(stderr,"Model index unavailable (fallback required)\n");
        session_close(session);
        return 1;
    }
    if(indexed_mode)fprintf(stderr,"Indexed model entities=%zu\n",session->model_count);
    MusaNativeScene baseline={0};
    int baseline_status=musa_scene_build(session->dwg,&baseline);
    int baseline_count=baseline.primitives;
    size_t top=0,leaf=0,total=0,pages=0,stream_at=0;
    int failure=0,finished=0;
    while(pages<100000){
        MusaNativeScene page={0};
        size_t next_top=top,next_leaf=leaf;
        int has_context=context_mode&&session_ensure_root_leaf_contexts(session,top,leaf);
        if(has_context)fprintf(stderr,
            "Exact leaf-instance context used top=%zu leaf=%zu instances=%zu\n",
            top,leaf,session->root_leaf_count);
        int status=has_context
                ?musa_scene_build_page_nested_cached(session->dwg,&page,top,leaf,
                     &next_top,&next_leaf,(size_t)n,
                     session->model_entities,session->model_count,top,
                     session->root_leaf_object,session->root_leaf_contexts,
                     session->root_leaf_count)
                :indexed_mode
                ?musa_scene_build_page_nested_indexed(session->dwg,&page,top,leaf,
                     &next_top,&next_leaf,(size_t)n,
                     session->model_entities,session->model_count)
                :musa_scene_build_page_nested(session->dwg,&page,top,leaf,
                     &next_top,&next_leaf,(size_t)n);
        if(status<0||page.truncated||page.count>(size_t)n||
           (status==1 && (next_top<top||
             (next_top==top&&next_leaf<=leaf))) ||
           (status==0&&next_top<top)){
            fprintf(stderr,"Nested scene rejected page=%zu top=%zu leaf=%zu status=%d next=%zu:%zu words=%zu\n",
                    pages,top,leaf,status,next_top,next_leaf,page.count);
            failure=1;
            musa_scene_free(&page);
            break;
        }
        total+=(size_t)page.primitives;
        /* Bit-for-bit comparison of geometry, color metadata and record order
         * on an untruncated baseline. Header bounds/counters are per-page. */
        if(baseline_status==0){
            const size_t words=page.count>=7?page.count-7:0;
            const size_t reference=baseline.count>=7?baseline.count-7:0;
            if(stream_at>reference||words>reference-stream_at||
               memcmp(baseline.values+7+stream_at,page.values+7,
                      words*sizeof(float))!=0){
                fprintf(stderr,"Nested stream fidelity mismatch page=%zu at=%zu words=%zu\n",
                        pages,stream_at,words);
                failure=1;
            }
            if(!failure)stream_at+=words;
        }
        fprintf(stderr,"Nested scene page=%zu top=%zu leaf=%zu next=%zu:%zu primitives=%d words=%zu more=%d\n",
                pages,top,leaf,next_top,next_leaf,page.primitives,page.count,status==1);
        musa_scene_free(&page);
        if(failure)break;
        top=next_top;leaf=next_leaf;++pages;
        if(status==0){finished=1;break;}
    }
    if(!finished||baseline_status<0||
       (baseline_status==0 && (total!=(size_t)baseline_count||
          stream_at!=(baseline.count>=7?baseline.count-7:0)))||
       (baseline_status==1 && total<(size_t)baseline_count))failure=1;
    if(!failure)fprintf(stderr,"%sNested paged scene verified: pages=%zu primitives=%zu legacy_count=%d legacy_status=%d\n",
                        context_mode?"Context ":indexed_mode?"Indexed ":"",
                        pages,total,baseline_count,baseline_status);
    else fprintf(stderr,"Nested paged scene failed: finished=%d pages=%zu total=%zu legacy=%d status=%d\n",
                 finished,pages,total,baseline_count,baseline_status);
    musa_scene_free(&baseline);
    session_close(session);
    return failure?1:0;
}

int main(int argc,char **argv){
    if(argc==4&&strcmp(argv[1],"--context-nested-paged-probe")==0)
        return host_nested_page_probe(argv[2],argv[3],1,1);
    if(argc==4&&strcmp(argv[1],"--indexed-nested-paged-probe")==0)
        return host_nested_page_probe(argv[2],argv[3],1,0);
    if(argc==4&&strcmp(argv[1],"--nested-paged-probe")==0)
        return host_nested_page_probe(argv[2],argv[3],0,0);
    if(argc==4&&strcmp(argv[1],"--paged-probe")==0)
        return host_paged_probe(argv[2],argv[3]);
    if(argc!=3)return 2;
    int read_error=0;
    MusaCadSession *session=session_open(argv[1],&read_error);
    if(!session){fprintf(stderr,"MusaCAD native open failed: %d\n",read_error);return 1;}
    MusaNativeScene scene={0};
    int scene_status=musa_scene_build(session->dwg,&scene);
    fprintf(stderr,"MusaCAD native scene: status=%d primitives=%d floats=%zu\n",scene_status,scene.primitives,scene.count);
    if(scene_status<0||scene.primitives<=0){musa_scene_free(&scene);session_close(session);return 1;}
    musa_scene_free(&scene);
    int result=session_export_dxf(session,argv[2]);
    session_close(session);
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

JNIEXPORT jfloatArray JNICALL Java_com_musa_cad_NativeCadEngine_sceneNative(JNIEnv *env,jclass cls,jlong handle){
    (void)cls;
    MusaCadSession *session=(MusaCadSession*)(intptr_t)handle;
    if(!session)return NULL;
    MusaNativeScene scene={0};
    pthread_mutex_lock(&engine_lock);
    int status=musa_scene_build(session->dwg,&scene);
    pthread_mutex_unlock(&engine_lock);
    if(status<0||scene.count==0||scene.count>(size_t)2147483647){musa_scene_free(&scene);return NULL;}
    jfloatArray out=(*env)->NewFloatArray(env,(jsize)scene.count);
    if(out)(*env)->SetFloatArrayRegion(env,out,0,(jsize)scene.count,(const jfloat*)scene.values);
    musa_scene_free(&scene);
    return out;
}

/*
 * Opt-in paged preview transport. Existing sceneNative stays untouched.
 * Each returned float array holds exactly one complete top-level-entity page.
 * out[0] is next top-level cursor, out[1] is hasMore (1/0).
 */
JNIEXPORT jfloatArray JNICALL Java_com_musa_cad_NativeCadEngine_scenePageNative(
        JNIEnv *env,jclass cls,jlong handle,jlong start,jint maxWords,jlongArray nextOut){
    (void)cls;
    MusaCadSession *session=(MusaCadSession*)(intptr_t)handle;
    if(!session||start<0||maxWords<256||maxWords>9*1024*1024||
       !nextOut||(*env)->GetArrayLength(env,nextOut)<2){
        throw_io(env,"DWG sahne sayfası parametreleri geçersiz");
        return NULL;
    }
    MusaNativeScene page={0};
    size_t next=(size_t)start;
    pthread_mutex_lock(&engine_lock);
    int status=musa_scene_build_page(session->dwg,&page,(size_t)start,&next,(size_t)maxWords);
    pthread_mutex_unlock(&engine_lock);
    if(status<0||next<=(size_t)start||page.count>(size_t)maxWords||
       page.count>(size_t)2147483647||page.truncated){
        musa_scene_free(&page);
        char message[120];
        snprintf(message,sizeof(message),"DWG sahne sayfası üretilemedi (%d)",status);
        throw_io(env,message);
        return NULL;
    }
    jlong result[2]={(jlong)next,(jlong)(status==1?1:0)};
    jfloatArray raw=(*env)->NewFloatArray(env,(jsize)page.count);
    if(raw){
        (*env)->SetFloatArrayRegion(env,raw,0,(jsize)page.count,(const jfloat*)page.values);
        if(!(*env)->ExceptionCheck(env))
            (*env)->SetLongArrayRegion(env,nextOut,0,2,result);
    }
    musa_scene_free(&page);
    return raw;
}

/*
 * EXPERIMENTAL opt-in nested paging with a two-component continuation cursor.
 * JNI and Java do not enable it in the production drawing view yet.
 */
JNIEXPORT jfloatArray JNICALL Java_com_musa_cad_NativeCadEngine_sceneNestedPageNative(
        JNIEnv *env,jclass cls,jlong handle,jlong startTop,jlong startLeaf,
        jint maxWords,jlongArray nextOut){
    (void)cls;
    MusaCadSession *session=(MusaCadSession*)(intptr_t)handle;
    if(!session||startTop<0||startLeaf<0||maxWords<256||
       maxWords>9*1024*1024||!nextOut||(*env)->GetArrayLength(env,nextOut)<3){
        throw_io(env,"İç blok DWG sahne sayfası parametreleri geçersiz");
        return NULL;
    }
    MusaNativeScene page={0};
    size_t nextTop=(size_t)startTop,nextLeaf=(size_t)startLeaf;
    pthread_mutex_lock(&engine_lock);
    int use_index=session_ensure_model_index(session);
    int use_context=use_index&&session_ensure_root_leaf_contexts(
        session,(size_t)startTop,(size_t)startLeaf);
    int status=use_context
        ?musa_scene_build_page_nested_cached(session->dwg,&page,
          (size_t)startTop,(size_t)startLeaf,&nextTop,&nextLeaf,(size_t)maxWords,
          session->model_entities,session->model_count,
          (size_t)startTop,session->root_leaf_object,
          session->root_leaf_contexts,session->root_leaf_count)
        :use_index
        ?musa_scene_build_page_nested_indexed(session->dwg,&page,
          (size_t)startTop,(size_t)startLeaf,&nextTop,&nextLeaf,(size_t)maxWords,
          session->model_entities,session->model_count)
        :musa_scene_build_page_nested(session->dwg,&page,
          (size_t)startTop,(size_t)startLeaf,&nextTop,&nextLeaf,(size_t)maxWords);
    pthread_mutex_unlock(&engine_lock);
    if(status<0||page.truncated||page.count>(size_t)maxWords||
       page.count>(size_t)2147483647||
       nextTop<(size_t)startTop||
       (nextTop==(size_t)startTop&&nextLeaf<=(size_t)startLeaf)){
        musa_scene_free(&page);
        char message[128];
        snprintf(message,sizeof(message),"İç blok DWG sahne sayfası üretilemedi (%d)",status);
        throw_io(env,message);
        return NULL;
    }
    jlong result[3]={(jlong)nextTop,(jlong)nextLeaf,(jlong)(status==1)};
    jfloatArray raw=(*env)->NewFloatArray(env,(jsize)page.count);
    if(raw){
        (*env)->SetFloatArrayRegion(env,raw,0,(jsize)page.count,(const jfloat*)page.values);
        if(!(*env)->ExceptionCheck(env))
            (*env)->SetLongArrayRegion(env,nextOut,0,3,result);
    }
    musa_scene_free(&page);
    return raw;
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
