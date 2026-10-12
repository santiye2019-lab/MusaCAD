cmake_minimum_required(VERSION 3.22)
project(musacad_native C)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(LIBREDWG_LIBONLY ON CACHE BOOL "" FORCE)
set(LIBREDWG_DISABLE_WRITE ON CACHE BOOL "" FORCE)
set(LIBREDWG_DISABLE_JSON ON CACHE BOOL "" FORCE)
set(DISABLE_WERROR ON CACHE BOOL "" FORCE)
set(ENABLE_LTO OFF CACHE BOOL "" FORCE)
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
set(BUILD_TESTING OFF CACHE BOOL "" FORCE)
set(LIBREDWG_SOURCE "${CMAKE_CURRENT_LIST_DIR}/../../../../third_party/libredwg")
if(NOT EXISTS "${LIBREDWG_SOURCE}/CMakeLists.txt")
  message(FATAL_ERROR "Fetch pinned LibreDWG source as described in NATIVE_BUILD.md")
endif()
# MusaCAD R2013+ ACIS compatibility. Only adjusts the source-version check
# before LibreDWG compiles; no changes to 2D first paint or CAD navigation.
# This feature is enabled on the isolated development branch ONLY. Keep
# production/main and existing APK unchanged until Android device smoke tests.
# The R2013 SAB/history change is still an experimental HOST converter fix.
# It must not silently enter an Android APK while incomplete B-Rep cases remain.
if(ANDROID)
  set(_MUSACAD_SAB_HISTORY_DEFAULT OFF)
else()
  set(_MUSACAD_SAB_HISTORY_DEFAULT ON)
endif()
option(MUSACAD_PRESERVE_R2013_SAB_HISTORY
  "Retain R2013+ source AcDs/SAB record history while exporting legacy SAT"
  ${_MUSACAD_SAB_HISTORY_DEFAULT})
if(ANDROID AND MUSACAD_PRESERVE_R2013_SAB_HISTORY)
  message(FATAL_ERROR
    "Experimental R2013 SAB history conversion cannot be included in an Android APK until geometry and real-device checks pass.")
endif()
if(MUSACAD_PRESERVE_R2013_SAB_HISTORY)
  set(_musa_sab_source "${LIBREDWG_SOURCE}/src/out_dxf.c")
  file(READ "${_musa_sab_source}" _musa_sab_text)
  set(_musa_sab_old "if (dwg_version < R_2010)\n    has_history = 0;")
  set(_musa_sab_new "if (dwg_version < R_2010 && (!dwg || dwg->header.version < R_2010))\n    has_history = 0;")
  string(FIND "${_musa_sab_text}" "${_musa_sab_old}" _musa_sab_old_pos)
  string(FIND "${_musa_sab_text}" "${_musa_sab_new}" _musa_sab_new_pos)
  if(NOT _musa_sab_old_pos EQUAL -1)
    string(REPLACE "${_musa_sab_old}" "${_musa_sab_new}" _musa_sab_patched "${_musa_sab_text}")
    file(WRITE "${_musa_sab_source}" "${_musa_sab_patched}")
    message(STATUS "MusaCAD development: keep original R2013+ SAB history records")
  elseif(_musa_sab_new_pos EQUAL -1)
    message(FATAL_ERROR "Pinned LibreDWG SAB conversion changed; do not silently patch a different source")
  endif()
endif()
# Host-only diagnostic for public pinned R2013/R2018 fixtures.
# Neither Android binaries nor normal production conversion are instrumented.
# Marker counts only: no drawing bytes, coordinates, names or handles.
if(NOT ANDROID)
  option(MUSACAD_HOST_ACDS_SIGNATURE_AUDIT
    "Count SAB markers in decoded public AcDs sections" OFF)
  if(MUSACAD_HOST_ACDS_SIGNATURE_AUDIT)
    set(_musa_acds_spec "${LIBREDWG_SOURCE}/src/acds.spec")
    file(READ "${_musa_acds_spec}" _musa_acds_text)
    set(_musa_acds_old "    num_acis_sab_data = 0;")
    set(_musa_acds_probe [=[
    size_t musa_starts = 0, musa_asm_starts = 0, musa_ends = 0;
    const char musa_asm_binary[] = "ASM BinaryFile";
    const size_t musa_sl = strlen (start), musa_al = strlen (musa_asm_binary);
    const size_t musa_el = strlen (end);
    for (size_t musa_i = 0; musa_i < dat->size; musa_i++)
      {
        if (musa_i + musa_sl <= dat->size &&
            memcmp (&dat->chain[musa_i], start, musa_sl) == 0)
          musa_starts++;
        if (musa_i + musa_al <= dat->size &&
            memcmp (&dat->chain[musa_i], musa_asm_binary, musa_al) == 0)
          musa_asm_starts++;
        if (musa_i + musa_el <= dat->size &&
            memcmp (&dat->chain[musa_i], end, musa_el) == 0)
          musa_ends++;
      }
    fprintf (stderr,
             "MUSACAD_PUBLIC_ACDS_MARKERS bytes=%zu starts=%zu asm_starts=%zu ends=%zu\n",
             (size_t)dat->size, musa_starts, musa_asm_starts, musa_ends);
    num_acis_sab_data = 0;
]=])
    string(FIND "${_musa_acds_text}" "${_musa_acds_old}" _musa_acds_pos)
    string(FIND "${_musa_acds_text}" "MUSACAD_PUBLIC_ACDS_MARKERS" _musa_acds_done)
    if(NOT _musa_acds_pos EQUAL -1)
      string(REPLACE "${_musa_acds_old}" "${_musa_acds_probe}"
        _musa_acds_patched "${_musa_acds_text}")
      file(WRITE "${_musa_acds_spec}" "${_musa_acds_patched}")
    elseif(_musa_acds_done EQUAL -1)
      message(FATAL_ERROR "Pinned AcDs SAB scanner changed; host-only audit unavailable")
    endif()
  endif()
endif()

add_subdirectory("${LIBREDWG_SOURCE}" "libredwg")
if(ANDROID)
  add_library(musacad_dwg SHARED dwg_bridge.c native_scene.c)
  set(MUSA_TARGET musacad_dwg)
  target_link_options(musacad_dwg PRIVATE "-Wl,-z,max-page-size=16384")
else()
  add_executable(musacad_dwg_convert dwg_bridge.c native_scene.c)
  set(MUSA_TARGET musacad_dwg_convert)
  target_compile_definitions(musacad_dwg_convert PRIVATE MUSA_HOST=1)
endif()
target_include_directories(${MUSA_TARGET} PRIVATE "${LIBREDWG_SOURCE}/include" "${LIBREDWG_SOURCE}/src" "${CMAKE_CURRENT_BINARY_DIR}/libredwg/src")
target_link_libraries(${MUSA_TARGET} PRIVATE redwg m)
# Separate host-only read-only fixture analyzer. Never shipped in the APK.
if(NOT ANDROID)
  add_executable(musacad_sab_audit "${CMAKE_CURRENT_LIST_DIR}/../../../../tests/native_acis_sab_audit.c")
  target_include_directories(musacad_sab_audit PRIVATE "${LIBREDWG_SOURCE}/include" "${LIBREDWG_SOURCE}/src" "${CMAKE_CURRENT_BINARY_DIR}/libredwg/src")
  target_link_libraries(musacad_sab_audit PRIVATE redwg m)
endif()
