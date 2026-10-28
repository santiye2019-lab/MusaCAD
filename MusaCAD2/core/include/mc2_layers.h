#ifndef MC2_LAYERS_H
#define MC2_LAYERS_H
#include "mc2_geometry.h"
#include <stddef.h>
#include <stdint.h>
typedef struct {
    uint32_t id;
    uint32_t color_rgb;
    int is_on;
    int is_frozen;
} Mc2Layer;
typedef enum { MC2_COLOR_EXPLICIT = 0, MC2_COLOR_BY_LAYER = 1, MC2_COLOR_BY_BLOCK = 2 } Mc2ColorMode;
typedef struct {
    uint32_t rgb;
    Mc2ColorMode mode;
} Mc2Color;
typedef struct {
    uint32_t effective_rgb;
    int visible;
} Mc2EntityAppearance;
/* Resolve against a preselected layer; caller guarantees layer identity. */
int mc2_resolve_appearance_on_layer(const Mc2Entity2 *entity,
                                    Mc2Color color, const Mc2Layer *layer,
                                    uint32_t parent_block_rgb, int parent_visible,
                                    Mc2EntityAppearance *out);
/* Returns 0 on missing layer, duplicate matching layer id, or invalid color. */
int mc2_resolve_appearance(const Mc2Entity2 *entity,
                           Mc2Color color,
                           const Mc2Layer *layers, size_t layer_count,
                           uint32_t parent_block_rgb,
                           int parent_visible,
                           Mc2EntityAppearance *out);
#endif
