#!/usr/bin/env bash
# One actual first-party C engine and three JNI transport entrypoints.
set -euo pipefail
: "${JAVA_HOME:?Set JAVA_HOME to test JDK}"
mkdir -p /tmp/musacad-mc1032-migrate
cc -std=c11 -Wall -Wextra -Werror -fPIC -shared \
  -I"${JAVA_HOME}/include" -I"${JAVA_HOME}/include/linux" \
  -Icore/native/include \
  core/native/src/musacad_document.c \
  core/native/src/musacad_mc1032.c \
  core/native/src/musacad_handle_index.c \
  core/native/src/musacad_mc1032_v2.c \
  core/native/src/musacad_mc1032_migrate.c \
  app/src/main/cpp/mc1032_jni.c \
  app/src/main/cpp/mc1032_v2_jni.c \
  app/src/main/cpp/mc1032_migrate_jni.c \
  -lm -o /tmp/musacad-mc1032-migrate/libmusacad_dwg.so
