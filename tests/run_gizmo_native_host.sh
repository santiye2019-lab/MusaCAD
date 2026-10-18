#!/usr/bin/env bash
set -euo pipefail
JHOME="${JAVA_HOME_17_X64:-${JAVA_HOME:-}}"
if [ -z "$JHOME" ]; then
  JHOME="$(dirname "$(dirname "$(readlink -f "$(command -v javac)")")")"
fi
mkdir -p /tmp/musacad-gizmo-jni /tmp/musacad-gizmo-java
cc -std=c11 -Wall -Wextra -Werror -fPIC -shared \
  -I"$JHOME/include" -I"$JHOME/include/linux" \
  -Icore/native/include \
  core/native/src/musacad_gizmo.c core/native/src/musacad_ucs.c \
  app/src/main/cpp/gizmo_jni.c app/src/main/cpp/ucs_jni.c \
  -lm -o /tmp/musacad-gizmo-jni/libmusacad_dwg.so
javac -encoding UTF-8 -d /tmp/musacad-gizmo-java \
  app/src/main/java/com/musa/cad/CadUcsFrame.java \
  app/src/main/java/com/musa/cad/CadGizmoMath.java \
  tests/CadGizmoNativeHostTest.java tests/CadGizmoMathTest.java
java -cp /tmp/musacad-gizmo-java CadGizmoMathTest
java -Djava.library.path=/tmp/musacad-gizmo-jni \
  -cp /tmp/musacad-gizmo-java CadGizmoNativeHostTest
