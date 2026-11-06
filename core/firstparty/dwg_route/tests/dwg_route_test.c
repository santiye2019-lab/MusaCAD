/* SPDX-License-Identifier: MIT */
#include "musa_dwg_route.h"
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <assert.h>
#include <stdint.h>
#include <string.h>

static void expect(const char *signature, MusaDwgRouteFormat version,
                   MusaDwgRouteAdapter adapter, MusaDwgRouteStatus status) {
    uint8_t file[64];
    uint8_t original[64];
    memcpy(file, signature, 6u);
    for (size_t i = 6u; i < sizeof file; ++i)
        file[i] = (uint8_t)(i * 31u);
    memcpy(original, file, sizeof file);
    const MusaDwgRoute result = musa_dwg_route_probe(file, sizeof file);
    assert(result.format == version);
    assert(result.adapter == adapter);
    assert(result.status == status);
    assert(memcmp(file, original, sizeof file) == 0);
}

int main(void) {
    expect("AC1009", MUSA_DWG_ROUTE_FORMAT_AC1009, MUSA_DWG_ROUTE_ADAPTER_NONE, MUSA_DWG_ROUTE_KNOWN_NOT_ROUTED);
    expect("AC1012", MUSA_DWG_ROUTE_FORMAT_AC1012, MUSA_DWG_ROUTE_ADAPTER_NONE, MUSA_DWG_ROUTE_KNOWN_NOT_ROUTED);
    expect("AC1014", MUSA_DWG_ROUTE_FORMAT_AC1014, MUSA_DWG_ROUTE_ADAPTER_NONE, MUSA_DWG_ROUTE_KNOWN_NOT_ROUTED);
    expect("AC1015", MUSA_DWG_ROUTE_FORMAT_AC1015, MUSA_DWG_ROUTE_ADAPTER_NONE, MUSA_DWG_ROUTE_KNOWN_NOT_ROUTED);
    expect("AC1018", MUSA_DWG_ROUTE_FORMAT_AC1018, MUSA_DWG_ROUTE_ADAPTER_AC1018, MUSA_DWG_ROUTE_ADAPTER_CANDIDATE);
    expect("AC1021", MUSA_DWG_ROUTE_FORMAT_AC1021, MUSA_DWG_ROUTE_ADAPTER_NONE, MUSA_DWG_ROUTE_KNOWN_NOT_ROUTED);
    expect("AC1024", MUSA_DWG_ROUTE_FORMAT_AC1024, MUSA_DWG_ROUTE_ADAPTER_AC1024, MUSA_DWG_ROUTE_ADAPTER_CANDIDATE);
    expect("AC1027", MUSA_DWG_ROUTE_FORMAT_AC1027, MUSA_DWG_ROUTE_ADAPTER_NONE, MUSA_DWG_ROUTE_KNOWN_NOT_ROUTED);
    expect("AC1032", MUSA_DWG_ROUTE_FORMAT_AC1032, MUSA_DWG_ROUTE_ADAPTER_AC1032, MUSA_DWG_ROUTE_ADAPTER_CANDIDATE);
    assert(musa_dwg_route_probe(NULL, 6u).status == MUSA_DWG_ROUTE_MALFORMED);
    assert(musa_dwg_route_probe((const uint8_t *)"AC1032", 5u).status == MUSA_DWG_ROUTE_MALFORMED);
    assert(musa_dwg_route_probe((const uint8_t *)"AC1032", SIZE_MAX).status == MUSA_DWG_ROUTE_MALFORMED);
    assert(musa_dwg_route_probe((const uint8_t *)"AC1033", 6u).status == MUSA_DWG_ROUTE_UNKNOWN_FORMAT);
    assert(musa_dwg_route_probe((const uint8_t *)"AC10x2", 6u).status == MUSA_DWG_ROUTE_MALFORMED);
    assert(musa_dwg_route_probe((const uint8_t *)"XX1032", 6u).status == MUSA_DWG_ROUTE_MALFORMED);
    for (size_t i = 0u; i < 6u; ++i) {
        uint8_t mutated[] = {'A','C','1','0','3','2'};
        mutated[i] = 0xffu;
        assert(musa_dwg_route_probe(mutated, sizeof mutated).status == MUSA_DWG_ROUTE_MALFORMED);
    }
    /* Binary body is allowed; only six-byte header participates in routing. */
    uint8_t binary[] = {'A','C','1','0','2','4',0x00u,0xffu,0x13u};
    assert(musa_dwg_route_probe(binary, sizeof binary).adapter == MUSA_DWG_ROUTE_ADAPTER_AC1024);
    /* Product year is not a DWG format signature. */
    assert(musa_dwg_route_probe((const uint8_t *)"AC2027", 6u).status == MUSA_DWG_ROUTE_UNKNOWN_FORMAT);
    return 0;
}
