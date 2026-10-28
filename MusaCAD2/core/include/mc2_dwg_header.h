#ifndef MC2_DWG_HEADER_H
#define MC2_DWG_HEADER_H
#include <stddef.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
typedef enum {
    MC2_DWG_UNKNOWN = 0,
    MC2_DWG_AC1015,
    MC2_DWG_AC1018,
    MC2_DWG_AC1021,
    MC2_DWG_AC1024,
    MC2_DWG_AC1027,
    MC2_DWG_AC1032
} Mc2DwgVersion;
typedef struct {
    Mc2DwgVersion version;
    char signature[7];
    size_t file_length;
} Mc2DwgHeader;
int mc2_dwg_header_probe(const uint8_t *data, size_t length, Mc2DwgHeader *out);
#ifdef __cplusplus
}
#endif
#endif
