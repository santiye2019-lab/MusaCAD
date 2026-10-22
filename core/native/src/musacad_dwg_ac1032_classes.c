#include "musacad_dwg_ac1032_classes.h"
#include "musacad_dwg_ac1032_sections.h"
#include <stdlib.h>
#include <string.h>
static const uint8_t class_marker[16]={
    0x8d,0xa1,0xc4,0xb8,0xc4,0xa9,0xf8,0xc5,
    0xc0,0xdc,0xf4,0x5f,0xe7,0xcf,0xb6,0x8a
};
static uint32_t u32(const uint8_t *p){
    return (uint32_t)p[0]|((uint32_t)p[1]<<8u)|
           ((uint32_t)p[2]<<16u)|((uint32_t)p[3]<<24u);
}
static int nonoverlap(const void *a,size_t n,const void *b,size_t m){
    const uintptr_t x=(uintptr_t)a,y=(uintptr_t)b;
    return !((x<=y&&y-x<n)||(y<x&&x-y<m));
}
int musa_dwg_ac1032_classes_frame_probe(
    const uint8_t *file,size_t len,MusaDwgAc1032ClassesFrame *out){
    if(!file||!out||len<256u||len>64u*1024u*1024u||
       !nonoverlap(file,len,out,sizeof(*out)))return 0;
    size_t count=0u;
    if(!musa_dwg_ac1032_section_catalog(file,len,NULL,0u,&count)||
       !count||count>256u)return 0;
    MusaDwgAc1032SectionCatalogEntry *items=
        (MusaDwgAc1032SectionCatalogEntry*)calloc(count,sizeof(*items));
    if(!items)return 0;
    int ok=musa_dwg_ac1032_section_catalog(file,len,items,count,&count);
    uint64_t logical=0u;
    uint32_t section_id=0u;
    unsigned found=0u;
    if(ok)for(size_t i=0;i<count;i++){
        if(!strcmp((const char*)items[i].name,"AcDb:Classes")){
            found++;
            logical=items[i].logical_bytes;
            section_id=items[i].section_id;
            if(!items[i].all_physical_page_checksums_verified)ok=0;
        }
    }
    free(items);
    if(!ok||found!=1u||logical<29u||
       logical>8u*1024u*1024u)return 0;
    uint8_t *bytes=(uint8_t*)malloc((size_t)logical);
    if(!bytes)return 0;
    size_t written=0u;
    ok=musa_dwg_ac1032_extract_named_section(
       file,len,"AcDb:Classes",bytes,(size_t)logical,&written)&&
       written==logical&&
       !memcmp(bytes,class_marker,sizeof(class_marker));
    MusaDwgAc1032ClassesFrame result={0};
    if(ok){
        /* R2018 stores a 64-bit class data area length (two raw
         * little-endian longs), then a 32-bit bit count. The public
         * genuine AC1032 fixture has area=6531, high=0, bits=52242.
         * Reject >32-bit areas until a bounded streaming reader exists. */
        const uint32_t area=u32(bytes+16u);
        const uint32_t high=u32(bytes+20u);
        const uint32_t bits=u32(bytes+24u);
        ok=area>0u&&!high&&area<=logical-28u&&
           bits>0u&&(uint64_t)bits<=(uint64_t)area*8u;
        if(ok){
            result.class_area_bytes=area;
            result.total_data_bits=bits;
            result.section_logical_bytes=logical;
            result.section_id=section_id;
            result.original_sentinel_verified=1u;
            result.page_crcs_verified=1u;
        }
    }
    free(bytes);
    if(!ok)return 0;
    *out=result;return 1;
}

/* The DWG proprietary class data stream packs the two-bit BS/BL opcode
 * MSB-first. Raw numeric bytes themselves are little-endian. This is
 * intentionally DISTINCT from musacad_bits_reader (internal LSB-first).
 * Cursor remains inside the declared class-area bit count. */
