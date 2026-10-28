#ifndef MC2_SECTION_MAP_H
#define MC2_SECTION_MAP_H
#include <stddef.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
typedef struct {
    uint32_t id;
    size_t offset;
    size_t length;
} Mc2SectionRange;
typedef struct {
    Mc2SectionRange entries[128];
    size_t count;
    size_t file_length;
} Mc2SectionMap;
/* Generic verified byte ranges, not an AC1032 on-disk directory parser. */
int mc2_section_map_init(Mc2SectionMap *map, size_t file_length);
int mc2_section_map_add(Mc2SectionMap *map, uint32_t id,
                        size_t offset, size_t length);
int mc2_section_map_find(const Mc2SectionMap *map, uint32_t id,
                         Mc2SectionRange *out);
int mc2_section_map_slice(const Mc2SectionMap *map, uint32_t id,
                          const uint8_t *file, size_t actual_length,
                          const uint8_t **data, size_t *length);
#ifdef __cplusplus
}
#endif
#endif
