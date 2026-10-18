/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef MUSACAD_MC1032_V2_H
#define MUSACAD_MC1032_V2_H
#include <stddef.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
/* Original MusaCAD MC1032 revision 2, NOT AC1032/DWG.
 * One portable C ABI for Android, Windows, macOS, Linux and iOS.
 * v1 read/write APIs remain unchanged. v2 adds named layers and 3D WCS
 * LINE and POINT, not complete 3D solids or a DWG decoder.
 */
#define MUSA_MC2_MAX_ENTITIES 4096u
#define MUSA_MC2_MAX_LAYERS 64u
#define MUSA_MC2_HEADER_SIZE 48u
#define MUSA_MC2_LAYER_SIZE 64u
#define MUSA_MC2_RECORD_SIZE 80u
#define MUSA_MC2_MAX_BYTES (MUSA_MC2_HEADER_SIZE + MUSA_MC2_LAYER_SIZE*MUSA_MC2_MAX_LAYERS + MUSA_MC2_RECORD_SIZE*MUSA_MC2_MAX_ENTITIES)
#define MUSA_MC2_LAYER_NAME_BYTES 48u
enum MusaMC2Result {
    MUSA_MC2_OK=0,
    MUSA_MC2_INVALID=-1,
    MUSA_MC2_UNSUPPORTED=-2,
    MUSA_MC2_CORRUPT=-3,
    MUSA_MC2_LIMIT=-4,
    MUSA_MC2_NO_MEMORY=-5,
    MUSA_MC2_NOT_FOUND=-6
};
enum MusaMC2Kind {MUSA_MC2_LINE3D=1,MUSA_MC2_POINT3D=2};
typedef struct MusaMC2Scene MusaMC2Scene;
typedef struct {
    uint64_t handle;
    uint32_t kind,layer;
    double x1,y1,z1,x2,y2,z2;
} MusaMC2Entity;
typedef struct {
    uint32_t rgba,visible;
    char name[MUSA_MC2_LAYER_NAME_BYTES+1u];
} MusaMC2Layer;
/* New scenes always start with layer #0 ("0", opaque white). */
MusaMC2Scene *musa_mc2_scene_create(uint32_t max_entities);
void musa_mc2_scene_destroy(MusaMC2Scene *scene);
uint32_t musa_mc2_scene_entity_count(const MusaMC2Scene *scene);
uint32_t musa_mc2_scene_layer_count(const MusaMC2Scene *scene);
int musa_mc2_scene_add_layer(MusaMC2Scene *scene,const char *name,
                             uint32_t rgba,uint32_t *id);
int musa_mc2_scene_get_layer(const MusaMC2Scene *scene,uint32_t id,
                             MusaMC2Layer *out);
int musa_mc2_scene_add_entity(MusaMC2Scene *scene,const MusaMC2Entity *entity,
                              uint64_t *actual_handle);
int musa_mc2_scene_get_index(const MusaMC2Scene *scene,uint32_t index,
                             MusaMC2Entity *out);
int musa_mc2_scene_get(const MusaMC2Scene *scene,uint64_t handle,
                       MusaMC2Entity *out);
int musa_mc2_scene_translate(MusaMC2Scene *scene,uint64_t handle,
                             double dx,double dy,double dz);
int musa_mc2_scene_delete(MusaMC2Scene *scene,uint64_t handle);
/* Size query: dst=NULL,cap=0, written gets exact byte count.
 * Writes to a private buffer then publishes ALL bytes only on success.
 * On failure any provided destination remains unchanged.
 */
int musa_mc2_encode(const MusaMC2Scene *scene,unsigned char *dst,
                    size_t capacity,size_t *written);
int musa_mc2_decode(const unsigned char *bytes,size_t length,
                    MusaMC2Scene **out);
#ifdef __cplusplus
}
#endif
#endif
