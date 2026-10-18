/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "musacad_bits.h"
#include <limits.h>

static int valid_buffer(const void *p,size_t n){
    return n<=SIZE_MAX/8u && (!n||p!=NULL);
}
int musa_bits_reader(MusaBits *c,const uint8_t *src,size_t bytes){
    if(!c||!valid_buffer(src,bytes))return 0;
    MusaBits tmp={src,NULL,bytes,0u};*c=tmp;return 1;
}
int musa_bits_writer(MusaBits *c,uint8_t *dst,size_t bytes){
    if(!c||!valid_buffer(dst,bytes))return 0;
    MusaBits tmp={NULL,dst,bytes,0u};*c=tmp;return 1;
}
size_t musa_bits_remaining(const MusaBits *c){
    if(!c||c->byte_count>SIZE_MAX/8u||
       c->bit_position>c->byte_count*8u)return 0;
    return c->byte_count*8u-c->bit_position;
}
int musa_bits_seek(MusaBits *c,size_t bit_position){
    if(!c||c->byte_count>SIZE_MAX/8u||
       bit_position>c->byte_count*8u||
       (c->read_bytes==NULL&&c->write_bytes==NULL))return 0;
    c->bit_position=bit_position;return 1;
}
int musa_bits_align_byte(MusaBits *c){
    if(!c)return 0;
    const size_t gap=(8u-(c->bit_position&7u))&7u;
    if(gap>musa_bits_remaining(c))return 0;
    return musa_bits_seek(c,c->bit_position+gap);
}
static int valid_span(const MusaBits *c,unsigned width,int write){
    if(!c||width==0u||width>64u||width>musa_bits_remaining(c))
        return 0;
    return write?c->write_bytes!=NULL:c->read_bytes!=NULL;
}
int musa_bits_read(MusaBits *c,unsigned width,uint64_t *out){
    if(!out||!valid_span(c,width,0))return 0;
    uint64_t value=0;
    for(unsigned k=0u;k<width;k++){
        size_t pos=c->bit_position+(size_t)k;
        unsigned bit=(c->read_bytes[pos>>3]>>(pos&7u))&1u;
        value|=((uint64_t)bit<<k);
    }
    c->bit_position+=(size_t)width;*out=value;return 1;
}
int musa_bits_write(MusaBits *c,unsigned width,uint64_t value){
    if(!valid_span(c,width,1)||
       (width<64u && (value>>width)!=0u))return 0;
    for(unsigned k=0u;k<width;k++){
        size_t pos=c->bit_position+(size_t)k;
        uint8_t mask=(uint8_t)(1u<<(pos&7u));
        uint8_t *dst=&c->write_bytes[pos>>3];
        if((value>>k)&UINT64_C(1))*dst=(uint8_t)(*dst|mask);
        else *dst=(uint8_t)(*dst & (uint8_t)~mask);
    }
    c->bit_position+=(size_t)width;return 1;
}
int musa_bits_read_uleb128(MusaBits *c,uint64_t *out){
    if(!c||!out||!c->read_bytes)return 0;
    MusaBits temp=*c;
    uint64_t value=0;
    for(unsigned i=0;i<10u;i++){
        uint64_t chunk=0;
        if(!musa_bits_read(&temp,8u,&chunk))return 0;
        unsigned payload=(unsigned)chunk&0x7fu;
        if(i==9u&&payload>1u)return 0; /* overflow 64 bits */
        value|=((uint64_t)payload<<(7u*i));
        if((chunk&0x80u)==0u){
            if(i>0u&&payload==0u)return 0; /* overlong/noncanonical */
            *c=temp;*out=value;return 1;
        }
    }
    return 0; /* continuation beyond 64 bits: reject with rollback */
}
int musa_bits_write_uleb128(MusaBits *c,uint64_t value){
    if(!c||!c->write_bytes)return 0;
    uint8_t encoded[10];
    size_t count=0;
    uint64_t remaining=value;
    do{
        uint8_t b=(uint8_t)(remaining&0x7fu);
        remaining>>=7u;
        if(remaining)b|=0x80u;
        encoded[count++]=b;
    }while(remaining);
    if(count>10u||musa_bits_remaining(c)<count*8u)return 0;
    for(size_t i=0;i<count;i++){
        /* Capacity and writer permission were fully checked above. */
        if(!musa_bits_write(c,8u,encoded[i]))return 0;
    }
    return 1;
}
