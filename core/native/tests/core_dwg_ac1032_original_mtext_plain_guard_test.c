/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Genuine unchanged R2018 Text.dwg H1C0 is plain Unicode MTEXT.
 * Formatting syntax is NOT decoded: it must be omitted from provisional
 * v6 first paint, not shown as literal backslash control sequences. */
#include "musacad_dwg_ac1032_preview_v6.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static int preflight_cases(void){
    const char *accepted[]={
        "Plain Unicode MTEXT",
        "Çizim ölçü — Δοκιμή — 中文"
    };
    const char *rejected[]={
        "", "line\\Pnext", "color\\C1;red",
        "{\\fArial|b0;rich}", "x{group}y", "x}y",
        "line\nbreak", "tab\tstop", "raw\rreturn",
        "bad\xC0\xAF", "surrogate\xED\xA0\x80",
        "out-of-range\xF4\x90\x80\x80",
        "truncated\xE2\x82", "paragraph\xE2\x80\xA8",
        "separator\xE2\x80\xA9", "escaped\\~space"
    };
    for(size_t i=0u;i<sizeof(accepted)/sizeof(accepted[0]);i++){
        const size_t n=strlen(accepted[i]);
        if(!musa_dwg_ac1032_mtext_v6_plain_source_preflight(
            accepted[i],n))return 0;
    }
    for(size_t i=0u;i<sizeof(rejected)/sizeof(rejected[0]);i++){
        const size_t n=strlen(rejected[i]);
        if(musa_dwg_ac1032_mtext_v6_plain_source_preflight(
            rejected[i],n))return 0;
    }
    return !musa_dwg_ac1032_mtext_v6_plain_source_preflight(NULL,0u)&&
           !musa_dwg_ac1032_mtext_v6_plain_source_preflight("ok",1u)&&
           !musa_dwg_ac1032_mtext_v6_plain_source_preflight("ok",2048u);
}
static int genuine(const char *filename){
    FILE *f=fopen(filename,"rb");
    if(!f)return 0;
    if(fseek(f,0,SEEK_END)){fclose(f);return 0;}
    long n=ftell(f);
    if(n<256L||n>64L*1024L*1024L||fseek(f,0,SEEK_SET)){
        fclose(f);return 0;
    }
    uint8_t *src=(uint8_t*)malloc((size_t)n);
    uint8_t *saved=(uint8_t*)malloc((size_t)n);
    if(!src||!saved){free(src);free(saved);fclose(f);return 0;}
    int ok=fread(src,1u,(size_t)n,f)==(size_t)n;
    if(fclose(f))ok=0;
    if(ok)memcpy(saved,src,(size_t)n);
    ok=ok&&!memcmp(src,"AC1032",6u);
    MusaDwgAc1032SceneIndex *scene=NULL;
    if(ok)ok=musa_dwg_ac1032_scene_index_open(
        src,(size_t)n,&scene)&&!memcmp(src,saved,(size_t)n);
    free(src);free(saved);
    if(!ok)return 0;
    MusaDwgAc1032MTextSource m={0};
    MusaAc1032PreviewV6Stats old={0},now={0};
    /* Independent AutoCAD Text.txt oracle: this is AcDbMText H1C0,
     * NOT an OT1 TEXT and not a rich-format paragraph fixture. */
    const char *oracle="This a multiline text to check if libredwg is able to read this in the current file format version";
    ok=musa_dwg_ac1032_scene_index_read_mtext_source(
          scene,UINT64_C(0x1c0),&m)&&
       m.original_record_crc_verified&&m.indexed_handle==UINT64_C(0x1c0)&&
       !strcmp(m.utf8,oracle)&&m.utf8_bytes==strlen(oracle)&&
       musa_dwg_ac1032_mtext_v6_plain_source_preflight(
          m.utf8,m.utf8_bytes)&&
       musa_dwg_ac1032_preview_v6_planar_source_aci(
          scene,NULL,0u,&old)&&
       musa_dwg_ac1032_preview_v6_planar_source_with_mtext(
          scene,NULL,0u,&now)&&
       now.rendered_primitives>old.rendered_primitives&&
       now.partial_geometry&&now.provisional_aci_style;
    fprintf(stderr,"%s genuine R2018 OT44 H1C0 plain Unicode admitted; "
        "formatted MTEXT safely omitted, full paragraph/font/owner/viewport unresolved\n",
        ok?"PASS":"FAIL");
    musa_dwg_ac1032_scene_index_close(scene);
    return ok;
}
int main(int argc,char **argv){
    if(!preflight_cases())return 1;
    if(argc==1)return 0;
    if(argc!=2)return 2;
    return genuine(argv[1])?0:1;
}
