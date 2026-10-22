/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Real-file P0 acceptance of the first-party AC1032 PHYSICAL container.
 * The public upstream fixture is only READ as binary input; LibreDWG
 * decoding/conversion/library must NOT be called by this test.
 *
 * Covers original R2018 signature, encrypted file header, protected
 * page map, entire physical section directory and all data-page Adler
 * checksums. No entity, class graph, XREF or edit/save claim. */
#include "musacad_dwg_ac1032_page_map.h"
#include "musacad_dwg_ac1032_sections.h"
#include "musacad_dwg_ac1032_classes.h"
#include "musacad_dwg_ac1032_handles.h"
#include "musacad_dwg_ac1032_record.h"
#include "musacad_dwg_ac1032_object_type.h"
#include "musacad_dwg_ac1032_class_resolution.h"
#include "musacad_dwg_ac1018_lz.h"
#include "musacad_crc16.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define FILE_LIMIT (64u*1024u*1024u)
#define SECTION_LIMIT (32u*1024u*1024u)

static int real_file(const char *name,uint8_t **out,size_t *n){
    FILE *f=fopen(name,"rb");
    if(!f){perror(name);return 0;}
    if(fseek(f,0,SEEK_END)!=0){fclose(f);return 0;}
    const long bytes=ftell(f);
    if(bytes<256||bytes>(long)FILE_LIMIT||
       fseek(f,0,SEEK_SET)!=0){fclose(f);return 0;}
    uint8_t *p=(uint8_t*)malloc((size_t)bytes);
    if(!p){fclose(f);return 0;}
    int ok=fread(p,1,(size_t)bytes,f)==(size_t)bytes;
    if(fgetc(f)!=EOF)ok=0;
    if(fclose(f)!=0)ok=0;
    if(!ok){free(p);return 0;}
    *out=p;*n=(size_t)bytes;return 1;
}
/* Diagnostic on a REAL public fixture only. Uses the SAME first-party
 * LZ decoder; no external CAD library or format reinterpretation. */
