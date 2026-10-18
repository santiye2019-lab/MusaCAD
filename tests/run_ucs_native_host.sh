#!/usr/bin/env bash
set -euo pipefail
JHOME="${JAVA_HOME_17_X64:-${JAVA_HOME:-}}"
if [ -z "$JHOME" ]; then
  JHOME="$(dirname "$(dirname "$(readlink -f "$(command -v javac)")")")"
fi
mkdir -p /tmp/musacad-ucs-jni /tmp/musacad-ucs-java
cc -std=c11 -Wall -Wextra -Werror -fPIC -shared \
  -I"$JHOME/include" -I"$JHOME/include/linux" \
  -Icore/native/include \
  core/native/src/musacad_ucs.c app/src/main/cpp/ucs_jni.c \
  -lm -o /tmp/musacad-ucs-jni/libmusacad_dwg.so
javac -encoding UTF-8 -d /tmp/musacad-ucs-java \
  app/src/main/java/com/musa/cad/CadUcsFrame.java \
  tests/CadUcsFrameTest.java tests/CadUcsNativeHostTest.java
java -cp /tmp/musacad-ucs-java CadUcsFrameTest
java -Djava.library.path=/tmp/musacad-ucs-jni \
  -cp /tmp/musacad-ucs-java CadUcsNativeHostTest
