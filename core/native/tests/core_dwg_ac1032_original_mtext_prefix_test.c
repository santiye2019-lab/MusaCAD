/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Original AutoCAD 2018/Text.dwg contains MTEXT H=1C0, never OT1 TEXT.
 * First-party C11 source evidence only, NOT a text/glyph renderer. */
#include "musacad_dwg_ac1032_object_payload.h"
#include "musacad_dwg_ac1032_object_streams.h"
#include "musacad_dwg_ac1032_entity_prefix.h"
#include "musacad_dwg_ac1015_values.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static int load(const char *path,uint8_t **out,size_t *n){
    FILE *f=fopen(path,"rb");
    if(!f)return 0;
    if(fseek(f,0,SEEK_END)!=0){fclose(f);return 0;}
    long bytes=ftell(f);
    if(bytes<256L||bytes>64L*1024L*1024L||
       fseek(f,0,SEEK_SET)!=0){fclose(f);return 0;}
    uint8_t *b=(uint8_t*)malloc((size_t)bytes);
    if(!b){fclose(f);return 0;}
    int ok=fread(b,1u,(size_t)bytes,f)==(size_t)bytes;
    if(fclose(f)!=0)ok=0;
    if(!ok){free(b);return 0;}
    *out=b;*n=(size_t)bytes;return 1;
}
int main(int argc,char **argv){
    if(argc!=2)return 2;
    uint8_t *file=NULL,*raw=NULL;size_t n=0u,need=0u,got=0u;
    if(!load(argv[1],&file,&n))return 1;
    MusaDwgAc1032PayloadInfo meta={0};
    MusaDwgAc1032ObjectStreams layout={0};
    const uint64_t h=UINT64_C(0x1c0);
    int ok=!memcmp(file,"AC1032",6u)&&
       musa_dwg_ac1032_object_payload_file(
           file,n,h,NULL,0u,&need,&meta)&&
       meta.original_record_crc_verified&&meta.raw_object_type==44u&&
       musa_dwg_ac1032_stream_layout_file(file,n,h,&layout)&&
       layout.original_record_crc_verified&&
       layout.raw_object_type==44u&&
       layout.total_payload_bits==(uint64_t)need*8u;
    if(!ok){fprintf(stderr,"FAIL original MTEXT H1C0 authenticated payload/layout\n");free(file);return 1;}
    raw=(uint8_t*)malloc(need);
    if(!raw){free(file);return 1;}
    ok=musa_dwg_ac1032_object_payload_file(
         file,n,h,raw,need,&got,&meta)&&got==need;
    MusaDwgAc1032EntityCommon common={0};
    int common_ok=ok&&musa_dwg_ac1032_entity_common_bytes(
         raw,need,&layout,44u,&common);
    const size_t end=(size_t)layout.data_stream_bits;
    uint32_t flag=0u,low=0u,high=0u;
    MusaDwgAc1015Bits b={0};
    int tu_frame=ok&&end>=17u&&
       musa_dwg_ac1015_bits_init(&b,raw,need,end-1u,end)&&
       musa_dwg_ac1015_bits_read_u(&b,1u,&flag)&&
       musa_dwg_ac1015_bits_init(&b,raw,need,end-17u,end-1u)&&
       musa_dwg_ac1015_bits_read_u(&b,8u,&low)&&
       musa_dwg_ac1015_bits_read_u(&b,8u,&high);
    const uint32_t bits=low|(high<<8u);
    fprintf(stderr,"ORIGINAL MTEXT H1C0: payload=%zu data_bits=%llu H_bits=%llu common=%d geom_start=%zu TU_frame=%d TU_flag=%u TU_bits=%u\n",
         need,(unsigned long long)layout.data_stream_bits,
         (unsigned long long)layout.handle_stream_bits,
         common_ok,common.geometry_start_bit,tu_frame,flag,bits);
    /* The raw MTEXT entity-common decoder is only admitted if its own
     * CRC/H-authenticated real-file preflight actually succeeds. */
    ok=ok&&common_ok&&common.common_decoded&&
       common.prefix.raw_object_type==44u&&
       common.prefix.indexed_handle==h&&
       common.geometry_start_bit<end&&
       !memcmp(file,"AC1032",6u);
    free(raw);free(file);
    return ok?0:1;
}
