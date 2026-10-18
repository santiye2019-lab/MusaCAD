/* SPDX-License-Identifier: GPL-3.0-or-later */
/*
 * Host-only acceptance probe for pinned, publicly distributable DWG fixtures.
 * This uses ONLY the first-party portable MusaCAD core, not LibreDWG.
 * Never log source filename, source bytes, coordinates or private entity IDs.
 * A positive result proves only file header / two framed-section checks.
 */
#include "musacad_dwg_ac1015.h"
#include "musacad_dwg_ac1015_frame.h"
#include "musacad_dwg_ac1015_handles.h"
#include "musacad_dwg_ac1015_object.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

enum { MAX_FIXTURE_BYTES = 64 * 1024 * 1024 };

static int load_readonly(const char *name,uint8_t **out,size_t *length) {
    if (!name || !out || !length) return 0;
    FILE *fp=fopen(name,"rb");
    if(!fp) return 0;
    if(fseek(fp,0,SEEK_END)!=0){fclose(fp);return 0;}
    const long length_signed=ftell(fp);
    if(length_signed<=0 || (unsigned long)length_signed>MAX_FIXTURE_BYTES){
        fclose(fp);return 0;
    }
    if(fseek(fp,0,SEEK_SET)!=0){fclose(fp);return 0;}
    size_t n=(size_t)length_signed;
    uint8_t *data=(uint8_t*)malloc(n);
    if(!data){fclose(fp);return 0;}
    const int ok=fread(data,1u,n,fp)==n && ferror(fp)==0;
    fclose(fp);
    if(!ok){free(data);return 0;}
    *out=data;*length=n;return 1;
}

int main(int argc,char **argv){
    if(argc!=3 ||
       (strcmp(argv[1],"accept-r2000")!=0 &&
        strcmp(argv[1],"reject-modern")!=0)){
        fputs("FAIL: expected mode and fixture path\n",stderr);
        return 2;
    }
    uint8_t *file=NULL;
    size_t n=0u;
    if(!load_readonly(argv[2],&file,&n)){
        fputs("FAIL: fixture unavailable or outside size limit\n",stderr);
        return 1;
    }

    if(strcmp(argv[1],"reject-modern")==0){
        MusaDwgAc1015Locators loc={0};
        MusaDwgAc1015Frame frame={0};
        const int accepted_loc=musa_dwg_ac1015_read_locators(file,n,&loc);
        const int accepted_frame=musa_dwg_ac1015_parse_crc_verified_frame(
            file,n,0u,&frame);
        free(file);
        if(accepted_loc||accepted_frame){
            fputs("FAIL: unsupported modern DWG must be refused\n",stderr);
            return 1;
        }
        puts("PASS: modern DWG is rejected by AC1015-specific reader");
        return 0;
    }

    MusaDwgAc1015Locators loc={0};
    if(!musa_dwg_ac1015_read_locators(file,n,&loc)){
        free(file);
        fputs("FAIL: real R2000 file header/CRC or section directory\n",stderr);
        return 1;
    }
    if(loc.crc_verified!=1 || loc.region_count<3u ||
       loc.region_count>MUSA_DWG_AC1015_MAX_LOCATORS){
        free(file);
        fputs("FAIL: real R2000 header acceptance flags\n",stderr);
        return 1;
    }
    /* This pinned real fixture specifically exercises the six-record R2000
     * layout with an absent/zero-length optional section 3. A refactor that
     * silently strips or borrows it must fail the real-file gate. */
    if(loc.region_count!=6u ||
       loc.regions[3].section_kind!=3u ||
       loc.regions[3].length!=0u ||
       loc.regions[5].section_kind!=5u){
        free(file);
        fputs("FAIL: genuine six-locator and empty optional section semantics\n",stderr);
        return 1;
    }
    const uint8_t *optional_span=file+2u;
    size_t optional_length=777u;
    if(musa_dwg_ac1015_readonly_section(file,n,3u,
                                        &optional_span,&optional_length)||
       optional_span!=file+2u || optional_length!=777u){
        free(file);
        fputs("FAIL: absent optional section must never be borrowed\n",stderr);
        return 1;
    }
    for(uint32_t section=0u;section<2u;section++){
        MusaDwgAc1015Frame frame={0};
        if(!musa_dwg_ac1015_parse_crc_verified_frame(
                file,n,section,&frame) ||
           frame.section_kind!=section ||
           frame.envelope_verified!=1 ||
           frame.payload_crc_verified!=1 ||
           !frame.payload){
            free(file);
            fputs(section==0u?
                  "FAIL: real R2000 HEADER frame or payload CRC\n":
                  "FAIL: real R2000 CLASSES frame or payload CRC\n",stderr);
            return 1;
        }
    }
    size_t object_count=0u;
    if(!musa_dwg_ac1015_handles_index(file,n,NULL,0u,&object_count)||
       object_count==0u){
        free(file);
        fputs("FAIL: real R2000 object map CRC/index acceptance\n",stderr);
        return 1;
    }
    if(object_count>100000u){
        free(file);
        fputs("FAIL: too many public real DWG records for bounded test\n",stderr);
        return 1;
    }
    MusaDwgAc1015HandleEntry *entries=(MusaDwgAc1015HandleEntry*)
         calloc(object_count,sizeof(MusaDwgAc1015HandleEntry));
    if(!entries){
        free(file);return 1;
    }
    size_t confirmed_count=0u;
    if(!musa_dwg_ac1015_handles_index(file,n,entries,
                                       object_count,&confirmed_count)||
       confirmed_count!=object_count){
        free(entries);free(file);
        fputs("FAIL: real DWG handle index mismatch\n",stderr);
        return 1;
    }
    size_t complete=0u;
    for(size_t i=0u;i<object_count;i++){
        MusaDwgAc1015ObjectRecord rec={0};
        if(!musa_dwg_ac1015_read_object_record(file,n,&entries[i],&rec)||
           rec.record_crc_verified!=1||rec.geometry_decoded!=0||
           rec.indexed_handle!=entries[i].handle){
            free(entries);free(file);
            fputs("FAIL: indexed real R2000 object record CRC/type boundary\n",stderr);
            return 1;
        }
        complete++;
    }
    free(entries);
    printf("PASS: real R2000 independently checked %zu object records (CRC + type header only)\n",
           complete);
    printf("PASS: real R2000 HEADER/CLASSES CRC + object handle map (%zu indexed objects)\n",
           object_count);
    free(file);
    return 0;
}
