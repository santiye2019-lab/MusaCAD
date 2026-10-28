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
/* Little-endian bit fields: first bit read has numeric weight 2^0. */
int mc2_read_bits_lsb64(Mc2BitReader *r, unsigned count, uint64_t *out);
int mc2_seek_bits(Mc2BitReader *r, size_t position);
int mc2_skip_bits(Mc2BitReader *r, size_t count);
int mc2_align_byte(Mc2BitReader *r);
/* DWG bit-coded signed integers (BS and BL). Failed reads are transactional:
 * neither cursor nor output changes.  Raw payload bytes are little-endian. */
int mc2_read_dwg_bitshort(Mc2BitReader *r, int16_t *out);
int mc2_read_dwg_bitlong(Mc2BitReader *r, int32_t *out);
/* DWG BD: raw IEEE-754 double, implicit 1.0, or implicit 0.0. */
int mc2_read_dwg_bitdouble(Mc2BitReader *r, double *out);
/* DWG H references: preserve the ownership/pointer code and resolve relative
 * handle offsets against the supplied object handle. */
typedef struct { uint8_t code; uint64_t handle; } Mc2DwgHandleRef;
int mc2_read_dwg_handle(Mc2BitReader *r, uint64_t base_handle,
                        Mc2DwgHandleRef *out);
#ifdef __cplusplus
}
#endif
#endif
