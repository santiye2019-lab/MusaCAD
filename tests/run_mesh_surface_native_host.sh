#!/usr/bin/env bash
set -euo pipefail
JHOME="${JAVA_HOME_17_X64:-${JAVA_HOME:-}}"
if [ -z "$JHOME" ]; then
    JHOME="$(dirname "$(dirname "$(readlink -f "$(command -v javac)")")")"
fi
mkdir -p /tmp/musacad-surface-jni /tmp/musacad-surface-java
javac -encoding UTF-8 -d /tmp/musacad-surface-java \
  app/src/main/java/com/musa/cad/Cad3dSurfaceMetrics.java \
  app/src/main/java/com/musa/cad/Cad3dNativeMeshSurface.java \
  tests/Cad3dSurfaceMetricsTest.java \
  tests/Cad3dNativeMeshSurfaceTest.java \
  tests/Cad3dSurfaceMetricsNativeHostTest.java \
  tests/Cad3dSurfaceMetricsMissingCoreTest.java
java -cp /tmp/musacad-surface-java Cad3dSurfaceMetricsTest
java -cp /tmp/musacad-surface-java Cad3dNativeMeshSurfaceTest
java -cp /tmp/musacad-surface-java Cad3dSurfaceMetricsMissingCoreTest
cc -std=c11 -Wall -Wextra -Werror -fPIC -shared \
  -I"$JHOME/include" -I"$JHOME/include/linux" \
  -Icore/native/include \
  core/native/src/musacad_mesh_surface.c \
  app/src/main/cpp/mesh_surface_jni.c -lm \
  -o /tmp/musacad-surface-jni/libmusacad_dwg.so
java -Djava.library.path=/tmp/musacad-surface-jni \
  -cp /tmp/musacad-surface-java Cad3dSurfaceMetricsNativeHostTest
