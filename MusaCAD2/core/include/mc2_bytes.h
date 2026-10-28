#ifndef MC2_BYTES_H
#define MC2_BYTES_H
#include <stddef.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
typedef struct { const uint8_t *data; size_t length; size_t offset; } Mc2Reader;
int mc2_reader_init(Mc2Reader *r, const uint8_t *data, size_t length);
size_t mc2_reader_remaining(const Mc2Reader *r);
int mc2_read_u8(Mc2Reader *r, uint8_t *out);
int mc2_read_u16le(Mc2Reader *r, uint16_t *out);
int mc2_read_u32le(Mc2Reader *r, uint32_t *out);
int mc2_read_bytes(Mc2Reader *r, uint8_t *out, size_t count);
int mc2_read_dwg_version(const uint8_t *data, size_t length, char out[7]);
#ifdef __cplusplus
}
#endif
#endif
