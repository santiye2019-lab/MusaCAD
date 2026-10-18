#!/usr/bin/env bash
# Host acceptance for the SAME first-party C JNI codec shipped on Android.
# Compiles no third-party CAD dependency, generates no APK.
set -euo pipefail
: "${JAVA_HOME:?Test requires JDK JAVA_HOME}"
mkdir -p /tmp/musacad-mc1032-host
cc -std=c11 -Wall -Wextra -Werror -fPIC -shared \
  -I"${JAVA_HOME}/include" -I"${JAVA_HOME}/include/linux" \
  -Icore/native/include \
  core/native/src/musacad_document.c \
  core/native/src/musacad_mc1032.c \
  app/src/main/cpp/mc1032_jni.c \
  -lm -o /tmp/musacad-mc1032-host/libmusacad_dwg.so
