/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "musacad_dwg_ac1018_eed.h"
#include <stdint.h>
#include <stddef.h>
int musa_dwg_ac1018_eed_skip(MusaDwgAc1015Bits *bits,
                             uint32_t *raw_eed_bytes) {
    if (!bits || !raw_eed_bytes || !bits->data ||
        bits->bit_pos > bits->bit_end) return 0;
    MusaDwgAc1015Bits b = *bits;
    uint32_t sum = 0u;
    for (unsigned group = 0u; group <= 64u; ++group) {
        uint16_t payload_bytes = 0u;
        if (!musa_dwg_ac1015_bits_read_bs(&b, &payload_bytes)) return 0;
        if (!payload_bytes) {
            *bits = b;
            *raw_eed_bytes = sum;
            return 1;
        }
        if (group == 64u || payload_bytes > 4096u - sum) return 0;
        uint32_t descriptor = 0u;
        if (!musa_dwg_ac1015_bits_read_u(&b, 8u, &descriptor)) return 0;
        const unsigned handle_size = descriptor & 15u;
        if ((descriptor >> 4u) != 5u || handle_size == 0u ||
            handle_size > 8u) return 0;
        /* EED APPID handle + raw opaque EED bytes are length-delimited.
         * Never present EED bytes as geometry or silently drop their content
         * in a future scene writer.
         */
        const size_t total_bits = (size_t)(handle_size + payload_bytes) * 8u;
        if (total_bits > b.bit_end - b.bit_pos) return 0;
        b.bit_pos += total_bits;
        sum += payload_bytes;
    }
    return 0;
}
