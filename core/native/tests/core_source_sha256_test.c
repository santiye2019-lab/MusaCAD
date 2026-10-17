/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "musacad_source_sha256.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static void verify(const unsigned char *input,size_t len,const char *hex){
    MusaSourceSha256 *s=musa_core_source_sha256_create();
    unsigned char got[32]={0};
    assert(s);
    /* Every possible simple split should produce the same hash. */
    for(size_t i=0;i<len;i++)
        assert(musa_core_source_sha256_update(s,input+i,1));
    assert(musa_core_source_sha256_finish(s,got));
    assert(!musa_core_source_sha256_finish(s,got));
    assert(!musa_core_source_sha256_update(s,input,len));
    static const char digits[]="0123456789abcdef";
    for(size_t i=0;i<32;i++){
        assert(digits[got[i]>>4]==hex[2*i]);
        assert(digits[got[i]&15]==hex[2*i+1]);
    }
    musa_core_source_sha256_destroy(s);
}
int main(void){
    verify((const unsigned char *)"",0,
      "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855");
    verify((const unsigned char *)"abc",3,
      "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad");
    MusaSourceSha256 *s=musa_core_source_sha256_create();
    unsigned char buf[1000],got[32]={0};
    assert(s);
    memset(buf,'a',sizeof(buf));
    for(int i=0;i<1000;i++)
        assert(musa_core_source_sha256_update(s,buf,sizeof(buf)));
    assert(musa_core_source_sha256_finish(s,got));
    const char *expected="cdc76e5c9914fb9281a1c7e284d73e67f1809a48a497200e046d39ccc7112cd0";
    static const char digits[]="0123456789abcdef";
    for(size_t i=0;i<32;i++){
        assert(digits[got[i]>>4]==expected[2*i]);
        assert(digits[got[i]&15]==expected[2*i+1]);
    }
    musa_core_source_sha256_destroy(s);
    assert(!musa_core_source_sha256_update(NULL,buf,10));
    puts("PASS: FIPS SHA-256 empty/abc/million-a cross-platform source digest");
    return 0;
}
