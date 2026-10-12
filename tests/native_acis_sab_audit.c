/* Read-only diagnostic for PINNED PUBLIC LibreDWG fixtures only.
 * Never print binary bytes, CAD text, handles, filenames or user geometry.
 * The production Android library does not build this test executable.
 */
#include "dwg.h"
#include "dwg_api.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static uint32_t u32(const unsigned char *p){
  return (uint32_t)p[0]|((uint32_t)p[1]<<8)|((uint32_t)p[2]<<16)|((uint32_t)p[3]<<24);
}
static uint16_t u16(const unsigned char *p){return (uint16_t)(p[0]|(p[1]<<8));}
static int consume(const unsigned char *b,size_t n,size_t *at,size_t amount){
  (void)b;
  if(*at>n||amount>n-*at)return 0;
  *at+=amount;return 1;
}
static int scan_sab(const Dwg_Entity_3DSOLID *solid,int item){
  const unsigned char *b=(const unsigned char*)solid->acis_data;
  size_t n=solid->sab_size ? (size_t)solid->sab_size
                          : (solid->block_size ? (size_t)solid->block_size[0] : 0);
  if(!b||n<31||n>16u*1024u*1024u||memcmp(b,"ACIS BinaryFile",15)!=0){
    printf("SAB_SOURCE item=%d available=0 bytes=%zu\n",item,n);
    return 1;
  }
  const uint32_t header_version=u32(b+15);
  const uint32_t declared=u32(b+19);
  const uint32_t entities=u32(b+23);
  const uint32_t has_history=u32(b+27);
  size_t at=31;
  /* Exactly mirror the three version/product/date fields and three tolerance
     doubles that LibreDWG reads before the record tags. */
  for(int i=0;i<3;i++){
    if(at+2>n){puts("SAB_SOURCE truncated_text_header");return 1;}
    unsigned length=b[at+1];at+=2;
    if(!consume(b,n,&at,length)){puts("SAB_SOURCE invalid_header_length");return 1;}
  }
  for(int i=0;i<3;i++){
    if(at+9>n){puts("SAB_SOURCE truncated_tolerance");return 1;}
    at+=9;
  }
  unsigned long count=0,ptrs=0,outside=0,largest=0,unknown=0;
  unsigned long tag13=0,records=0;unsigned tag=0;
  while(at<n){
    tag=b[at++];
    size_t len=0;
    switch(tag){
      case 17: records++;break; /* '#': end of SAB record */
      case 13: tag13++;
      case 7: case 14:
        if(!consume(b,n,&at,1))goto invalid;
        len=b[at-1];if(!consume(b,n,&at,len))goto invalid;
        break;
      case 8:
        if(at+2>n)goto invalid;
        len=u16(b+at);at+=2;if(!consume(b,n,&at,len))goto invalid;
        break;
      case 9:
        if(at+4>n)goto invalid;
        len=u32(b+at);at+=4;if(!consume(b,n,&at,len))goto invalid;
        break;
      case 4: case 12: case 21: {
        if(at+4>n)goto invalid;
        uint32_t pointer=u32(b+at);at+=4;
        ptrs++;if(pointer>largest)largest=pointer;
        if(declared!=0&&pointer>=declared)outside++;
        break;
      }
      case 2: if(!consume(b,n,&at,1))goto invalid;break;
      case 3: if(!consume(b,n,&at,2))goto invalid;break;
      case 5: if(!consume(b,n,&at,4))goto invalid;break;
      case 6: if(!consume(b,n,&at,8))goto invalid;break;
      case 19: case 20: if(!consume(b,n,&at,24))goto invalid;break;
      case 23: if(!consume(b,n,&at,8))goto invalid;break;
      case 10: case 11: case 15: case 16: break;
      default: unknown++;goto invalid;
    }
    if(++count>1000000)goto invalid;
  }
  printf("SAB_SOURCE item=%d available=1 version=%u declared_records=%u declared_entities=%u history=%u end_records=%lu record_identifiers=%lu pointers=%lu outside_declared=%lu largest_pointer=%lu unknown_tags=%lu\n",
         item,header_version,declared,entities,has_history,records,tag13,ptrs,outside,largest,unknown);
  return 0;
invalid:
  printf("SAB_SOURCE item=%d parse_incomplete=1 version=%u declared_records=%u parsed_records=%lu parsed_tags=%lu last_tag=%u unknown_tags=%lu\n",
         item,header_version,declared,records,count,tag,unknown);
  return 1;
}
int main(int argc,char **argv){
  if(argc!=2)return 2;
  Dwg_Data dwg={0};
  int err=dwg_read_file(argv[1],&dwg);
  if(err>=DWG_ERR_CRITICAL){fprintf(stderr,"Failed to read public SAB fixture\n");return 1;}
  int item=0,bad=0;
  for(BITCODE_BL i=0;i<dwg.num_objects;i++){
    Dwg_Object *o=&dwg.object[i];
    if(!o->tio.entity)continue;
    Dwg_Entity_3DSOLID *solid=NULL;
    if(o->fixedtype==DWG_TYPE__3DSOLID)solid=o->tio.entity->tio._3DSOLID;
    else if(o->fixedtype==DWG_TYPE_REGION)solid=o->tio.entity->tio.REGION;
    else if(o->fixedtype==DWG_TYPE_BODY)solid=o->tio.entity->tio.BODY;
    if(solid)bad+=scan_sab(solid,++item);
  }
  dwg_free(&dwg);
  if(!item){fprintf(stderr,"Public fixture lacks ACIS entities\n");return 1;}
  printf("SAB_SOURCE_SUMMARY bodies=%d partial_or_missing=%d\n",item,bad);
  return 0; /* Probe is observational, not a data-fidelity certification. */
}
