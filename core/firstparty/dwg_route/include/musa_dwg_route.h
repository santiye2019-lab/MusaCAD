/* SPDX-License-Identifier: MIT
 * MusaCAD-owned, format-only DWG reader route. Not a DWG decoder.
 */
#ifndef MUSA_DWG_ROUTE_H
#define MUSA_DWG_ROUTE_H
#include <stddef.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif

typedef enum MusaDwgRouteFormat {
    MUSA_DWG_ROUTE_FORMAT_UNKNOWN = 0,
    MUSA_DWG_ROUTE_FORMAT_AC1009 = 1009,
    MUSA_DWG_ROUTE_FORMAT_AC1012 = 1012,
    MUSA_DWG_ROUTE_FORMAT_AC1014 = 1014,
    MUSA_DWG_ROUTE_FORMAT_AC1015 = 1015,
    MUSA_DWG_ROUTE_FORMAT_AC1018 = 1018,
    MUSA_DWG_ROUTE_FORMAT_AC1021 = 1021,
    MUSA_DWG_ROUTE_FORMAT_AC1024 = 1024,
    MUSA_DWG_ROUTE_FORMAT_AC1027 = 1027,
    MUSA_DWG_ROUTE_FORMAT_AC1032 = 1032
} MusaDwgRouteFormat;

typedef enum MusaDwgRouteAdapter {
    MUSA_DWG_ROUTE_ADAPTER_NONE = 0,
    MUSA_DWG_ROUTE_ADAPTER_AC1018 = 1018,
    MUSA_DWG_ROUTE_ADAPTER_AC1024 = 1024,
    MUSA_DWG_ROUTE_ADAPTER_AC1032 = 1032
} MusaDwgRouteAdapter;

typedef enum MusaDwgRouteStatus {
    MUSA_DWG_ROUTE_MALFORMED = 0,
    MUSA_DWG_ROUTE_UNKNOWN_FORMAT = 1,
    MUSA_DWG_ROUTE_KNOWN_NOT_ROUTED = 2,
    MUSA_DWG_ROUTE_ADAPTER_CANDIDATE = 3
} MusaDwgRouteStatus;

typedef struct MusaDwgRoute {
    MusaDwgRouteFormat format;
    MusaDwgRouteAdapter adapter;
    MusaDwgRouteStatus status;
} MusaDwgRoute;

/* Inspect only six header bytes; trailing bytes may be binary. A candidate
 * says which independently authenticated version reader SHOULD receive this
 * file; it never certifies that the file is parseable, safe to paint, or
 * that the reader is implemented or wired into the application.
 * Fail closed on malformed/unrecognized/unsupported version signatures.
 */
MusaDwgRoute musa_dwg_route_probe(const uint8_t *data, size_t length);
#ifdef __cplusplus
}
#endif
#endif