typedef struct DwgClassBits {
    const uint8_t *p;
    size_t valid_bits;
    size_t at;
} DwgClassBits;
static int rawbits(DwgClassBits *b,unsigned width,uint32_t *out){
    if(!b||!out||!width||width>8u||
       b->at>b->valid_bits||width>b->valid_bits-b->at)return 0;
    uint32_t v=0u;
    for(unsigned i=0u;i<width;i++){
        const size_t pos=b->at+(size_t)i;
        v=(v<<1u)|((b->p[pos>>3u]>>(7u-(pos&7u)))&1u);
    }
    b->at+=width;
    *out=v;return 1;
}
static int lebytes(DwgClassBits *b,unsigned n,uint32_t *out){
    if(!b||!out||n==0u||n>4u)return 0;
    DwgClassBits tmp=*b;
    uint32_t v=0u;
    for(unsigned i=0u;i<n;i++){
        uint32_t byte=0u;
        if(!rawbits(&tmp,8u,&byte))return 0;
        v|=byte<<(8u*i);
    }
    *b=tmp;*out=v;return 1;
}
static int bitshort(DwgClassBits *b,uint16_t *out){
    if(!b||!out)return 0;
    DwgClassBits tmp=*b;
    uint32_t kind=0u,value=0u;
    if(!rawbits(&tmp,2u,&kind))return 0;
    switch(kind){
        case 0u:if(!lebytes(&tmp,2u,&value))return 0;break;
        case 1u:if(!lebytes(&tmp,1u,&value))return 0;break;
        case 2u:value=0u;break;
        case 3u:value=256u;break;
        default:return 0;
    }
    *b=tmp;*out=(uint16_t)value;return 1;
}
static int bitlong(DwgClassBits *b,uint32_t *out){
    if(!b||!out)return 0;
    DwgClassBits tmp=*b;
    uint32_t kind=0u,value=0u;
    if(!rawbits(&tmp,2u,&kind))return 0;
    switch(kind){
        case 0u:if(!lebytes(&tmp,4u,&value))return 0;break;
        case 1u:if(!lebytes(&tmp,1u,&value))return 0;break;
        case 2u:value=0u;break;
        default:return 0; /* reserved 11 in DWG bitlong */
    }
    *b=tmp;*out=value;return 1;
}
int musa_dwg_ac1032_classes_identity_prefix(
    const uint8_t *file,size_t len,
    MusaDwgAc1032ClassIdentityPrefix *out){
    if(!file||!out||len<256u||len>64u*1024u*1024u||
       !nonoverlap(file,len,out,sizeof(*out)))return 0;
    MusaDwgAc1032ClassesFrame frame={0};
    if(!musa_dwg_ac1032_classes_frame_probe(file,len,&frame)||
       frame.section_logical_bytes>8u*1024u*1024u||
       frame.section_logical_bytes<32u||frame.total_data_bits<24u)
        return 0;
    uint8_t *plain=(uint8_t*)malloc((size_t)frame.section_logical_bytes);
    if(!plain)return 0;
    size_t got=0u;
    int ok=musa_dwg_ac1032_extract_named_section(
       file,len,"AcDb:Classes",plain,
       (size_t)frame.section_logical_bytes,&got)&&
       got==frame.section_logical_bytes;
    MusaDwgAc1032ClassIdentityPrefix result={0};
    if(ok){
        DwgClassBits bits={plain+28u,frame.total_data_bits-32u,0u};
        uint32_t maxnum=0u,headerflag=0u;
        uint16_t first=0u,flags=0u;
        ok=bitlong(&bits,&maxnum)&&rawbits(&bits,1u,&headerflag)&&
           bitshort(&bits,&first)&&bitshort(&bits,&flags)&&
           maxnum>=500u&&maxnum<=65535u&&
           first>=500u&&first<=maxnum&&headerflag<=1u;
        if(ok){
            result.max_class_number=maxnum;
            result.first_class_number=first;
            result.first_proxy_flags=flags;
            result.header_boolean=(uint8_t)headerflag;
            result.source_crc_verified=1u;
        }
    }
    free(plain);
    if(!ok)return 0;
    *out=result;return 1;
}
