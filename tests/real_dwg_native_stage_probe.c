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
#include "musacad_dwg_ac1018_scene.h"
#include "musacad_dwg_ac1018_page_map.h"
#include "musacad_dwg_ac1024_section_envelope.h"
#include "musacad_dwg_ac1018_sections.h"
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

static void report(const char *signature, const char *result,
                   const char *stage, size_t evidence_count,
                   int r2010_envelope_crc, int r2010_strict_lz,
                   int r2010_dir_candidate, size_t r2010_dir_refs,
                   int r2010_links_candidate,size_t r2010_link_count,
                   size_t r2010_headers,size_t r2010_payloads,
                   size_t r2010_header_crc,
                   size_t r2010_lz_eligible,size_t r2010_lz_verified,
                   int handles_join,size_t handles_logical_bytes,
                   int handles_reason,int handles_inner_reason,
                   int objects_join,size_t objects_bytes,
                   size_t objects_candidate_bytes,int objects_inner_reason,
                   int handles_crc_pages,size_t handles_crc_data_pages,
                   size_t handles_crc_payload_bytes,
                   size_t candidate_profiles,size_t candidate_pairs,
                   size_t self_crc_prefix,size_t numeric_matches,
                   int profile,int full_self,int full_numeric) {
    printf("{\"signature\":\"%s\",\"result\":\"%s\","
           "\"native_stage\":\"%s\",\"evidence_count\":%zu,"
           "\"geometry_verified\":false,\"rendering_verified\":false,"
           "\"source_modified\":false,"
           "\"ac1024_section_map_crc_verified\":%s,"
           "\"ac1024_section_map_lz_completed\":%s,"
           "\"ac1024_directory_shape_candidate\":%s,"
           "\"ac1024_directory_page_refs_candidate\":%zu,"
           "\"ac1024_directory_physical_links_candidate\":%s,"
           "\"ac1024_directory_matched_physical_refs_candidate\":%zu,"
           "\"ac1024_data_masked_headers_verified\":%zu,"
           "\"ac1024_data_payload_adler_verified\":%zu,"
           "\"ac1024_data_header_adler_verified\":%zu,"
           "\"ac1024_data_lz_eligible_pages\":%zu,"
           "\"ac1024_data_lz_verified\":%zu,"
           "\"ac1024_handles_join_verified\":%s,"
           "\"ac1024_handles_join_logical_bytes\":%zu,"
           "\"ac1024_handles_join_probe_reason\":%d,"
           "\"ac1024_handles_join_inner_failure_stage\":%d,"
           "\"ac1024_objects_join_verified\":%s,"
           "\"ac1024_objects_join_logical_bytes\":%zu,"
           "\"ac1024_objects_candidate_logical_bytes\":%zu,"
           "\"ac1024_objects_join_inner_failure_stage\":%d,"
           "\"ac1024_handles_crc_frames_verified\":%s,"
           "\"ac1024_handles_crc_data_pages\":%zu,"
           "\"ac1024_handles_crc_page_data_bytes\":%zu,"
           "\"ac1024_handle_candidate_profiles\":%zu,"
           "\"ac1024_handle_candidate_pairs\":%zu,"
           "\"ac1024_handle_crc_self_prefix_pairs\":%zu,"
           "\"ac1024_handle_candidate_numeric_matches\":%zu,"
           "\"ac1024_handle_candidate_profile\":%d,"
           "\"ac1024_handle_complete_self_index_verified\":%s,"
           "\"ac1024_handle_complete_numeric_agreement\":%s}\n",
           signature, result, stage, evidence_count,
           r2010_envelope_crc ? "true" : "false",
           r2010_strict_lz ? "true" : "false",
           r2010_dir_candidate ? "true" : "false",r2010_dir_refs,
           r2010_links_candidate ? "true" : "false",r2010_link_count,
           r2010_headers,r2010_payloads,r2010_header_crc,
           r2010_lz_eligible,r2010_lz_verified,
           handles_join?"true":"false",handles_logical_bytes,
           handles_reason,handles_inner_reason,
           objects_join?"true":"false",objects_bytes,
           objects_candidate_bytes,objects_inner_reason,
           handles_crc_pages?"true":"false",handles_crc_data_pages,
           handles_crc_payload_bytes,
           candidate_profiles,candidate_pairs,self_crc_prefix,numeric_matches,
           profile,full_self?"true":"false",full_numeric?"true":"false");
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
    int ac1024_section_crc=0,ac1024_section_lz=0;
    int ac1024_directory_candidate=0;
    size_t ac1024_directory_refs=0u;
    int ac1024_links_candidate=0;
    size_t ac1024_links_matched=0u;
    size_t ac1024_masked_headers=0u,ac1024_payloads=0u,ac1024_header_crc=0u;
    size_t ac1024_lz_eligible=0u,ac1024_lz_verified=0u;
    int ac1024_handles_join=0;
    size_t ac1024_handles_bytes=0u;
    int ac1024_handles_reason=0;
    int ac1024_handles_inner_reason=0;
    int ac1024_objects_join=0;
    size_t ac1024_objects_bytes=0u,ac1024_objects_candidate_bytes=0u;
    int ac1024_objects_inner_reason=0;
    int ac1024_handles_crc_pages=0;
    size_t ac1024_handles_crc_data_pages=0u;
    size_t ac1024_handles_crc_payload_bytes=0u;
    size_t ac1024_handle_candidate_profiles=0u;
    size_t ac1024_handle_candidate_pairs=0u;
    size_t ac1024_handle_crc_self_prefix_pairs=0u;
    size_t ac1024_handle_candidate_numeric_matches=0u;
    int ac1024_handle_candidate_profile=0;
    int ac1024_handle_complete_self_index_verified=0;
    int ac1024_handle_complete_numeric_agreement=0;

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
            /* A verified handle index is not proof that all indexed object
             * MS record frames, CRCs and physical spans are consistent.
             * Close the temporary native scene on every branch. */
            MusaDwgAc1018Scene *scene = NULL;
            MusaDwgAc1018RecordRangeAudit ranges = {0};
            if (!musa_dwg_ac1018_scene_open(file, bytes, &scene)) {
                stage = "ac1018_native_scene_open";
            } else if (!musa_dwg_ac1018_scene_audit_record_ranges(
                           scene, &ranges) ||
                       !ranges.ranges_nonoverlapping ||
                       ranges.indexed_records != count ||
                       !ranges.indexed_records) {
                stage = "ac1018_record_ranges_and_crcs";
            } else {
                result = "accepted";
                stage = "ac1018_record_ranges_and_crcs";
                count = ranges.indexed_records;
            }
            musa_dwg_ac1018_scene_close(scene);
        }

    } else if (format == MUSA_DWG_2010) {
        /* First-party R2010 original protected physical map stage ONLY.
         * Do not reuse R2004 object/scene/entity semantics. */
        MusaDwgAc1018FileMapHeader header = {0};
        size_t locations = 0u;
        result = "rejected";
        if (!musa_dwg_ac1024_page_map_header(file,bytes,&header)) {
            stage = "ac1024_page_map_header";
        } else if (!musa_dwg_ac1024_page_locations(
                       file,bytes,NULL,0u,&locations) || !locations) {
            stage = "ac1024_physical_page_locations";
        } else {
            result = "accepted";
            stage = "ac1024_physical_page_locations";
            count = locations;
            MusaDwgAc1024SectionEnvelopeEvidence envelope={0};
            if(musa_dwg_ac1024_section_envelope_probe(
                   file,bytes,&envelope)){
                ac1024_section_crc=envelope.adler_verified!=0u;
                ac1024_section_lz=envelope.strict_lz_completed!=0u;
                ac1024_directory_candidate=
                    envelope.directory_shape_candidate!=0u;
                ac1024_directory_refs=
                    envelope.directory_referenced_pages_candidate;
                ac1024_links_candidate=
                    envelope.directory_physical_links_candidate!=0u;
                ac1024_links_matched=
                    envelope.directory_matched_physical_refs_candidate;
                ac1024_masked_headers=
                    envelope.source_data_masked_headers_verified;
                ac1024_payloads=
                    envelope.source_data_payload_adler_verified;
                ac1024_header_crc=
                    envelope.source_data_header_adler_verified;
                ac1024_lz_eligible=
                    envelope.source_data_lz_eligible_pages;
                ac1024_lz_verified=
                    envelope.source_data_lz_verified;
                ac1024_handles_join=
                    envelope.source_handles_logical_join_verified!=0u;
                ac1024_handles_bytes=
                    envelope.source_handles_logical_bytes;
                ac1024_handles_inner_reason=
                    (int)envelope.source_handles_join_inner_failure_stage;
                ac1024_handles_crc_pages=
                    envelope.source_handles_crc_frames_verified!=0u;
                ac1024_handles_crc_data_pages=
                    envelope.source_handles_crc_data_pages;
                ac1024_handles_crc_payload_bytes=
                    envelope.source_handles_crc_page_data_bytes;
                ac1024_handle_candidate_profiles=
                    envelope.source_handle_candidate_profiles;
                ac1024_handle_candidate_pairs=
                    envelope.source_handle_candidate_pairs;
                ac1024_handle_crc_self_prefix_pairs=
                    envelope.source_handle_crc_self_prefix_pairs;
                ac1024_handle_candidate_numeric_matches=
                    envelope.source_handle_numeric_matches;
                ac1024_handle_candidate_profile=
                    (int)envelope.source_handle_candidate_profile;
                ac1024_handle_complete_self_index_verified=
                    envelope.source_handle_complete_self_index_verified!=0u;
                ac1024_handle_complete_numeric_agreement=
                    envelope.source_handle_complete_numeric_agreement!=0u;
                ac1024_objects_join=
                    envelope.source_objects_logical_join_verified!=0u;
                ac1024_objects_bytes=
                    envelope.source_objects_logical_bytes;
                ac1024_objects_candidate_bytes=
                    envelope.source_objects_candidate_logical_bytes;
                ac1024_objects_inner_reason=
                    (int)envelope.source_objects_join_inner_failure_stage;
                if(!ac1024_handles_join&&envelope.directory_shape_candidate){
                    if(!envelope.source_handles_name_candidates)
                        ac1024_handles_reason=1;
                    else if(envelope.source_directory_duplicate_names)
                        ac1024_handles_reason=2;
                    else if(envelope.source_handles_candidate_compression!=2u)
                        ac1024_handles_reason=3;
                    else if(envelope.source_handles_candidate_encryption!=0u)
                        ac1024_handles_reason=4;
                    else if(!envelope.source_handles_candidate_logical_bytes||
                            envelope.source_handles_candidate_logical_bytes>
                                16u*1024u*1024u)
                        ac1024_handles_reason=5;
                    else if(!envelope.source_handles_candidate_pages)
                        ac1024_handles_reason=7;
                    else if(envelope.source_handles_logical_offset_mismatch_pages)
                        ac1024_handles_reason=12;
                    else if(envelope.source_handles_candidate_page_bytes>1048576u)
                        ac1024_handles_reason=13;
                    else if(envelope.source_handles_lz_eligible_pages<
                            envelope.source_handles_candidate_pages)
                        ac1024_handles_reason=14;
                    else if(envelope.source_handles_lz_verified_pages<
                            envelope.source_handles_candidate_pages)
                        ac1024_handles_reason=15;
                    else if(!envelope.source_handles_candidate_page_bytes){
                        const uint32_t logical=
                            envelope.source_handles_candidate_logical_bytes;
                        ac1024_handles_reason=
                            ((logical-1u)/29696u+1u!=
                             envelope.source_handles_candidate_pages)?8:11;
                    }else{
                        const uint32_t logical=
                            envelope.source_handles_candidate_logical_bytes;
                        const uint32_t page=
                            envelope.source_handles_candidate_page_bytes;
                        ac1024_handles_reason=
                            ((logical-1u)/page+1u!=
                             envelope.source_handles_candidate_pages)?9:10;
                    }
                }
            }
        }
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
    report(name, result, stage, count,
           ac1024_section_crc, ac1024_section_lz,
           ac1024_directory_candidate,ac1024_directory_refs,
           ac1024_links_candidate,ac1024_links_matched,
           ac1024_masked_headers,ac1024_payloads,ac1024_header_crc,
           ac1024_lz_eligible,ac1024_lz_verified,
           ac1024_handles_join,ac1024_handles_bytes,
           ac1024_handles_reason,ac1024_handles_inner_reason,
           ac1024_objects_join,ac1024_objects_bytes,
           ac1024_objects_candidate_bytes,ac1024_objects_inner_reason,
           ac1024_handles_crc_pages,ac1024_handles_crc_data_pages,
           ac1024_handles_crc_payload_bytes,
           ac1024_handle_candidate_profiles,ac1024_handle_candidate_pairs,
           ac1024_handle_crc_self_prefix_pairs,
           ac1024_handle_candidate_numeric_matches,
           ac1024_handle_candidate_profile,
           ac1024_handle_complete_self_index_verified,
           ac1024_handle_complete_numeric_agreement);
    free(file);
    return 0;
}
