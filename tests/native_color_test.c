#include <stdio.h>
#include <stdlib.h>
#include "../app/src/main/cpp/native_color.h"

static void expect_kind(MusaNativeColorToken t,MusaNativeColorKind kind,const char *name){
    if(t.kind!=kind){fprintf(stderr,"%s kind %d != %d\n",name,(int)t.kind,(int)kind);exit(1);}
}
static void expect_aci(MusaNativeColorToken t,int aci,const char *name){
    expect_kind(t,MUSA_COLOR_ACI,name);
    if(t.aci!=aci){fprintf(stderr,"%s ACI %d != %d\n",name,t.aci,aci);exit(1);}
}
static void expect_rgb(MusaNativeColorToken t,unsigned int rgb,const char *name){
    expect_kind(t,MUSA_COLOR_RGB,name);
    if(t.rgb!=(rgb&0x00ffffffu)){fprintf(stderr,"%s RGB %06x != %06x\n",name,t.rgb,rgb&0xffffffu);exit(1);}
}

int main(void){
    expect_kind(musa_native_color_token(0xc0,256,0xc0000000u),MUSA_COLOR_INHERIT_LAYER,"ByLayer method");
    expect_kind(musa_native_color_token(0xc1,0,0xc1000000u),MUSA_COLOR_INHERIT_BLOCK,"ByBlock method");

    /* The regression: non-palette TrueColor commonly arrives with index 256. */
    expect_rgb(musa_native_color_token(0xc3,256,0xc3123456u),0x123456u,"TrueColor index 256");
    expect_rgb(musa_native_color_token(0xc2,256,0xc2a1b2c3u),0xa1b2c3u,"Entity RGB index 256");
    expect_rgb(musa_native_color_token(0,256,0xc30000ffu),0x0000ffu,"Method recovered from rgb high byte");

    /* Palette-backed colors keep exact ACI semantics. */
    expect_aci(musa_native_color_token(0xc3,1,0xc3ff0000u),1,"TrueColor matching ACI");
    expect_aci(musa_native_color_token(0,5,0),5,"Legacy ACI");
    expect_kind(musa_native_color_token(0,256,0),MUSA_COLOR_INHERIT_LAYER,"Legacy ByLayer");
    expect_kind(musa_native_color_token(0,0,0),MUSA_COLOR_INHERIT_BLOCK,"Legacy ByBlock");

    /* True black must not be mistaken for missing RGB. */
    expect_rgb(musa_native_color_token(0xc3,256,0xc3000000u),0x000000u,"True black");

    puts("Native first-paint color cases passed");
    return 0;
}
