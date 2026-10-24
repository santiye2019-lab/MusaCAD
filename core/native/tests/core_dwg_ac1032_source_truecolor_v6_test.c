/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Portable C11 source-color v6 packet selector: exact RGB24, not ARGB32.
 * These synthetic conditions exercise semantics; true original DWG H/CRC
 * acceptance remains in the existing five genuine R2018 file tests. */
#include "musacad_dwg_ac1032_preview_v6.h"
#include <stdint.h>
#include <stdio.h>

static int test(void){
    MusaDwgAc1032EntityCommon c={0};
    c.common_decoded=1u;
    for(uint8_t kind=1u;kind<=4u;kind++){
        const uint32_t values[]={0u,UINT32_C(0x123456),UINT32_C(0xffffff)};
        for(size_t i=0u;i<sizeof(values)/sizeof(values[0]);i++){
            c.has_rgb=1u;c.rgb=values[i];c.color_index=256u;
            c.has_color_reference=0u;c.has_transparency=0u;
            c.invisibility=0u;
            float code=0.0f,rgb=0.0f;uint8_t extra=0u,exact=0u;
            if(!musa_dwg_ac1032_preview_v6_entity_source_style_code(
                 &c,kind,&code,&rgb,&extra,&exact)||
               code!=-(float)(kind*256u+2u)||
               rgb!=(float)values[i]||extra!=1u||exact!=1u||
               /* Android NativeScene v6: negative primitive code and
                * exact 24-bit RGB extra word, no alpha-bit precision loss. */
               ((uint32_t)rgb)!=values[i]||
               ((uint32_t)(-code-1.0f)>>8u)!=kind)
                return 0;
            /* Compatibility API must remain strictly ACI-only. */
            float aci=0.0f;uint8_t aci_exact=1u;
            if(!musa_dwg_ac1032_preview_v6_entity_aci_code(
                &c,kind,&aci,&aci_exact)||
               aci!=(float)(kind*256u+7u)||aci_exact!=0u)
                return 0;
        }
        float code=-9.0f,rgb=-9.0f;uint8_t extra=9u,exact=9u;
        c.has_rgb=0u;c.color_index=5u;
        if(!musa_dwg_ac1032_preview_v6_entity_source_style_code(
             &c,kind,&code,&rgb,&extra,&exact)||
           code!=(float)(kind*256u+5u)||rgb!=0.0f||
           extra!=0u||exact!=1u)return 0;
        const uint16_t inherited[]={0u,256u};
        for(size_t i=0u;i<2u;i++){
            c.color_index=inherited[i];
            if(!musa_dwg_ac1032_preview_v6_entity_source_style_code(
                 &c,kind,&code,&rgb,&extra,&exact)||
               code!=(float)(kind*256u+7u)||rgb!=0.0f||
               extra!=0u||exact!=0u)return 0;
        }
        c.has_rgb=1u;c.rgb=0x7799aau;c.color_index=256u;
        c.has_color_reference=1u;
        if(!musa_dwg_ac1032_preview_v6_entity_source_style_code(
             &c,kind,&code,&rgb,&extra,&exact)||
           code!=(float)(kind*256u+7u)||extra||exact)return 0;
        c.has_color_reference=0u;c.has_transparency=1u;
        if(!musa_dwg_ac1032_preview_v6_entity_source_style_code(
             &c,kind,&code,&rgb,&extra,&exact)||
           code!=(float)(kind*256u+7u)||extra||exact)return 0;
        c.has_transparency=0u;c.invisibility=1u;
        if(!musa_dwg_ac1032_preview_v6_entity_source_style_code(
             &c,kind,&code,&rgb,&extra,&exact)||
           code!=(float)(kind*256u+7u)||extra||exact)return 0;
        c.invisibility=0u;c.has_rgb=0u;c.color_index=0u;
    }
    float code=341.0f,rgb=342.0f;
    uint8_t extra=17u,exact=18u;
    if(musa_dwg_ac1032_preview_v6_entity_source_style_code(
         &c,0u,&code,&rgb,&extra,&exact)||
       code!=341.0f||rgb!=342.0f||extra!=17u||exact!=18u)
       return 0;
    if(musa_dwg_ac1032_preview_v6_entity_source_style_code(
         &c,5u,&code,&rgb,&extra,&exact)||
       code!=341.0f||rgb!=342.0f||extra!=17u||exact!=18u)
       return 0;
    if(musa_dwg_ac1032_preview_v6_entity_source_style_code(
         &c,1u,&code,&code,&extra,&exact)||
       code!=341.0f||rgb!=342.0f||extra!=17u||exact!=18u)
       return 0;
    if(musa_dwg_ac1032_preview_v6_entity_source_style_code(
         NULL,1u,&code,&rgb,&extra,&exact)||
       code!=341.0f||rgb!=342.0f||extra!=17u||exact!=18u)
       return 0;
    c.common_decoded=0u;
    if(musa_dwg_ac1032_preview_v6_entity_source_style_code(
         &c,1u,&code,&rgb,&extra,&exact)||
       code!=341.0f||rgb!=342.0f||extra!=17u||exact!=18u)
       return 0;
    return 1;
}
int main(void){
    const int ok=test();
    fprintf(stderr,"%s source RGB24 truecolor exact f32 v6 C ABI; "
           "BYLAYER BYBLOCK DBCOLOR alpha invisible stay provisional\n",
           ok?"PASS":"FAIL");
    return ok?0:1;
}
