#include "mc2_bytes.h"
#include <string.h>

int mc2_reader_init(Mc2Reader *r, const uint8_t *data, size_t length) {
    if (!r || (!data && length)) return 0;
    r->data = data; r->length = length; r->offset = 0;
    return 1;
}
size_t mc2_reader_remaining(const Mc2Reader *r) {
    if (!r || r->offset > r->length) return 0;
    return r->length - r->offset;
}
int mc2_read_bytes(Mc2Reader *r, uint8_t *out, size_t count) {
    if (!r || (!out && count) || mc2_reader_remaining(r) < count) return 0;
    if (count) memcpy(out, r->data + r->offset, count);
    r->offset += count;
    return 1;
}
int mc2_read_u8(Mc2Reader *r, uint8_t *out) {
    return mc2_read_bytes(r, out, 1);
}
int mc2_read_u16le(Mc2Reader *r, uint16_t *out) {
    uint8_t b[2];
    if (!out || !mc2_read_bytes(r, b, sizeof b)) return 0;
    *out = (uint16_t)((uint16_t)b[0] | ((uint16_t)b[1] << 8));
    return 1;
}
int mc2_read_u32le(Mc2Reader *r, uint32_t *out) {
    uint8_t b[4];
    if (!out || !mc2_read_bytes(r, b, sizeof b)) return 0;
    *out = (uint32_t)b[0] | ((uint32_t)b[1] << 8) |
           ((uint32_t)b[2] << 16) | ((uint32_t)b[3] << 24);
    return 1;
}
int mc2_read_dwg_version(const uint8_t *data, size_t length, char out[7]) {
    if (!data || !out || length < 6) return 0;
    if (data[0] != 'A' || data[1] != 'C') return 0;
    for (size_t i = 2; i < 6; ++i) {
        if (data[i] < '0' || data[i] > '9') return 0;
    }
    memcpy(out, data, 6);
    out[6] = '\0';
    return 1;
}
