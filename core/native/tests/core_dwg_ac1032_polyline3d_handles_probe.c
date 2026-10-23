/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Diagnostic only: original R2018 OT16/11 handle stream grammar.
 * Prints raw CRC-authenticated bits and independently authored handle
 * primitive results. NEVER promotes guessed semantic roles.
 */
#include "musacad_dwg_ac1032_object_payload.h"
#include "musacad_dwg_ac1032_object_streams.h"
#include "musacad_dwg_ac1015_values.h"
#include "musacad_dwg_ac1015_handle_refs.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#define MAX_FILE (64u*1024u*1024u)
static int load(const char *name,uint8_t **data,size_t *n){
  FILE *f=fopen(name,"rb");if(!f)return 0;
  if(fseek(f,0,SEEK_END)!=0){fclose(f);return 0;}
  long size=ftell(f);
  if(size<256||size>(long)MAX_FILE||fseek(f,0,SEEK_SET)!=0){fclose(f);return 0;}
  uint8_t *p=(uint8_t*)malloc((size_t)size);
  if(!p){fclose(f);return 0;}
  int ok=fread(p,1u,(size_t)size,f)==(size_t)size;
  if(fclose(f)!=0)ok=0;
  if(!ok){free(p);return 0;}
  *data=p;*n=(size_t)size;return 1;
}
static int inspect(const uint8_t *file,size_t n,uint64_t h){
   MusaDwgAc1032ObjectStreams layout={0};
   size_t needed=0u,copied=0u;
   MusaDwgAc1032PayloadInfo p={0},proof={0};
   if(!musa_dwg_ac1032_stream_layout_file(file,n,h,&layout)||
      !musa_dwg_ac1032_object_payload_file(
         file,n,h,NULL,0u,&needed,&p)||needed>65536u)return 0;
   uint8_t *raw=(uint8_t*)malloc(needed);
   if(!raw)return 0;
   int ok=musa_dwg_ac1032_object_payload_file(
       file,n,h,raw,needed,&copied,&proof)&&copied==needed&&
       proof.original_record_crc_verified;
   if(!ok){free(raw);return 0;}
   fprintf(stderr,"HANDLE_PROBE H=%llX OT=%u"
     " data_bits=%llu handles_bits=%llu payload_bytes=%zu\n",
     (unsigned long long)h,layout.raw_object_type,
     (unsigned long long)layout.data_stream_bits,
     (unsigned long long)layout.handle_stream_bits,needed);
   const size_t at=(size_t)layout.data_stream_bits;
   fprintf(stderr,"HANDLE_PROBE H=%llX handle stream first 40 bytes (bit offset %zu):",
       (unsigned long long)h,at);
   for(size_t i=at;i<(size_t)layout.total_payload_bits&&i<at+320u;i+=8u){
       unsigned v=0u;
       for(size_t bit=i;bit<i+8u&&bit<(size_t)layout.total_payload_bits;bit++)
           v=(v<<1u)|((raw[bit/8u]>>(7u-bit%8u))&1u);
       fprintf(stderr," %02X",v);
   }
   fputc('\n',stderr);
   MusaDwgAc1015Bits b={0};
   ok=musa_dwg_ac1015_bits_init(&b,raw,needed,at,
         (size_t)layout.total_payload_bits);
   for(unsigned k=0u;ok&&k<32u&&b.bit_end-b.bit_pos>=8u;k++){
       size_t pos=b.bit_pos;
       uint32_t code=0u,count=0u;
       MusaDwgAc1015Bits peek=b;
       if(!musa_dwg_ac1015_bits_read_u(&peek,4u,&code)||
          !musa_dwg_ac1015_bits_read_u(&peek,4u,&count))break;
       MusaDwgAc1015HandleRef ref={0};
       int decoded=musa_dwg_ac1015_read_handle_ref(&b,h,&ref);
       fprintf(stderr,"HANDLE_PROBE H=%llX part=%u bit=%zu code=%u"
           " count=%u decoded=%d target=%llX\n",
           (unsigned long long)h,k,pos-at,code,count,decoded,
           (unsigned long long)ref.target);
       if(!decoded)break;
   }
   free(raw);return ok;
}
int main(int argc,char **argv){
   if(argc!=2)return 2;
   uint8_t *data=NULL;size_t n=0u;
   if(!load(argv[1],&data,&n))return 2;
   int ok=inspect(data,n,UINT64_C(0x1c1))&&
          inspect(data,n,UINT64_C(0x1c3))&&
          inspect(data,n,UINT64_C(0x1c4));
   free(data);return ok?0:1;
}
