#include "mc2_bitstream.h"
#include <stdint.h>
#include <stddef.h>
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
