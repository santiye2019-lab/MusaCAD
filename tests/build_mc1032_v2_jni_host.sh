#!/usr/bin/env bash
# Real Linux-host JNI check of the exact first-party C used by Android.
set -euo pipefail
: "${JAVA_HOME:?Set JAVA_HOME to test JDK}"
mkdir -p /tmp/musacad-mc1032-v2-host
cc -std=c11 -Wall -Wextra -Werror -fPIC -shared \
  -I"${JAVA_HOME}/include" -I"${JAVA_HOME}/include/linux" \
  -Icore/native/include \
  core/native/src/musacad_mc1032_v2.c \
  app/src/main/cpp/mc1032_v2_jni.c \
  -lm -o /tmp/musacad-mc1032-v2-host/libmusacad_dwg.so
