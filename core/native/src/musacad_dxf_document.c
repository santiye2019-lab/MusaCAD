/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Minimal native DXF entity decoder with explicit rejection of data loss.
 * This is a real first-party ASCII DXF SUBSET reader, not an AC1032 codec.
 */
#include "musacad_dxf_document.h"
#include "musacad_dxf_group.h"
#include "musacad_dxf_handle.h"
#include <math.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>

#define MUSA_TEXT_MAX (16u*1024u*1024u)
#define MUSA_LINE_MAX 4096u
#define MUSA_ENTITIES_MAX 4000u

typedef struct {const char *ptr;size_t len;} Field;
typedef struct {
    const unsigned char *text;
    size_t len,pos;
} Scanner;
typedef struct {
    MusaDocumentEntity entity;
    unsigned seen;
    unsigned required;
} Pending;

static Field trim(Field f){
    while(f.len&&(unsigned char)*f.ptr<=32u){f.ptr++;f.len--;}
    while(f.len&&(unsigned char)f.ptr[f.len-1u]<=32u)f.len--;
    return f;
}
static int equals(Field f,const char *word){
    const size_t n=strlen(word);
    return f.len==n&&memcmp(f.ptr,word,n)==0;
}
static int read_line(Scanner *s,Field *out){
    if(s->pos>=s->len)return 0;
    size_t start=s->pos;
    while(s->pos<s->len&&s->text[s->pos]!='\n'){
        const unsigned char c=s->text[s->pos];
        if(c==0||c>127u)return -1; /* strict ASCII subset; not a codepage decoder */
        if(s->pos-start>=MUSA_LINE_MAX)return -1;
        ++s->pos;
    }
    size_t stop=s->pos;
    if(s->pos<s->len)++s->pos;
    if(stop>start&&s->text[stop-1u]=='\r')--stop;
    out->ptr=(const char *)s->text+start;
    out->len=stop-start;
    return 1;
}
static int pair(Scanner *s,int *code,Field *value){
    Field group;
    int result=read_line(s,&group);
    if(result<=0)return result;
    if(!musa_core_dxf_group_parse(group.ptr,group.len,code))
        return -1;
    if(read_line(s,value)!=1)return -1;
    return 1;
}
/* Locale-independent ASCII decimal reader, finite double only.
 * Length bound keeps both mantissa accumulation and exponent bounded.
 */
