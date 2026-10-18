#!/usr/bin/env bash
set -euo pipefail
JHOME="${JAVA_HOME_17_X64:-${JAVA_HOME:-}}"
if [ -z "$JHOME" ]; then
  JHOME="$(dirname "$(dirname "$(readlink -f "$(command -v javac)")")")"
fi
mkdir -p /tmp/musacad-topology-jni /tmp/musacad-topology-java
javac -encoding UTF-8 -d /tmp/musacad-topology-java \
  app/src/main/java/com/musa/cad/Cad3dMeshTopology.java \
  tests/Cad3dMeshTopologyTest.java \
  tests/Cad3dMeshTopologyNativeHostTest.java \
  tests/Cad3dMeshTopologyMissingCoreTest.java
java -cp /tmp/musacad-topology-java Cad3dMeshTopologyTest
java -cp /tmp/musacad-topology-java Cad3dMeshTopologyMissingCoreTest
cc -std=c11 -Wall -Wextra -Werror -fPIC -shared \
  -I"$JHOME/include" -I"$JHOME/include/linux" -Icore/native/include \
  core/native/src/musacad_mesh_topology.c app/src/main/cpp/mesh_topology_jni.c \
  -lm -o /tmp/musacad-topology-jni/libmusacad_dwg.so
java -Djava.library.path=/tmp/musacad-topology-jni \
  -cp /tmp/musacad-topology-java Cad3dMeshTopologyNativeHostTest
