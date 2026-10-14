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


static void expect_resolved(MusaNativeSceneColor actual, int aci, unsigned int rgb,
                            int truecolor, const char *name){
    if(actual.aci!=aci||actual.truecolor!=truecolor||
       (truecolor&&actual.rgb!=(rgb&0x00ffffffu))){
        fprintf(stderr,"%s color mismatch ACI=%d RGB=%06x true=%d\n",
                name,actual.aci,actual.rgb,actual.truecolor);exit(1);
    }
}
static void test_first_paint_inherited_colors(void){
    MusaNativeSceneColor root={7,0,0},layer={3,0,0};
    MusaNativeSceneColor red_insert={7,0xff0000u,1};
    MusaNativeSceneColor blue_layer={7,0x0000ffu,1};
    expect_resolved(
        musa_native_color_resolve(
            musa_native_color_token(0xc0,256,0xc0000000u),layer,red_insert),
        3,0,0,"ByLayer uses object layer");
    expect_resolved(
        musa_native_color_resolve(
            musa_native_color_token(0xc1,0,0xc1000000u),layer,red_insert),
        7,0xff0000u,1,"ByBlock inherits truecolor insert");
    expect_resolved(
        musa_native_color_resolve(
            musa_native_color_token(0xc3,256,0xc3000000u),layer,red_insert),
        7,0x000000u,1,"explicit true black does not inherit");
    expect_resolved(
        musa_native_color_resolve(
            musa_native_color_token(0xc2,5,0xc2000000u),layer,red_insert),
        5,0,0,"explicit ACI overrides both inherited colors");
    expect_resolved(
        musa_native_effective_layer_color(1,1,blue_layer,red_insert),
        7,0xff0000u,1,"nested layer zero inherits insert layer");
    expect_resolved(
        musa_native_effective_layer_color(1,0,blue_layer,red_insert),
        7,0x0000ffu,1,"named layer retains its own truecolor");
    expect_resolved(
        musa_native_effective_layer_color(0,1,blue_layer,root),
        7,0x0000ffu,1,"model-space layer zero uses its own layer color");
    expect_kind(musa_native_color_token(0,(-2147483647-1),0),
                MUSA_COLOR_INHERIT_LAYER,"safe INT_MIN color index");
}
int main(void){
    test_first_paint_inherited_colors();
    expect_kind(musa_native_color_token(0xc0,256,0xc0000000u),MUSA_COLOR_INHERIT_LAYER,"ByLayer method");
    expect_kind(musa_native_color_token(0xc1,0,0xc1000000u),MUSA_COLOR_INHERIT_BLOCK,"ByBlock method");

    /* LibreDWG method c2 is ACI, including the two inheritance sentinels. */
    expect_aci(musa_native_color_token(0xc2,5,0xc2000000u),5,"ACI method");
    expect_kind(musa_native_color_token(0xc2,256,0xc2a1b2c3u),MUSA_COLOR_INHERIT_LAYER,"ACI ByLayer sentinel");
    expect_kind(musa_native_color_token(0xc2,0,0xc2a1b2c3u),MUSA_COLOR_INHERIT_BLOCK,"ACI ByBlock sentinel");

    /* LibreDWG method c3 is TrueColor even when index looks like an ACI. */
    expect_rgb(musa_native_color_token(0xc3,256,0xc3123456u),0x123456u,"TrueColor index 256");
    expect_rgb(musa_native_color_token(0xc3,1,0xc3123456u),0x123456u,"TrueColor with palette-like index");
    expect_rgb(musa_native_color_token(0,256,0xc30000ffu),0x0000ffu,"Method recovered from rgb high byte");

    /* Legacy index-only colors keep exact inheritance/ACI semantics. */
    expect_aci(musa_native_color_token(0,5,0),5,"Legacy ACI");
    expect_kind(musa_native_color_token(0,256,0),MUSA_COLOR_INHERIT_LAYER,"Legacy ByLayer");
    expect_kind(musa_native_color_token(0,0,0),MUSA_COLOR_INHERIT_BLOCK,"Legacy ByBlock");

    /* True black must not be mistaken for missing RGB. */
    expect_rgb(musa_native_color_token(0xc3,256,0xc3000000u),0x000000u,"True black");

    puts("Native first-paint AutoCAD color method cases passed");
    return 0;
}