static int number(Field f,double *out){
    f=trim(f);
    if(f.len==0||f.len>80||!out)return 0;
    size_t i=0;int sign=1,exp_sign=1,exponent=0,frac=0,digits=0,nonzero=0;
    if(f.ptr[i]=='-'||f.ptr[i]=='+'){
        if(f.ptr[i]=='-')sign=-1;
        if(++i==f.len)return 0;
    }
    long double mantissa=0.0L;
    for(;i<f.len&&f.ptr[i]>='0'&&f.ptr[i]<='9';i++){
        int d=(int)(f.ptr[i]-'0');
        nonzero|=d!=0;
        mantissa=mantissa*10.0L+(long double)d;
        digits++;
    }
    if(i<f.len&&f.ptr[i]=='.'){
        i++;
        for(;i<f.len&&f.ptr[i]>='0'&&f.ptr[i]<='9';i++){
            int d=(int)(f.ptr[i]-'0');
            nonzero|=d!=0;
            mantissa=mantissa*10.0L+(long double)d;
            digits++;frac++;
        }
    }
    if(!digits)return 0;
    if(i<f.len&&(f.ptr[i]=='e'||f.ptr[i]=='E')){
        i++;
        if(i<f.len&&(f.ptr[i]=='+'||f.ptr[i]=='-')){
            if(f.ptr[i]=='-')exp_sign=-1;
            i++;
        }
        if(i==f.len||f.ptr[i]<'0'||f.ptr[i]>'9')return 0;
        for(;i<f.len&&f.ptr[i]>='0'&&f.ptr[i]<='9';i++){
            if(exponent>4000)return 0;
            exponent=exponent*10+(int)(f.ptr[i]-'0');
        }
    }
    if(i!=f.len||exponent>4000)return 0;
    const int power=exponent*exp_sign-frac;
    if(power>400||power< -400)return 0;
    const long double candidate=(long double)sign*mantissa*powl(10.0L,power);
    const double value=(double)candidate;
    if(!isfinite(value)||(value==0.0&&nonzero))return 0;
    *out=value;return 1;
}
static int parse_handle(Field f,uint64_t *out){
    f=trim(f);
    return musa_core_dxf_handle_parse(f.ptr,f.len,out);
}
enum {
    F_HANDLE=1u<<0,F_LAYER=1u<<1,
    F_X1=1u<<2,F_Y1=1u<<3,F_X2=1u<<4,F_Y2=1u<<5,
    F_RADIUS=1u<<6,F_START=1u<<7,F_END=1u<<8
};
static int initialize(Pending *p,Field kind){
    memset(p,0,sizeof(*p));
    if(equals(kind,"LINE")){
        p->entity.kind=MUSA_DOCUMENT_LINE;
        p->required=F_X1|F_Y1|F_X2|F_Y2;
    }else if(equals(kind,"POINT")){
        p->entity.kind=MUSA_DOCUMENT_POINT;
        p->required=F_X1|F_Y1;
    }else if(equals(kind,"CIRCLE")){
        p->entity.kind=MUSA_DOCUMENT_CIRCLE;
        p->required=F_X1|F_Y1|F_RADIUS;
    }else if(equals(kind,"ARC")){
        p->entity.kind=MUSA_DOCUMENT_ARC;
        p->required=F_X1|F_Y1|F_RADIUS|F_START|F_END;
    }else return MUSA_DXF_DOCUMENT_UNSUPPORTED;
    return MUSA_DXF_DOCUMENT_OK;
}
static int accept_tag(Pending *p,int code,Field value){
    unsigned bit=0;
    double *out=NULL;
    switch(code){
      case 5:
        bit=F_HANDLE;
        if(p->seen&bit||!parse_handle(value,&p->entity.handle))
            return MUSA_DXF_DOCUMENT_MALFORMED;
        break;
      case 8:
        bit=F_LAYER;
        if(p->seen&bit)return MUSA_DXF_DOCUMENT_MALFORMED;
        if(!equals(trim(value),"0"))return MUSA_DXF_DOCUMENT_UNSUPPORTED;
        break;
      case 10:bit=F_X1;out=&p->entity.x1;break;
      case 20:bit=F_Y1;out=&p->entity.y1;break;
      case 11:
        if(p->entity.kind!=MUSA_DOCUMENT_LINE)return MUSA_DXF_DOCUMENT_UNSUPPORTED;
        bit=F_X2;out=&p->entity.x2;break;
      case 21:
        if(p->entity.kind!=MUSA_DOCUMENT_LINE)return MUSA_DXF_DOCUMENT_UNSUPPORTED;
        bit=F_Y2;out=&p->entity.y2;break;
      case 40:
        if(p->entity.kind!=MUSA_DOCUMENT_CIRCLE&&p->entity.kind!=MUSA_DOCUMENT_ARC)
            return MUSA_DXF_DOCUMENT_UNSUPPORTED;
        bit=F_RADIUS;out=&p->entity.radius;break;
      case 50:
        if(p->entity.kind!=MUSA_DOCUMENT_ARC)return MUSA_DXF_DOCUMENT_UNSUPPORTED;
        bit=F_START;out=&p->entity.start_deg;break;
      case 51:
        if(p->entity.kind!=MUSA_DOCUMENT_ARC)return MUSA_DXF_DOCUMENT_UNSUPPORTED;
        bit=F_END;out=&p->entity.end_deg;break;
      default:return MUSA_DXF_DOCUMENT_UNSUPPORTED;
    }
    if(p->seen&bit)return MUSA_DXF_DOCUMENT_MALFORMED;
    if(out&&!number(value,out))return MUSA_DXF_DOCUMENT_MALFORMED;
    p->seen|=bit;
    return MUSA_DXF_DOCUMENT_OK;
}
static int finalize(MusaDocument *doc,Pending *pending){
    if((pending->seen&pending->required)!=pending->required)
        return MUSA_DXF_DOCUMENT_MALFORMED;
    uint64_t id=0;
    const int status=musa_document_load_entity(doc,&pending->entity,&id);
    if(status==MUSA_DOCUMENT_LIMIT)return MUSA_DXF_DOCUMENT_LIMIT;
    if(status==MUSA_DOCUMENT_NO_MEMORY)return MUSA_DXF_DOCUMENT_NO_MEMORY;
    if(status!=MUSA_DOCUMENT_OK)return MUSA_DXF_DOCUMENT_MALFORMED;
    return MUSA_DXF_DOCUMENT_OK;
}
int musa_dxf_document_read(const unsigned char *bytes,size_t length,
                           size_t max_entities,MusaDocument **out_document){
    if(!out_document)return MUSA_DXF_DOCUMENT_MALFORMED;
    *out_document=NULL;
    if(!bytes||!length)return MUSA_DXF_DOCUMENT_MALFORMED;
    if(length>MUSA_TEXT_MAX||max_entities==0||max_entities>MUSA_ENTITIES_MAX)
        return MUSA_DXF_DOCUMENT_LIMIT;
    MusaDocument *doc=musa_document_create(max_entities);
    if(!doc)return MUSA_DXF_DOCUMENT_NO_MEMORY;
    Scanner scanner={bytes,length,0};
    enum {START,NAME,ENTITIES,CLOSED,DONE} state=START;
    Pending current={0};
    int inside=0;
    int result=MUSA_DXF_DOCUMENT_MALFORMED;
    while(scanner.pos<scanner.len){
        int code=0;Field value={0};
        if(pair(&scanner,&code,&value)!=1)goto failure;
        if(code!=0){
            if(state==NAME&&code==2&&equals(trim(value),"ENTITIES")){
                state=ENTITIES;continue;
            }
            if(state!=ENTITIES||!inside)goto failure;
            result=accept_tag(&current,code,value);
            if(result!=MUSA_DXF_DOCUMENT_OK)goto failure;
            continue;
        }
        value=trim(value);
        if(state==START&&equals(value,"SECTION")){
            state=NAME;continue;
        }
        if(state==ENTITIES){
            if(inside){
                result=finalize(doc,&current);
                if(result!=MUSA_DXF_DOCUMENT_OK)goto failure;
                inside=0;
            }
            if(equals(value,"ENDSEC")){
                state=CLOSED;continue;
            }
            result=initialize(&current,value);
            if(result!=MUSA_DXF_DOCUMENT_OK)goto failure;
            inside=1;continue;
        }
        if(state==CLOSED&&equals(value,"EOF")){
            state=DONE;
            if(scanner.pos!=scanner.len)goto failure;
            *out_document=doc;
            return MUSA_DXF_DOCUMENT_OK;
        }
        goto failure;
    }
failure:
    musa_document_destroy(doc);
    return result==MUSA_DXF_DOCUMENT_UNSUPPORTED||
           result==MUSA_DXF_DOCUMENT_LIMIT||
           result==MUSA_DXF_DOCUMENT_NO_MEMORY
           ?result:MUSA_DXF_DOCUMENT_MALFORMED;
}
