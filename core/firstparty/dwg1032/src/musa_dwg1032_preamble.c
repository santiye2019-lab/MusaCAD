/* SPDX-License-Identifier: MIT
 * Original bounded AC1032 fixed preamble interpretation, standard C11.
 * Field positions are format facts from the ODA DWG specification,
 * chapters 4.1, 6, 7 and 8; no third-party implementation was used.
 */
#include "musa_dwg1032_preamble.h"
#include <stdint.h>
#include <stddef.h>

static uint16_t le16(const uint8_t *p) {
    return (uint16_t)((uint16_t)p[0] | ((uint16_t)p[1] << 8u));
}
static uint32_t le32(const uint8_t *p) {
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8u) |
           ((uint32_t)p[2] << 16u) | ((uint32_t)p[3] << 24u);
}
static int overlaps(const void *a, size_t an, const void *b, size_t bn) {
    if (!an || !bn) return 0;
    const uintptr_t x=(uintptr_t)a, y=(uintptr_t)b;
    return x<=y ? y-x<an : x-y<bn;
}
static int address_ok(uint32_t offset, uint64_t length) {
    return offset==0u || (offset>=0x100u && (uint64_t)offset<(uint64_t)length);
}
int musa_dwg1032_read_preamble_prefix(const uint8_t *file,
                                      size_t prefix_bytes,
                                      uint64_t file_bytes,
                                      MusaDwg1032Preamble *out) {
    if (!file || !out || prefix_bytes!=0x100u || file_bytes<0x100u ||
        overlaps(file,prefix_bytes,out,sizeof(*out)) ||
        file[0]!='A' || file[1]!='C' || file[2]!='1' ||
        file[3]!='0' || file[4]!='3' || file[5]!='2')
        return 0;
    for (size_t i=0x06u;i<0x0bu;++i)
        if (file[i]!=0u) return 0;
    if (file[0x0cu]!=0u && file[0x0cu]!=1u && file[0x0cu]!=3u)
        return 0;
    for (size_t i=0x15u;i<0x18u;++i)
        if (file[i]!=0u) return 0;
    if (le32(file+0x28u)!=0x80u) return 0;
    for (size_t i=0x2cu;i<0x80u;++i)
        if (file[i]!=0u) return 0;
    MusaDwg1032Preamble next={0};
    next.maintenance_version=file[0x0bu];
    next.application_marker=file[0x0cu];
    next.writer_version=file[0x11u];
    next.writer_maintenance=file[0x12u];
    next.codepage=le16(file+0x13u);
    next.security_flags=le32(file+0x18u);
    next.preview_offset=le32(file+0x0du);
    next.summary_offset=le32(file+0x20u);
    next.vba_offset=le32(file+0x24u);
    if (!address_ok(next.preview_offset,file_bytes) ||
        !address_ok(next.summary_offset,file_bytes) ||
        !address_ok(next.vba_offset,file_bytes)) return 0;
    *out=next;
    return 1;
}

/* Retain the legacy whole-buffer API and its stricter full-file alias guard.
 * The prefix variant never dereferences bytes beyond the first 0x100. */
int musa_dwg1032_read_preamble(const uint8_t *file, size_t file_bytes,
                               MusaDwg1032Preamble *out) {
    if (!file || !out || file_bytes<0x100u || file_bytes>PTRDIFF_MAX ||
        overlaps(file,file_bytes,out,sizeof(*out))) return 0;
    return musa_dwg1032_read_preamble_prefix(file,0x100u,
                                              (uint64_t)file_bytes,out);
}
