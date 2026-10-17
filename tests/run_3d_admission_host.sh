#!/usr/bin/env bash
# Pure-JVM 3D file fixtures always run against the SAME shared C engine as Android.
# Compiles test-only host JNI transport, not an Android APK or release artifact.
set -euo pipefail
: "${JAVA_HOME:?JAVA_HOME must point to the installed JDK 17}"
mkdir -p /tmp/musacad-3d-only
javac -encoding UTF-8 -d /tmp/musacad-3d-only \
  app/src/main/java/com/musa/cad/FileTransfer.java \
  app/src/main/java/com/musa/cad/DxfColor.java \
  app/src/main/java/com/musa/cad/DxfLineStyle.java \
  app/src/main/java/com/musa/cad/DxfLineTypeParser.java \
  app/src/main/java/com/musa/cad/DxfTextStyle.java \
  app/src/main/java/com/musa/cad/DxfStream.java \
  app/src/main/java/com/musa/cad/DxfLayerState.java \
  app/src/main/java/com/musa/cad/DxfBlocks.java \
  app/src/main/java/com/musa/cad/AcisSatMesh.java \
  app/src/main/java/com/musa/cad/Dxf3dMesh.java \
  app/src/main/java/com/musa/cad/Cad3dOnlyOpenGuard.java \
  tests/Cad3dOnlyOpenGuardTest.java \
  tests/Cad3dOnlyMissingCoreTest.java
# Start a separate JVM without any native library: fail closed, never guess.
java -cp /tmp/musacad-3d-only Cad3dOnlyMissingCoreTest
cc -std=c11 -Wall -Wextra -Werror -fPIC -shared \
  -I"${JAVA_HOME}/include" -I"${JAVA_HOME}/include/linux" \
  -Icore/native/include \
  core/native/src/musacad_3d_admission.c \
  app/src/main/cpp/open3d_guard_jni.c \
  -o /tmp/musacad-3d-only/libmusacad_dwg.so
java -Djava.library.path=/tmp/musacad-3d-only \
  -cp /tmp/musacad-3d-only Cad3dOnlyOpenGuardTest
