#ifndef MC2_BITSTREAM_H
#define MC2_BITSTREAM_H
#include <stddef.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
typedef struct {
    const uint8_t *data;
    size_t byte_length;
    size_t bit_position;
} Mc2BitReader;
int mc2_bitreader_init(Mc2BitReader *r, const uint8_t *data, size_t length);
size_t mc2_bits_remaining(const Mc2BitReader *r);
int mc2_read_bits_msb(Mc2BitReader *r, unsigned count, uint32_t *out);
int mc2_read_bits_msb64(Mc2BitReader *r, unsigned count, uint64_t *out);
int mc2_seek_bits(Mc2BitReader *r, size_t position);
int mc2_skip_bits(Mc2BitReader *r, size_t count);
int mc2_align_byte(Mc2BitReader *r);
#ifdef __cplusplus
}
#endif
#endif
