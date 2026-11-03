/*
 * MusaCAD first-party, read-only DWG native decoder-stage probe.
 *
 * Reports only verifiable container / handle-index gates for supported
 * versions. Never claims geometry, ownership, color, visibility or DWG edit
 * correctness and never changes the original input.
 */
#include "musacad_dwg_signature.h"
#include "musacad_dwg_ac1015.h"
#include "musacad_dwg_ac1018_file_index.h"
#include "musacad_dwg_ac1018_page_map.h"
#include "musacad_dwg_ac1018_sections.h"
#include "musacad_dwg_ac1018_scene.h"
#include "musacad_dwg_ac1018_handles.h"
#include "musacad_crc16.h"
#include "musacad_dwg_ac1032_page_map.h"
#include "musacad_dwg_ac1032_handles.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_REAL_DWG_BYTES (256u * 1024u * 1024u)

static int load_source(const char *path, uint8_t **out, size_t *out_bytes) {
    FILE *stream = fopen(path, "rb");
    if (!stream) return 0;
    if (fseek(stream, 0, SEEK_END) != 0) {
        fclose(stream); return 0;
    }
    long length = ftell(stream);
    if (length < 6 || (unsigned long)length > MAX_REAL_DWG_BYTES ||
        fseek(stream, 0, SEEK_SET) != 0) {
        fclose(stream); return 0;
    }
    uint8_t *data = (uint8_t *)malloc((size_t)length);
    if (!data) {
        fclose(stream); return 0;
    }
    const size_t n = fread(data, 1u, (size_t)length, stream);
    const int closed = fclose(stream);
    if (n != (size_t)length || closed != 0) {
        free(data); return 0;
    }
    *out = data; *out_bytes = (size_t)length;
    return 1;
}

static const char *signature_name(MusaCoreDwgSignature format) {
    switch (format) {
    case MUSA_DWG_R12: return "AC1009";
    case MUSA_DWG_R13: return "AC1012";
    case MUSA_DWG_R14: return "AC1014";
    case MUSA_DWG_2000: return "AC1015";
    case MUSA_DWG_2004: return "AC1018";
    case MUSA_DWG_2007: return "AC1021";
    case MUSA_DWG_2010: return "AC1024";
    case MUSA_DWG_2013: return "AC1027";
    case MUSA_DWG_2018_2026: return "AC1032";
    default: return "unknown";
    }
}

/*
 * AC1024/R2010 is not yet supported by the original MusaCAD decoder.
 * On a disposable, in-memory copy of the source, explore whether the
 * documented older R2004-family container *shape* matches existing strict
 * native page-map checks. This is NOT a decoder and NEVER accepted evidence.
 * The 6 signature bytes are restored before the caller continues.
 */
/* Diagnostic-only R2004-family MS envelope/CRC: NO type or geometry
   decoding. Offset belongs to an independently CRC-checked handle map.
   Return false for malformed ranges instead of reading out of bounds. */
static int ac1024_candidate_frame_crc(
    const uint8_t *objects, size_t object_bytes, uint64_t offset) {
    if (!objects || offset >= (uint64_t)object_bytes) return 0;
    const size_t at = (size_t)offset;
    const size_t remaining = object_bytes - at;
    if (remaining < 6u) return 0;
    const uint8_t *p = objects + at;
    const uint16_t first = (uint16_t)p[0] | ((uint16_t)p[1] << 8u);
    size_t prefix = 2u, payload = (size_t)first;
    if ((first & 0x8000u) != 0u) {
        const uint16_t hi = (uint16_t)p[2] | ((uint16_t)p[3] << 8u);
        if (!hi || (hi & 0x8000u)) return 0;
        prefix = 4u;
        payload = (size_t)(first & 0x7fffu) | ((size_t)hi << 15u);
    }
    if (payload < 8u || prefix > remaining - 2u ||
        payload > remaining - prefix - 2u) return 0;
    uint16_t computed = 0u;
    if (!musa_core_crc16_arc(
          0xc0c1u, p, prefix + payload, &computed)) return 0;
    const size_t at_crc = prefix + payload;
    const uint16_t stored = (uint16_t)p[at_crc] |
                            ((uint16_t)p[at_crc + 1u] << 8u);
    return computed == stored;
}

