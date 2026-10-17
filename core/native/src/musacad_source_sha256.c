/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Portable SHA-256 streaming compression following FIPS 180-4 section 6.2.2.
 * No dependency on OpenSSL, Android, Windows CryptoAPI, or Apple CommonCrypto.
 * Native fixtures include NIST empty/abc/million-'a' vectors and chunk parity.
 */
#include "musacad_source_sha256.h"
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

struct MusaSourceSha256 {
    uint32_t h[8];
    uint64_t total;
    unsigned char block[64];
    size_t used;
    int finished;
};
static const uint32_t K[64]={
    0x428a2f98u,0x71374491u,0xb5c0fbcfu,0xe9b5dba5u,
    0x3956c25bu,0x59f111f1u,0x923f82a4u,0xab1c5ed5u,
    0xd807aa98u,0x12835b01u,0x243185beu,0x550c7dc3u,
    0x72be5d74u,0x80deb1feu,0x9bdc06a7u,0xc19bf174u,
    0xe49b69c1u,0xefbe4786u,0x0fc19dc6u,0x240ca1ccu,
    0x2de92c6fu,0x4a7484aau,0x5cb0a9dcu,0x76f988dau,
    0x983e5152u,0xa831c66du,0xb00327c8u,0xbf597fc7u,
    0xc6e00bf3u,0xd5a79147u,0x06ca6351u,0x14292967u,
    0x27b70a85u,0x2e1b2138u,0x4d2c6dfcu,0x53380d13u,
    0x650a7354u,0x766a0abbu,0x81c2c92eu,0x92722c85u,
    0xa2bfe8a1u,0xa81a664bu,0xc24b8b70u,0xc76c51a3u,
    0xd192e819u,0xd6990624u,0xf40e3585u,0x106aa070u,
    0x19a4c116u,0x1e376c08u,0x2748774cu,0x34b0bcb5u,
    0x391c0cb3u,0x4ed8aa4au,0x5b9cca4fu,0x682e6ff3u,
    0x748f82eeu,0x78a5636fu,0x84c87814u,0x8cc70208u,
    0x90befffau,0xa4506cebu,0xbef9a3f7u,0xc67178f2u
};
static uint32_t ror(uint32_t x,unsigned int n){
    return (x>>n)|(x<<(32u-n));
}
static void compress(MusaSourceSha256 *s,const unsigned char data[64]){
    uint32_t w[64];
    for(size_t i=0;i<16;i++){
        const size_t j=4*i;
        w[i]=((uint32_t)data[j]<<24)|((uint32_t)data[j+1]<<16)|
             ((uint32_t)data[j+2]<<8)|(uint32_t)data[j+3];
    }
    for(size_t i=16;i<64;i++){
        const uint32_t x=w[i-15],y=w[i-2];
        const uint32_t sig0=ror(x,7)^ror(x,18)^(x>>3);
        const uint32_t sig1=ror(y,17)^ror(y,19)^(y>>10);
        w[i]=w[i-16]+sig0+w[i-7]+sig1;
    }
    uint32_t a=s->h[0],b=s->h[1],c=s->h[2],d=s->h[3];
    uint32_t e=s->h[4],f=s->h[5],g=s->h[6],h=s->h[7];
    for(size_t i=0;i<64;i++){
        const uint32_t t1=h+(ror(e,6)^ror(e,11)^ror(e,25))+
                          ((e&f)^((~e)&g))+K[i]+w[i];
        const uint32_t t2=(ror(a,2)^ror(a,13)^ror(a,22))+
                          ((a&b)^(a&c)^(b&c));
        h=g;g=f;f=e;e=d+t1;d=c;c=b;b=a;a=t1+t2;
    }
    s->h[0]+=a;s->h[1]+=b;s->h[2]+=c;s->h[3]+=d;
    s->h[4]+=e;s->h[5]+=f;s->h[6]+=g;s->h[7]+=h;
    memset(w,0,sizeof(w));
}
MusaSourceSha256 *musa_core_source_sha256_create(void){
    MusaSourceSha256 *s=(MusaSourceSha256 *)calloc(1,sizeof(*s));
    if(!s)return NULL;
    static const uint32_t initial[8]={
        0x6a09e667u,0xbb67ae85u,0x3c6ef372u,0xa54ff53au,
        0x510e527fu,0x9b05688cu,0x1f83d9abu,0x5be0cd19u
    };
    memcpy(s->h,initial,sizeof(initial));
    return s;
}
int musa_core_source_sha256_update(MusaSourceSha256 *s,
                                  const unsigned char *data,size_t len){
    if(!s||s->finished||(!data&&len))return 0;
    /* SHA-256 message length is encoded in 64 bits measured in bits. */
    if((uint64_t)len>(UINT64_MAX/8u)-s->total)return 0;
    s->total+=(uint64_t)len;
    while(len){
        const size_t capacity=64u-s->used;
        const size_t n=len<capacity?len:capacity;
        memcpy(s->block+s->used,data,n);
        s->used+=n;data+=n;len-=n;
        if(s->used==64u){
            compress(s,s->block);
            s->used=0;
        }
    }
    return 1;
}
int musa_core_source_sha256_finish(MusaSourceSha256 *s,
                                  unsigned char out_digest[32]){
    if(!s||!out_digest||s->finished)return 0;
    const uint64_t bits=s->total*8u;
    s->block[s->used++]=0x80u;
    if(s->used>56u){
        memset(s->block+s->used,0,64u-s->used);
        compress(s,s->block);
        s->used=0;
    }
    memset(s->block+s->used,0,56u-s->used);
    for(size_t i=0;i<8;i++)
        s->block[56+i]=(unsigned char)(bits>>(56u-8u*i));
    compress(s,s->block);
    for(size_t i=0;i<8;i++){
        out_digest[4*i]=(unsigned char)(s->h[i]>>24);
        out_digest[4*i+1]=(unsigned char)(s->h[i]>>16);
        out_digest[4*i+2]=(unsigned char)(s->h[i]>>8);
        out_digest[4*i+3]=(unsigned char)s->h[i];
    }
    s->finished=1;
    return 1;
}
void musa_core_source_sha256_destroy(MusaSourceSha256 *s){
    if(s){memset(s,0,sizeof(*s));free(s);}
}
