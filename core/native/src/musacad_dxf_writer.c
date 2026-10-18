/* SPDX-License-Identifier: GPL-3.0-or-later */
/* First-party, source-safe, bounded ASCII DXF entity-only serialization.
 * No LibreDWG, ODA, operating system, or CAD parser dependency.
 */
#include "musacad_dxf_writer.h"
#include "musacad_dxf_handle.h"
#include <locale.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#define MAX_DXF_OUTPUT (16u*1024u*1024u)
#define MAX_DXF_ENTITIES 4000u

typedef struct {
    char *dst;
    size_t used,capacity;
} Output;

static int append(Output *out,const char *text,size_t n){
    if(!out||!text||n>MAX_DXF_OUTPUT-out->used)
        return 0;
    if(out->dst){
        if(n>out->capacity-out->used)return 0;
        memcpy(out->dst+out->used,text,n);
    }
    out->used+=n;
    return 1;
}
static int literal(Output *o,const char *text){
    return append(o,text,strlen(text));
}
/* The host C locale can use e.g. a comma decimal point.
 * Serialize strictly invariant ASCII '.' with 17 significant digits.
 * We reject unknown locale encodings rather than write ambiguous geometry.
 */
static int number(Output *o,double value){
    if(!isfinite(value))return 0;
    char raw[128],ascii[128];
    int n=snprintf(raw,sizeof(raw),"%.17g",value);
    if(n<=0||(size_t)n>=sizeof(raw))return 0;
    const struct lconv *lc=localeconv();
    const char *decimal=(lc&&lc->decimal_point&&lc->decimal_point[0])
                       ?lc->decimal_point:".";
    size_t dlen=strlen(decimal);
    if(!dlen||dlen>16u)return 0;
    size_t i=0,k=0;
    while(i<(size_t)n){
        if(raw[i]!='.' && i+dlen<=(size_t)n &&
           memcmp(raw+i,decimal,dlen)==0){
            ascii[k++]='.';i+=dlen;continue;
        }
        unsigned char ch=(unsigned char)raw[i++];
        if(!((ch>='0'&&ch<='9')||ch=='+'||ch=='-'||
             ch=='.'||ch=='e'||ch=='E'))return 0;
        ascii[k++]=(char)ch;
    }
    return append(o,ascii,k);
}
static int scalar(Output *o,const char *group,double value){
    return literal(o,group)&&number(o,value)&&literal(o,"\n");
}
static int entity(Output *out,const MusaDocumentEntity *item){
    if(!item||!item->handle||
       !isfinite(item->x1)||!isfinite(item->y1)||
       !isfinite(item->x2)||!isfinite(item->y2)||
       !isfinite(item->radius)||!isfinite(item->start_deg)||
       !isfinite(item->end_deg))return 0;
    const char *type;
    switch(item->kind){
      case MUSA_DOCUMENT_LINE:type="LINE";break;
      case MUSA_DOCUMENT_POINT:type="POINT";break;
      case MUSA_DOCUMENT_CIRCLE:type="CIRCLE";break;
      case MUSA_DOCUMENT_ARC:type="ARC";break;
      default:return 0;
    }
    char hex[17]={0};
    if(!musa_core_dxf_handle_format(item->handle,hex))return 0;
    if(!literal(out,"0\n")||!literal(out,type)||!literal(out,"\n5\n")||
       !literal(out,hex)||!literal(out,"\n8\n0\n")||
       !scalar(out,"10\n",item->x1)||
       !scalar(out,"20\n",item->y1))return 0;
    switch(item->kind){
      case MUSA_DOCUMENT_LINE:
        return item->radius==0&&item->start_deg==0&&item->end_deg==0&&
               scalar(out,"11\n",item->x2)&&scalar(out,"21\n",item->y2);
      case MUSA_DOCUMENT_POINT:
        return item->x2==0&&item->y2==0&&item->radius==0&&
               item->start_deg==0&&item->end_deg==0;
      case MUSA_DOCUMENT_CIRCLE:
        return item->x2==0&&item->y2==0&&item->radius>0&&
               item->start_deg==0&&item->end_deg==0&&
               scalar(out,"40\n",item->radius);
      case MUSA_DOCUMENT_ARC:
        return item->x2==0&&item->y2==0&&item->radius>0&&
               scalar(out,"40\n",item->radius)&&
               scalar(out,"50\n",item->start_deg)&&
               scalar(out,"51\n",item->end_deg);
      default:return 0;
    }
}
static int serialize(Output *o,const MusaDocument *doc){
    size_t count=musa_document_count(doc);
    if(count>MAX_DXF_ENTITIES)return MUSA_DXF_DOCUMENT_LIMIT;
    if(!literal(o,"0\nSECTION\n2\nENTITIES\n"))
        return MUSA_DXF_DOCUMENT_LIMIT;
    for(size_t i=0;i<count;i++){
        MusaDocumentEntity item={0};
        if(musa_document_get_index(doc,i,&item)!=MUSA_DOCUMENT_OK)
            return MUSA_DXF_DOCUMENT_MALFORMED;
        if(!entity(o,&item))
            return MUSA_DXF_DOCUMENT_UNSUPPORTED;
    }
    return literal(o,"0\nENDSEC\n0\nEOF\n")
           ?MUSA_DXF_DOCUMENT_OK:MUSA_DXF_DOCUMENT_LIMIT;
}
int musa_dxf_document_write(const MusaDocument *doc,char *output,
                            size_t capacity,size_t *written){
    if(!doc||!written||(output==NULL&&capacity!=0))
        return MUSA_DXF_DOCUMENT_MALFORMED;
    *written=0;
    Output size_pass={NULL,0,0};
    int status=serialize(&size_pass,doc);
    if(status!=MUSA_DXF_DOCUMENT_OK)return status;
    const size_t required=size_pass.used;
    *written=required;
    if(!output)return MUSA_DXF_DOCUMENT_OK;
    if(required>=capacity)return MUSA_DXF_DOCUMENT_LIMIT;
    char *private_stage=(char *)malloc(required+1u);
    if(!private_stage)return MUSA_DXF_DOCUMENT_NO_MEMORY;
    Output write_pass={private_stage,0,required};
    status=serialize(&write_pass,doc);
    if(status==MUSA_DXF_DOCUMENT_OK&&write_pass.used==required){
        private_stage[required]='\0';
        memcpy(output,private_stage,required+1u);
    }else if(status==MUSA_DXF_DOCUMENT_OK){
        status=MUSA_DXF_DOCUMENT_MALFORMED;
    }
    memset(private_stage,0,required+1u);
    free(private_stage);
    return status;
}
