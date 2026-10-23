/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Authenticated whole indexed AC1032 record audit. No external DWG runtime.
 * Original AutoCAD 2018 reference files are supplied by CI, never altered.
 */
#include "musacad_dwg_ac1032_indexed_audit.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define MAX_FILE (64u*1024u*1024u)
static int load(const char *name,uint8_t **buf,size_t *sz){
    FILE *f=fopen(name,"rb");if(!f)return 0;
    if(fseek(f,0,SEEK_END)){fclose(f);return 0;}
    long n=ftell(f);
    if(n<256||n>(long)MAX_FILE||fseek(f,0,SEEK_SET)){
        fclose(f);return 0;
    }
    uint8_t *p=(uint8_t*)malloc((size_t)n);
    if(!p){fclose(f);return 0;}
    int ok=fread(p,1u,(size_t)n,f)==(size_t)n;
    if(fclose(f))ok=0;
    if(!ok){free(p);return 0;}
    *buf=p;*sz=(size_t)n;return 1;
}
static int one_original(const char *path,uint64_t h,uint16_t expected,
                        size_t minimum_dependents){
    uint8_t *file=NULL,*before=NULL;
    MusaDwgAc1032IndexedRecord *rows=NULL,*snapshot=NULL;
    size_t n=0u;
    int ok=load(path,&file,&n);
    if(!ok)return 0;
    before=(uint8_t*)malloc(n);
    if(!before){free(file);return 0;}
    memcpy(before,file,n);
    MusaDwgAc1032IndexedAudit report={0},again={0};
    ok=musa_dwg_ac1032_audit_indexed_objects_file(
        file,n,NULL,0u,&report)&&report.indexed_records>0u&&
        report.geometry_candidates>0u&&
        report.dependent_records>=minimum_dependents&&
        report.all_indexed_record_crcs_verified&&
        report.no_indexed_record_overlap&&
        report.indexed_records<=1000000u;
    if(ok){
        rows=(MusaDwgAc1032IndexedRecord*)malloc(
             report.indexed_records*sizeof(*rows));
        snapshot=(MusaDwgAc1032IndexedRecord*)malloc(
             report.indexed_records*sizeof(*snapshot));
        if(!rows||!snapshot)ok=0;
    }
    if(ok){
        ok=musa_dwg_ac1032_audit_indexed_objects_file(
             file,n,rows,report.indexed_records,&again)&&
           !memcmp(&report,&again,sizeof(report));
    }
    int seen=0;
    for(size_t i=0u;ok&&i<report.indexed_records;i++){
        if(!rows[i].record_crc_verified)ok=0;
        if(i&&rows[i].indexed_handle<=rows[i-1u].indexed_handle)ok=0;
        if(rows[i].indexed_handle==h){
            seen++;
            if(rows[i].raw_object_type!=expected||
               rows[i].category!=MUSA_AC1032_INDEXED_GEOMETRY_CANDIDATE)
                ok=0;
        }
    }
    if(ok&&!seen)ok=0;
    if(ok){
        memcpy(snapshot,rows,report.indexed_records*sizeof(*rows));
        const MusaDwgAc1032IndexedAudit saved=again;
        /* No partial success / buffer corruption when capacity is short. */
        ok=!musa_dwg_ac1032_audit_indexed_objects_file(
             file,n,rows,report.indexed_records-1u,&again)&&
           !memcmp(&again,&saved,sizeof(saved))&&
           !memcmp(snapshot,rows,report.indexed_records*sizeof(*rows));
        /* A malformed physical source cannot leave partial results. */
        const uint8_t first=file[0];
        file[0]^=1u;
        if(ok)ok=!musa_dwg_ac1032_audit_indexed_objects_file(
             file,n,rows,report.indexed_records,&again)&&
           !memcmp(&again,&saved,sizeof(saved))&&
           !memcmp(snapshot,rows,report.indexed_records*sizeof(*rows));
        file[0]=first;
    }
    if(ok)ok=!memcmp(file,before,n);
    fprintf(stderr,"%s original AC1032 indexed audit %s:"
            " records=%zu candidate=%zu dependent=%zu opaque=%zu"
            " target=0x%llX rawOT=%u\n",
            ok?"PASS":"FAIL",path,report.indexed_records,
            report.geometry_candidates,report.dependent_records,
            report.opaque_records,(unsigned long long)h,(unsigned)expected);
    free(snapshot);free(rows);free(before);free(file);
    return ok;
}
int main(int argc,char **argv){
    if(argc==1){
        uint8_t small[256]={0};
        MusaDwgAc1032IndexedAudit saved={0};
        saved.indexed_records=12345u;
        if(musa_dwg_ac1032_audit_indexed_objects_file(
             small,sizeof(small),NULL,0u,&saved)||
           saved.indexed_records!=12345u)return 1;
        memcpy(small,"AC1032",6u);
        if(musa_dwg_ac1032_audit_indexed_objects_file(
             small,sizeof(small),NULL,0u,&saved)||
           saved.indexed_records!=12345u)return 1;
        puts("PASS native indexed AC1032 negative/atomic smoke");
        return 0;
    }
    if(argc!=4)return 2;
    return (one_original(argv[1],UINT64_C(0x1ca),19u,0u)&&
            one_original(argv[2],UINT64_C(0x1bd),77u,0u)&&
            one_original(argv[3],UINT64_C(0x1c1),16u,6u))?0:1;
}
