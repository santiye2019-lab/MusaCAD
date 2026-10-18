/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "musacad_mc1032.h"
#include <assert.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void same(const MusaDocumentEntity *a,const MusaDocumentEntity *b){
    assert(a->kind==b->kind&&a->handle==b->handle);
    assert(a->x1==b->x1&&a->y1==b->y1&&a->x2==b->x2&&a->y2==b->y2);
    assert(a->radius==b->radius);
    assert(a->start_deg==b->start_deg&&a->end_deg==b->end_deg);
}
static uint32_t crc32(const unsigned char *b,size_t n){
    uint32_t c=0xffffffffu;
    for(size_t i=0;i<n;i++){
        c^=b[i];
        for(int j=0;j<8;j++)c=(c>>1)^((0u-(c&1u))&0xedb88320u);
    }
    return ~c;
}
static void rechecksum(unsigned char *buf,size_t size){
    uint32_t c=crc32(buf+32,size-32);
    for(int k=0;k<4;k++)buf[24+k]=(unsigned char)(c>>(k*8));
    c=crc32(buf,28);
    for(int k=0;k<4;k++)buf[28+k]=(unsigned char)(c>>(k*8));
}
static void reject(const unsigned char *buf,size_t bytes,int status){
    MusaDocument *partial=(MusaDocument *)(uintptr_t)1;
    const int actual=musa_mc1032_decode(buf,bytes,&partial);
    assert(actual==status);
    assert(partial==NULL);
}
int main(void){
    /* Cross-platform golden fixture: same empty 32 bytes on all toolchains. */
    static const unsigned char golden_empty[32]={
        0x4d,0x43,0x31,0x30,0x33,0x32,0x01,0x00,
        0x04,0x03,0x02,0x01,0x20,0x00,0x00,0x00,
        0x48,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
        0x00,0x00,0x00,0x00,0xc3,0x11,0x45,0xb2
    };
    MusaDocument *empty=musa_document_create(1);
    assert(empty);
    unsigned char empty_out[32]={0};
    size_t written=0;
    assert(musa_mc1032_encode(empty,empty_out,sizeof(empty_out),&written)==MUSA_MC1032_OK);
    assert(written==32&&memcmp(empty_out,golden_empty,32)==0);
    MusaDocument *round=NULL;
    assert(musa_mc1032_decode(empty_out,32,&round)==MUSA_MC1032_OK);
    assert(round&&musa_document_count(round)==0);
    musa_document_destroy(round);musa_document_destroy(empty);

    MusaDocument *source=musa_document_create(8);
    assert(source);
    uint64_t a=0,b=0,c=0,d=0;
    assert(musa_document_add_line(source,-100.25,0.125,23.5,44.75,&a)==MUSA_DOCUMENT_OK);
    assert(musa_document_add_circle(source,1.5,2.25,5.75,&b)==MUSA_DOCUMENT_OK);
    assert(musa_document_add_arc(source,-3.25,2,4,30,270,&c)==MUSA_DOCUMENT_OK);
    assert(musa_document_add_point(source,12345.125,-7.5,&d)==MUSA_DOCUMENT_OK);
    const size_t size=musa_mc1032_encoded_size(source);
    assert(size==32+4*72);
    assert(musa_mc1032_encode(source,NULL,0,&written)==MUSA_MC1032_OK&&written==size);
    unsigned char *bytes=(unsigned char *)malloc(size);
    unsigned char *copy=(unsigned char *)malloc(size);
    assert(bytes&&copy);
    memset(bytes,0xa5,size);
    assert(musa_mc1032_encode(source,bytes,size-1,&written)==MUSA_MC1032_LIMIT);
    for(size_t i=0;i<size;i++)assert(bytes[i]==0xa5); /* atomic failure */
    assert(musa_mc1032_encode(source,bytes,size,&written)==MUSA_MC1032_OK);
    assert(written==size&&memcmp(bytes,"MC1032",6)==0);
    /* Real standard-C file IO on Linux/Windows/macOS; byte codec is identical.
     * This is a test file, never the user's original drawing.
     */
    FILE *on_disk=tmpfile();
    assert(on_disk);
    assert(fwrite(bytes,1u,size,on_disk)==size);
    assert(fflush(on_disk)==0);
    assert(fseek(on_disk,0,SEEK_SET)==0);
    unsigned char *from_disk=(unsigned char *)malloc(size);
    assert(from_disk);
    assert(fread(from_disk,1u,size,on_disk)==size);
    assert(fclose(on_disk)==0);
    MusaDocument *file_reopened=NULL;
    assert(musa_mc1032_decode(from_disk,size,&file_reopened)==MUSA_MC1032_OK);
    assert(file_reopened && musa_document_count(file_reopened)==4);
    musa_document_destroy(file_reopened);
    free(from_disk);
    assert(musa_mc1032_decode(bytes,size,&round)==MUSA_MC1032_OK);
    assert(round && musa_document_count(round)==4);
    for(size_t i=0;i<4;i++){
        MusaDocumentEntity e={0},f={0};
        assert(musa_document_get_index(source,i,&e)==MUSA_DOCUMENT_OK);
        assert(musa_document_get_index(round,i,&f)==MUSA_DOCUMENT_OK);
        same(&e,&f);
    }
    assert(musa_document_translate(round,a,-0.5,1.0)==MUSA_DOCUMENT_OK);
    assert(musa_document_delete(round,b)==MUSA_DOCUMENT_OK);
    unsigned char again[32+4*72]={0};
    assert(musa_mc1032_encode(round,again,sizeof(again),&written)==MUSA_MC1032_OK);
    MusaDocument *edited=NULL;
    assert(musa_mc1032_decode(again,written,&edited)==MUSA_MC1032_OK);
    assert(musa_document_count(edited)==3);
    MusaDocumentEntity check={0};
    assert(musa_document_get(edited,a,&check)==MUSA_DOCUMENT_OK);
    assert(check.x1==-100.75&&check.y1==1.125);
    assert(musa_document_get(edited,b,&check)==MUSA_DOCUMENT_NOT_FOUND);
    assert(musa_document_get(edited,c,&check)==MUSA_DOCUMENT_OK);
    musa_document_destroy(edited);musa_document_destroy(round);

    /* Genuine DWG AC1032 must NEVER be misidentified as MC1032. */
    reject((const unsigned char *)"AC1032",6,MUSA_MC1032_INVALID);
    reject(bytes,31,MUSA_MC1032_INVALID);
    reject(bytes,size-1,MUSA_MC1032_CORRUPT);
    memcpy(copy,bytes,size);copy[0]='A';reject(copy,size,MUSA_MC1032_INVALID);
    memcpy(copy,bytes,size);copy[6]=2;reject(copy,size,MUSA_MC1032_UNSUPPORTED);
    memcpy(copy,bytes,size);copy[7]=1;reject(copy,size,MUSA_MC1032_UNSUPPORTED);
    memcpy(copy,bytes,size);copy[8]=0;reject(copy,size,MUSA_MC1032_UNSUPPORTED);
    memcpy(copy,bytes,size);copy[32+16]^=1;reject(copy,size,MUSA_MC1032_CORRUPT);
    memcpy(copy,bytes,size);copy[28]^=1;reject(copy,size,MUSA_MC1032_CORRUPT);
    memcpy(copy,bytes,size);copy[32+12]=1;rechecksum(copy,size);
    reject(copy,size,MUSA_MC1032_UNSUPPORTED);
    memcpy(copy,bytes,size);copy[32+8]=255;rechecksum(copy,size);
    reject(copy,size,MUSA_MC1032_UNSUPPORTED);
    memcpy(copy,bytes,size);
    /* duplicate handle in second object: payload checksum is valid. */
    memcpy(copy+32+72,copy+32,8);rechecksum(copy,size);
    reject(copy,size,MUSA_MC1032_CORRUPT);
    memcpy(copy,bytes,size);
    /* NaN coordinate with correct file checksum still must be rejected. */
    for(int k=0;k<8;k++)copy[32+16+k]=k==6?0xf8:(k==7?0x7f:0);
    rechecksum(copy,size);
    reject(copy,size,MUSA_MC1032_UNSUPPORTED);
    memcpy(copy,bytes,size);
    reject(copy,size-72,MUSA_MC1032_CORRUPT);
    unsigned char trailing[32+4*72+1]={0};
    memcpy(trailing,bytes,size);
    reject(trailing,sizeof(trailing),MUSA_MC1032_CORRUPT);
    assert(musa_mc1032_decode(bytes,size,NULL)==MUSA_MC1032_INVALID);
    musa_document_destroy(source);
    free(copy);free(bytes);
    puts("PASS: first-party MC1032 binary golden fixture, edit-write-read, CRC, bounds and identity");
    return 0;
}
