#!/usr/bin/env bash
# Test-only host JNI transport of the SAME portable C ABI used by Android.
# Never produces an APK, app package, or DWG writer.
set -euo pipefail
: "${JAVA_HOME:?JAVA_HOME must point to the test JDK}"
mkdir -p /tmp/musacad-dxf-handle-host
cc -std=c11 -Wall -Wextra -Werror -fPIC -shared \
  -I"${JAVA_HOME}/include" -I"${JAVA_HOME}/include/linux" \
  -Icore/native/include \
  core/native/src/musacad_dxf_handle.c core/native/src/musacad_dxf_handle_set.c \
  app/src/main/cpp/dxf_handle_jni.c app/src/main/cpp/dxf_saved_handles_jni.c \
  core/native/src/musacad_dxf_group.c core/native/src/musacad_dxf_pair.c app/src/main/cpp/dxf_group_jni.c \
  core/native/src/musacad_source_sha256.c app/src/main/cpp/source_sha256_jni.c \
  core/native/src/musacad_dxf_section.c app/src/main/cpp/dxf_section_jni.c \
  core/native/src/musacad_dxf_byte_lines.c app/src/main/cpp/dxf_byte_lines_jni.c \
  -o /tmp/musacad-dxf-handle-host/libmusacad_dwg.so
