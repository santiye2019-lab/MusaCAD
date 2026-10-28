#include "mc2_dwg_header.h"
#include "mc2_bytes.h"
#include <string.h>
int mc2_dwg_header_probe(const uint8_t *data, size_t length, Mc2DwgHeader *out) {
    static const struct { const char *sig; Mc2DwgVersion version; } versions[] = {
        {"AC1015", MC2_DWG_AC1015},
        {"AC1018", MC2_DWG_AC1018},
        {"AC1021", MC2_DWG_AC1021},
        {"AC1024", MC2_DWG_AC1024},
        {"AC1027", MC2_DWG_AC1027},
        {"AC1032", MC2_DWG_AC1032}
    };
    char sig[7];
    if (!out || !mc2_read_dwg_version(data, length, sig)) return 0;
    for (size_t i = 0; i < sizeof versions / sizeof versions[0]; ++i) {
        if (strcmp(sig, versions[i].sig) == 0) {
            Mc2DwgHeader result;
            result.version = versions[i].version;
            memcpy(result.signature, sig, sizeof sig);
            result.file_length = length;
            *out = result;
            return 1;
        }
    }
    return 0;
}
