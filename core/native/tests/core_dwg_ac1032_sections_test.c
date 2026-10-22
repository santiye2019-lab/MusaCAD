/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Original bounded, protected physical page-map envelope fixture.
 * This uses the R2004-physical-compatible layout specified for R2018,
 * but is NOT an independent AutoCAD-produced AC1032 drawing. The object
 * payloads here remain synthetic R2004 and must NEVER be passed through
 * an AC1032 AcDb grammar/geometry decoder. */
#define main existing_ac1018_scene_test_entrypoint
#include "core_dwg_ac1018_scene_test.c"
#undef main
#include "musacad_dwg_ac1032_sections.h"
int main(void){
    uint8_t source[1120],before[1120];
    construct(source,0);
    MusaDwgAc1032SectionDescriptor info={0};
    MusaDwgAc1032SectionPage page[2]={{0}};
    size_t count=0u;
    assert(!musa_dwg_ac1032_section_pages(
         source,sizeof(source),MUSA_DWG_AC1032_OBJECTS,
         page,2u,&info,&count));
    memcpy(source,"AC1032",6u);
    memcpy(before,source,sizeof(before));
    assert(musa_dwg_ac1032_section_pages(
         source,sizeof(source),MUSA_DWG_AC1032_OBJECTS,
         page,2u,&info,&count));
    assert(count==1u&&info.page_count==1u&&
           info.logical_bytes==sizeof(poly_record)+sizeof(line_record)&&
           info.data_page_checksums_verified&&
           info.map_checksum_verified&&
           page[0].data_checksum_verified&&
           page[0].header_checksum_verified&&
           page[0].page_id==1u&&page[0].logical_offset==0u);
    assert(!musa_dwg_ac1018_section_pages(
         source,sizeof(source),MUSA_DWG_AC1018_OBJECTS,
         page,2u,&info,&count));
    uint8_t objects[sizeof(poly_record)+sizeof(line_record)+3u];
    memset(objects,0xAB,sizeof(objects));
    size_t copied=999u;
    assert(musa_dwg_ac1032_extract_section(
         source,sizeof(source),MUSA_DWG_AC1032_OBJECTS,
         objects,sizeof(objects),&copied));
    assert(copied==sizeof(poly_record)+sizeof(line_record)&&
           !memcmp(objects,poly_record,sizeof(poly_record))&&
           !memcmp(objects+sizeof(poly_record),
                   line_record,sizeof(line_record))&&
           objects[copied]==0xAB);
    uint8_t handles[16]={0},expected[14]={0};
    make_handles(expected,0);
    assert(musa_dwg_ac1032_extract_section(
         source,sizeof(source),MUSA_DWG_AC1032_HANDLES,
         handles,sizeof(handles),&copied));
    assert(copied==sizeof(expected)&&
           !memcmp(handles,expected,sizeof(expected)));
    assert(!memcmp(source,before,sizeof(source)));
    MusaDwgAc1032SectionDescriptor saved=info;
    MusaDwgAc1032SectionPage saved_page=page[0];
    const size_t prior_count=count;
    copied=777u;
    assert(!musa_dwg_ac1032_extract_section(
         source,sizeof(source),MUSA_DWG_AC1032_OBJECTS,
         objects,sizeof(poly_record)+sizeof(line_record)-1u,&copied));
    assert(copied==777u);
    assert(!musa_dwg_ac1032_extract_section(
         source,sizeof(source),MUSA_DWG_AC1032_OBJECTS,
         source,sizeof(source),&copied)); /* never modify input */
    assert(!musa_dwg_ac1032_section_pages(
         source,sizeof(source),MUSA_DWG_AC1032_OBJECTS,
         page,0u,&info,&count));
    assert(!musa_dwg_ac1032_section_pages(
         source,sizeof(source),99,
         page,2u,&info,&count));
    assert(!memcmp(&info,&saved,sizeof(saved))&&
           !memcmp(page,&saved_page,sizeof(saved_page))&&
           count==prior_count);
    source[288u]^=1u; /* broken original data page checksum */
    assert(!musa_dwg_ac1032_section_pages(
         source,sizeof(source),MUSA_DWG_AC1032_OBJECTS,
         page,2u,&info,&count));
    assert(!musa_dwg_ac1032_extract_section(
         source,sizeof(source),MUSA_DWG_AC1032_OBJECTS,
         objects,sizeof(objects),&copied));
    assert(!memcmp(&info,&saved,sizeof(saved))&&
           !memcmp(page,&saved_page,sizeof(saved_page))&&
           count==prior_count&&copied==777u);
    memcpy(source,before,sizeof(source));
    source[608u]^=1u; /* broken protected handle page checksum */
    assert(!musa_dwg_ac1032_extract_section(
         source,sizeof(source),MUSA_DWG_AC1032_HANDLES,
         handles,sizeof(handles),&copied));
    assert(copied==777u);
    memcpy(source,before,sizeof(source));
    source[788u]^=1u; /* damaged section-map system header */
    assert(!musa_dwg_ac1032_section_pages(
         source,sizeof(source),MUSA_DWG_AC1032_HANDLES,
         page,2u,&info,&count));
    assert(!memcmp(&info,&saved,sizeof(saved)));
    memcpy(source,before,sizeof(source));
    assert(!musa_dwg_ac1032_section_pages(
         source,1080u,MUSA_DWG_AC1032_HANDLES,
         page,2u,&info,&count));
    assert(!memcmp(&info,&saved,sizeof(saved)));
    /* New feature: complete protected section DIRECTORY, including
     * non-geometry/unknown sections, with no partial result. */
    memcpy(source,before,sizeof(source));
    MusaDwgAc1032SectionCatalogEntry catalog[2]={{0}},backup[2]={{0}};
    size_t catalog_count=999u;
    assert(musa_dwg_ac1032_section_catalog(
                source,sizeof(source),NULL,0u,&catalog_count));
    assert(catalog_count==2u);
    assert(musa_dwg_ac1032_section_catalog(
                source,sizeof(source),catalog,2u,&catalog_count));
    assert(catalog_count==2u&&
           !strcmp((const char*)catalog[0].name,"AcDb:AcDbObjects")&&
           !strcmp((const char*)catalog[1].name,"AcDb:Handles")&&
           catalog[0].section_id==11u&&
           catalog[1].section_id==10u&&
           catalog[0].page_count==1u&&catalog[1].page_count==1u&&
           catalog[0].all_physical_page_checksums_verified&&
           catalog[1].all_physical_page_checksums_verified);
    memcpy(backup,catalog,sizeof(backup));
    size_t previous_catalog_count=catalog_count;
    assert(!musa_dwg_ac1032_section_catalog(
                source,sizeof(source),catalog,1u,&catalog_count));
    assert(!musa_dwg_ac1032_section_catalog(
                source,sizeof(source),
                (MusaDwgAc1032SectionCatalogEntry*)(void*)source,
                2u,&catalog_count));
    assert(!musa_dwg_ac1032_section_catalog(
                source,sizeof(source),catalog,2u,
                (size_t*)(void*)catalog));
    assert(catalog_count==previous_catalog_count&&
           !memcmp(catalog,backup,sizeof(catalog)));
    source[608u]^=1u; /* corrupt Handles: catalog must inspect ALL pages */
    assert(!musa_dwg_ac1032_section_catalog(
                source,sizeof(source),catalog,2u,&catalog_count));
    assert(catalog_count==previous_catalog_count&&
           !memcmp(catalog,backup,sizeof(catalog)));
    memcpy(source,before,sizeof(source));
    /* Re-label second descriptor as original unknown content, keeping
     * physical record and map CRC independently valid. */
    const size_t second_name=768u+20u+2u+132u+32u;
    memset(source+second_name,0,64u);
    memcpy(source+second_name,"AcDb:AcDsPrototype_1b",22u);
    sys_header(source+768u,UINT32_C(0x4163003b),244u,247u);
    assert(musa_dwg_ac1032_section_catalog(
               source,sizeof(source),catalog,2u,&catalog_count));
    assert(catalog_count==2u&&
           !strcmp((const char*)catalog[1].name,"AcDb:AcDsPrototype_1b")&&
           catalog[1].all_physical_page_checksums_verified);
    assert(!musa_dwg_ac1032_section_pages(
               source,sizeof(source),MUSA_DWG_AC1032_HANDLES,
               page,2u,&info,&count)); /* no fabricated Handles section */
    /* Two descriptors with the same name must be rejected without writes
     * even when the section-map checksum has been regenerated. */
    memset(source+second_name,0,64u);
    memcpy(source+second_name,"AcDb:AcDbObjects",16u);
    sys_header(source+768u,UINT32_C(0x4163003b),244u,247u);
    memcpy(backup,catalog,sizeof(backup));
    previous_catalog_count=catalog_count;
    assert(!musa_dwg_ac1032_section_catalog(
                source,sizeof(source),catalog,2u,&catalog_count));
    assert(catalog_count==previous_catalog_count&&
           !memcmp(catalog,backup,sizeof(catalog)));
    return 0;
}
