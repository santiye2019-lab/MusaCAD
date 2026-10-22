/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "musacad_dwg_ac1032_class_resolution.h"
#include <assert.h>
#include <string.h>
int main(void){
    MusaDwgAc1032ClassNumeric numeric[2]={{0}};
    MusaDwgAc1032ClassNames names[2]={{0}};
    numeric[0].class_number=500u;numeric[0].item_class_id=0x1f3u;
    numeric[0].dwg_version=35u;
    numeric[1].class_number=501u;numeric[1].item_class_id=0x1f2u;
    numeric[1].was_proxy=1u;
    names[0].class_number=500u;names[1].class_number=501u;
    strcpy(names[0].application,"App");
    strcpy(names[0].cpp_class,"CppObject");
    strcpy(names[0].dxf_name,"DXFOBJECT");
    strcpy(names[1].application,"App");
    strcpy(names[1].cpp_class,"CppEntity");
    strcpy(names[1].dxf_name,"DXFENTITY");
    MusaDwgAc1032ClassResolution out={0};
    assert(musa_dwg_ac1032_class_join(501u,7u,numeric,names,2u,&out));
    assert(out.indexed_handle==7u&&out.class_number==501u);
    assert(out.item_class_id==0x1f2u&&out.is_custom_class&&out.was_proxy);
    assert(out.source_class_metadata_verified&&
           strcmp(out.dxf_name,"DXFENTITY")==0);
    assert(musa_dwg_ac1032_class_join(500u,0u,numeric,names,2u,&out));
    assert(!out.indexed_handle&&out.item_class_id==0x1f3u);
    assert(out.dwg_version==35u);
    assert(musa_dwg_ac1032_class_join(48u,1u,numeric,names,2u,&out));
    assert(out.raw_object_type==48u&&!out.class_number&&
           !out.is_custom_class&&!out.source_class_metadata_verified);
    MusaDwgAc1032ClassResolution snapshot=out;
    assert(!musa_dwg_ac1032_class_join(502u,7u,numeric,names,2u,&out));
    names[1].class_number=500u; /* mismatch */
    assert(!musa_dwg_ac1032_class_join(501u,7u,numeric,names,2u,&out));
    names[1].class_number=501u;
    numeric[1].class_number=500u; /* duplicate */
    assert(!musa_dwg_ac1032_class_join(500u,7u,numeric,names,2u,&out));
    numeric[1].class_number=501u;
    numeric[1].item_class_id=1u; /* not entity/object */
    assert(!musa_dwg_ac1032_class_join(500u,7u,numeric,names,2u,&out));
    numeric[1].item_class_id=0x1f2u;
    names[1].dxf_name[0]=0; /* never publish blank/unknown class name */
    assert(!musa_dwg_ac1032_class_join(501u,7u,numeric,names,2u,&out));
    names[1].dxf_name[0]='D';
    assert(!musa_dwg_ac1032_class_join(501u,7u,numeric,names,2u,
            (MusaDwgAc1032ClassResolution*)(void*)names));
    assert(!memcmp(&out,&snapshot,sizeof(snapshot)));
    return 0;
}
