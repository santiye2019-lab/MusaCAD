/*
 * Read-only, bounded diagnostics of a rejected first-party DWG physical map.
 * This tool never grants admission to the core or changes the source and
 * never exports any drawing bytes, handles, coordinates, names or object data.
 */
#include "musacad_dwg_ac1018_page_map.h"
#include "musacad_dwg_ac1032_page_map.h"
#include "musacad_dwg_ac1018_sections.h"
#include "musacad_dwg_ac1032_sections.h"
#include "musacad_dwg_ac1018_lz.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MUSA_DIAG_MAX_FILE (256u * 1024u * 1024u)
#define MUSA_DIAG_MAX_MAP (1024u * 1024u)

static uint32_t read32(const uint8_t *p) {
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8u) |
           ((uint32_t)p[2] << 16u) | ((uint32_t)p[3] << 24u);
}

static int load_readonly(const char *path, uint8_t **out, size_t *size) {
    FILE *f = fopen(path, "rb");
    if (!f) return 0;
    if (fseek(f, 0, SEEK_END) != 0) { fclose(f); return 0; }
    const long n = ftell(f);
    if (n < 256 || (unsigned long)n > MUSA_DIAG_MAX_FILE ||
        fseek(f, 0, SEEK_SET) != 0) { fclose(f); return 0; }
    uint8_t *data = malloc((size_t)n);
    if (!data) { fclose(f); return 0; }
    const int read_ok = fread(data, 1u, (size_t)n, f) == (size_t)n;
    const int close_ok = fclose(f) == 0;
    if (!read_ok || !close_ok) { free(data); return 0; }
    *out = data; *size = (size_t)n; return 1;
}

