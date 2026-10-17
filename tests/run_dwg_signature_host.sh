#!/usr/bin/env bash
# Compile a host-only JNI adapter against the exact common C source.
# Not an Android APK, not a DWG binary reader/writer, not an app release.
set -euo pipefail
: "${JAVA_HOME:?JAVA_HOME must point to installed JDK 17}"
mkdir -p /tmp/musacad-dwg-signature
javac -encoding UTF-8 -d /tmp/musacad-dwg-signature \
  app/src/main/java/com/musa/cad/CadDwgFormat.java \
  tests/CadDwgFormatTest.java tests/CadDwgFormatMissingCoreTest.java
java -cp /tmp/musacad-dwg-signature CadDwgFormatMissingCoreTest
cc -std=c11 -Wall -Wextra -Werror -fPIC -shared \
  -I"${JAVA_HOME}/include" -I"${JAVA_HOME}/include/linux" \
  -Icore/native/include \
  core/native/src/musacad_dwg_signature.c \
  app/src/main/cpp/dwg_signature_jni.c \
  core/native/src/musacad_source_sha256.c app/src/main/cpp/source_sha256_jni.c \
  -o /tmp/musacad-dwg-signature/libmusacad_dwg.so
java -Djava.library.path=/tmp/musacad-dwg-signature \
  -cp /tmp/musacad-dwg-signature CadDwgFormatTest
