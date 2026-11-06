/* SPDX-License-Identifier: MIT */
#include "musa_dwg_route.h"
#include <stdint.h>
#include <string.h>

typedef struct MusaKnownVersion {
    char bytes[6];
    MusaDwgRouteFormat format;
    MusaDwgRouteAdapter adapter;
} MusaKnownVersion;

/* These six-byte tokens describe DWG *file formats*, not AutoCAD product years.
 * In particular AutoCAD 2018–2027 may all carry the AC1032 token.
 * Recognition never implies that the corresponding decoder is complete.
 */
static const MusaKnownVersion known_versions[] = {
    {{'A','C','1','0','0','9'}, MUSA_DWG_ROUTE_FORMAT_AC1009, MUSA_DWG_ROUTE_ADAPTER_NONE},
    {{'A','C','1','0','1','2'}, MUSA_DWG_ROUTE_FORMAT_AC1012, MUSA_DWG_ROUTE_ADAPTER_NONE},
    {{'A','C','1','0','1','4'}, MUSA_DWG_ROUTE_FORMAT_AC1014, MUSA_DWG_ROUTE_ADAPTER_NONE},
    {{'A','C','1','0','1','5'}, MUSA_DWG_ROUTE_FORMAT_AC1015, MUSA_DWG_ROUTE_ADAPTER_NONE},
    {{'A','C','1','0','1','8'}, MUSA_DWG_ROUTE_FORMAT_AC1018, MUSA_DWG_ROUTE_ADAPTER_AC1018},
    {{'A','C','1','0','2','1'}, MUSA_DWG_ROUTE_FORMAT_AC1021, MUSA_DWG_ROUTE_ADAPTER_NONE},
    {{'A','C','1','0','2','4'}, MUSA_DWG_ROUTE_FORMAT_AC1024, MUSA_DWG_ROUTE_ADAPTER_AC1024},
    {{'A','C','1','0','2','7'}, MUSA_DWG_ROUTE_FORMAT_AC1027, MUSA_DWG_ROUTE_ADAPTER_NONE},
    {{'A','C','1','0','3','2'}, MUSA_DWG_ROUTE_FORMAT_AC1032, MUSA_DWG_ROUTE_ADAPTER_AC1032}
};

MusaDwgRoute musa_dwg_route_probe(const uint8_t *data, size_t length) {
    MusaDwgRoute out = { MUSA_DWG_ROUTE_FORMAT_UNKNOWN,
                         MUSA_DWG_ROUTE_ADAPTER_NONE,
                         MUSA_DWG_ROUTE_MALFORMED };
    if (!data || length < 6u || length > PTRDIFF_MAX ||
        data[0] != (uint8_t)'A' || data[1] != (uint8_t)'C')
        return out;
    for (size_t i = 2u; i < 6u; ++i)
        if (data[i] < (uint8_t)'0' || data[i] > (uint8_t)'9')
            return out;
    out.status = MUSA_DWG_ROUTE_UNKNOWN_FORMAT;
    for (size_t i = 0u; i < sizeof known_versions / sizeof known_versions[0]; ++i) {
        if (memcmp(data, known_versions[i].bytes, 6u) != 0)
            continue;
        out.format = known_versions[i].format;
        out.adapter = known_versions[i].adapter;
        out.status = out.adapter == MUSA_DWG_ROUTE_ADAPTER_NONE
                   ? MUSA_DWG_ROUTE_KNOWN_NOT_ROUTED
                   : MUSA_DWG_ROUTE_ADAPTER_CANDIDATE;
        break;
    }
    return out;
}
