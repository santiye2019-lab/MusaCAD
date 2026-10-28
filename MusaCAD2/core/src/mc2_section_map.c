#include "mc2_section_map.h"
#include <stddef.h>
#include <stdint.h>
int mc2_section_map_init(Mc2SectionMap *map, size_t file_length) {
    if (!map) return 0;
    map->count = 0;
    map->file_length = file_length;
    return 1;
}
int mc2_section_map_add(Mc2SectionMap *map, uint32_t id,
                        size_t offset, size_t length) {
    if (!map || map->count >= 128u || offset > map->file_length ||
        length > map->file_length - offset) return 0;
    for (size_t i = 0; i < map->count; ++i) {
        const Mc2SectionRange *s = &map->entries[i];
        if (s->id == id) return 0;
        if (length && s->length &&
            offset < s->offset + s->length &&
            s->offset < offset + length) return 0;
    }
    Mc2SectionRange *next = &map->entries[map->count];
    next->id = id;
    next->offset = offset;
    next->length = length;
    ++map->count;
    return 1;
}
int mc2_section_map_find(const Mc2SectionMap *map, uint32_t id,
                         Mc2SectionRange *out) {
    if (!map || !out) return 0;
    for (size_t i = 0; i < map->count; ++i) {
        if (map->entries[i].id == id) {
            *out = map->entries[i];
            return 1;
        }
    }
    return 0;
}
int mc2_section_map_slice(const Mc2SectionMap *map, uint32_t id,
                          const uint8_t *file, size_t actual_length,
                          const uint8_t **data, size_t *length) {
    Mc2SectionRange range;
    if (!file || !data || !length ||
        !mc2_section_map_find(map, id, &range) ||
        actual_length < map->file_length ||
        range.offset > actual_length ||
        range.length > actual_length - range.offset) return 0;
    *data = file + range.offset;
    *length = range.length;
    return 1;
}
