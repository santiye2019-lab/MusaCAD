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

/* First complete bounded numeric class-record pass: Unicode TU strings
 * are intentionally not read from the independent string bitstream.
 * A record must have a real source index and entity/object declaration.
 * Never populate any partially decoded record on a failed input. */
int musa_dwg_ac1032_classes_numeric_records(
    const uint8_t *file,size_t len,MusaDwgAc1032ClassNumeric *output,
    size_t capacity,size_t *count){
    if(!file||!count||len<256u||len>64u*1024u*1024u||
       (!output&&capacity))return 0;
    if(!nonoverlap(file,len,count,sizeof(*count)))return 0;
    if(output){
        if(capacity>SIZE_MAX/sizeof(*output))return 0;
        const size_t bytes=capacity*sizeof(*output);
        if(!nonoverlap(file,len,output,bytes)||
           !nonoverlap(output,bytes,count,sizeof(*count)))
            return 0;
    }
    MusaDwgAc1032ClassesFrame frame={0};
    MusaDwgAc1032ClassIdentityPrefix prefix={0};
    if(!musa_dwg_ac1032_classes_frame_probe(file,len,&frame)||
       !musa_dwg_ac1032_classes_identity_prefix(file,len,&prefix)||
       frame.section_logical_bytes<32u||
       prefix.max_class_number<500u||
       prefix.max_class_number>65535u||
       frame.total_data_bits<64u)return 0;
    const size_t limit=(size_t)prefix.max_class_number-499u;
    if(!limit||limit>4096u|| (output&&capacity<limit))return 0;
    uint8_t *original=(uint8_t*)malloc((size_t)frame.section_logical_bytes);
    MusaDwgAc1032ClassNumeric *temp=
        (MusaDwgAc1032ClassNumeric*)calloc(limit,sizeof(*temp));
    if(!original||!temp){free(original);free(temp);return 0;}
    size_t written=0u;
    int ok=musa_dwg_ac1032_extract_named_section(
       file,len,"AcDb:Classes",original,
       (size_t)frame.section_logical_bytes,&written)&&
       written==frame.section_logical_bytes;
    size_t found=0u;
    if(ok){
        DwgClassBits bits={original+28u,
                           frame.total_data_bits-32u,0u};
        uint32_t maxnum=0u,headerflag=0u;
        ok=bitlong(&bits,&maxnum)&&rawbits(&bits,1u,&headerflag)&&
           maxnum==prefix.max_class_number&&headerflag==1u;
        uint16_t previous=499u;
        while(ok&&previous<maxnum&&found<limit){
            MusaDwgAc1032ClassNumeric record={0};
            uint32_t zombie=0u;
            ok=bitshort(&bits,&record.class_number)&&
               bitshort(&bits,&record.proxy_flags)&&
               rawbits(&bits,1u,&zombie)&&
               bitshort(&bits,&record.item_class_id)&&
               bitlong(&bits,&record.object_instance_count)&&
               bitshort(&bits,&record.dwg_version)&&
               bitshort(&bits,&record.maintenance_version)&&
               bitlong(&bits,&record.unknown_number1)&&
               bitlong(&bits,&record.unknown_number2);
            if(!ok)break;
            if(record.class_number<=previous||
               record.class_number>maxnum||
               (record.item_class_id!=0x1f2u&&
                record.item_class_id!=0x1f3u)){
                ok=0;break;
            }
            record.was_proxy=(uint8_t)zombie;
            temp[found++]=record;
            previous=record.class_number;
        }
        if(previous!=maxnum||!found||
           temp[0].class_number!=prefix.first_class_number||
           temp[0].proxy_flags!=prefix.first_proxy_flags)
            ok=0;
    }
    if(ok){
        if(output)memcpy(output,temp,found*sizeof(*output));
        *count=found;
    }
    free(temp);
    free(original);
    return ok;
}

/* R2010+ R2018 class data appends the TU stream before a 16/32-bit
 * length descriptor and a trailing presence bit. All access is bounded
 * against the original authenticated logical section. Unicode is not
 * interpreted until its exact source span can be demonstrated. */
int musa_dwg_ac1032_classes_text_region(
    const uint8_t *file,size_t file_len,MusaDwgAc1032ClassTextRegion *out){
    if(!file||!out||file_len<256u||file_len>64u*1024u*1024u||
       !nonoverlap(file,file_len,out,sizeof(*out)))return 0;
    MusaDwgAc1032ClassesFrame frame={0};
    if(!musa_dwg_ac1032_classes_frame_probe(file,file_len,&frame)||
       frame.section_logical_bytes>8u*1024u*1024u)return 0;
    uint8_t *bytes=(uint8_t*)malloc((size_t)frame.section_logical_bytes);
    if(!bytes)return 0;
    size_t written=0u;
    int ok=musa_dwg_ac1032_extract_named_section(
       file,file_len,"AcDb:Classes",bytes,
       (size_t)frame.section_logical_bytes,&written)&&
       written==frame.section_logical_bytes;
    MusaDwgAc1032ClassTextRegion region={0};
    if(ok){
        const uint64_t flag=(uint64_t)frame.total_data_bits+191u;
        if(flag>=frame.section_logical_bytes*8u||flag<240u)
            ok=0;
        else{
            DwgClassBits bits={bytes,frame.section_logical_bytes*8u,(size_t)flag};
            uint32_t present=0u;
            ok=rawbits(&bits,1u,&present);
            region.flag_bit=flag;
            region.present=(uint8_t)present;
            region.source_verified=1u;
            if(ok&&present){
                if(flag<16u)ok=0;
                else{
                    bits.at=(size_t)flag-16u;
                    uint32_t low=0u,high=0u;
                    ok=lebytes(&bits,2u,&low);
                    uint64_t length=low;
                    uint64_t before=flag-16u;
                    if(ok&&(low&0x8000u)){
                        if(before<16u)ok=0;
                        else{
                            bits.at=(size_t)(before-16u);
                            ok=lebytes(&bits,2u,&high);
                            length=(uint64_t)(low&0x7fffu)+
                                   ((uint64_t)high<<15u);
                            before-=16u;
                        }
                    }
                    if(ok&&(!length||length>before-224u))
                        ok=0;
                    if(ok){
                        region.start_bit=before-length;
                        region.length_bits=length;
                    }
                }
            }
        }
    }
    free(bytes);
    if(!ok)return 0;
    *out=region;return 1;
}

/* AC1032 Unicode class names are sourced from the verified string stream. */

static int musacad_class_emit_utf8(uint32_t cp,char *dst,size_t cap,size_t *n){
    if(!cp||cp>0x10ffffu||(cp>=0xd800u&&cp<=0xdfffu))return 0;
    const size_t k=cp<128u?1u:cp<2048u?2u:cp<65536u?3u:4u;
    if(*n>=cap||k>=cap-*n)return 0;
    if(k==4u)dst[(*n)++]=(char)(0xf0u|(cp>>18u));
    if(k>=3u)dst[(*n)++]=(char)((k==3u?0xe0u:0x80u)|((cp>>(k==3u?12u:12u))&(k==3u?15u:63u)));
    if(k>=2u)dst[(*n)++]=(char)((k==2u?0xc0u:0x80u)|((cp>>6u)&(k==2u?31u:63u)));
    dst[(*n)++]=(char)((k==1u?0u:0x80u)|(cp&(k==1u?127u:63u)));
    dst[*n]=0;return 1;
}
