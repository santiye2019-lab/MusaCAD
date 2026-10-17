/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "musacad_dxf_group.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

static void ok(const char *raw,int wanted){
    int value=-99;
    assert(musa_core_dxf_group_parse(raw,strlen(raw),&value));
    assert(value==wanted);
}
static void fail(const char *raw){
    int value=91;
    assert(!musa_core_dxf_group_parse(raw,strlen(raw),&value));
    assert(value==91); /* Failure never overwrites caller's value. */
}
int main(void){
    ok("0",0);ok("1071",1071);ok("  10\t",10);
    ok("0000000000000005",5);ok("+105",105);
    ok("  01071\r\n",1071);
    fail("");fail(" ");fail("+");fail("-1");
    fail("1072");fail("99999999999999999999999999999");
    fail("1e2");fail("0x10");fail("1 0");
    fail("١٠");fail("９"); /* Non-ASCII numeric code bytes must fail. */
    int unchanged=88;
    assert(!musa_core_dxf_group_parse(NULL,0,&unchanged));
    assert(!musa_core_dxf_group_parse("0",1,NULL));
    assert(unchanged==88);
    puts("PASS: bounded shared-core DXF group-code parsing and overflow guards");
    return 0;
}
