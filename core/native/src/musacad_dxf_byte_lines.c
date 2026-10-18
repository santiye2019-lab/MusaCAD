/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "musacad_dxf_byte_lines.h"
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

struct MusaDxfByteLines {
    unsigned char *bytes;
    size_t length,capacity,maximum;
    int finished;
    int cr_mode;
};
MusaDxfByteLines *musa_core_dxf_byte_lines_create_mode(size_t maximum,
                                                        int cr_mode){
    if(!maximum||maximum>1048576u||
       (cr_mode!=MUSA_DXF_CR_DROP_ALL &&
        cr_mode!=MUSA_DXF_CR_TRAILING_ONLY))return NULL;
    MusaDxfByteLines *reader=(MusaDxfByteLines *)calloc(1,sizeof(*reader));
    if(!reader)return NULL;
    reader->capacity=maximum<256u?maximum:256u;
    reader->bytes=(unsigned char *)malloc(reader->capacity);
    if(!reader->bytes){free(reader);return NULL;}
    reader->maximum=maximum;
    reader->cr_mode=cr_mode;
    return reader;
}
MusaDxfByteLines *musa_core_dxf_byte_lines_create(size_t maximum){
    return musa_core_dxf_byte_lines_create_mode(maximum,MUSA_DXF_CR_DROP_ALL);
}
static size_t emitted_length(const MusaDxfByteLines *reader){
    const size_t size=reader->length;
    if(reader->cr_mode==MUSA_DXF_CR_TRAILING_ONLY && size &&
       reader->bytes[size-1u]==13u)return size-1u;
    return size;
}
static int append(MusaDxfByteLines *reader,unsigned char byte){
    if(reader->length>=reader->maximum)return MUSA_DXF_BYTE_TOO_LONG;
    if(reader->length==reader->capacity){
        size_t next=reader->capacity*2u;
        if(next>reader->maximum)next=reader->maximum;
        if(next<=reader->capacity)return MUSA_DXF_BYTE_TOO_LONG;
        unsigned char *new_bytes=(unsigned char *)realloc(reader->bytes,next);
        if(!new_bytes)return MUSA_DXF_BYTE_NO_MEMORY;
        reader->bytes=new_bytes;reader->capacity=next;
    }
    reader->bytes[reader->length++]=byte;
    return MUSA_DXF_BYTE_MORE;
}
int musa_core_dxf_byte_lines_step(MusaDxfByteLines *reader,
    const unsigned char *chunk,size_t chunk_len,size_t *consumed,
    const unsigned char **line,size_t *line_len){
    if(!reader||!chunk||!consumed||!line||!line_len||reader->finished)
        return MUSA_DXF_BYTE_BAD_ARGUMENT;
    *consumed=0;*line=NULL;*line_len=0;
    for(size_t i=0;i<chunk_len;i++){
        const unsigned char value=chunk[i];
        *consumed=i+1u;
        if(value==10u){
            *line=reader->bytes;
            *line_len=emitted_length(reader);
            reader->length=0;
            return MUSA_DXF_BYTE_LINE;
        }
        if(value==13u && reader->cr_mode==MUSA_DXF_CR_DROP_ALL)continue;
        const int result=append(reader,value);
        if(result!=MUSA_DXF_BYTE_MORE)return result;
    }
    return MUSA_DXF_BYTE_MORE;
}
int musa_core_dxf_byte_lines_finish(MusaDxfByteLines *reader,
    const unsigned char **line,size_t *line_len){
    if(!reader||!line||!line_len||reader->finished)
        return MUSA_DXF_BYTE_BAD_ARGUMENT;
    reader->finished=1;
    *line=NULL;*line_len=0;
    if(reader->length==0)return MUSA_DXF_BYTE_EOF;
    *line=reader->bytes;*line_len=emitted_length(reader);
    reader->length=0;
    return MUSA_DXF_BYTE_LINE;
}
void musa_core_dxf_byte_lines_destroy(MusaDxfByteLines *reader){
    if(!reader)return;
    if(reader->bytes){memset(reader->bytes,0,reader->capacity);free(reader->bytes);}
    memset(reader,0,sizeof(*reader));free(reader);
}
