/* Test only the NEW DWG bit-coded classes prefix, not legacy geometry.
 * Compile original first-party C11 source to reach file-local, bounded
 * MSB-first BL/BS primitives; no third-party CAD library is involved. */
#include <assert.h>
#include <stdint.h>
#include <string.h>
#include "../src/musacad_dwg_ac1032_classes.c"
int main(void){
    uint16_t s=999u;
    uint32_t l=999u;
    const uint8_t zero[]={0x80u};
    DwgClassBits b0={zero,2u,0u};
    assert(bitshort(&b0,&s)&&s==0u&&b0.at==2u);
    const uint8_t special[]={0xc0u};
    DwgClassBits b1={special,2u,0u};
    assert(bitshort(&b1,&s)&&s==256u);
    const uint8_t uchar42[]={0x4au,0x80u};
    DwgClassBits b2={uchar42,10u,0u};
    assert(bitshort(&b2,&s)&&s==42u&&b2.at==10u);
    const uint8_t short845[]={0x11u,0x40u,0xc0u};
    DwgClassBits b3={short845,18u,0u};
    assert(bitshort(&b3,&s)&&s==0x345u&&b3.at==18u);
    const uint8_t bitlong_zero[]={0x80u};
    DwgClassBits b4={bitlong_zero,2u,0u};
    assert(bitlong(&b4,&l)&&l==0u);
    DwgClassBits b5={uchar42,10u,0u};
    assert(bitlong(&b5,&l)&&l==42u);
    const uint8_t forbidden[]={0xc0u};
    DwgClassBits b6={forbidden,2u,0u};
    const uint32_t saved_l=l;
    assert(!bitlong(&b6,&l)&&b6.at==0u&&l==saved_l);
    DwgClassBits b7={short845,17u,0u};
    const uint16_t saved_s=s;
    assert(!bitshort(&b7,&s)&&b7.at==0u&&s==saved_s);
    DwgClassBits b8={short845,18u,0u};
    const uint32_t prior_l=l;
    assert(!bitlong(&b8,&l)&&b8.at==0u&&l==prior_l);
    /* Read a combined header with two individually coded values.
     * Both fields advance in proper MSB order without byte alignment. */
    const uint8_t joined[]={0x9au,0xa0u}; /* 10 (0), 01 10101010 (170) */
    DwgClassBits b9={joined,12u,0u};
    assert(bitlong(&b9,&l)&&l==0u&&b9.at==2u);
    assert(bitshort(&b9,&s)&&s==170u&&b9.at==12u);
    return 0;
}
