/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "musacad_dwg_ac1015_common.h"
#include "musacad_dwg_ac1015_line.h"
#include "musacad_crc16.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define CHECK(x) do{if(!(x)){fprintf(stderr,"FAIL line %d %s\n",__LINE__,#x);exit(1);}}while(0)
typedef struct W{uint8_t b[256];size_t at;}W;
static void bit(W*w,unsigned v){CHECK(w->at<sizeof(w->b)*8u);size_t pos=w->at++;if(v)w->b[pos/8u]|=(uint8_t)(1u<<(7u-(pos%8u)));}
static void u(W*w,uint32_t v,unsigned n){CHECK(n>0u&&n<=32u);for(unsigned i=0;i<n;i++)bit(w,(v>>(n-i-1u))&1u);}
static void raw(W*w,uint64_t v,unsigned n){for(unsigned i=0;i<n;i++)u(w,(uint8_t)(v>>(i*8u)),8u);}
static void rd(W*w,double x){uint64_t t=0u;memcpy(&t,&x,8u);raw(w,t,8u);}
static void setbit(W*w,size_t p,unsigned v){CHECK(p<w->at);uint8_t m=(uint8_t)(1u<<(7u-(p%8u)));if(v)w->b[p/8u]|=m;else w->b[p/8u]&=(uint8_t)~m;}
static void setrl(W*w,size_t p,uint32_t val){
    for(unsigned i=0;i<4u;i++){uint8_t byte=(uint8_t)(val>>(8u*i));
      for(unsigned j=0;j<8u;j++)setbit(w,p+i*8u+j,(byte>>(7u-j))&1u);
    }
}
static void pack(int faulty,uint8_t file[512],size_t *field_start){
    W w={0};
    u(&w,1u,2u);raw(&w,19u,1u);
    size_t endbit_pos=w.at;raw(&w,0u,4u);
    u(&w,faulty==1?1u:0u,4u); /* self H code, must be 0 */
    u(&w,1u,4u);raw(&w,faulty==2?0x3bu:0x3au,1u);
    if(faulty==3){u(&w,1u,2u);raw(&w,4u,1u);} /* EED nonzero => reject */
    else u(&w,2u,2u); /* EED BS zero */
    u(&w,faulty==4?1u:0u,1u); /* preview unsupported */
    u(&w,1u,1u); /* no previous/next links */
    if(faulty==5){u(&w,0u,2u);raw(&w,300u,2u);}/* bad ACI */
    else {u(&w,1u,2u);raw(&w,7u,1u);}
    if(faulty==6)u(&w,2u,2u); /* zero ltype scale rejected */
    else u(&w,1u,2u); /* scale 1 */
    u(&w,0u,2u);u(&w,0u,2u); /* ltype+plot flags */
    u(&w,2u,2u); /* invisible BS = 0 */
    raw(&w,25u,1u); /* lineweight RC */
    *field_start=w.at;
    /* LINE: Zs zero, x RD with DD=default, y RD with DD=default. */
    u(&w,1u,1u);rd(&w,123.25);u(&w,0u,2u);
    rd(&w,-46.75);u(&w,0u,2u);
    u(&w,1u,1u);u(&w,1u,1u); /* zero BT and WCS BE */
    size_t data_end=w.at;
    CHECK(data_end<=UINT32_MAX);
    setrl(&w,endbit_pos,(uint32_t)data_end);
    u(&w,0x44u,8u); /* end handles stream sentinel */
    size_t n=(w.at+7u)/8u;
    CHECK(n<0x8000u&&n+132u<=512u);
    memset(file,0,512);memcpy(file,"AC1015",6u);
    file[128]=(uint8_t)n;file[129]=(uint8_t)(n>>8);
    memcpy(file+130,w.b,n);
    uint16_t crc=0u;
    CHECK(musa_core_crc16_arc(0xc0c1u,file+128,n+2u,&crc));
    file[130+n]=(uint8_t)crc;
    file[131+n]=(uint8_t)(crc>>8u);
}
static MusaDwgAc1015ObjectRecord get(uint8_t f[512]){
    MusaDwgAc1015HandleEntry i={0x3au,128u};
    MusaDwgAc1015ObjectRecord rec={0};
    CHECK(musa_dwg_ac1015_read_object_record(f,512u,&i,&rec));
    CHECK(rec.type_code==19u&&rec.record_crc_verified==1&&rec.geometry_decoded==0);
    return rec;
}
int main(void){
    uint8_t f[512],backup[512];size_t start=0u;
    pack(0,f,&start);memcpy(backup,f,sizeof(f));
    MusaDwgAc1015ObjectRecord r=get(f);
    MusaDwgAc1015SimpleCommon common={0};
    CHECK(musa_dwg_ac1015_read_simple_common(&r,&common));
    CHECK(common.prefix_decoded==1&&common.geometric_start_bit==start);
    CHECK(common.indexed_handle==0x3au&&common.type_code==19u);
    CHECK(common.color_index==7u&&common.linetype_scale==1.0);
    CHECK(common.nolinks==1u&&common.lineweight==25u);
    MusaDwgAc1015LineFields line={0};
    CHECK(musa_dwg_ac1015_decode_line_fields(&r,common.geometric_start_bit,&line));
    CHECK(line.start[0]==123.25&&line.end[0]==123.25);
    CHECK(line.start[1]==-46.75&&line.end[1]==-46.75);
    CHECK(line.start[2]==0.0&&line.end[2]==0.0);
    CHECK(line.end_field_bit==r.pre_handle_bits&&r.geometry_decoded==0);
    CHECK(memcmp(f,backup,sizeof(f))==0);
    MusaDwgAc1015ObjectRecord wrong=r;wrong.record_crc_verified=0;
    MusaDwgAc1015SimpleCommon previous=common;
    CHECK(!musa_dwg_ac1015_read_simple_common(&wrong,&common));
    CHECK(memcmp(&previous,&common,sizeof(common))==0);
    wrong=r;wrong.pre_handle_bits=4u;
    CHECK(!musa_dwg_ac1015_read_simple_common(&wrong,&common));
    CHECK(memcmp(&previous,&common,sizeof(common))==0);
    for(int invalid=1;invalid<=6;invalid++){
        pack(invalid,f,&start);r=get(f);
        CHECK(!musa_dwg_ac1015_read_simple_common(&r,&common));
        CHECK(memcmp(&previous,&common,sizeof(common))==0);
    }
    puts("PASS R2000 checked common self-H/EED0/graphic0/color/scale/flags + LINE source CRC rollback");
    return 0;
}
