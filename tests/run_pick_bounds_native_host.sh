#!/usr/bin/env bash
set -euo pipefail
JHOME="${JAVA_HOME_17_X64:-${JAVA_HOME:-}}"
if [ -z "$JHOME" ]; then
    JHOME="$(dirname "$(dirname "$(readlink -f "$(command -v javac)")")")"
fi
mkdir -p /tmp/musacad-pick-java /tmp/musacad-pick-jni
javac -encoding UTF-8 -d /tmp/musacad-pick-java \
  app/src/main/java/com/musa/cad/CadEdit.java \
  app/src/main/java/com/musa/cad/CadPickBounds.java \
  tests/CadPickBoundsTest.java tests/CadPickBoundsNativeHostTest.java \
  tests/CadPickBoundsMissingCoreTest.java
java -cp /tmp/musacad-pick-java CadPickBoundsTest
java -cp /tmp/musacad-pick-java CadPickBoundsMissingCoreTest
cc -std=c11 -Wall -Wextra -Werror -fPIC -shared \
  -I"$JHOME/include" -I"$JHOME/include/linux" -Icore/native/include \
  core/native/src/musacad_pick_bounds.c app/src/main/cpp/pick_bounds_jni.c \
  -lm -o /tmp/musacad-pick-jni/libmusacad_dwg.so
java -Djava.library.path=/tmp/musacad-pick-jni \
  -cp /tmp/musacad-pick-java CadPickBoundsNativeHostTest
