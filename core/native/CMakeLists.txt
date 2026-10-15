cmake_minimum_required(VERSION 3.22)
project(musacad_core LANGUAGES C)
# Pure-C portable static library: Android NDK, Windows (MSVC), Apple clang.
# The embedded Android DWG integration links exactly this target.
add_library(musacad_core STATIC src/musacad_core.c)
set_target_properties(musacad_core PROPERTIES POSITION_INDEPENDENT_CODE ON)
target_compile_features(musacad_core PUBLIC c_std_11)
target_include_directories(musacad_core PUBLIC "${CMAKE_CURRENT_LIST_DIR}/include")
if(UNIX AND NOT APPLE)
    target_link_libraries(musacad_core PUBLIC m)
endif()
