/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef MUSACAD_DWG_AC1032_CLASS_RESOLUTION_H
#define MUSACAD_DWG_AC1032_CLASS_RESOLUTION_H
#include "musacad_dwg_ac1032_classes.h"
#include <stddef.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
/* R2018 object type 500+ is a custom AcDb class number, but type
 * codes <500 require separate BUILTIN decoders and may NOT be named
 * by a guessed string. Missing custom class is a hard failure.
 * These APIs do NOT decode owner graphs, geometry or proxy contents. */
typedef struct MusaDwgAc1032ClassResolution {
    uint64_t indexed_handle;
    uint16_t raw_object_type;
    uint16_t class_number; /* 0 for builtin/unknown semantics */
    uint16_t item_class_id; /* 0 unknown, 0x1f2 entity, 0x1f3 object */
    uint16_t proxy_flags;
    uint16_t dwg_version,maintenance_version;
    uint8_t is_custom_class;
    uint8_t was_proxy;
    uint8_t original_record_crc_verified;
    uint8_t source_class_metadata_verified;
    char application[MUSA_AC1032_CLASS_NAME_UTF8];
    char cpp_class[MUSA_AC1032_CLASS_NAME_UTF8];
    char dxf_name[MUSA_AC1032_CLASS_NAME_UTF8];
} MusaDwgAc1032ClassResolution;

/* Pure C11 joining policy for ALREADY independently source-verified
 * numeric class names and class IDs; no file reads or output mutation
 * on invalid/missing/duplicate class numbers. Output remains unchanged
 * on error. The caller is responsible for authenticating input arrays. */
int musa_dwg_ac1032_class_join(
    uint16_t original_type,uint64_t indexed_handle,
    const MusaDwgAc1032ClassNumeric *numeric,
    const MusaDwgAc1032ClassNames *names,size_t class_count,
    MusaDwgAc1032ClassResolution *out);

/* Original full AC1032 source proof: decoded indexed object OT + CRC,
 * physical checksum/certified classes numeric and Unicode names.
 * Fails closed on missing custom class. Builtin types are reported as
 * raw numeric only and NOT assumed fully supported. */
int musa_dwg_ac1032_resolve_object_class_file(
    const uint8_t *file,size_t file_bytes,uint64_t handle,
    MusaDwgAc1032ClassResolution *out);

/* Independently validate/look up a registered custom AcDb class ID
 * from original source, even if no object of that class is present.
 * Source provenance is recorded separately from real-object CRC. */
int musa_dwg_ac1032_resolve_declared_class_file(
    const uint8_t *file,size_t file_bytes,uint16_t class_number,
    MusaDwgAc1032ClassResolution *out);
#ifdef __cplusplus
}
#endif
#endif
