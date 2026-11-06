/* SPDX-License-Identifier: MIT */
#include "musa_dwg_handoff.h"
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <assert.h>
#include <math.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
static MusaDwgHandoffEntity make_entity(uint64_t handle, unsigned kind) {
    MusaDwgHandoffEntity e = {0};
    e.source_handle = handle;
    e.source_owner_handle = 7u; /* independent source-verified owner */
    e.source_layer_handle = 0u; /* real layer 0 is not an invalid handle */
    e.entity_kind = kind;
    e.color_mode = MUSA_DWG_HANDOFF_COLOR_BYLAYER;
    e.record_verified = 1u; e.ownership_verified = 1u;
    e.layer_verified = 1u; e.color_verified = 1u;
    e.geometry_verified = 1u;
    if (kind == MUSA_DWG_HANDOFF_LINE) {
        e.geometry[0] = 10.; e.geometry[1] = -20.;
        e.geometry[2] = 30.; e.geometry[3] = 50.;
    } else if (kind == MUSA_DWG_HANDOFF_CIRCLE) {
        e.geometry[0] = 2.; e.geometry[1] = 3.;
        e.geometry[2] = 5.;
    } else {
        e.geometry[0] = 1.; e.geometry[3] = -1.;
        e.geometry[4] = 100.; e.geometry[5] = -200.;
    }
    return e;
}
static void expect_fail(const uint8_t *bytes, size_t length,
                        MusaDwgRouteAdapter adapter,
                        const MusaDwgHandoffEntity *e, size_t count,
                        size_t source_count, int inventory, int empty) {
    MusaDwgHandoffSummary summary = {MUSA_DWG_ROUTE_FORMAT_AC1009,9u,8u,7u,6u};
    MusaDwgHandoffSummary old = summary;
    assert(!musa_dwg_handoff_validate(bytes,length,adapter,e,count,
                    source_count,inventory,empty,&summary));
    assert(memcmp(&summary,&old,sizeof summary)==0);
}
int main(void) {
    const char *signatures[] = {"AC1018", "AC1024", "AC1032"};
    const MusaDwgRouteAdapter adapters[] = {
        MUSA_DWG_ROUTE_ADAPTER_AC1018, MUSA_DWG_ROUTE_ADAPTER_AC1024,
        MUSA_DWG_ROUTE_ADAPTER_AC1032};
    const MusaDwgRouteFormat formats[] = {
        MUSA_DWG_ROUTE_FORMAT_AC1018, MUSA_DWG_ROUTE_FORMAT_AC1024,
        MUSA_DWG_ROUTE_FORMAT_AC1032};
    MusaDwgHandoffEntity e[3] = {
        make_entity(11u,MUSA_DWG_HANDOFF_LINE),
        make_entity(12u,MUSA_DWG_HANDOFF_CIRCLE),
        make_entity(13u,MUSA_DWG_HANDOFF_INSERT)
    };
    for(size_t i=0;i<3u;++i) {
        uint8_t bytes[16]={0};
        memcpy(bytes, signatures[i], 6u);
        bytes[6]=0xffu; bytes[7]=0x00u;
        uint8_t before[16]; memcpy(before,bytes,sizeof bytes);
        MusaDwgHandoffSummary result={0};
        assert(musa_dwg_handoff_validate(bytes,sizeof bytes,adapters[i],e,3u,
                                        3u,1,0,&result));
        assert(result.source_format==formats[i]);
        assert(result.total==3u && result.line_count==1u &&
               result.circle_count==1u && result.insert_count==1u);
        assert(memcmp(bytes,before,sizeof bytes)==0);
        expect_fail(bytes,sizeof bytes,adapters[(i+1u)%3u],e,3u,3u,1,0);
        expect_fail(bytes,sizeof bytes,adapters[i],e,3u,4u,1,0);
        expect_fail(bytes,sizeof bytes,adapters[i],e,3u,3u,0,0);
        expect_fail(bytes,sizeof bytes,adapters[i],e,3u,3u,1,1);
        expect_fail(bytes,5u,adapters[i],e,3u,3u,1,0);
        expect_fail(bytes,sizeof bytes,adapters[i],NULL,3u,3u,1,0);
        MusaDwgHandoffSummary empty={0};
        assert(musa_dwg_handoff_validate(bytes,sizeof bytes,adapters[i],
                                         NULL,0u,0u,1,1,&empty));
        expect_fail(bytes,sizeof bytes,adapters[i],NULL,0u,0u,1,0);
    }
    uint8_t ac1018[]={'A','C','1','0','1','8',0xffu};
    MusaDwgHandoffEntity changed[3];
    memcpy(changed,e,sizeof e);
    changed[1].source_handle=changed[0].source_handle;
    expect_fail(ac1018,sizeof ac1018,adapters[0],changed,3u,3u,1,0);
    memcpy(changed,e,sizeof e);
    changed[0].source_handle=0u;
    expect_fail(ac1018,sizeof ac1018,adapters[0],changed,3u,3u,1,0);
    memcpy(changed,e,sizeof e);
    changed[0].source_owner_handle=changed[0].source_handle;
    expect_fail(ac1018,sizeof ac1018,adapters[0],changed,3u,3u,1,0);
    memcpy(changed,e,sizeof e);
    changed[1].geometry[2]=0.;
    expect_fail(ac1018,sizeof ac1018,adapters[0],changed,3u,3u,1,0);
    memcpy(changed,e,sizeof e);
    changed[2].geometry[3]=0.;
    expect_fail(ac1018,sizeof ac1018,adapters[0],changed,3u,3u,1,0);
    memcpy(changed,e,sizeof e);
    changed[0].geometry[0]=NAN;
    expect_fail(ac1018,sizeof ac1018,adapters[0],changed,3u,3u,1,0);
    memcpy(changed,e,sizeof e);
    changed[0].geometry[0]=INFINITY;
    expect_fail(ac1018,sizeof ac1018,adapters[0],changed,3u,3u,1,0);
    memcpy(changed,e,sizeof e);
    changed[2].entity_kind=42u;
    expect_fail(ac1018,sizeof ac1018,adapters[0],changed,3u,3u,1,0);
    memcpy(changed,e,sizeof e);
    changed[1].color_mode=0u;
    expect_fail(ac1018,sizeof ac1018,adapters[0],changed,3u,3u,1,0);
    memcpy(changed,e,sizeof e);
    changed[1].geometry_verified=2u;
    expect_fail(ac1018,sizeof ac1018,adapters[0],changed,3u,3u,1,0);
    memcpy(changed,e,sizeof e);
    changed[1].layer_verified=0u;
    expect_fail(ac1018,sizeof ac1018,adapters[0],changed,3u,3u,1,0);
    memcpy(changed,e,sizeof e);
    changed[1].record_verified=0u;
    expect_fail(ac1018,sizeof ac1018,adapters[0],changed,3u,3u,1,0);
    expect_fail((const uint8_t *)"AC1021",6u,MUSA_DWG_ROUTE_ADAPTER_AC1024,
                e,3u,3u,1,0);
    return 0;
}
