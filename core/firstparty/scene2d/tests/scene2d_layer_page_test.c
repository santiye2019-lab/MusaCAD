/* SPDX-License-Identifier: MIT
 * Independent, synthetic 20k-layer paging and rollback regression.
 * No DWG fixture or third-party CAD code is used.
 */
#include "musa_scene2d_layer.h"
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <assert.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>

enum { N = 20000, PAGE = 127 };
static MusaScene2dCadLayer cad[N];
static MusaScene2dLayer full[N];
static uint32_t palette[256];

int main(void) {
    for (size_t i=0;i<256u;++i) palette[i]=(uint32_t)i*0x010101u;
    for (size_t i=0;i<N;++i) {
        cad[i].handle=(uint64_t)i;
        cad[i].signed_aci=(int16_t)(i%7u==0u ? -3 : (i%7u==1u ? 7 : 3));
        cad[i].frozen=(uint8_t)(i%11u==0u);
        cad[i].viewport_frozen=(uint8_t)(i%13u==0u);
        cad[i].has_truecolor=(uint8_t)(i%17u==0u);
        cad[i].truecolor_rgb=(uint32_t)(i&0xffffffu);
    }
    assert(musa_scene2d_layers_from_cad(cad,N,palette,256,0x112233u,full,N));
    MusaScene2dLayer page[PAGE];
    size_t next=0, pages=0, written=0, total=0;
    while (next<N) {
        memset(page,0xa5,sizeof page);
        assert(musa_scene2d_layers_from_cad_page(cad,N,next,palette,256,
                  0x112233u,page,PAGE,&written,&total));
        assert(total==N && written>0 && written<=PAGE);
        assert(memcmp(page,full+next,written*sizeof(*page))==0);
        const uint8_t *raw=(const uint8_t *)page;
        for (size_t i=written*sizeof(*page);i<sizeof(page);++i)
            assert(raw[i]==0xa5u);
        next+=written;
        ++pages;
    }
    assert(next==N && pages>100u);
    memset(page,0xa5,sizeof page);
    written=123; total=456;
    assert(musa_scene2d_layers_from_cad_page(cad,N,SIZE_MAX,palette,256,
                  0x112233u,NULL,PAGE,&written,&total));
    assert(written==0 && total==N);
    assert(musa_scene2d_layers_from_cad_page(cad,N,0,palette,256,
                  0x112233u,NULL,0,&written,&total));
    assert(written==0 && total==N);
    for (size_t i=0;i<sizeof page;++i)
        assert(((const uint8_t *)page)[i]==0xa5u);
    written=123; total=456;
    assert(!musa_scene2d_layers_from_cad_page(cad,N,0,palette,256,
                  0x112233u,NULL,1,&written,&total));
    assert(!musa_scene2d_layers_from_cad_page(cad,N,0,palette,256,
                  0x112233u,page,PAGE,&written,&written));
    assert(!musa_scene2d_layers_from_cad_page(cad,N,0,palette,256,
                  0x112233u,(MusaScene2dLayer *)(void *)cad,PAGE,
                  &written,&total));
    assert(!musa_scene2d_layers_from_cad_page(cad,N,0,palette,256,
                  0x112233u,page,PAGE,(size_t *)(void *)page,&total));
    assert(!musa_scene2d_layers_from_cad_page(cad,N,0,palette,256,
                  0x112233u,page,PAGE,&written,(size_t *)(void *)cad));
    assert(!musa_scene2d_layers_from_cad_page(cad,N,0,palette,255,
                  0x112233u,page,PAGE,&written,&total));
    assert(!musa_scene2d_layers_from_cad_page(cad,N,0,palette,256,
                  0x1000000u,page,PAGE,&written,&total));
    assert(!musa_scene2d_layers_from_cad_page(cad,SIZE_MAX,0,palette,256,
                  0x112233u,page,PAGE,&written,&total));
    cad[N-1].frozen=2u; /* A late invalid record cannot publish partial page. */
    assert(!musa_scene2d_layers_from_cad_page(cad,N,N-20u,palette,256,
                  0x112233u,page,PAGE,&written,&total));
    assert(written==123 && total==456);
    for (size_t i=0;i<sizeof page;++i)
        assert(((const uint8_t *)page)[i]==0xa5u);
    cad[N-1].frozen=0u;
    assert(musa_scene2d_layers_from_cad_page(NULL,0,0,NULL,0,
                  0,NULL,0,&written,&total));
    assert(written==0 && total==0);
    return 0;
}