typedef struct MusaAc1024CompatibilityHint {
    int header, physical_map, object_pages, handle_pages, handle_index;
    size_t candidate_handles;
    int legacy_frame_probe_completed;
    size_t legacy_frame_crc_ok, legacy_frame_crc_failed;
    size_t raw_type_17_arc, raw_type_18_circle;
    size_t raw_type_19_line, raw_type_77_lwpolyline;
    int continuous_offset_candidate_index;
    size_t continuous_candidate_handles;
    size_t continuous_frame_crc_ok, continuous_frame_crc_failed;
    int page_absolute_candidate_index;
    size_t page_absolute_candidate_handles;
    size_t page_absolute_frame_crc_ok, page_absolute_frame_crc_failed;
} MusaAc1024CompatibilityHint;

static MusaAc1024CompatibilityHint ac1024_family_hint(
    uint8_t *scratch, size_t length) {
    MusaAc1024CompatibilityHint hint = {0};
    if (!scratch || length < 256u || memcmp(scratch, "AC1024", 6u) != 0) {
        return hint;
    }
    /* Re-use existing bounded structural validators only; never copy
       LibreDWG source or include GPL reference-tool headers here. */
    memcpy(scratch, "AC1018", 6u);
    MusaDwgAc1018FileMapHeader header = {0};
    MusaDwgAc1018SectionDescriptor desc = {0};
    size_t locations = 0u, object_pages = 0u, handle_pages = 0u;
    hint.header = musa_dwg_ac1018_page_map_header(scratch, length, &header);
    if (hint.header) {
        hint.physical_map = musa_dwg_ac1018_page_locations(
            scratch, length, NULL, 0u, &locations) && locations > 0u;
    }
    if (hint.physical_map) {
        hint.object_pages = musa_dwg_ac1018_section_pages(
            scratch, length, MUSA_DWG_AC1018_OBJECTS,
            NULL, 0u, &desc, &object_pages) && object_pages > 0u;
    }
    if (hint.object_pages) {
        hint.handle_pages = musa_dwg_ac1018_section_pages(
            scratch, length, MUSA_DWG_AC1018_HANDLES,
            NULL, 0u, &desc, &handle_pages) && handle_pages > 0u;
    }
    if (hint.handle_pages) {
        size_t handles = 0u;
        hint.handle_index = musa_dwg_ac1018_file_handle_index(
            scratch, length, NULL, 0u, &handles) && handles > 0u;
        if (hint.handle_index) hint.candidate_handles = handles;
    }
    /* Separate bounded experiment: try the existing R2004 CRC-framed
       record vocabulary against the authenticated candidate index.
       A numeric BS type is NEVER a decoded AC1024 entity/class/owner.
       This is diagnostic and not a normal-open or rendering path. */
    if (hint.handle_index && hint.candidate_handles <= 400000u &&
        length <= 12u * 1024u * 1024u) {
        MusaDwgAc1018Scene *scene = NULL;
        if (musa_dwg_ac1018_scene_open(scratch, length, &scene) &&
            scene && musa_dwg_ac1018_scene_handle_count(scene) ==
                     hint.candidate_handles) {
            hint.legacy_frame_probe_completed = 1;
            for (size_t i = 0u; i < hint.candidate_handles; i++) {
                MusaDwgAc1018RecordProbe record = {0};
                if (!musa_dwg_ac1018_scene_probe_record_at(scene, i, &record)
                    || !record.record_crc_verified) {
                    hint.legacy_frame_crc_failed++;
                    continue;
                }
                hint.legacy_frame_crc_ok++;
                switch (record.declared_type) {
                case 17u: hint.raw_type_17_arc++; break;
                case 18u: hint.raw_type_18_circle++; break;
                case 19u: hint.raw_type_19_line++; break;
                case 77u: hint.raw_type_77_lwpolyline++; break;
                default: break;
                }
            }
        }
        musa_dwg_ac1018_scene_close(scene);
    }
    /* The 2010 handle map may use continuously advancing logical
       AcDbObjects offsets instead of the legacy R2004 per-map-page
       reset. Compare this ALTERNATE CRC-checked map in isolation.
       Do not choose either profile for the actual application yet. */
    if (hint.handle_index && length <= 12u * 1024u * 1024u &&
        hint.candidate_handles <= 400000u) {
        MusaDwgAc1018SectionDescriptor obj_info = {0}, h_info = {0};
        size_t obj_pages2 = 0u, h_pages2 = 0u;
        if (musa_dwg_ac1018_section_pages(
                scratch, length, MUSA_DWG_AC1018_OBJECTS,
                NULL, 0u, &obj_info, &obj_pages2) &&
            musa_dwg_ac1018_section_pages(
                scratch, length, MUSA_DWG_AC1018_HANDLES,
                NULL, 0u, &h_info, &h_pages2) &&
            obj_pages2 && h_pages2 && obj_info.logical_bytes > 0u &&
            h_info.logical_bytes > 0u &&
            obj_info.logical_bytes <= 96u * 1024u * 1024u &&
            h_info.logical_bytes <= 16u * 1024u * 1024u) {
            const size_t nb = (size_t)obj_info.logical_bytes;
            const size_t hb = (size_t)h_info.logical_bytes;
            uint8_t *objects = (uint8_t *)malloc(nb);
            uint8_t *handles = (uint8_t *)malloc(hb);
            size_t got_obj = 0u, got_h = 0u, candidate_count = 0u;
            if (objects && handles &&
                musa_dwg_ac1018_extract_section(
                    scratch, length, MUSA_DWG_AC1018_OBJECTS,
                    objects, nb, &got_obj) && got_obj == nb &&
                musa_dwg_ac1018_extract_section(
                    scratch, length, MUSA_DWG_AC1018_HANDLES,
                    handles, hb, &got_h) && got_h == hb &&
                musa_dwg_ac1018_handles_index_continuous_offsets(
                    handles, hb, nb, NULL, 0u, &candidate_count) &&
                candidate_count == hint.candidate_handles &&
                candidate_count <= SIZE_MAX / sizeof(MusaDwgAc1018HandleEntry)) {
                MusaDwgAc1018HandleEntry *entries =
                    (MusaDwgAc1018HandleEntry *)malloc(
                        candidate_count * sizeof(*entries));
                size_t actual = 0u;
                if (entries &&
                    musa_dwg_ac1018_handles_index_continuous_offsets(
                        handles, hb, nb, entries, candidate_count, &actual) &&
                    actual == candidate_count) {
                    hint.continuous_offset_candidate_index = 1;
                    hint.continuous_candidate_handles = actual;
                    for (size_t i = 0u; i < actual; i++) {
                        if (ac1024_candidate_frame_crc(
                              objects, nb, entries[i].object_offset))
                            hint.continuous_frame_crc_ok++;
                        else hint.continuous_frame_crc_failed++;
                    }
                }
                free(entries);
            }
            /* Third, distinct CRC-backed handle-map encoding; a missing
               continuous-index result must not be taken as permission to
               invent record positions. */
            size_t absolute_count = 0u;
            if (objects && handles && got_obj == nb && got_h == hb &&
                musa_dwg_ac1018_handles_index_page_absolute_handles(
                    handles, hb, nb, NULL, 0u, &absolute_count) &&
                absolute_count == hint.candidate_handles &&
                absolute_count <= SIZE_MAX / sizeof(MusaDwgAc1018HandleEntry)) {
                MusaDwgAc1018HandleEntry *absolute_entries =
                    (MusaDwgAc1018HandleEntry *)malloc(
                        absolute_count * sizeof(*absolute_entries));
                size_t actual_abs = 0u;
                if (absolute_entries &&
                    musa_dwg_ac1018_handles_index_page_absolute_handles(
                        handles, hb, nb, absolute_entries,
                        absolute_count, &actual_abs) &&
                    actual_abs == absolute_count) {
                    hint.page_absolute_candidate_index = 1;
                    hint.page_absolute_candidate_handles = actual_abs;
                    for (size_t j = 0u; j < actual_abs; j++) {
                        if (ac1024_candidate_frame_crc(
                              objects, nb, absolute_entries[j].object_offset))
                            hint.page_absolute_frame_crc_ok++;
                        else hint.page_absolute_frame_crc_failed++;
                    }
                }
                free(absolute_entries);
            }
            free(handles);
            free(objects);
        }
    }
    memcpy(scratch, "AC1024", 6u);
    return hint;
}

