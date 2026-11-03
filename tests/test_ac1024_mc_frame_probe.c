/* Pure first-party test-only R2010 record-envelope regression.
 * No LibreDWG library, source, ABI or geometry is linked here.
 * Include the diagnostic source with a renamed CLI main only to exercise
 * its static frame helper against synthetic checked fixtures. */
#define main musa_native_stage_probe_cli_main
#include "real_dwg_native_stage_probe.c"
#undef main

static int check_r2010_mc_fixture(size_t mc_len) {
    if (mc_len < 1u || mc_len > 3u) return 0;
    uint8_t record[64] = {0};
    const size_t ms_payload_bytes = 8u;
    record[0] = (uint8_t)ms_payload_bytes;
    record[1] = 0u;
    for (size_t i = 0u; i < mc_len; i++)
        record[2u + i] = (uint8_t)(i + 1u < mc_len ? 0x81u : 0x01u);
    for (size_t i = 0u; i < ms_payload_bytes; i++)
        record[2u + mc_len + i] = (uint8_t)(0x20u + i);
    const size_t crc_pos = 2u + mc_len + ms_payload_bytes;
    uint16_t crc = 0u;
    if (!musa_core_crc16_arc(0xc0c1u, record, crc_pos, &crc)) return 0;
    record[crc_pos] = (uint8_t)crc;
    record[crc_pos + 1u] = (uint8_t)(crc >> 8u);
    size_t span = 0u, observed_mc = 0u;
    int crc_ok = 0;
    const size_t true_span = crc_pos + 2u;
    if (!ac1024_r2010_mc_frame(
            record, true_span, 0u, &span, &observed_mc, &crc_ok)
        || !crc_ok || span != true_span || observed_mc != mc_len)
        return 0;
    uint16_t type = 0xffffu;
    if (!ac1024_r2010_declared_ot(
            record, true_span, 0u, mc_len, &type) || type != 128u)
        return 0;
    record[crc_pos] ^= 0x80u;
    span = 0u; observed_mc = 0u; crc_ok = 1;
    if (!ac1024_r2010_mc_frame(
            record, true_span, 0u, &span, &observed_mc, &crc_ok)
        || crc_ok || span != true_span || observed_mc != mc_len)
        return 0;
    if (ac1024_r2010_mc_frame(
            record, true_span - 1u, 0u,
            &span, &observed_mc, &crc_ok))
        return 0;
    return 1;
}

static int check_ot_encoding_variants(void) {
    uint8_t record[32] = {0};
    record[0] = 8u; /* MS length excludes the separate MC header */
    record[2] = 0x01u; /* one-byte unsigned handle-stream-bit-size marker */
    const unsigned selectors[] = {0u, 1u, 2u, 3u};
    const uint16_t expected[] = {0u, 0x1f0u, 0u, 0u};
    for (size_t i = 0u; i < 4u; i++) {
        record[3] = (uint8_t)(selectors[i] << 6u);
        record[4] = 0u;
        record[5] = 0u;
        const size_t crc_at = 11u;
        uint16_t crc = 0u;
        if (!musa_core_crc16_arc(
                0xc0c1u, record, crc_at, &crc))
            return 0;
        record[crc_at] = (uint8_t)crc;
        record[crc_at + 1u] = (uint8_t)(crc >> 8u);
        size_t span = 0u, mc_len = 0u;
        int crc_ok = 0;
        if (!ac1024_r2010_mc_frame(
                record, crc_at + 2u, 0u,
                &span, &mc_len, &crc_ok) ||
            !crc_ok || span != crc_at + 2u || mc_len != 1u)
            return 0;
        uint16_t raw = 0xffffu;
        if (!ac1024_r2010_declared_ot(
                record, span, 0u, mc_len, &raw) ||
            raw != expected[i])
            return 0;
    }
    return 1;
}

int main(void) {
    for (size_t length = 1u; length <= 3u; length++) {
        if (!check_r2010_mc_fixture(length)) {
            fputs("synthetic R2010 MS+MC frame regression failed\n", stderr);
            return 1;
        }
    }
    if (!check_ot_encoding_variants()) {
        fputs("synthetic R2010 OT selector regression failed\n", stderr);
        return 3;
    }
    /* Unknown or malformed MC continuation MUST fail closed. */
    uint8_t truncated[32] = {0};
    truncated[0] = 8u;
    for (size_t i = 2u; i < 12u; i++)
        truncated[i] = 0x81u;
    size_t span = 123u, mc_bytes = 123u;
    int crc_ok = 1;
    if (ac1024_r2010_mc_frame(
            truncated, sizeof(truncated), 0u,
            &span, &mc_bytes, &crc_ok))
        return 2;
    puts("Synthetic AC1024 R2010 MS+MC frame CRC regression: passed");
    return 0;
}
