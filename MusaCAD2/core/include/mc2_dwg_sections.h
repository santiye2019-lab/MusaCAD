#ifndef MC2_DWG_SECTIONS_H
#define MC2_DWG_SECTIONS_H
#include <stddef.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
/* Only the legacy AC1015 section table is implemented here.
 * AC1032 has a different layout and MUST be rejected by this parser. */
typedef struct {
    uint8_t section_id;
    uint32_t offset;
    uint32_t length;
} Mc2LegacySection;
typedef struct {
    Mc2LegacySection entries[64];
    size_t count;
} Mc2LegacySectionTable;
/* Requires an AC1015 header and complete 6-byte section descriptors.
 * No partial output on failure. Overlapping sections are rejected. */
int mc2_ac1015_sections(const uint8_t *file, size_t length,
                        Mc2LegacySectionTable *out);
#ifdef __cplusplus
}
#endif
#endif