static void report(const char *signature, const char *result,
                   const char *stage, size_t evidence_count,
                   const MusaAc1024CompatibilityHint *hint) {
    printf("{\"signature\":\"%s\",\"result\":\"%s\","
           "\"native_stage\":\"%s\",\"evidence_count\":%zu,"
           "\"geometry_verified\":false,\"rendering_verified\":false,"
           "\"source_modified\":false",
           signature, result, stage, evidence_count);
    if (hint) {
        printf(",\"ac1024_compatibility_hypothesis\":{"
               "\"patched_signature_in_memory_only\":true,"
               "\"validated_original_ac1024_decoder\":false,"
               "\"page_map_header\":%s,\"physical_page_map\":%s,"
               "\"objects_section_pages\":%s,\"handles_section_pages\":%s,"
               "\"candidate_handle_index\":%s,\"candidate_handle_count\":%zu,"
               "\"legacy_frame_probe_completed\":%s,"
               "\"legacy_frame_crc_ok\":%zu,\"legacy_frame_crc_failed\":%zu,"
               "\"legacy_raw_type_17_arc\":%zu,"
               "\"legacy_raw_type_18_circle\":%zu,"
               "\"legacy_raw_type_19_line\":%zu,"
               "\"legacy_raw_type_77_lwpolyline\":%zu,"
               "\"continuous_offset_candidate_index\":%s,"
               "\"continuous_candidate_handles\":%zu,"
               "\"continuous_frame_crc_ok\":%zu,"
               "\"continuous_frame_crc_failed\":%zu,"
               "\"page_absolute_candidate_index\":%s,"
               "\"page_absolute_candidate_handles\":%zu,"
               "\"page_absolute_frame_crc_ok\":%zu,"
               "\"page_absolute_frame_crc_failed\":%zu}",
               hint->header ? "true" : "false",
               hint->physical_map ? "true" : "false",
               hint->object_pages ? "true" : "false",
               hint->handle_pages ? "true" : "false",
               hint->handle_index ? "true" : "false",
               hint->candidate_handles,
               hint->legacy_frame_probe_completed ? "true" : "false",
               hint->legacy_frame_crc_ok,
               hint->legacy_frame_crc_failed,
               hint->raw_type_17_arc,
               hint->raw_type_18_circle,
               hint->raw_type_19_line,
               hint->raw_type_77_lwpolyline,
               hint->continuous_offset_candidate_index ? "true" : "false",
               hint->continuous_candidate_handles,
               hint->continuous_frame_crc_ok,
               hint->continuous_frame_crc_failed,
               hint->page_absolute_candidate_index ? "true" : "false",
               hint->page_absolute_candidate_handles,
               hint->page_absolute_frame_crc_ok,
               hint->page_absolute_frame_crc_failed);
    }
    puts("}");
}

