#ifndef MUSACAD_DWG_AC1032_CLASSES_H
#define MUSACAD_DWG_AC1032_CLASSES_H
#include <stddef.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
/* Read-only AC1032 Classes framing proof. Validates physical section,
 * original sentinel and data boundaries, NOT full class names or geometry.
 * Same independently written C11 engine on Android, Windows and iOS. */
typedef struct MusaDwgAc1032ClassesFrame {
    uint32_t class_area_bytes;
    uint32_t total_data_bits;
    uint64_t section_logical_bytes;
    uint32_t section_id;
    uint8_t original_sentinel_verified;
    uint8_t page_crcs_verified;
} MusaDwgAc1032ClassesFrame;
int musa_dwg_ac1032_classes_frame_probe(
    const uint8_t *file,size_t bytes,
    MusaDwgAc1032ClassesFrame *out);
/* Incremental real AC1032 CLASS IDENTITIES (not names or full object map).
 * Decodes DWG-specific MSB-first BL/BS values from the verified class
 * data bitstream, not the internal LSB-first MusaBits utility.
 * Class index ordering is preserved; does not infer class names or
 * mark any class as a renderable entity. Fail-atomic on all errors. */
typedef struct MusaDwgAc1032ClassIdentityPrefix {
    uint32_t max_class_number;
    uint16_t first_class_number;
    uint16_t first_proxy_flags;
    uint8_t header_boolean;
    uint8_t source_crc_verified;
} MusaDwgAc1032ClassIdentityPrefix;
int musa_dwg_ac1032_classes_identity_prefix(
    const uint8_t *file,size_t bytes,
    MusaDwgAc1032ClassIdentityPrefix *out);

/* Independently decode ALL NUMERIC class-record metadata (not class
 * names; the R2007+ TU strings live in a separate Unicode stream).
 * Ordered by original source class index; class numbers and declared
 * type 0x1f2 entity / 0x1f3 object are verified. No class contents or
 * AcDbObject layout are interpreted, no DWG reserialization.
 * On error, output records and out_count are unchanged. Query counts
 * with records=NULL, capacity=0; all source bytes remain unchanged. */
typedef struct MusaDwgAc1032ClassNumeric {
    uint16_t class_number,proxy_flags,item_class_id;
    uint16_t dwg_version,maintenance_version;
    uint8_t was_proxy;
    uint32_t object_instance_count;
    uint32_t unknown_number1,unknown_number2;
} MusaDwgAc1032ClassNumeric;
int musa_dwg_ac1032_classes_numeric_records(
    const uint8_t *file,size_t file_bytes,
    MusaDwgAc1032ClassNumeric *records,size_t capacity,
    size_t *out_count);
/* Original AC1032 class-section Unicode stream positions are BIT offsets. */
typedef struct MusaDwgAc1032ClassTextRegion {
    uint64_t start_bit;
    uint64_t length_bits;
    uint64_t flag_bit;
    uint8_t present;
    uint8_t source_verified;
} MusaDwgAc1032ClassTextRegion;
int musa_dwg_ac1032_classes_text_region(const uint8_t *file,
    size_t file_bytes,MusaDwgAc1032ClassTextRegion *out);


/* Source-backed R2018 class name triplets. Each returned string is UTF-8
 * converted from the verified DWG TU UCS-2/UTF-16 string bit stream.
 * Indices match musa_dwg_ac1032_classes_numeric_records in file order.
 * Original class/object semantics are not inferred from spelling.
 * The fixed per-name limit fails CLOSED on oversize/invalid sequences.
 * Query with entries=NULL/capacity=0; no partial output on error. */
#define MUSA_AC1032_CLASS_NAME_UTF8 256u
typedef struct MusaDwgAc1032ClassNames {
    uint16_t class_number;
    char application[MUSA_AC1032_CLASS_NAME_UTF8];
    char cpp_class[MUSA_AC1032_CLASS_NAME_UTF8];
    char dxf_name[MUSA_AC1032_CLASS_NAME_UTF8];
} MusaDwgAc1032ClassNames;
int musa_dwg_ac1032_classes_names(
    const uint8_t *file,size_t file_bytes,
    MusaDwgAc1032ClassNames *entries,size_t capacity,
    size_t *out_count);

#ifdef __cplusplus
}
#endif
#endif
