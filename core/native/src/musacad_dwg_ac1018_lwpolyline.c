/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "musacad_dwg_ac1018_lwpolyline.h"
#include "musacad_ocs.h"
#include "musacad_dwg_ac1018_eed.h"
#include "musacad_dwg_ac1015_values.h"
#include "musacad_crc16.h"
#include <math.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

/* First-party R2004 record grammar as documented in ODA public .DWG spec.
 * The record is separate from page maps, classes and handle graph. */
static int read_ms(const uint8_t *p, size_t n, size_t *num_bytes, size_t *value) {
    if (!p || !num_bytes || !value || n < 2u) return 0;
    const uint16_t first = (uint16_t)p[0] | ((uint16_t)p[1] << 8u);
    if ((first & 0x8000u) == 0u) {
        if (first < 8u) return 0;
        *num_bytes = 2u; *value = first; return 1;
    }
    if (n < 4u) return 0;
    const uint16_t second = (uint16_t)p[2] | ((uint16_t)p[3] << 8u);
    if (!second || (second & 0x8000u)) return 0;
    *num_bytes = 4u;
    *value = ((size_t)(first & 0x7fffu)) | ((size_t)second << 15u);
    return *value >= 8u;
}
static int read_self(MusaDwgAc1015Bits *bits, uint64_t *handle) {
    uint32_t head = 0u;
    if (!musa_dwg_ac1015_bits_read_u(bits, 8u, &head) ||
        (head >> 4u) != 0u || (head & 15u) == 0u || (head & 15u) > 8u)
        return 0;
    uint64_t h = 0u;
    for (unsigned i=0u; i<(head & 15u); ++i) {
        uint32_t octet = 0u;
        if (!musa_dwg_ac1015_bits_read_u(bits, 8u, &octet)) return 0;
        h = (h << 8u) | (uint64_t)octet;
    }
    *handle = h;
    return 1;
}
static int finite_bd(MusaDwgAc1015Bits *b, double *out) {
    return musa_dwg_ac1015_bits_read_bd(b, out) && isfinite(*out);
}
static int read_lw_impl(const uint8_t *section,
    size_t section_bytes, size_t offset, uint64_t indexed_handle,
    uint32_t permitted_reactors, MusaDwgAc1018LWVertex *out_vertices,
    size_t capacity, MusaDwgAc1018LWPolyline *out) {
    if (!section || !out || !out_vertices || !indexed_handle ||
        offset >= section_bytes) return 0;
    const uint8_t *p = section + offset;
    const size_t remain = section_bytes - offset;
    size_t len_bytes = 0u, data_bytes = 0u;
    if (!read_ms(p, remain, &len_bytes, &data_bytes) ||
        remain < len_bytes + 2u || data_bytes > remain - len_bytes - 2u ||
        data_bytes > SIZE_MAX / 8u) return 0;
    uint16_t crc = 0u;
    if (!musa_core_crc16_arc(0xc0c1u,p,len_bytes + data_bytes,&crc)) return 0;
    const size_t crc_at = len_bytes + data_bytes;
    if (crc != ((uint16_t)p[crc_at] | ((uint16_t)p[crc_at+1u] << 8u))) return 0;
    MusaDwgAc1015Bits b={0};
    if (!musa_dwg_ac1015_bits_init(&b,p+len_bytes,data_bytes,0u,data_bytes*8u))
        return 0;
    uint16_t typ=0;
    uint32_t before_handles=0u;
    MusaDwgAc1018LWPolyline value={0};
    uint32_t flag=0u,reactors=0u;
    if (!musa_dwg_ac1015_bits_read_bs(&b,&typ) || typ!=77u ||
        !musa_dwg_ac1015_bits_read_rl(&b,&before_handles) ||
        (size_t)before_handles>data_bytes*8u ||
        !read_self(&b,&value.handle) || value.handle!=indexed_handle ||
        !musa_dwg_ac1018_eed_skip(&b,&value.eed_raw_bytes_skipped) ||
        !musa_dwg_ac1015_bits_read_u(&b,1u,&flag) || flag!=0u ||
        !musa_dwg_ac1015_bits_read_u(&b,2u,&flag) || flag==3u ||
        !musa_dwg_ac1015_bits_read_bl(&b,&reactors) || reactors!=permitted_reactors)
        return 0;
    value.entity_mode=(uint8_t)flag;
    if (!musa_dwg_ac1015_bits_read_u(&b,1u,&flag)) return 0;
    value.xdictionary_missing=(uint8_t)flag;
    if (!musa_dwg_ac1015_bits_read_bs(&b,&value.color) ||
        (value.color & 0xc000u)!=0u) return 0;
    if ((value.color & 0x2000u)!=0u) {
        uint32_t alpha=0u;
        if (!musa_dwg_ac1015_bits_read_bl(&b,&alpha)) return 0;
    }
    if (!finite_bd(&b,&value.linetype_scale) ||
        !musa_dwg_ac1015_bits_read_u(&b,2u,&flag)) return 0;
    value.linetype_flags=(uint8_t)flag;
    if (!musa_dwg_ac1015_bits_read_u(&b,2u,&flag)) return 0;
    value.plotstyle_flags=(uint8_t)flag;
    if (!musa_dwg_ac1015_bits_read_bs(&b,&value.invisibility) ||
        !musa_dwg_ac1015_bits_read_u(&b,8u,&flag)) return 0;
    value.lineweight=(uint8_t)flag;
    if (!musa_dwg_ac1015_bits_read_bs(&b,&value.flags) ||
        (value.flags & ~UINT16_C(0x033f))!=0u) return 0;
    value.closed = (uint8_t)((value.flags & 512u)!=0u);
    value.extrusion[2]=1.0;
    if ((value.flags & 4u)!=0u && !finite_bd(&b,&value.constant_width)) return 0;
    if ((value.flags & 8u)!=0u && !finite_bd(&b,&value.elevation)) return 0;
    if ((value.flags & 2u)!=0u && !finite_bd(&b,&value.thickness)) return 0;
    if ((value.flags & 1u)!=0u) {
        for (unsigned i=0u;i<3u;++i)
            if (!finite_bd(&b,&value.extrusion[i])) return 0;
        if (value.extrusion[0]==0.0 && value.extrusion[1]==0.0 &&
            value.extrusion[2]==0.0) return 0;
    }
    if (!musa_dwg_ac1015_bits_read_bl(&b,&value.vertex_count) ||
        !value.vertex_count || value.vertex_count>100000u ||
        value.vertex_count>capacity) return 0;
    if ((value.flags & 16u)!=0u &&
        !musa_dwg_ac1015_bits_read_bl(&b,&value.bulge_count)) return 0;
    if ((value.flags & 32u)!=0u &&
        !musa_dwg_ac1015_bits_read_bl(&b,&value.width_count)) return 0;
    if (value.bulge_count>value.vertex_count ||
        value.width_count>value.vertex_count) return 0;
    /* Atomic result via bounded scratch allocation, only committed after
     * whole record and declared pre-handles boundary have been checked. */
    MusaDwgAc1018LWVertex *vertices=(MusaDwgAc1018LWVertex *)calloc(
        (size_t)value.vertex_count,sizeof(*vertices));
    if (!vertices) return 0;
    int good=musa_dwg_ac1015_bits_read_rd(&b,&vertices[0].x) &&
             musa_dwg_ac1015_bits_read_rd(&b,&vertices[0].y);
    for (uint32_t i=1u;good && i<value.vertex_count;++i) {
        good=musa_dwg_ac1015_bits_read_dd(&b,vertices[i-1u].x,&vertices[i].x) &&
             musa_dwg_ac1015_bits_read_dd(&b,vertices[i-1u].y,&vertices[i].y);
    }
    for (uint32_t i=0u;good && i<value.bulge_count;++i)
        good=finite_bd(&b,&vertices[i].bulge);
    for (uint32_t i=0u;good && i<value.width_count;++i)
        good=finite_bd(&b,&vertices[i].start_width) &&
             finite_bd(&b,&vertices[i].end_width);
    good=good && b.bit_pos==(size_t)before_handles;
    for (uint32_t i=0u;good && i<value.vertex_count;++i)
        good=isfinite(vertices[i].x) && isfinite(vertices[i].y);
    if (good) {
        memcpy(out_vertices,vertices,(size_t)value.vertex_count*sizeof(*vertices));
        *out=value;
    }
    free(vertices);
    return good;
}