static uint32_t u32_probe(const uint8_t *p){
    return (uint32_t)p[0] | ((uint32_t)p[1]<<8u) |
           ((uint32_t)p[2]<<16u) | ((uint32_t)p[3]<<24u);
}
static void explain_real_page_map(const uint8_t *file,size_t n,
    const MusaDwgAc1032FileMapHeader *h){
    const size_t pos=(size_t)h->page_map_offset+20u;
    if(h->page_map_offset>SIZE_MAX-20u||pos>n||
       h->map_compressed_bytes>n-pos||
       h->map_decoded_bytes>4u*1024u*1024u)return;
    uint8_t *plain=(uint8_t*)malloc(h->map_decoded_bytes);
    if(!plain)return;
    size_t consumed=0u;
    const int ok=musa_dwg_ac1018_lz_decode(file+pos,
        h->map_compressed_bytes,plain,h->map_decoded_bytes,&consumed);
    fprintf(stderr,"Real AC1032 page-map decode=%d expanded=%u"
           " packed=%u consumed=%zu\n",ok,h->map_decoded_bytes,
           h->map_compressed_bytes,consumed);
    if(ok){
        size_t at=0u;unsigned idx=0u;uint64_t position=256u;
        while(at+8u<=h->map_decoded_bytes && idx<2048u){
            const uint32_t raw=u32_probe(plain+at);
            const uint32_t len=u32_probe(plain+at+4u);
            const int negative=(raw&UINT32_C(0x80000000))!=0u;
            if(idx<8u||at+24u>=h->map_decoded_bytes)
                fprintf(stderr," map entry[%u] raw=%08X"
                  " size=%u file_off=%llu gap=%d\n",
                  idx,raw,len,(unsigned long long)position,negative);
            if(negative&&at+24u>h->map_decoded_bytes)break;
            at+=negative?24u:8u;
            position+=len;
            idx++;
        }
        fprintf(stderr," Real AC1032 page-map summary records=%u"
                " parsed_bytes=%zu file_end=%llu real_size=%zu\n",
                idx,at,(unsigned long long)position,n);
    }
    free(plain);
}
static void explain_real_sections(const uint8_t *file,size_t n,
    const MusaDwgAc1032FileMapHeader *h){
    size_t count=0u;
    if(!musa_dwg_ac1032_page_locations(file,n,NULL,0u,&count)||
       !count||count>131072u)return;
    MusaDwgAc1032PageLocation *pages=
        (MusaDwgAc1032PageLocation*)calloc(count,sizeof(*pages));
    if(!pages)return;
    if(!musa_dwg_ac1032_page_locations(file,n,pages,count,&count)){
        free(pages);return;
    }
    for(size_t i=0u;i<count;i++){
        if(pages[i].page_id!=h->section_map_id)continue;
        const size_t at=(size_t)pages[i].file_offset;
        if(at>n||n-at<20u){break;}
        const uint32_t typ=u32_probe(file+at);
        const uint32_t expanded=u32_probe(file+at+4u);
        const uint32_t packed=u32_probe(file+at+8u);
        const uint32_t method=u32_probe(file+at+12u);
        fprintf(stderr,"R2018 original section-map system page:"
                " id=%u pos=%zu span=%u type=%08X expanded=%u"
                " packed=%u codec=%u\n",pages[i].page_id,at,
                pages[i].file_span_bytes,typ,expanded,packed,method);
        if(expanded<20u||expanded>1024u*1024u||
           packed>n-at-20u)break;
        uint8_t *mem=(uint8_t*)malloc(expanded);
        if(!mem)break;
        size_t consumed=0u;
        const int ok=musa_dwg_ac1018_lz_decode(
             file+at+20u,packed,mem,expanded,&consumed);
        fprintf(stderr,"R2018 original section-map LZ=%d"
                " consumed=%zu sections=%u\n",
                ok,consumed,ok?u32_probe(mem):0u);
        if(ok){
            size_t off=20u;
            for(uint32_t k=0u;k<u32_probe(mem)&&k<12u;k++){
                if(off+96u>expanded)break;
                const uint8_t *d=mem+off;
                const uint32_t pages_n=u32_probe(d+8u);
                const uint32_t maxbytes=u32_probe(d+12u);
                const uint32_t compression=u32_probe(d+20u);
                const uint32_t sectid=u32_probe(d+24u);
                const uint32_t encrypted=u32_probe(d+28u);
                fprintf(stderr," descriptor %u:"
                   " pages=%u page_bytes=%u compr=%u id=%u"
                   " encrypted=%u name=%.64s\n",
                   k,pages_n,maxbytes,compression,sectid,encrypted,
                   (const char*)(d+32u));
                off+=96u+(size_t)pages_n*16u;
                if(off>expanded)break;
            }
        }
        free(mem);
        break;
    }
    free(pages);
}
static void fail(const char *stage,const uint8_t *file,size_t n){
    fprintf(stderr,
        "FAIL first-party AC1032 real DWG physical acceptance: %s;"
        " source_size=%zu; signature=%c%c%c%c%c%c; source UNCHANGED\n",
        stage,n,n>=6?file[0]:'?',n>=6?file[1]:'?',
        n>=6?file[2]:'?',n>=6?file[3]:'?',
        n>=6?file[4]:'?',n>=6?file[5]:'?');
}
int main(int argc,char **argv){
    if(argc!=2){fprintf(stderr,"usage: %s public-ac1032.dwg\n",argv[0]);return 2;}
    uint8_t *file=NULL;size_t len=0u;int accepted=0;
    if(!real_file(argv[1],&file,&len))return 2;
    if(memcmp(file,"AC1032",6u)!=0){
        fail("wrong original signature",file,len);goto done;
    }
    MusaDwgAc1032FileMapHeader header={0};
    if(!musa_dwg_ac1032_page_map_header(file,len,&header)){
        fprintf(stderr,"Original protected 108-byte header at 0x80:");
        for(size_t i=0;i<0x6cu;i++)fprintf(stderr," %02X",file[0x80u+i]);
        fputc('\n',stderr);
        fail("protected R2018 header/page-map envelope",file,len);goto done;
    }
    fprintf(stderr,"PASS original AC1032 protected map header:"
            " physical_pages=%u, first_map_off=%llu\n",
            header.page_count,(unsigned long long)header.page_map_offset);
    size_t map_count=0u;
    if(!musa_dwg_ac1032_page_locations(
                 file,len,NULL,0u,&map_count)||
       !map_count||map_count!=header.page_count){
        explain_real_page_map(file,len,&header);
        fail("decoded original AC1032 page locations",file,len);goto done;
    }
    MusaDwgAc1032PageLocation *map=(MusaDwgAc1032PageLocation*)calloc(
                                      map_count,sizeof(*map));
    if(!map||!musa_dwg_ac1032_page_locations(
                 file,len,map,map_count,&map_count)){
        fail("bounded physical map locations",file,len);
        free(map);goto done;
    }
    free(map);
    fprintf(stderr,"PASS original AC1032 physical location map: %zu pages\n",map_count);
    size_t section_count=0u;
    if(!musa_dwg_ac1032_section_catalog(
                 file,len,NULL,0u,&section_count)||
       !section_count||section_count>256u){
        explain_real_sections(file,len,&header);
        fail("FULL physical section directory and every page CRC",
             file,len);goto done;
    }
    MusaDwgAc1032SectionCatalogEntry *entry=
        (MusaDwgAc1032SectionCatalogEntry*)calloc(
                                      section_count,sizeof(*entry));
    if(!entry||!musa_dwg_ac1032_section_catalog(
                      file,len,entry,section_count,&section_count)){
        fail("bounded complete section metadata",file,len);
        free(entry);goto done;
    }
    int objects=0,handles=0;
    for(size_t i=0;i<section_count;i++){
        const MusaDwgAc1032SectionCatalogEntry *e=&entry[i];
        if(!e->all_physical_page_checksums_verified){
            fail("unverified physical page in catalog",file,len);
            free(entry);goto done;
        }
        fprintf(stderr,"AC1032 physical section[%zu] name='%s'"
                 " logical_bytes=%llu page_count=%u id=%u\n",
                 i,(const char*)e->name,(unsigned long long)e->logical_bytes,
                 e->page_count,e->section_id);
        if(!strcmp((const char*)e->name,"AcDb:AcDbObjects"))objects++;
        if(!strcmp((const char*)e->name,"AcDb:Handles"))handles++;
    }
    if(objects!=1||handles!=1){
        fail("missing unique AcDbObjects/Handles in complete R2018 map",
             file,len);
        free(entry);goto done;
    }
    const char *extracts[]={"AcDb:AcDbObjects","AcDb:Handles"};
    for(size_t i=0;i<2u;i++){
        const MusaDwgAc1032SectionCatalogEntry *e=NULL;
        for(size_t j=0;j<section_count;j++)
            if(!strcmp((const char*)entry[j].name,extracts[i]))e=&entry[j];
        if(!e||!e->logical_bytes||e->logical_bytes>SECTION_LIMIT){
            fail("unbounded AC1032 extraction size",file,len);
            free(entry);goto done;
        }
        uint8_t *plain=(uint8_t*)malloc((size_t)e->logical_bytes);
        size_t written=0u;
        if(!plain||!musa_dwg_ac1032_extract_named_section(
                       file,len,extracts[i],plain,(size_t)e->logical_bytes,
                       &written)||written!=e->logical_bytes){
            fail("original R2018 physical data-page decompression",
                 file,len);
            free(plain);free(entry);goto done;
        }
        fprintf(stderr,"PASS first-party opaque AC1032 section '%s': %zu bytes\n",
                extracts[i],written);
        free(plain);
    }
    MusaDwgAc1032ClassesFrame class_frame={0};
    if(!musa_dwg_ac1032_classes_frame_probe(file,len,&class_frame)){
        for(size_t i=0;i<section_count;i++)
            if(!strcmp((const char*)entry[i].name,"AcDb:Classes") &&
               entry[i].logical_bytes>=32u &&
               entry[i].logical_bytes<=8192u){
                const size_t n=(size_t)entry[i].logical_bytes;
                uint8_t *bytes=(uint8_t*)calloc(n,1u);
                size_t got=0u;
                const int ok=bytes&&musa_dwg_ac1032_extract_named_section(
                         file,len,"AcDb:Classes",bytes,n,&got);
                fprintf(stderr,"Classes raw-section extraction=%d"
                        " bytes=%zu expected=%zu first32:",ok,got,n);
                if(ok)for(size_t j=0;j<32u;j++)
                    fprintf(stderr," %02X",bytes[j]);
                fputc('\n',stderr);
                free(bytes);
            }
        fail("first-party AC1032 Classes section framing and sentinel",
             file,len);
        free(entry);goto done;
    }
    if(!class_frame.original_sentinel_verified||
       !class_frame.page_crcs_verified||
       !class_frame.class_area_bytes||!class_frame.total_data_bits){
        fail("uncertified original R2018 classes frame",file,len);
        free(entry);goto done;
    }
    fprintf(stderr,"PASS real AC1032 Classes frame: area=%u bytes"
            " bits=%u physical_section=%u\n",
            class_frame.class_area_bytes,class_frame.total_data_bits,
            class_frame.section_id);
    MusaDwgAc1032ClassIdentityPrefix first_class={0};
    if(!musa_dwg_ac1032_classes_identity_prefix(
                    file,len,&first_class)){
        /* Real-file-only diagnostic. Source remains unchanged and no
         * classes are invented if the bit layout is still unsupported. */
        uint8_t payload[8192];size_t count_bytes=0u;
        const int got=musa_dwg_ac1032_extract_named_section(
           file,len,"AcDb:Classes",payload,sizeof(payload),&count_bytes);
        fprintf(stderr,"real Classes first bit-coded bytes (%d):",got);
        if(got&&count_bytes>44u)
            for(size_t i=28u;i<44u;i++)
                fprintf(stderr," %02X",payload[i]);
        fputc('\n',stderr);
        fail("independent R2018 class identity prefix (BL+BS) mismatch",
             file,len);
        free(entry);goto done;
    }
    if(!first_class.source_crc_verified ||
       first_class.first_class_number<500u ||
       first_class.max_class_number<first_class.first_class_number){
        fail("invalid original class number proof",file,len);
        free(entry);goto done;
    }
    fprintf(stderr,"PASS REAL AC1032 first-class number=%u"
        " maxclass=%u proxy_flags=%u headerflag=%u\n",
        first_class.first_class_number,
        first_class.max_class_number,
        first_class.first_proxy_flags,
        first_class.header_boolean);
    MusaDwgAc1032ClassNumeric numbered[50]={{0}};
    size_t numbered_count=0u;
    if(!musa_dwg_ac1032_classes_numeric_records(
              file,len,numbered,50u,&numbered_count)||
       numbered_count!=50u||numbered[0].class_number!=500u||
       numbered[49].class_number!=549u){
        fail("real AC1032 numeric class list",file,len);
        free(entry);goto done;
    }
    fprintf(stderr,"PASS AC1032 real 50 numeric class identities\n");
    MusaDwgAc1032ClassTextRegion unicode_region={0};
    if(!musa_dwg_ac1032_classes_text_region(
          file,len,&unicode_region)||
       !unicode_region.source_verified||
       !unicode_region.present||
       unicode_region.length_bits<256u||
       unicode_region.start_bit<224u){
        fail("independent R2018 class Unicode text region",file,len);
        free(entry);goto done;
    }
    fprintf(stderr,
        "PASS REAL AC1032 Unicode stream: start=%llu bits=%llu flag=%llu\n",
        (unsigned long long)unicode_region.start_bit,
        (unsigned long long)unicode_region.length_bits,
        (unsigned long long)unicode_region.flag_bit);
    MusaDwgAc1032ClassNames class_names[50]={{0}};
    size_t names_count=0u;
    if(!musa_dwg_ac1032_classes_names(
             file,len,class_names,50u,&names_count)||
       names_count!=50u){
        fail("original R2018 Unicode TU class name decoding",file,len);
        free(entry);goto done;
    }
    if(class_names[0].class_number!=500u||
       class_names[49].class_number!=549u||
       !class_names[0].application[0]||
       !class_names[0].cpp_class[0]||
       !class_names[0].dxf_name[0]){
        fail("original Unicode name to class ID alignment",file,len);
        free(entry);goto done;
    }
    fprintf(stderr,"PASS REAL AC1032 names: first app='%s'"
            " cpp='%s' dxf='%s'\n",
            class_names[0].application,
            class_names[0].cpp_class,class_names[0].dxf_name);
    /* New read-only semantic step: locate genuine R2018 object handles
     * in ORIGINAL decompressed AcDb:AcDbObjects logical address space.
     * The AC1018 page-delta codec is shared, but modern object payload
     * bytes are NOT decoded by the older object parser. */
    size_t object_logical=0u;
    for(size_t i=0u;i<section_count;i++){
        if(!strcmp((const char*)entry[i].name,"AcDb:AcDbObjects"))
            object_logical=(size_t)entry[i].logical_bytes;
    }
    size_t index_count=0u;
    if(!object_logical||
       !musa_dwg_ac1032_handles_index(file,len,NULL,0u,&index_count)||
       index_count<1u||index_count>1000000u){
        fail("original AC1032 bounded object handle-map count",file,len);
        free(entry);goto done;
    }
    MusaDwgAc1032HandleEntry *original_handles=
         (MusaDwgAc1032HandleEntry*)calloc(index_count,
                                           sizeof(*original_handles));
    size_t listed_count=0u;
    if(!original_handles||
       !musa_dwg_ac1032_handles_index(file,len,original_handles,
                                       index_count,&listed_count)||
       listed_count!=index_count){
        fail("original AC1032 handle / relative object-offset decode",
             file,len);
        free(original_handles);free(entry);goto done;
    }
    for(size_t i=0u;i<index_count;i++){
        if(!original_handles[i].handle||
           original_handles[i].object_offset>=object_logical||
           (i>0u&&original_handles[i].handle<=original_handles[i-1u].handle)){
            fail("AC1032 original source invalid handle map order or offset",
                 file,len);
            free(original_handles);free(entry);goto done;
        }
    }
    /* Caller failure must not publish partial output. */
    const MusaDwgAc1032HandleEntry preserved=original_handles[0];
    const size_t preserved_count=listed_count;
    if(musa_dwg_ac1032_handles_index(file,len,original_handles,
                index_count-1u,&listed_count)||
       listed_count!=preserved_count||
       memcmp(&original_handles[0],&preserved,sizeof(preserved))){
        fail("AC1032 original handle index short-buffer rollback",
             file,len);
        free(original_handles);free(entry);goto done;
    }
    fprintf(stderr,"PASS REAL AC1032 source-backed handles: count=%zu"
            " first_H=%llu first_object_offset=%llu\n",
            index_count,
            (unsigned long long)original_handles[0].handle,
            (unsigned long long)original_handles[0].object_offset);

    /* GENUINE original R2018 object MS/MC/CRC envelopes from real indexed
     * handles. A successful physical page or handle map alone is NOT
     * evidence of an actual object record decode. */
    const size_t source_samples[3]={0u,index_count/2u,index_count-1u};
    for(size_t i=0u;i<3u;i++){
        const MusaDwgAc1032HandleEntry *e=&original_handles[source_samples[i]];
        MusaDwgAc1032RecordEnvelope record={0};
        if(!musa_dwg_ac1032_record_envelope_file(
                file,len,e->handle,&record)||
           !record.record_crc_verified||
           record.indexed_handle!=e->handle||
           record.object_section_offset!=e->object_offset||
           record.payload_bytes<2u||record.total_record_bytes<7u){
            fprintf(stderr,"REAL R2018 object MS/MC/CRC rejected"
                    " sample=%zu handle=%llu offset=%llu\\n",
                    i,(unsigned long long)e->handle,
                    (unsigned long long)e->object_offset);
            /* Read ORIGINAL authenticated Objects section for precise
             * on-failure diagnostics. Never weaken proof to make CI green. */
            uint8_t *raw=(uint8_t*)malloc(object_logical);
            size_t original_bytes=0u;
            if(raw && musa_dwg_ac1032_extract_named_section(
                 file,len,"AcDb:AcDbObjects",raw,object_logical,
                 &original_bytes)&&original_bytes==object_logical){
                const size_t at=(size_t)e->object_offset;
                fprintf(stderr,"original R2018 object bytes at %zu:",at);
                for(size_t k=0u;k<32u&&k<original_bytes-at;k++)
                    fprintf(stderr," %02X",raw[at+k]);
        
                if(original_bytes-at>512u){
                    const size_t ms=(size_t)raw[at]|((size_t)raw[at+1u]<<8u);
                    fprintf(stderr,"record MS=%zu first MC=%02X %02X "
                          "next indexed offset=%llu\\n",ms,
                          raw[at+2u],raw[at+3u],
                          (unsigned long long)original_handles[1u].object_offset);
                    for(int delta=-4;delta<=4;delta++){
                        const size_t crcpos=at+2u+ms+(size_t)((int64_t)delta);
                        if(crcpos<at||crcpos+2u>original_bytes)continue;
                        uint16_t crc=0u;
                        if(musa_core_crc16_arc(0xc0c1u,raw+at,crcpos-at,&crc))
                          fprintf(stderr,
                            "record CRC delta %+d computed=%04X storedLE=%04X storedBE=%04X\\n",
                            delta,crc,
                            (unsigned)(raw[crcpos]|(raw[crcpos+1u]<<8u)),
                            (unsigned)((raw[crcpos]<<8u)|raw[crcpos+1u]));
                    }
                }
        fputc('\n',stderr);
            }
            free(raw);
            fail("original AC1032 object record envelope",file,len);
            free(original_handles);free(entry);goto done;
        }
        fprintf(stderr,"PASS original AC1032 indexed object envelope:"
               " handle=%llu payload=%u handle_bits=%u CRC=verified\\n",
               (unsigned long long)record.indexed_handle,
               record.payload_bytes,record.handle_stream_bits);
    }
    /* New source-backed R2018 variable-length OT record type proof.
     * All samples have verified original H, MS, MC and CRC already.
     * No type-specific object body is decoded or called renderable. */
    for(size_t i=0u;i<3u;i++){
        const MusaDwgAc1032HandleEntry *e=&original_handles[source_samples[i]];
        MusaDwgAc1032ObjectType obj={0};
        if(!musa_dwg_ac1032_object_type_file(
                  file,len,e->handle,&obj)||
           !obj.record_crc_verified||
           obj.indexed_handle!=e->handle||
           obj.object_section_offset!=e->object_offset||
           !obj.type_code||obj.ot_opcode>2u||
           (i==0u&&obj.type_code!=48u)){
            fprintf(stderr,"FAIL first-party genuine AC1032 OT"
                           " source index=%zu handle=%llu offset=%llu\\n",
                    source_samples[i],(unsigned long long)e->handle,
                    (unsigned long long)e->object_offset);
            fail("original R2018 bit-coded OT object type",file,len);
            free(original_handles);free(entry);goto done;
        }
        fprintf(stderr,"PASS REAL AC1032 object OT:"
               " handle=%llu type=%u mode=%u original_CRC=verified\\n",
               (unsigned long long)obj.indexed_handle,
               obj.type_code,obj.ot_opcode);
    }
    /* Pair a TRUE source-CRC-checked builtin OT with its correct
     * provisional meaning (numeric ONLY), and a real Unicode/numeric
     * AcDb:Classes directory entry with its actual original DXF name. */
    MusaDwgAc1032ClassResolution builtin={0},registered={0};
    if(!musa_dwg_ac1032_resolve_object_class_file(
             file,len,original_handles[0].handle,&builtin)||
       builtin.raw_object_type!=48u||
       builtin.indexed_handle!=original_handles[0].handle||
       !builtin.original_record_crc_verified||
       builtin.is_custom_class||builtin.class_number||
       builtin.source_class_metadata_verified){
        fail("R2018 original built-in type incorrectly classified",file,len);
        free(original_handles);free(entry);goto done;
    }
    if(!musa_dwg_ac1032_resolve_declared_class_file(
             file,len,500u,&registered)||
       registered.class_number!=500u||
       registered.raw_object_type!=500u||
       registered.indexed_handle!=0u||
       registered.original_record_crc_verified||
       !registered.source_class_metadata_verified||
       !registered.is_custom_class||
       (registered.item_class_id!=0x1f2u&&
        registered.item_class_id!=0x1f3u)||
       strcmp(registered.dxf_name,"ACDBDICTIONARYWDFLT")){
        fail("R2018 original named class 500 numeric/Unicode join",file,len);
        free(original_handles);free(entry);goto done;
    }
    const MusaDwgAc1032ClassResolution saved_registered=registered;
    if(musa_dwg_ac1032_resolve_declared_class_file(
         file,len,550u,&registered)||
       memcmp(&saved_registered,&registered,sizeof(registered))){
        fail("AC1032 unregistered class must fail atomically",file,len);
        free(original_handles);free(entry);goto done;
    }
    fprintf(stderr,"PASS real original R2018 class join:"
            " builtin_OT=%u class_500='%s' kind=%u\\n",
            builtin.raw_object_type,saved_registered.dxf_name,
            saved_registered.item_class_id);
    free(original_handles);
    const MusaDwgAc1032ClassesFrame saved_classes=class_frame;
    const uint8_t original_sig=file[0];
    file[0]=(uint8_t)(original_sig^1u);
    if(musa_dwg_ac1032_classes_frame_probe(file,len,&class_frame)){
        fail("wrong AC1032 signature incorrectly accepted by class probe",file,len);
        file[0]=original_sig;free(entry);goto done;
    }
    file[0]=original_sig;
    if(memcmp(&class_frame,&saved_classes,sizeof(saved_classes))||
       musa_dwg_ac1032_classes_frame_probe(file,len,
            (MusaDwgAc1032ClassesFrame*)(void*)(file+32u))){
        fail("Classes invalid input not rejected atomically",file,len);
        free(entry);goto done;
    }
    free(entry);
    fprintf(stderr,
        "PASS independent AC1032 real public fixture: all protected"
        " pages/sections admitted by MusaCAD C11 ONLY;"
        " no R2018 object/class/editing parity claimed\n");
    accepted=1;
done:
    free(file);
    return accepted?0:1;
}
