cmake_minimum_required(VERSION 3.22)
project(musacad_native C)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
# Reusable core is the SAME C implementation on Android and desktop/iOS.
add_subdirectory("${CMAKE_CURRENT_LIST_DIR}/../../../../core/native" "${CMAKE_CURRENT_BINARY_DIR}/musacad-core")
# Owner decision (2026-10-18): Android ships first-party CAD ONLY.
# Do NOT build, link, load or execute the old LibreDWG reader in the app.
# The not-yet-implemented native DWG codec fails closed in its JNI adapter.
# Host compatibility test tooling remains physically separate below and MUST
# never be included in an Android shared library or release package.
if(ANDROID)
  add_library(musacad_dwg SHARED
    firstparty_dwg_unavailable_jni.c
    ac1032_firstparty_preview_jni.c
    ac1018_inspection_jni.c
    mesh_mass_jni.c mesh_surface_jni.c mesh_topology_jni.c pick_bounds_jni.c open3d_guard_jni.c
    dwg_signature_jni.c gizmo_jni.c face_light_jni.c mc1032_jni.c mc1032_v2_jni.c mc1032_migrate_jni.c dxf_handle_jni.c dxf_saved_handles_jni.c
    dxf_group_jni.c ucs_jni.c source_sha256_jni.c
    dxf_section_jni.c dxf_byte_lines_jni.c)
  target_link_options(musacad_dwg PRIVATE "-Wl,-z,max-page-size=16384")
  target_include_directories(musacad_dwg PRIVATE
    "${CMAKE_CURRENT_LIST_DIR}/../../../../core/native/include")
  target_link_libraries(musacad_dwg PRIVATE m musacad_core)
  return()
endif()

# Deprecated HOST-ONLY third-party oracle. Not part of Android runtime.
# Delete after independent first-party binary DWG reader / writer is proven.
set(LIBREDWG_LIBONLY ON CACHE BOOL "" FORCE)
# Normal Android and host converter stay read-only. DWG writing is compiled only
# by explicitly opting in to the isolated host proof; never affect production.
option(MUSACAD_HOST_DWG_WRITE_PILOT "Build isolated non-production DXF to R2000 DWG proof" OFF)
if(ANDROID AND MUSACAD_HOST_DWG_WRITE_PILOT)
  message(FATAL_ERROR "DWG write pilot is restricted to host CI; never an Android release")
endif()
if(MUSACAD_HOST_DWG_WRITE_PILOT)
  set(LIBREDWG_DISABLE_WRITE OFF CACHE BOOL "" FORCE)
else()
  set(LIBREDWG_DISABLE_WRITE ON CACHE BOOL "" FORCE)
endif()
set(LIBREDWG_DISABLE_JSON ON CACHE BOOL "" FORCE)
set(DISABLE_WERROR ON CACHE BOOL "" FORCE)
set(ENABLE_LTO OFF CACHE BOOL "" FORCE)
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
set(BUILD_TESTING OFF CACHE BOOL "" FORCE)
set(LIBREDWG_SOURCE "${CMAKE_CURRENT_LIST_DIR}/../../../../third_party/libredwg")
if(NOT EXISTS "${LIBREDWG_SOURCE}/CMakeLists.txt")
  message(FATAL_ERROR "Fetch pinned LibreDWG source as described in NATIVE_BUILD.md")
endif()
add_subdirectory("${LIBREDWG_SOURCE}" "libredwg")
if(ANDROID)
  add_library(musacad_dwg SHARED dwg_bridge.c native_scene.c mesh_mass_jni.c mesh_surface_jni.c open3d_guard_jni.c dwg_signature_jni.c dxf_handle_jni.c dxf_saved_handles_jni.c dxf_group_jni.c source_sha256_jni.c dxf_section_jni.c dxf_byte_lines_jni.c)
  set(MUSA_TARGET musacad_dwg)
  target_link_options(musacad_dwg PRIVATE "-Wl,-z,max-page-size=16384")
else()
  add_executable(musacad_dwg_convert dwg_bridge.c native_scene.c)
  set(MUSA_TARGET musacad_dwg_convert)
  target_compile_definitions(musacad_dwg_convert PRIVATE MUSA_HOST=1)
endif()
target_include_directories(${MUSA_TARGET} PRIVATE "${LIBREDWG_SOURCE}/include" "${LIBREDWG_SOURCE}/src" "${CMAKE_CURRENT_BINARY_DIR}/libredwg/src")
target_link_libraries(${MUSA_TARGET} PRIVATE redwg m musacad_core)

# Host-only proof: the existing DWG reader and Android application are unchanged.
if(NOT ANDROID AND MUSACAD_HOST_DWG_WRITE_PILOT)
  add_executable(musacad_dxf2dwg_pilot dwg_write_pilot.c)
  target_include_directories(musacad_dxf2dwg_pilot PRIVATE
    "${LIBREDWG_SOURCE}/include" "${LIBREDWG_SOURCE}/src"
    "${CMAKE_CURRENT_BINARY_DIR}/libredwg/src")
  target_link_libraries(musacad_dxf2dwg_pilot PRIVATE redwg m)
endif()
