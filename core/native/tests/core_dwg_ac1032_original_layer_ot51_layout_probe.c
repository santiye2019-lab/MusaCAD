/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Original AutoCAD 2018 binary LAYER OT51 raw DATA/H geometry proof.
 * Diagnostic evidence only: no guessed color, frozen, off or BYLAYER.
 * All bytes come from CRC + H indexed ORIGINAL DWG; no LibreDWG decoder. */
#include "musacad_dwg_ac1032_object_payload.h"
#include "musacad_dwg_ac1032_object_streams.h"
#include "musacad_dwg_ac1032_object_type.h"
#include "musacad_dwg_ac1015_values.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define ORIGINAL_LAYER UINT64_C(0x10)
static int read_file(const char *path,uint8_t **data,size_t *n){
    FILE *f=fopen(path,"rb");if(!f)return 0;
    if(fseek(f,0,SEEK_END)){fclose(f);return 0;}
    const long k=ftell(f);
    if(k<256||k>64L*1024L*1024L||fseek(f,0,SEEK_SET)){
        fclose(f);return 0;
    }
    uint8_t *raw=(uint8_t*)malloc((size_t)k);
    if(!raw){fclose(f);return 0;}
    int ok=fread(raw,1u,(size_t)k,f)==(size_t)k;
    if(fclose(f)!=0)ok=0;
    if(!ok){free(raw);return 0;}
    *data=raw;*n=(size_t)k;return 1;
}
int main(int argc,char **argv){
    if(argc!=2)return 2;
    uint8_t *source=NULL,*snapshot=NULL,*raw=NULL;
    size_t n=0u,wanted=0u,copied=0u;
    if(!read_file(argv[1],&source,&n))return 2;
    snapshot=(uint8_t*)malloc(n);
    if(!snapshot){free(source);return 2;}
    memcpy(snapshot,source,n);
    MusaDwgAc1032ObjectType type={0};
    MusaDwgAc1032PayloadInfo meta={0},again={0};
    MusaDwgAc1032ObjectStreams layout={0};
    int ok=musa_dwg_ac1032_object_type_file(
         source,n,ORIGINAL_LAYER,&type)&&
       type.record_crc_verified&&type.indexed_handle==ORIGINAL_LAYER&&
       type.type_code==51u&&type.type_field_bits==10u&&
       musa_dwg_ac1032_object_payload_file(
         source,n,ORIGINAL_LAYER,NULL,0u,&wanted,&meta)&&
       wanted>10u&&wanted<=65536u&&
       musa_dwg_ac1032_stream_layout_file(
         source,n,ORIGINAL_LAYER,&layout)&&
       layout.original_record_crc_verified&&
       layout.indexed_handle==ORIGINAL_LAYER&&
       layout.raw_object_type==51u&&
       layout.object_type_field_bits==10u&&
       layout.total_payload_bits==(uint64_t)wanted*8u&&
       layout.handle_stream_bits>0u&&
       layout.handle_stream_start_bit==layout.data_stream_bits&&
       layout.data_stream_bits>=10u;
    if(ok){
        raw=(uint8_t*)malloc(wanted);
        ok=raw&&musa_dwg_ac1032_object_payload_file(
             source,n,ORIGINAL_LAYER,raw,wanted,&copied,&again)&&
           copied==wanted&&again.original_record_crc_verified&&
           again.indexed_handle==ORIGINAL_LAYER&&
           again.raw_object_type==51u&&
           !memcmp(source,snapshot,n);
    }
    MusaDwgAc1015Bits bits={0};
    uint32_t kind=0u,value=0u;
    if(ok)ok=layout.data_stream_bits<=SIZE_MAX&&
        musa_dwg_ac1015_bits_init(
          &bits,raw,wanted,0u,(size_t)layout.data_stream_bits)&&
        musa_dwg_ac1015_bits_read_u(&bits,2u,&kind)&&kind==0u&&
        musa_dwg_ac1015_bits_read_u(&bits,8u,&value)&&value==51u;
    if(ok){
        fprintf(stderr,"AUTHENTIC R2018 LAYER H=0x10 OT=51 "
          "CRC=1 payload_bytes=%zu data_bits=%llu handles_bits=%llu\n",
          wanted,(unsigned long long)layout.data_stream_bits,
          (unsigned long long)layout.handle_stream_bits);
        fprintf(stderr,"LAYER DATA start after OT: bit=%zu. "
          "Payload first 96 source bytes:\n",bits.bit_pos);
        for(size_t i=0;i<wanted&&i<96u;i++)
          fprintf(stderr,"%02X%s",raw[i],((i+1u)%16u==0u)?"\n":" ");
        fprintf(stderr,"\nLAYER original HANDLES first 80 bits:");
        for(size_t i=0;i<layout.handle_stream_bits&&i<80u;i++){
           uint32_t v=0u;
           if(!musa_dwg_ac1032_stream_read_bits(
             raw,wanted,&layout,MUSA_AC1032_STREAM_HANDLES,
             i,1u,&v)){ok=0;break;}
           fprintf(stderr,"%u",(unsigned)v);
           if(i%8u==7u)fputc(' ',stderr);
        }
        fputc('\n',stderr);
    }
    if(ok){
       MusaDwgAc1032ObjectType not_layer={0};
       ok=musa_dwg_ac1032_object_type_file(
           source,n,UINT64_C(0x1ca),&not_layer)&&
           not_layer.type_code==19u&&
           !memcmp(source,snapshot,n);
    }
    fprintf(stderr,"%s original AutoCAD 2018 LAYER "
       "H/OT/CRC/binary partition inspection (NOT color decoding)\n",
       ok?"PASS":"FAIL");
    free(raw);free(snapshot);free(source);
    return ok?0:1;
}
