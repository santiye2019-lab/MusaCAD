#!/usr/bin/env bash
set -euo pipefail
JHOME="${JAVA_HOME_17_X64:-${JAVA_HOME:-}}"
if [ -z "$JHOME" ]; then
    JHOME="$(dirname "$(dirname "$(readlink -f "$(command -v javac)")")")"
fi
mkdir -p /tmp/musacad-mass-jni /tmp/musacad-mass-java
javac -encoding UTF-8 -d /tmp/musacad-mass-java \
  app/src/main/java/com/musa/cad/Cad3dMeshTopology.java \
  app/src/main/java/com/musa/cad/Cad3dSignedVolume.java \
  app/src/main/java/com/musa/cad/Cad3dNativeMeshMass.java \
  tests/Cad3dMeshTopologyTest.java \
  tests/Cad3dSignedVolumeTest.java \
  tests/Cad3dNativeMeshMassTest.java \
  tests/Cad3dSignedVolumeNativeHostTest.java \
  tests/Cad3dSignedVolumeMissingCoreTest.java
java -cp /tmp/musacad-mass-java Cad3dMeshTopologyTest
java -cp /tmp/musacad-mass-java Cad3dSignedVolumeTest
java -cp /tmp/musacad-mass-java Cad3dNativeMeshMassTest
java -cp /tmp/musacad-mass-java Cad3dSignedVolumeMissingCoreTest
cc -std=c11 -Wall -Wextra -Werror -fPIC -shared \
  -I"$JHOME/include" -I"$JHOME/include/linux" \
  -Icore/native/include \
  core/native/src/musacad_mesh_topology.c \
  core/native/src/musacad_mesh_mass.c \
  app/src/main/cpp/mesh_topology_jni.c \
  app/src/main/cpp/mesh_mass_jni.c -lm \
  -o /tmp/musacad-mass-jni/libmusacad_dwg.so
java -Djava.library.path=/tmp/musacad-mass-jni \
  -cp /tmp/musacad-mass-java Cad3dSignedVolumeNativeHostTest
