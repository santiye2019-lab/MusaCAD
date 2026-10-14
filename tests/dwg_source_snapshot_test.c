/* Exact-byte pilot input contract, independent from LibreDWG. */
#define _POSIX_C_SOURCE 200809L
#include <assert.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <utime.h>
#include "../app/src/main/cpp/dwg_source_snapshot.h"

static void replace_bytes(const char *path,const char *bytes,size_t n) {
  FILE *fp=fopen(path,"wb");
  assert(fp);
  assert(fwrite(bytes,1,n,fp)==n);
  assert(fclose(fp)==0);
}

int main(void) {
  char path[]="/tmp/musacad-dxf-snapshot-XXXXXX";
  int fd=mkstemp(path);
  assert(fd>=0);
  assert(close(fd)==0);
  const char *original="0\nSECTION\n2\nENTITIES\n0\nLINE\n10\n7\n0\nENDSEC\n0\nEOF\n";
  size_t length=strlen(original);
  replace_bytes(path,original,length);
  struct stat st;
  assert(stat(path,&st)==0);
  MusaDxfSourceSnapshot snapshot={0};
  assert(musa_dxf_source_capture(path,1024*1024,&snapshot));
  assert(snapshot.length==length);
  assert(musa_dxf_source_matches(path,&snapshot));

  char *modified=(char *)malloc(length+1);
  assert(modified);
  memcpy(modified,original,length+1);
  modified[34] = modified[34]=='7'?'8':'7';
  replace_bytes(path,modified,length);
  struct utimbuf times={st.st_atime,st.st_mtime};
  assert(utime(path,&times)==0);
  assert(stat(path,&st)==0&&st.st_size==(off_t)length);
  assert(!musa_dxf_source_matches(path,&snapshot));

  replace_bytes(path,original,length);
  assert(musa_dxf_source_matches(path,&snapshot));
  assert(!musa_dxf_source_capture(path,length-1,&(MusaDxfSourceSnapshot){0}));

  /* Same exact bytes from a different inode are NOT the same open drawing. */
  char other[]="/tmp/musacad-dxf-snapshot-swap-XXXXXX";
  fd=mkstemp(other);
  assert(fd>=0);
  assert(close(fd)==0);
  replace_bytes(other,original,length);
  assert(rename(other,path)==0);
  assert(!musa_dxf_source_matches(path,&snapshot));

  musa_dxf_source_release(&snapshot);
  assert(snapshot.bytes==NULL&&snapshot.length==0);
  free(modified);
  assert(unlink(path)==0);
  puts("R2000 writer exact original DXF snapshot: same-size, mtime, inode PASS");
  return 0;
}
