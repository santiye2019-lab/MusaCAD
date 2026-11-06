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

/* Diagnostic checksum for an already authenticated SYSTEM page. This
 * reports whether it matches the strict existing decoder; it never
 * bypasses a failed checksum or admits the file for rendering. */
static uint32_t diag_adler(uint32_t seed,const uint8_t *p,size_t n) {
    uint64_t low=seed&65535u,high=seed>>16u;
    size_t at=0u;
    while(at<n) {
        const size_t chunk=n-at<0x15b0u?n-at:0x15b0u;
        for(size_t i=0u;i<chunk;i++) {low+=p[at+i];high+=low;}
        low%=65521u;high%=65521u;at+=chunk;
    }
    return ((uint32_t)high<<16u)|(uint32_t)low;
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
    unsigned positive_order_violations = 0u, gap_order_violations = 0u;
    /* Source page IDs are authenticated by the compressed physical map,
     * but their claimed namespace may be inconsistent. Numeric evidence
     * only; never emit actual source IDs or raw file content. */
    size_t positive_id_duplicates = 0u, gap_id_duplicates = 0u;
    size_t positive_gap_id_collisions = 0u, ids_above_admission_cap = 0u;
    int page_id_namespace_audit_complete = 0;
    uint32_t previous_positive_id = 0u, previous_gap_id = 0u;
    int last_entry_is_map = 0, last_entry_is_gap = 0;
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
                /* The first-party reader rejects IDs >1,048,576.
                 * Bounded 1 MiB scratch classifies ID reuse without
                 * quadratic scans on large untrusted CAD files. */
                uint8_t *seen = modern ?
                    calloc(UINT32_C(1048576)+1u,1u) : NULL;
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
                    if (modern && seen) {
                        if(id>UINT32_C(1048576)){
                            ++ids_above_admission_cap;
                        } else {
                            const uint8_t mark=gap?2u:1u;
                            const uint8_t prior=seen[id];
                            if(prior&mark){
                                if(gap)++gap_id_duplicates;
                                else ++positive_id_duplicates;
                            }else if(prior && !(prior&mark)){
                                ++positive_gap_id_collisions;
                            }
                            seen[id]=(uint8_t)(prior|mark);
                        }
                    }
                    if (id <= last_id || !id) ordering_violations++;
                    if(gap) {
                        if (id <= previous_gap_id || !id) gap_order_violations++;
                        previous_gap_id = id;
                    } else {
                        if (id <= previous_positive_id || !id)
                            positive_order_violations++;
                        previous_positive_id = id;
                    }
                    last_entry_is_map = !gap && id == header.page_map_id;
                    last_entry_is_gap = gap;
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
                page_id_namespace_audit_complete=
                    modern && seen && physical_scan_safe &&
                    offset==header.map_decoded_bytes;
                free(seen);
            }
            free(plain);
        }
    }
    if (n >= 108u)
        eof_header_copy = memcmp(file + n - 108u, file + 0x80u, 108u) == 0;
    /* Diagnostic only: map allocation vs actual source EOF header copy.
     * No guessed footer, geometry or source-byte content is exported. */
    size_t r2010_mapped_eof_header_overlap_bytes = 0u;
    long long r2010_map_body_vs_eof_header_start = 0LL;
    int r2010_empty_system_marker_after_map_body = 0;
    if (r2010 && n >= 108u && eof_header_copy) {
        const uint64_t copy_start=(uint64_t)n-108u;
        const uint64_t body_end=header.page_map_offset+20u+
                                (uint64_t)header.map_compressed_bytes;
        if (physical_end>copy_start && physical_end<=(uint64_t)n)
            r2010_mapped_eof_header_overlap_bytes=
                (size_t)(physical_end-copy_start);
        if (body_end<=(uint64_t)n) {
            r2010_map_body_vs_eof_header_start=
                (long long)((int64_t)body_end-(int64_t)copy_start);
            if ((uint64_t)n-body_end>=20u) {
                static const uint8_t empty_system[20]={
                    0x3bu,0x0eu,0x63u,0x41u,0u,0u,0u,0u,
                    0u,0u,0u,0u,2u,0u,0u,0u,0u,0u,0u,0u
                };
                r2010_empty_system_marker_after_map_body=
                    memcmp(file+(size_t)body_end,empty_system,
                           sizeof(empty_system))==0;
            }
        }
    }

    int system_found=0, system_envelope_ok=0;
    int system_adler_ok=0, system_lz_ok=0;
    int system_lz_trace_attempted=0;
    MusaDwgAc1018LzDiagnostic system_lz_trace={0};
    /* A diagnostic-only alternate output budget MUST NOT be promoted
     * to accepted DWG content; the authenticated advertised size remains
     * the sole admission limit for all production decoders. */
    int system_lz_alt_attempted=0, system_lz_alt_completed=0;
    size_t system_lz_compressed_budget=0u;
    size_t system_lz_alt_extra=0u;
    MusaDwgAc1018LzDiagnostic system_lz_alt_trace={0};
    uint32_t system_kind=0u,system_codec=0u,system_expanded=0u;
    uint32_t system_section_count=0u;
    if(locations_ok && native_pages && native_pages <= 131072u) {
        MusaDwgAc1018PageLocation *pages=
            calloc(native_pages,sizeof(*pages));
        if(pages) {
            size_t actual_pages=0u;
            const int ok=modern?
                musa_dwg_ac1032_page_locations(
                    file,n,pages,native_pages,&actual_pages):
                musa_dwg_ac1018_page_locations(
                    file,n,pages,native_pages,&actual_pages);
            if(ok && actual_pages==native_pages) {
                for(size_t i=0u;i<native_pages;i++) {
                    if(pages[i].page_id!=header.section_map_id)continue;
                    system_found=1;
                    const size_t pos=(size_t)pages[i].file_offset;
                    if(pages[i].file_offset>n || n-pos<20u ||
                       pages[i].file_span_bytes<20u)break;
                    const uint8_t *sys=file+pos;
                    system_kind=read32(sys);
                    system_expanded=read32(sys+4u);
                    const uint32_t packed=read32(sys+8u);
                    system_codec=read32(sys+12u);
                    const uint32_t stored=read32(sys+16u);
                    if(system_kind!=UINT32_C(0x4163003b)||
                       system_codec!=2u||!packed||
                       !system_expanded||
                       system_expanded>MUSA_DIAG_MAX_MAP||
                       packed>n-pos-20u||
                       packed>pages[i].file_span_bytes-20u)break;
                    system_envelope_ok=1;
                    uint8_t clean[20];
                    memcpy(clean,sys,20u);
                    memset(clean+16u,0,4u);
                    system_adler_ok=
                        diag_adler(diag_adler(0u,clean,20u),
                                   sys+20u,packed)==stored;
                    if(!system_adler_ok)break;
                    uint8_t *buf=malloc(system_expanded);
                    if(!buf)break;
                    size_t used=0u;
                    system_lz_trace_attempted=1;
                    system_lz_compressed_budget=packed;
                    system_lz_ok=musa_dwg_ac1018_lz_diagnose(
                        sys+20u,packed,system_expanded,&system_lz_trace);
                    if(system_lz_ok)
                        system_lz_ok=musa_dwg_ac1018_lz_decode(
                            sys+20u,packed,buf,system_expanded,&used);
                    if(legacy && !system_lz_ok &&
                       system_lz_trace.failure_phase==
                           MUSA_DWG_AC1018_LZ_PHASE_OUTPUT_OVERRUN &&
                       system_lz_trace.overflow_match_bytes >
                           system_lz_trace.overflow_available_bytes) {
                        const size_t delta=
                            system_lz_trace.overflow_match_bytes -
                            system_lz_trace.overflow_available_bytes;
                        /* Read-only hypothesis: does the exact same strict
                         * grammar terminate if the declared output budget
                         * were higher by precisely the failing match delta?
                         * Never mutate the real header or production open
                         * gate. Refuse unbounded candidate expansions. */
                        if(delta<=256u &&
                           system_expanded<=MUSA_DIAG_MAX_MAP-delta) {
                            system_lz_alt_attempted=1;
                            system_lz_alt_extra=delta;
                            system_lz_alt_completed=
                                musa_dwg_ac1018_lz_diagnose(
                                    sys+20u,packed,
                                    (size_t)system_expanded+delta,
                                    &system_lz_alt_trace);
                        }
                    }
                    if(system_lz_ok && system_expanded>=4u)
                        system_section_count=read32(buf);
                    free(buf);
                    break;
                }
            }
            free(pages);
        }
    }
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
           "\"page_id_namespace_audit_complete\":%s,"
           "\"page_id_duplicate_positive\":%zu,"
           "\"page_id_duplicate_gap\":%zu,"
           "\"page_id_cross_role_collisions\":%zu,"
           "\"page_ids_above_admission_cap\":%zu,"
           "\"page_id_order_violations\":%u,"
           "\"positive_order_violations\":%u,"
           "\"gap_order_violations\":%u,"
           "\"final_record_is_page_map\":%s,"
           "\"final_record_is_gap\":%s,"
           "\"invalid_spans\":%u,"
           "\"contains_page_map_id\":%s,"
           "\"contains_section_map_id\":%s,"
           "\"physical_end_vs_file_end\":%lld,"
           "\"r2010_mapped_eof_header_overlap_bytes\":%zu,"
           "\"r2010_map_body_vs_eof_header_start\":%lld,"
           "\"r2010_empty_system_marker_after_map_body\":%s,"
           "\"eof_header_copy\":%s,"
           "\"system_section_map_found\":%s,"
           "\"system_envelope_ok\":%s,"
           "\"system_adler_ok\":%s,"
           "\"system_lz_ok\":%s,"
           "\"system_lz_trace_attempted\":%s,"
           "\"system_lz_failure_phase\":%u,"
           "\"system_lz_input_consumed\":%zu,"
           "\"system_lz_output_produced\":%zu,"
           "\"system_lz_overflow_match_bytes\":%zu,"
           "\"system_lz_overflow_available_bytes\":%zu,"
           "\"system_lz_alt_attempted\":%s,"
           "\"system_lz_alt_extra_bytes\":%zu,"
           "\"system_lz_alt_completed\":%s,"
           "\"system_lz_alt_failure_phase\":%u,"
           "\"system_lz_alt_input_consumed\":%zu,"
           "\"system_lz_alt_output_produced\":%zu,"
           "\"system_lz_compressed_budget\":%zu,"
           "\"system_lz_literal_requested\":%zu,"
           "\"system_lz_literal_output_available\":%zu,"
           "\"system_lz_literal_input_available\":%zu,"
           "\"system_lz_alt_literal_requested\":%zu,"
           "\"system_lz_alt_literal_output_available\":%zu,"
           "\"system_lz_alt_literal_input_available\":%zu,"
           "\"system_descriptor_count\":%u,"
           "\"system_kind_code\":%u,"
           "\"system_codec\":%u,"
           "\"system_uncompressed_bytes\":%u,"
           "\"geometry_verified\":false,"
           "\"mutates_source\":false}\n",
           modern ? "AC1032" : "AC1018", header.page_count,
           locations_ok ? "true" : "false",
           objects_ok ? "true" : "false",
           handles_ok ? "true" : "false",
           decompressed ? "true" : "false", map_consumed,
           map_entries, positive, gaps,
           physical_scan_safe ? "true" : "false",
           page_id_namespace_audit_complete ? "true" : "false",
           positive_id_duplicates,
           gap_id_duplicates,
           positive_gap_id_collisions,
           ids_above_admission_cap,
           ordering_violations,
           positive_order_violations,gap_order_violations,
           last_entry_is_map ? "true" : "false",
           last_entry_is_gap ? "true" : "false",invalid_spans,
           found_map ? "true" : "false",
           found_section_map ? "true" : "false",
           (long long)((int64_t)physical_end - (int64_t)n),
           r2010_mapped_eof_header_overlap_bytes,
           r2010_map_body_vs_eof_header_start,
           r2010_empty_system_marker_after_map_body ? "true" : "false",
           eof_header_copy ? "true" : "false",
           system_found ? "true" : "false",
           system_envelope_ok ? "true" : "false",
           system_adler_ok ? "true" : "false",
           system_lz_ok ? "true" : "false",
           system_lz_trace_attempted ? "true" : "false",
           (unsigned)system_lz_trace.failure_phase,
           system_lz_trace.input_consumed,
           system_lz_trace.output_produced,
           system_lz_trace.overflow_match_bytes,
           system_lz_trace.overflow_available_bytes,
           system_lz_alt_attempted ? "true" : "false",
           system_lz_alt_extra,
           system_lz_alt_completed ? "true" : "false",
           (unsigned)system_lz_alt_trace.failure_phase,
           system_lz_alt_trace.input_consumed,
           system_lz_alt_trace.output_produced,
           system_lz_compressed_budget,
           system_lz_trace.literal_requested_bytes,
           system_lz_trace.literal_output_available_bytes,
           system_lz_trace.literal_input_available_bytes,
           system_lz_alt_trace.literal_requested_bytes,
           system_lz_alt_trace.literal_output_available_bytes,
           system_lz_alt_trace.literal_input_available_bytes,
           system_section_count,system_kind,system_codec,
           system_expanded);
    free(file);
    return 0;
}