/* Original read-only API remains unchanged and refuses all reactor links. */
int musa_dwg_ac1018_read_lwpolyline_record(const uint8_t *section,
    size_t section_bytes, size_t offset, uint64_t indexed_handle,
    MusaDwgAc1018LWVertex *out_vertices, size_t capacity,
    MusaDwgAc1018LWPolyline *out) {
    return read_lw_impl(section,section_bytes,offset,indexed_handle,
                        0u,out_vertices,capacity,out);
}
int musa_dwg_ac1018_read_lwpolyline_with_reactors(
    const uint8_t *section,size_t section_bytes,size_t offset,
    uint64_t indexed_handle,const MusaDwgAc1015HandleEntry *index,
    size_t index_count,MusaDwgAc1018LWVertex *vertices,
    size_t vertex_capacity,uint64_t *reactor_targets,
    size_t reactor_capacity,MusaDwgAc1018ReactorAudit *link_audit,
    MusaDwgAc1018LWPolyline *geometry) {
    if(!vertices||!vertex_capacity||!reactor_targets||
       !reactor_capacity||!link_audit||!geometry)return 0;
    uint64_t local_handles[64]={0};
    MusaDwgAc1018ReactorAudit checked={0};
    MusaDwgAc1018LWPolyline checked_geometry={0};
    if(!musa_dwg_ac1018_read_reactor_links(
            section,section_bytes,offset,indexed_handle,index,index_count,
            local_handles,64u,&checked) ||
       checked.reactor_count>reactor_capacity ||
       !read_lw_impl(section,section_bytes,offset,indexed_handle,
                     checked.reactor_count,vertices,vertex_capacity,
                     &checked_geometry))return 0;
    checked.geometry_decoded=1u;
    memcpy(reactor_targets,local_handles,
           (size_t)checked.reactor_count*sizeof(local_handles[0]));
    *link_audit=checked;
    *geometry=checked_geometry;
    return 1;
}

/* Single shared OCS-to-WCS path: no Android-specific coordinate math. */
int musa_dwg_ac1018_lwpolyline_vertex_to_wcs(
    const MusaDwgAc1018LWPolyline *poly,
    const MusaDwgAc1018LWVertex *vertex, double out_xyz[3]) {
    if (!poly || !vertex || !out_xyz) return 0;
    MusaCoreOcsBasis basis={0};
    const double xy[2]={vertex->x,vertex->y};
    return musa_core_ocs_basis(poly->extrusion,&basis) &&
           musa_core_ocs_to_world(&basis,xy,poly->elevation,out_xyz);
}
