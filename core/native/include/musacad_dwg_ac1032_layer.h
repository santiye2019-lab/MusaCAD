/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef MUSACAD_DWG_AC1032_LAYER_H
#define MUSACAD_DWG_AC1032_LAYER_H
#include "musacad_dwg_ac1032_object_streams.h"
#include <stddef.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
/* Original R2018 OT51 LAYER identity, NOT yet its style definition.
 * Source must already be independently H-indexed, checksum-authenticated
 * and partitioned into bounded DATA and HANDLES.  No presumed color or
 * layer visibility is ever supplied by this structure.
 * Shared first-party C11 across Windows, iOS and Android. */
typedef struct MusaDwgAc1032LayerIdentity {
    uint64_t indexed_handle;
    size_t data_fields_start_bit;
    uint16_t object_type;
    uint8_t original_record_crc_verified;
    uint8_t embedded_self_handle_verified;
    uint8_t empty_eed_verified;
    uint8_t no_reactors_verified;
    uint8_t xdictionary_missing;
} MusaDwgAc1032LayerIdentity;
/* Strict no-EED/no-reactors/missing-XDictionary subset. All outputs
 * fail-atomic and source unchanged, even if bitstream is corrupt. */
int musa_dwg_ac1032_layer_ot51_identity_at(
    const uint8_t *payload,size_t bytes,
    const MusaDwgAc1032ObjectStreams *layout,
    MusaDwgAc1032LayerIdentity *out);
/* R2018 LAYER name: UTF-16LE TU in the separately length-framed,
 * bit-aligned object string stream immediately preceding H records.
 * Validated source record and strict LAYER identity are required.
 * Unicode BMP and valid surrogate pairs become UTF-8; unsupported
 * names/resource limits reject without changing caller-owned output.
 * No BYLAYER color, freeze, off, or lock claim. */
typedef struct MusaDwgAc1032LayerName {
    uint64_t indexed_handle;
    size_t original_string_start_bit;
    uint32_t utf16_code_units;
    uint32_t utf8_bytes;
    uint8_t original_record_crc_verified;
    char utf8[512];
} MusaDwgAc1032LayerName;
int musa_dwg_ac1032_layer_ot51_name_at(
    const uint8_t *payload,size_t bytes,
    const MusaDwgAc1032ObjectStreams *layout,
    MusaDwgAc1032LayerName *out);
#ifdef __cplusplus
}
#endif
#endif
