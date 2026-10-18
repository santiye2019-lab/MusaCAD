#!/usr/bin/env bash
set -euo pipefail
JHOME="${JAVA_HOME_17_X64:-${JAVA_HOME:-}}"
if [ -z "$JHOME" ]; then
  JHOME="$(dirname "$(dirname "$(readlink -f "$(command -v javac)")")")"
fi
mkdir -p /tmp/musacad-face-jni /tmp/musacad-face-java
cc -std=c11 -Wall -Wextra -Werror -fPIC -shared \
  -I"$JHOME/include" -I"$JHOME/include/linux" -Icore/native/include \
  core/native/src/musacad_face_light.c app/src/main/cpp/face_light_jni.c \
  -lm -o /tmp/musacad-face-jni/libmusacad_dwg.so
javac -encoding UTF-8 -d /tmp/musacad-face-java \
  app/src/main/java/com/musa/cad/Cad3dFaceLighting.java \
  tests/Cad3dFaceLightingTest.java tests/Cad3dFaceLightingNativeHostTest.java
java -cp /tmp/musacad-face-java Cad3dFaceLightingTest
java -Djava.library.path=/tmp/musacad-face-jni \
  -cp /tmp/musacad-face-java Cad3dFaceLightingNativeHostTest
