/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "musacad_dwg_ac1015_frame.h"
#include "musacad_crc16.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
static const uint8_t header_begin[16]={
    0xcfu,0x7bu,0x1fu,0x23u,0xfdu,0xdeu,0x38u,0xa9u,
    0x5fu,0x7cu,0x68u,0xb8u,0x4eu,0x6du,0x33u,0x5fu
};
static const uint8_t class_begin[16]={
    0x8du,0xa1u,0xc4u,0xb8u,0xc4u,0xa9u,0xf8u,0xc5u,
    0xc0u,0xdcu,0xf4u,0x5fu,0xe7u,0xcfu,0xb6u,0x8au
};
static const uint8_t header_end[16]={
    0x95u,0xa0u,0x4eu,0x28u,0x99u,0x82u,0x1au,0xe5u,
    0x5eu,0x41u,0xe0u,0x5fu,0x9du,0x3au,0x4du,0x00u
};
static void le32(uint8_t *p,uint32_t x){
    p[0]=(uint8_t)x;p[1]=(uint8_t)(x>>8);
    p[2]=(uint8_t)(x>>16);p[3]=(uint8_t)(x>>24);
}
static void frame(uint8_t *p,const uint8_t *begin,uint32_t n){
    memcpy(p,begin,16u);
    le32(p+16u,n);
    for(uint32_t i=0;i<n;i++)p[20u+i]=(uint8_t)((i*7u+13u)&255u);
    /* Independent fixed golden CRCs: seed 0xC0C1 over LE32(n)+data.
     * Recomputed outside the implementation, not a self-verifying test. */
    assert(n==20u||n==8u);
    const uint16_t golden=n==20u?0xedf4u:0x9912u;
    p[20u+n]=(uint8_t)golden;
    p[21u+n]=(uint8_t)(golden>>8);
    for(size_t i=0u;i<16u;i++)p[22u+n+i]=(uint8_t)(begin[i]^0xffu);
}
static void repair_crc(uint8_t *file){
    uint16_t crc=0u;
    assert(musa_core_crc16_arc(0u,file,0x19u+3u*9u,&crc));
    crc^=0xa598u;
    const size_t at=0x19u+3u*9u;
    file[at]=(uint8_t)crc;file[at+1u]=(uint8_t)(crc>>8);
}
static void fixture(uint8_t *file){
    memset(file,0,512u);
    memcpy(file,"AC1015",6u);
    file[12]=1u;
    file[0x13u]=0xe4u;file[0x14u]=0x04u;
    le32(file+0x15u,3u);
    const uint32_t offset[]={128u,224u,304u};
    const uint32_t size[]={58u,46u,64u};
    for(size_t i=0u;i<3u;i++){
        const size_t at=0x19u+9u*i;
        file[at]=(uint8_t)i;
        le32(file+at+1u,offset[i]);
        le32(file+at+5u,size[i]);
    }
    memcpy(file+0x19u+3u*9u+2u,header_end,16u);
    repair_crc(file);
    frame(file+128u,header_begin,20u);
    frame(file+224u,class_begin,8u);
}
static void rejected_crc(const uint8_t *bytes,size_t n,uint32_t kind){
    MusaDwgAc1015Frame out={0};
    out.payload=bytes?bytes+4u:NULL;
    out.payload_bytes=777u;
    out.stored_payload_crc_unverified=0x1255u;
    out.section_kind=98u;
    out.envelope_verified=17;
    out.payload_crc_verified=33;
    const MusaDwgAc1015Frame initial=out;
    assert(!musa_dwg_ac1015_parse_crc_verified_frame(bytes,n,kind,&out));
    assert(memcmp(&out,&initial,sizeof(out))==0);
}
static void rejected(const uint8_t *bytes,size_t n,uint32_t kind){
    MusaDwgAc1015Frame out={0};
    out.payload=bytes?bytes+4u:NULL;
    out.payload_bytes=777u;
    out.stored_payload_crc_unverified=0x1255u;
    out.section_kind=98u;
    out.envelope_verified=17;
    out.payload_crc_verified=33;
    const MusaDwgAc1015Frame initial=out;
    assert(!musa_dwg_ac1015_parse_frame(bytes,n,kind,&out));
    assert(memcmp(&out,&initial,sizeof(out))==0);
}
int main(void){
    uint8_t x[512],before[512];
    fixture(x);memcpy(before,x,sizeof(x));
    for(uint32_t k=0;k<2u;k++){
        MusaDwgAc1015Frame out={0};
        assert(musa_dwg_ac1015_parse_frame(x,sizeof(x),k,&out));
        assert(out.section_kind==k&&out.envelope_verified==1);
        assert(out.payload_crc_verified==0);
        assert(out.payload==(k==0u?x+148u:x+244u));
        assert(out.payload_bytes==(k==0u?20u:8u));
        const uint16_t golden=k==0u?0xedf4u:0x9912u;
        assert(out.stored_payload_crc_unverified==golden);
        MusaDwgAc1015Frame verified={0};
        assert(musa_dwg_ac1015_parse_crc_verified_frame(
            x,sizeof(x),k,&verified));
        assert(verified.envelope_verified==1&&verified.payload_crc_verified==1);
        assert(verified.payload==out.payload&&verified.payload_bytes==out.payload_bytes);
        assert(verified.stored_payload_crc_unverified==golden);
    }
    assert(memcmp(before,x,sizeof(x))==0);
    rejected(x,sizeof(x),2u);rejected(x,sizeof(x),UINT32_MAX);
    rejected(x,310u,0u);rejected(x,sizeof(x)-450u,1u);
    rejected(NULL,sizeof(x),0u);
    assert(!musa_dwg_ac1015_parse_frame(x,sizeof(x),0u,NULL));
    /* Forged frame start, corrupted end and length mismatch all refuse. */
    x[128u]^=1u;rejected(x,sizeof(x),0u);x[128u]^=1u;
    x[128u+58u-1u]^=1u;rejected(x,sizeof(x),0u);
    x[128u+58u-1u]^=1u;
    x[128u+16u]^=1u;rejected(x,sizeof(x),0u);x[128u+16u]^=1u;
    x[224u]^=1u;rejected(x,sizeof(x),1u);x[224u]^=1u;
    x[224u+46u-1u]^=1u;rejected(x,sizeof(x),1u);
    x[224u+46u-1u]^=1u;
    x[224u+16u]=0xffu;rejected(x,sizeof(x),1u);
    x[224u+16u]=8u;
    /* Header mutated -> file-level CRC blocks all section access. */
    x[0x13u]^=1u;rejected(x,sizeof(x),0u);rejected(x,sizeof(x),1u);
    x[0x13u]^=1u;
    /* Valid header but invalid locator length must fail closed. */
    le32(x+0x19u+5u,37u);repair_crc(x);
    rejected(x,sizeof(x),0u);
    le32(x+0x19u+5u,58u);repair_crc(x);
    /* Backward-compatible envelope API does not assert CRC correctness;
     * strict API refuses the same damaged original bytes. */
    x[148u]^=1u;
    MusaDwgAc1015Frame out={0};
    assert(musa_dwg_ac1015_parse_frame(x,sizeof(x),0u,&out));
    assert(out.payload_crc_verified==0&&out.envelope_verified==1);
    rejected_crc(x,sizeof(x),0u);
    x[148u]^=1u;
    /* Each tamper leaves the header CRC valid but the DATA CRC invalid.
     * The old envelope-only API still accepts such data by design. */
    rejected_crc(x,sizeof(x),0u);
    x[148u]^=1u;
    rejected_crc(x,sizeof(x),0u);
    x[148u]^=1u;
    x[244u]^=1u;
    rejected_crc(x,sizeof(x),1u);
    x[244u]^=1u;
    x[168u]^=0x40u; /* HEADER stored section CRC */
    rejected_crc(x,sizeof(x),0u);
    x[168u]^=0x40u;
    x[252u]^=0x40u; /* CLASSES stored section CRC */
    rejected_crc(x,sizeof(x),1u);
    x[252u]^=0x40u;
    rejected_crc(x,sizeof(x),2u);
    rejected_crc(x,310u,0u);
    rejected_crc(NULL,sizeof(x),0u);
    assert(!musa_dwg_ac1015_parse_crc_verified_frame(
        x,sizeof(x),0u,NULL));
    /* CRC is not a signature: an attacker can rewrite payload AND CRC.
     * Never call this cryptographic authentication or decoded DWG data. */
    x[148u]^=1u;
    uint16_t rewritten=0u;
    assert(musa_core_crc16_arc(0xc0c1u,x+144u,24u,&rewritten));
    x[168u]=(uint8_t)rewritten;
    x[169u]=(uint8_t)(rewritten>>8);
    MusaDwgAc1015Frame forged={0};
    assert(musa_dwg_ac1015_parse_crc_verified_frame(
        x,sizeof(x),0u,&forged));
    assert(forged.payload_crc_verified==1);
    x[148u]^=1u;
    x[168u]=0xf4u;
    x[169u]=0xedu;
    assert(memcmp(before,x,sizeof(x))==0);
    puts("PASS: AC1015 HEADER/CLASSES seeded CRC-16, tamper rejection, unmodified outputs and no-authentication boundary");
    return 0;
}
