/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "musacad_dwg_ac1015.h"
#include "musacad_dwg_signature.h"
#include "musacad_bits.h"
#include "musacad_crc16.h"
#include <stdint.h>
#include <string.h>

/* ODA Open Design DWG format, R13-R15 file header chapter: legacy R2000.
 * The 16-byte post-CRC sentinel is format metadata, not executable code. */
static const uint8_t end_of_file_header[16]={
    0x95u,0xa0u,0x4eu,0x28u,0x99u,0x82u,0x1au,0xe5u,
    0x5eu,0x41u,0xe0u,0x5fu,0x9du,0x3au,0x4du,0x00u
};
/* Native DWG raw integers are byte-aligned, little endian; use the
 * existing, independently tested bounded C bit stream for every read. */
static int read_at(MusaBits *bits,size_t byte_offset,unsigned width,
                   uint64_t *out){
    if(!bits||!out||byte_offset>SIZE_MAX/8u)return 0;
    if(!musa_bits_seek(bits,byte_offset*8u))return 0;
    return musa_bits_read(bits,width,out);
}
int musa_dwg_ac1015_read_locators(const uint8_t *bytes,size_t n,
                                  MusaDwgAc1015Locators *out){
    if(!bytes||!out||n<0x19u||n>SIZE_MAX/8u||
       musa_core_dwg_identify_signature(bytes,n)!=MUSA_DWG_2000)return 0;
    MusaBits bits={0};
    if(!musa_bits_reader(&bits,bytes,n))return 0;
    uint64_t count=0u,image=0u,codepage=0u;
    if(!read_at(&bits,0x15u,32u,&count)||count<3u||
       count>MUSA_DWG_AC1015_MAX_LOCATORS)return 0;
    const size_t table_end=0x19u+(size_t)count*9u;
    const size_t header_end=table_end+2u+sizeof(end_of_file_header);
    if(n<header_end)return 0;
    uint64_t stored_crc=0u;
    if(memcmp(bytes+table_end+2u,end_of_file_header,
              sizeof(end_of_file_header))!=0||
       !read_at(&bits,0x0du,32u,&image)||
       !read_at(&bits,0x13u,16u,&codepage)||
       !read_at(&bits,table_end,16u,&stored_crc))return 0;
    /* The R13-R15 header checksum is computed from BOF through the final
     * locator record with initial 0 and a count-dependent XOR constant.
     * The CRC polynomial is independently implemented in MusaCAD Core. */
    uint16_t calculated_crc=0u;
    const uint16_t xor_mask=count==3u?0xa598u:
                            count==4u?0x8101u:0x3cc4u;
    if(!musa_core_crc16_arc(0u,bytes,table_end,&calculated_crc)||
       (uint16_t)(calculated_crc^xor_mask)!=(uint16_t)stored_crc)
        return 0;
    MusaDwgAc1015Locators result={0};
    result.codepage=(uint16_t)codepage;
    result.stored_header_crc=(uint16_t)stored_crc;
    result.image_seeker_unverified=(uint32_t)image;
    result.region_count=(uint32_t)count;
    result.header_end=(uint32_t)header_end;
    result.crc_verified=1;
    uint32_t seen=0u;
    for(size_t i=0u;i<(size_t)count;i++){
        const size_t pos=0x19u+i*9u;
        uint64_t id=0u,offset=0u,length=0u;
        if(!read_at(&bits,pos,8u,&id)||
           !read_at(&bits,pos+1u,32u,&offset)||
           !read_at(&bits,pos+5u,32u,&length)||
           id>4u||(seen&(1u<<(unsigned)id))!=0u||
           offset<header_end)return 0;
        seen|=1u<<(unsigned)id;
        result.regions[i].section_kind=(uint32_t)id;
        result.regions[i].offset=offset;
        result.regions[i].length=length;
    }
    /* Required records: DWG header variables, class and object map.
     * Unknown sixth record layout is intentionally refused. */
    if((seen&7u)!=7u||
       !musa_dwg_regions_validate((uint64_t)n,result.regions,
                                  (size_t)count))return 0;
    *out=result;
    return 1;
}
