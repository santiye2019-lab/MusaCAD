#include "mc2_bitstream.h"
#include <stdint.h>
#include <stddef.h>
#include <limits.h>
#include <float.h>
#include <math.h>
#include <string.h>
int mc2_bitreader_init(Mc2BitReader *r, const uint8_t *data, size_t length) {
    if (!r || (!data && length) || length > SIZE_MAX / 8u) return 0;
    r->data = data;
    r->byte_length = length;
    r->bit_position = 0;
    return 1;
}
size_t mc2_bits_remaining(const Mc2BitReader *r) {
    if (!r || r->byte_length > SIZE_MAX / 8u ||
        r->bit_position > r->byte_length * 8u) return 0;
    return r->byte_length * 8u - r->bit_position;
}
int mc2_read_bits_msb(Mc2BitReader *r, unsigned count, uint32_t *out) {
    if (!r || !out || count > 32u || (size_t)count > mc2_bits_remaining(r)) return 0;
    uint32_t value = 0;
    for (unsigned i = 0; i < count; ++i) {
        size_t bit = r->bit_position + i;
        uint8_t byte = r->data[bit / 8u];
        value = (value << 1) | ((byte >> (7u - (unsigned)(bit % 8u))) & 1u);
    }
    r->bit_position += count;
    *out = value;
    return 1;
}
int mc2_skip_bits(Mc2BitReader *r, size_t count) {
    if (!r || count > mc2_bits_remaining(r)) return 0;
    r->bit_position += count;
    return 1;
}
int mc2_align_byte(Mc2BitReader *r) {
    if (!r) return 0;
    size_t offset = r->bit_position % 8u;
    return offset ? mc2_skip_bits(r, 8u - offset) : 1;
}

int mc2_read_bits_msb64(Mc2BitReader *r, unsigned count, uint64_t *out) {
    if (!r || !out || count > 64u || (size_t)count > mc2_bits_remaining(r)) return 0;
    uint64_t value = 0;
    for (unsigned i = 0; i < count; ++i) {
        const size_t bit = r->bit_position + i;
        value = (value << 1) | (uint64_t)((r->data[bit / 8u] >> (7u - (unsigned)(bit % 8u))) & 1u);
    }
    r->bit_position += count;
    *out = value;
    return 1;
}
int mc2_seek_bits(Mc2BitReader *r, size_t position) {
    if (!r || r->byte_length > SIZE_MAX / 8u || position > r->byte_length * 8u) return 0;
    r->bit_position = position;
    return 1;
}

int mc2_read_bits_lsb64(Mc2BitReader *r, unsigned count, uint64_t *out) {
    if (!r || !out || count > 64u || (size_t)count > mc2_bits_remaining(r)) return 0;
    uint64_t value = 0;
    for (unsigned i = 0; i < count; ++i) {
        size_t bit = r->bit_position + i;
        value |= (uint64_t)((r->data[bit / 8u] >> (unsigned)(bit % 8u)) & 1u) << i;
    }
    r->bit_position += count;
    *out = value;
    return 1;
}

/* Independent implementation of the public DWG BS/BL bit-code rules.
 * A two-bit selector precedes optional little-endian raw payload bytes.
 * The bit stream itself is traversed MSB-first, including unaligned bytes. */
static int mc2_dwg_raw_le(Mc2BitReader *r, unsigned bytes, uint64_t *out) {
    uint64_t value = 0;
    for (unsigned i = 0; i < bytes; ++i) {
        uint32_t octet;
        if (!mc2_read_bits_msb(r, 8, &octet)) return 0;
        value |= (uint64_t)octet << (i * 8u);
    }
    *out = value;
    return 1;
}
int mc2_read_dwg_bitshort(Mc2BitReader *r, int16_t *out) {
    if (!r || !out) return 0;
    Mc2BitReader candidate = *r;
    uint32_t selector;
    uint64_t raw = 0;
    if (!mc2_read_bits_msb(&candidate, 2, &selector)) return 0;
    if (selector == 0 && !mc2_dwg_raw_le(&candidate, 2, &raw)) return 0;
    if (selector == 1 && !mc2_dwg_raw_le(&candidate, 1, &raw)) return 0;
    if (selector == 3) raw = 256;
    /* Explicit signed conversion avoids implementation-defined narrowing. */
    int16_t value = (int16_t)(raw <= 32767 ? (int32_t)raw : (int32_t)raw - 65536);
    *out = value;
    *r = candidate;
    return 1;
}
int mc2_read_dwg_bitlong(Mc2BitReader *r, int32_t *out) {
    if (!r || !out) return 0;
    Mc2BitReader candidate = *r;
    uint32_t selector;
    uint64_t raw = 0;
    if (!mc2_read_bits_msb(&candidate, 2, &selector) || selector == 3) return 0;
    if (selector == 0 && !mc2_dwg_raw_le(&candidate, 4, &raw)) return 0;
    if (selector == 1 && !mc2_dwg_raw_le(&candidate, 1, &raw)) return 0;
    int32_t value = (int32_t)(raw <= INT32_MAX ? (int64_t)raw : (int64_t)raw - INT64_C(4294967296));
    *out = value;
    *r = candidate;
    return 1;
}

int mc2_read_dwg_bitdouble(Mc2BitReader *r, double *out) {
    if (!r || !out || sizeof(double) != 8 || DBL_MANT_DIG != 53 ||
        FLT_RADIX != 2) return 0;
    Mc2BitReader candidate = *r;
    uint32_t selector;
    if (!mc2_read_bits_msb(&candidate, 2, &selector) || selector == 3) return 0;
    double value;
    if (selector == 1) value = 1.0;
    else if (selector == 2) value = 0.0;
    else {
        uint64_t bits;
        if (!mc2_dwg_raw_le(&candidate, 8, &bits)) return 0;
        memcpy(&value, &bits, sizeof value);
        if (!isfinite(value)) return 0;
    }
    *out = value;
    *r = candidate;
    return 1;
}