int main(int argc, char **argv) {
    if (argc != 2) return 2;
    uint8_t *file = NULL;
    size_t n = 0u;
    if (!load_readonly(argv[1], &file, &n)) return 2;
    const int modern = memcmp(file, "AC1032", 6u) == 0;
    const int legacy = memcmp(file, "AC1018", 6u) == 0;
    if (!modern && !legacy) {
        free(file);
        puts("{\"diagnostic\":\"unsupported_signature\"}");
        return 0;
    }

    MusaDwgAc1018FileMapHeader header = {0};
    const int header_ok = modern ?
        musa_dwg_ac1032_page_map_header(file, n, &header) :
        musa_dwg_ac1018_page_map_header(file, n, &header);
    if (!header_ok) {
        free(file);
        puts("{\"diagnostic\":\"authenticated_header_rejected\"}");
        return 0;
    }

    size_t native_pages = 0u;
    const int locations_ok = modern ?
        musa_dwg_ac1032_page_locations(file, n, NULL, 0u, &native_pages) :
        musa_dwg_ac1018_page_locations(file, n, NULL, 0u, &native_pages);

    int objects_ok = 0, handles_ok = 0;
    if (locations_ok) {
        MusaDwgAc1018SectionDescriptor desc = {0};
        size_t pages = 0u;
        objects_ok = modern ?
            musa_dwg_ac1032_section_pages(
                file, n, MUSA_DWG_AC1032_OBJECTS, NULL, 0u,
                &desc, &pages) :
            musa_dwg_ac1018_section_pages(
                file, n, MUSA_DWG_AC1018_OBJECTS, NULL, 0u,
                &desc, &pages);
        handles_ok = modern ?
            musa_dwg_ac1032_section_pages(
                file, n, MUSA_DWG_AC1032_HANDLES, NULL, 0u,
                &desc, &pages) :
            musa_dwg_ac1018_section_pages(
                file, n, MUSA_DWG_AC1018_HANDLES, NULL, 0u,
                &desc, &pages);
    }

    int decompressed = 0, physical_scan_safe = 0;
    size_t map_consumed = 0u, map_entries = 0u, gaps = 0u, positive = 0u;
    uint64_t physical_end = 256u;
    unsigned ordering_violations = 0u, invalid_spans = 0u;
    int found_map = 0, found_section_map = 0, eof_header_copy = 0;
    const size_t compressed_start = (size_t)header.page_map_offset + 20u;
    if (header.map_decoded_bytes &&
        header.map_decoded_bytes <= MUSA_DIAG_MAX_MAP &&
        header.page_map_offset <= n &&
        compressed_start >= (size_t)header.page_map_offset &&
        compressed_start <= n &&
        header.map_compressed_bytes <= n - compressed_start) {
        uint8_t *plain = malloc(header.map_decoded_bytes);
        if (plain) {
            decompressed = musa_dwg_ac1018_lz_decode(
                file + compressed_start, header.map_compressed_bytes,
                plain, header.map_decoded_bytes, &map_consumed);
            if (decompressed) {
                size_t offset = 0u;
                uint32_t last_id = 0u;
                physical_scan_safe = 1;
                while (offset < header.map_decoded_bytes) {
                    if (header.map_decoded_bytes - offset < 8u) {
                        physical_scan_safe = 0; break;
                    }
                    const uint32_t raw = read32(plain + offset);
                    const uint32_t span = read32(plain + offset + 4u);
                    const int gap = (raw & UINT32_C(0x80000000)) != 0u;
                    if (gap && header.map_decoded_bytes - offset < 24u) {
                        physical_scan_safe = 0; break;
                    }
                    const uint32_t id = gap ?
                        (uint32_t)(-(int64_t)(int32_t)raw) : raw;
                    if (id <= last_id || !id) ordering_violations++;
                    if (!span || physical_end > UINT64_MAX - span)
                        invalid_spans++;
                    if (invalid_spans) { physical_scan_safe = 0; break; }
                    if (id == header.page_map_id && !gap) found_map = 1;
                    if (id == header.section_map_id && !gap)
                        found_section_map = 1;
                    physical_end += span;
                    last_id = id;
                    ++map_entries;
                    if (gap) ++gaps;
                    else ++positive;
                    offset += gap ? 24u : 8u;
                }
            }
            free(plain);
        }
    }
    if (n >= 108u)
        eof_header_copy = memcmp(file + n - 108u, file + 0x80u, 108u) == 0;

    /* Numbers are only allocation/format metadata. No untrusted source
       strings, coordinates, entity bytes or page bodies are emitted. */
    printf("{\"diagnostic\":\"authenticated_header_exploration\","
           "\"version\":\"%s\","
           "\"header_page_count\":%u,"
           "\"native_locations_ok\":%s,"
           "\"native_objects_pages_ok\":%s,"
           "\"native_handles_pages_ok\":%s,"
           "\"map_lz_ok\":%s,"
           "\"map_lz_consumed\":%zu,"
           "\"map_entry_count\":%zu,"
           "\"map_positive_records\":%zu,"
           "\"map_gap_records\":%zu,"
           "\"physical_scan_safe\":%s,"
           "\"page_id_order_violations\":%u,"
           "\"invalid_spans\":%u,"
           "\"contains_page_map_id\":%s,"
           "\"contains_section_map_id\":%s,"
           "\"physical_end_vs_file_end\":%lld,"
           "\"eof_header_copy\":%s,"
           "\"geometry_verified\":false,"
           "\"mutates_source\":false}\n",
           modern ? "AC1032" : "AC1018", header.page_count,
           locations_ok ? "true" : "false",
           objects_ok ? "true" : "false",
           handles_ok ? "true" : "false",
           decompressed ? "true" : "false", map_consumed,
           map_entries, positive, gaps,
           physical_scan_safe ? "true" : "false",
           ordering_violations, invalid_spans,
           found_map ? "true" : "false",
           found_section_map ? "true" : "false",
           (long long)((int64_t)physical_end - (int64_t)n),
           eof_header_copy ? "true" : "false");
    free(file);
    return 0;
}
