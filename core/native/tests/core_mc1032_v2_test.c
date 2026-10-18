/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "musacad_mc1032_v2.h"
#include "musacad_mc1032.h"
#include <assert.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static uint32_t crc(const unsigned char *p,size_t n){
    uint32_t c=0xffffffffu;
    for(size_t i=0;i<n;i++){
        c^=p[i];for(unsigned j=0;j<8;j++)
            c=(c>>1)^(0xedb88320u&(0u-(c&1u)));
    }return ~c;
}
static void fixcrc(unsigned char *data,size_t n){
    uint32_t c=crc(data+48,n-48);
    for(int i=0;i<4;i++)data[36+i]=(unsigned char)(c>>(8*i));
    c=crc(data,40);
    for(int i=0;i<4;i++)data[40+i]=(unsigned char)(c>>(8*i));
}
static void rejected(const unsigned char *data,size_t n,int code){
    MusaMC2Scene *out=(MusaMC2Scene *)(uintptr_t)1;
    assert(musa_mc2_decode(data,n,&out)==code);
    assert(out==NULL);
}
static void equal(const MusaMC2Entity *a,const MusaMC2Entity *b){
    assert(a->handle==b->handle&&a->kind==b->kind&&a->layer==b->layer);
    assert(a->x1==b->x1&&a->y1==b->y1&&a->z1==b->z1);
    assert(a->x2==b->x2&&a->y2==b->y2&&a->z2==b->z2);
}
int main(void){
    assert(!musa_mc2_scene_create(0));
    assert(!musa_mc2_scene_create(MUSA_MC2_MAX_ENTITIES+1u));
    MusaMC2Scene *s=musa_mc2_scene_create(8);
    assert(s);
    uint32_t layer=99,structure=99;
    assert(musa_mc2_scene_layer_count(s)==1u);
    MusaMC2Layer l={0};
    assert(musa_mc2_scene_get_layer(s,0,&l)==MUSA_MC2_OK);
    assert(!strcmp(l.name,"0")&&l.rgba==0xffffffffu);
    assert(musa_mc2_scene_add_layer(s,"Mechanical_Supply",0x18aaffffu,&layer)==MUSA_MC2_OK);
    assert(layer==1u);
    assert(musa_mc2_scene_add_layer(s,"STRUCTURAL-STEEL",0x3344ccffu,&structure)==MUSA_MC2_OK);
    assert(structure==2u);
    assert(musa_mc2_scene_add_layer(s,"Mechanical_Supply",0u,&layer)==MUSA_MC2_INVALID);
    assert(musa_mc2_scene_add_layer(s,"Bad Name",0u,&layer)==MUSA_MC2_INVALID);
    MusaMC2Entity a={0};
    a.kind=MUSA_MC2_LINE3D;a.layer=1;
    a.x1=-11.25;a.y1=3.5;a.z1=450.25;
    a.x2=3.25;a.y2=-10.5;a.z2=905.5;
    uint64_t ha=0;
    assert(musa_mc2_scene_add_entity(s,&a,&ha)==MUSA_MC2_OK&&ha==1u);
    MusaMC2Entity b={0};
    b.kind=MUSA_MC2_POINT3D;b.layer=2;b.handle=205;
    b.x1=1;b.y1=2;b.z1=-99.75;
    uint64_t hb=0;
    assert(musa_mc2_scene_add_entity(s,&b,&hb)==MUSA_MC2_OK&&hb==205u);
    MusaMC2Entity duplicate=b;
    assert(musa_mc2_scene_add_entity(s,&duplicate,&hb)==MUSA_MC2_INVALID);
    MusaMC2Entity invalid=b;invalid.handle=0;invalid.x2=1;
    assert(musa_mc2_scene_add_entity(s,&invalid,&hb)==MUSA_MC2_INVALID);
    invalid=b;invalid.handle=0;invalid.z1=NAN;
    assert(musa_mc2_scene_add_entity(s,&invalid,&hb)==MUSA_MC2_INVALID);
    invalid=b;invalid.handle=0;invalid.layer=3;
    assert(musa_mc2_scene_add_entity(s,&invalid,&hb)==MUSA_MC2_INVALID);
    size_t n=0;
    assert(musa_mc2_encode(s,NULL,0,&n)==MUSA_MC2_OK);
    assert(n==48+3*64+2*80);
    unsigned char *bytes=(unsigned char *)malloc(n),*bad=(unsigned char *)malloc(n);
    assert(bytes&&bad);
    memset(bytes,0x9c,n);
    assert(musa_mc2_encode(s,bytes,n-1,&n)==MUSA_MC2_LIMIT);
    for(size_t i=0;i<n;i++)assert(bytes[i]==0x9c);
    assert(musa_mc2_encode(s,bytes,n,&n)==MUSA_MC2_OK);
    assert(memcmp(bytes,"MC1032",6)==0&&bytes[6]==2);
    /* MC1032 v1 codec must reject v2 without reinterpreting it. */
    MusaDocument *old=(MusaDocument *)(uintptr_t)1;
    assert(musa_mc1032_decode(bytes,n,&old)==MUSA_MC1032_UNSUPPORTED);
    assert(old==NULL);
    MusaMC2Scene *t=NULL;
    assert(musa_mc2_decode(bytes,n,&t)==MUSA_MC2_OK&&t);
    assert(musa_mc2_scene_entity_count(t)==2u);
    assert(musa_mc2_scene_layer_count(t)==3u);
    for(uint32_t i=0;i<2;i++){
        MusaMC2Entity x={0},y={0};
        assert(musa_mc2_scene_get_index(s,i,&x)==MUSA_MC2_OK);
        assert(musa_mc2_scene_get_index(t,i,&y)==MUSA_MC2_OK);
        equal(&x,&y);
    }
    assert(musa_mc2_scene_get_layer(t,1u,&l)==MUSA_MC2_OK);
    assert(!strcmp(l.name,"Mechanical_Supply")&&l.rgba==0x18aaffffu);
    assert(musa_mc2_scene_translate(t,1u,1.0,2.5,-5.0)==MUSA_MC2_OK);
    MusaMC2Entity moved={0};
    assert(musa_mc2_scene_get(t,1,&moved)==MUSA_MC2_OK);
    assert(moved.x1==-10.25&&moved.y1==6&&moved.z1==445.25);
    assert(moved.x2==4.25&&moved.y2==-8&&moved.z2==900.5);
    assert(musa_mc2_scene_translate(t,205u,1.0e308,0,0)==MUSA_MC2_OK);
    assert(musa_mc2_scene_translate(t,205u,1.0e308,0,0)==MUSA_MC2_LIMIT);
    assert(musa_mc2_scene_get(t,205u,&moved)==MUSA_MC2_OK&&moved.x1==1.0e308);
    assert(musa_mc2_scene_delete(t,205u)==MUSA_MC2_OK);
    assert(musa_mc2_scene_get(t,205u,&moved)==MUSA_MC2_NOT_FOUND);
    MusaMC2Entity c={0};c.kind=MUSA_MC2_POINT3D;c.layer=0;
    uint64_t hc=0;
    assert(musa_mc2_scene_add_entity(t,&c,&hc)==MUSA_MC2_OK&&hc==206u);
    size_t edited=0;
    assert(musa_mc2_encode(t,NULL,0,&edited)==MUSA_MC2_OK&&edited==n);
    unsigned char *tmp=(unsigned char *)malloc(edited);
    assert(tmp&&musa_mc2_encode(t,tmp,edited,&edited)==MUSA_MC2_OK);
    MusaMC2Scene *reopen=NULL;
    assert(musa_mc2_decode(tmp,edited,&reopen)==MUSA_MC2_OK);
    assert(musa_mc2_scene_get(reopen,1,&moved)==MUSA_MC2_OK&&moved.z1==445.25);
    assert(musa_mc2_scene_get(reopen,hc,&moved)==MUSA_MC2_OK);
    assert(musa_mc2_scene_get(reopen,205,&moved)==MUSA_MC2_NOT_FOUND);
    musa_mc2_scene_destroy(reopen);free(tmp);
    /* Confirm exact portable bytes written through host OS stdio. */
    FILE *disk=tmpfile();assert(disk);
    assert(fwrite(bytes,1,n,disk)==n&&fflush(disk)==0&&fseek(disk,0,SEEK_SET)==0);
    memset(bad,0,n);assert(fread(bad,1,n,disk)==n&&fclose(disk)==0);
    assert(memcmp(bytes,bad,n)==0);
    assert(musa_mc2_decode(bad,n,&reopen)==MUSA_MC2_OK);
    musa_mc2_scene_destroy(reopen);
    memcpy(bad,bytes,n);bad[0]='A';rejected(bad,n,MUSA_MC2_INVALID);
    memcpy(bad,bytes,n);bad[6]=3;rejected(bad,n,MUSA_MC2_UNSUPPORTED);
    memcpy(bad,bytes,n);bad[7]=1;rejected(bad,n,MUSA_MC2_UNSUPPORTED);
    memcpy(bad,bytes,n);bad[40]^=1;rejected(bad,n,MUSA_MC2_CORRUPT);
    memcpy(bad,bytes,n);bad[n-1]^=1;rejected(bad,n,MUSA_MC2_CORRUPT);
    rejected(bytes,n-1,MUSA_MC2_CORRUPT);
    /* Unknown layer names, reserved flags, dangling layer and duplicate
     * handles must not pass even when an attacker repairs both CRC values. */
    memcpy(bad,bytes,n);
    bad[48+16]='?';fixcrc(bad,n);rejected(bad,n,MUSA_MC2_UNSUPPORTED);
    memcpy(bad,bytes,n);
    bad[48+64+8]=2;fixcrc(bad,n);rejected(bad,n,MUSA_MC2_UNSUPPORTED);
    memcpy(bad,bytes,n);
    bad[48+3*64+12]=55;fixcrc(bad,n);rejected(bad,n,MUSA_MC2_UNSUPPORTED);
    memcpy(bad,bytes,n);
    memcpy(bad+48+3*64+80,bad+48+3*64,8);
    fixcrc(bad,n);rejected(bad,n,MUSA_MC2_CORRUPT);
    memcpy(bad,bytes,n);
    bad[48+3*64+64]=1;fixcrc(bad,n);rejected(bad,n,MUSA_MC2_UNSUPPORTED);
    /* Rev2 golden fixed header for one default layer and zero entities. */
    MusaMC2Scene *empty=musa_mc2_scene_create(1);
    unsigned char expected[48]={
        0x4d,0x43,0x31,0x30,0x33,0x32,0x02,0x00,
        0x04,0x03,0x02,0x01,0x30,0,0,0,
        0x40,0,0,0,0x50,0,0,0,0x01,0,0,0,0,0,0,0,
        0x40,0,0,0,0xdd,0xb9,0x3c,0x4c,
        0x9f,0x65,0xa1,0xc9,0,0,0,0
    };
    unsigned char golden[112]={0};size_t golden_len=0;
    assert(empty&&musa_mc2_encode(empty,golden,sizeof(golden),&golden_len)==MUSA_MC2_OK);
    assert(golden_len==112&&memcmp(expected,golden,48)==0);
    assert(musa_mc2_decode(golden,golden_len,&reopen)==MUSA_MC2_OK);
    musa_mc2_scene_destroy(reopen);musa_mc2_scene_destroy(empty);
    musa_mc2_scene_destroy(s);musa_mc2_scene_destroy(t);
    free(bytes);free(bad);
    puts("PASS: MC1032 rev2 3D layers write-read-edit, legacy isolation and corruption fail-closed");
    return 0;
}
