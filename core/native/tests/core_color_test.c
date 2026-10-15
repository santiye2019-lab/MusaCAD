/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "musacad_color.h"
#include <assert.h>
#include <limits.h>
#include <stdio.h>

static void equal_color(MusaNativeSceneColor color,int aci,unsigned rgb,int truecolor){
    assert(color.aci==aci);
    assert(color.truecolor==truecolor);
    if(truecolor)assert(color.rgb==(rgb&0xffffffu));
}
int main(void){
    MusaNativeSceneColor layer={3,0u,0},parent={7,0x224466u,1};
    MusaNativeColorToken bylayer=musa_native_color_token(0xc0u,256,0xc0000000u);
    MusaNativeColorToken byblock=musa_native_color_token(0xc1u,0,0xc1000000u);
    equal_color(musa_native_color_resolve(bylayer,layer,parent),3,0,0);
    equal_color(musa_native_color_resolve(byblock,layer,parent),7,0x224466u,1);
    equal_color(musa_native_color_resolve(musa_native_color_token(0xc2u,5,0xc2000000u),layer,parent),5,0,0);
    equal_color(musa_native_color_resolve(musa_native_color_token(0xc3u,256,0xc3000000u),layer,parent),7,0,1);
    equal_color(musa_native_color_resolve(musa_native_color_token(0xc3u,1,0xc3123456u),layer,parent),7,0x123456u,1);
    assert(musa_native_color_token(0xc2u,256,0xc2aabbccu).kind==MUSA_COLOR_INHERIT_LAYER);
    assert(musa_native_color_token(0xc2u,0,0xc2000000u).kind==MUSA_COLOR_INHERIT_BLOCK);
    assert(musa_native_color_token(0u,INT_MIN,0).kind==MUSA_COLOR_INHERIT_LAYER);
    assert(musa_native_color_token(0u,0,0).kind==MUSA_COLOR_INHERIT_BLOCK);
    assert(musa_native_color_token(0u,4,0).aci==4);
    equal_color(musa_native_effective_layer_color(1,1,layer,parent),7,0x224466,1);
    equal_color(musa_native_effective_layer_color(1,0,layer,parent),3,0,0);
    equal_color(musa_native_effective_layer_color(0,1,layer,parent),3,0,0);
    puts("MusaCAD common color: ACI RGB true black and DWG inheritance PASS");
    return 0;
}