int main(int argc, char **argv) {
    if (argc != 2) {
        fputs("usage: real_dwg_native_stage_probe FILE.dwg\n", stderr);
        return 2;
    }
    uint8_t *file = NULL;
    size_t bytes = 0u;
    if (!load_source(argv[1], &file, &bytes)) {
        fputs("real DWG input is inaccessible, too short or exceeds 256 MiB\n",
              stderr);
        return 2;
    }
    const MusaCoreDwgSignature format =
        musa_core_dwg_identify_signature(file, bytes);
    const char *name = signature_name(format);
    const char *result = "not_covered_by_this_native_probe";
    const char *stage = "signature_only";
    size_t count = 0u;
    MusaAc1024CompatibilityHint hint = {0};
    const MusaAc1024CompatibilityHint *hint_for_report = NULL;

    if (format == MUSA_DWG_2000) {
        MusaDwgAc1015Locators loc = {0};
        if (musa_dwg_ac1015_read_locators(file, bytes, &loc) &&
            loc.crc_verified && loc.region_count > 0u) {
            result = "accepted";
            stage = "ac1015_crc_verified_section_locators";
            count = loc.region_count;
        } else {
            result = "rejected";
            stage = "ac1015_section_locators";
        }
    } else if (format == MUSA_DWG_2004) {
        MusaDwgAc1018FileMapHeader header = {0};
        MusaDwgAc1018SectionDescriptor desc = {0};
        size_t locations = 0u, objects = 0u, handles = 0u;
        result = "rejected";
        if (!musa_dwg_ac1018_page_map_header(file, bytes, &header)) {
            stage = "ac1018_page_map_header";
        } else if (!musa_dwg_ac1018_page_locations(
                       file, bytes, NULL, 0u, &locations) || !locations) {
            stage = "ac1018_physical_page_locations";
        } else if (!musa_dwg_ac1018_section_pages(
                       file, bytes, MUSA_DWG_AC1018_OBJECTS,
                       NULL, 0u, &desc, &objects) || !objects) {
            stage = "ac1018_objects_section_pages";
        } else if (!musa_dwg_ac1018_section_pages(
                       file, bytes, MUSA_DWG_AC1018_HANDLES,
                       NULL, 0u, &desc, &handles) || !handles) {
            stage = "ac1018_handles_section_pages";
        } else if (!musa_dwg_ac1018_file_handle_index(
                       file, bytes, NULL, 0u, &count) || !count) {
            stage = "ac1018_crc_backed_handle_index";
            count = 0u;
        } else {
            result = "accepted";
            stage = "ac1018_crc_backed_handle_index";
        }
    } else if (format == MUSA_DWG_2010) {
        hint = ac1024_family_hint(file, bytes);
        hint_for_report = &hint;
        /* The compatibility hint does not authenticate an AC1024 reader.
           Never expose candidate handles as an accepted entity index. */
        result = "not_covered_by_this_native_probe";
        stage = "ac1024_family_hypothesis_only";
        count = 0u;
    } else if (format == MUSA_DWG_2018_2026) {
        MusaDwgAc1032FileMapHeader header = {0};
        size_t locations = 0u;
        if (!musa_dwg_ac1032_page_map_header(file, bytes, &header)) {
            result = "rejected";
            stage = "ac1032_page_map_header";
        } else if (musa_dwg_ac1032_page_locations(
                       file, bytes, NULL, 0u, &locations) && locations > 0u) {
            result = "accepted";
            stage = "ac1032_physical_page_locations";
            count = locations;
            size_t handles = 0u;
            if (musa_dwg_ac1032_handles_index(
                    file, bytes, NULL, 0u, &handles) && handles > 0u) {
                stage = "ac1032_crc_backed_handle_index";
                count = handles;
            }
        } else {
            result = "rejected";
            stage = "ac1032_physical_page_locations";
        }
    } else if (format == MUSA_DWG_UNKNOWN ||
               format == MUSA_DWG_SHORT_HEADER ||
               format == MUSA_DWG_NON_ASCII) {
        result = "rejected";
        stage = "invalid_signature";
    }
    report(name, result, stage, count, hint_for_report);
    free(file);
    return 0;
}
