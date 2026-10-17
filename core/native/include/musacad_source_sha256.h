/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef MUSACAD_SOURCE_SHA256_H
#define MUSACAD_SOURCE_SHA256_H
#include <stddef.h>
#ifdef __cplusplus
extern "C" {
#endif
/* Streaming SHA-256 of immutable CAD source bytes before staged export.
 * Platform file I/O remains in the adapter; hash bytes use identical portable
 * C code on Android, Windows and iOS. This hashes bytes, not CAD semantics.
 * One context per capture. Caller MUST destroy even after any failure.
 * Valid SHA-256, not a claim of authenticated or signed source provenance.
 */
typedef struct MusaSourceSha256 MusaSourceSha256;
MusaSourceSha256 *musa_core_source_sha256_create(void);
int musa_core_source_sha256_update(MusaSourceSha256 *state,
                                  const unsigned char *bytes,size_t length);
int musa_core_source_sha256_finish(MusaSourceSha256 *state,
                                  unsigned char out_digest[32]);
void musa_core_source_sha256_destroy(MusaSourceSha256 *state);
#ifdef __cplusplus
}
#endif
#endif
