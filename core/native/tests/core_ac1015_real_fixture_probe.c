/* SPDX-License-Identifier: GPL-3.0-or-later */
/*
 * Host-only acceptance probe for pinned, publicly distributable DWG fixtures.
 * This uses ONLY the first-party portable MusaCAD core, not LibreDWG.
 * Never log source filename, source bytes, coordinates or private entity IDs.
 * A positive result proves only file header / two framed-section checks.
 */
#include "musacad_dwg_ac1015.h"
#include "musacad_dwg_ac1015_frame.h"
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
    puts("PASS: real R2000 header locator, HEADER and CLASSES structural/data CRC gates");
    free(file);
    return 0;
}
