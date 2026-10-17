#!/usr/bin/env bash
# Test-only host JNI transport of the SAME portable C ABI used by Android.
# Never produces an APK, app package, or DWG writer.
set -euo pipefail
: "${JAVA_HOME:?JAVA_HOME must point to the test JDK}"
mkdir -p /tmp/musacad-dxf-handle-host
cc -std=c11 -Wall -Wextra -Werror -fPIC -shared \
  -I"${JAVA_HOME}/include" -I"${JAVA_HOME}/include/linux" \
  -Icore/native/include \
  core/native/src/musacad_dxf_handle.c app/src/main/cpp/dxf_handle_jni.c \
  core/native/src/musacad_dxf_group.c app/src/main/cpp/dxf_group_jni.c \
  -o /tmp/musacad-dxf-handle-host/libmusacad_dwg.so
