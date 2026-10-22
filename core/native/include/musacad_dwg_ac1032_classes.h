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
#ifdef __cplusplus
}
#endif
#endif
