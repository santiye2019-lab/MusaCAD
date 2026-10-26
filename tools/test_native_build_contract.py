#!/usr/bin/env python3
"""Regression checks for MusaCAD's isolated Linux host and Android JNI builds."""
from pathlib import Path
import re
import unittest

ROOT = Path(__file__).resolve().parents[1]


def source(path):
    return (ROOT / path).read_text(encoding="utf-8")


class NativeBuildContractTests(unittest.TestCase):
    def test_android_and_host_targets_are_strictly_separate(self):
        cmake = source("app/src/main/cpp/CMakeLists.txt")
        self.assertIn("if(ANDROID)\n", cmake)
        android_and_rest = cmake.split("if(ANDROID)\n", 1)[1]
        android, host_and_rest = android_and_rest.split("\nelse()\n", 1)
        host = host_and_rest.split("\nendif()\n", 1)[0]
        self.assertIn("add_library(musacad_dwg SHARED dwg_bridge.c native_scene.c)", android)
        self.assertIn("target_link_options(musacad_dwg PRIVATE \"-Wl,-z,max-page-size=16384\")", android)
        self.assertNotIn("MUSA_HOST=1", android)
        self.assertIn("add_executable(musacad_dwg_convert dwg_bridge.c native_scene.c)", host)
        self.assertIn("target_compile_definitions(musacad_dwg_convert PRIVATE MUSA_HOST=1)", host)
        self.assertNotIn("max-page-size=16384", host)
        self.assertNotIn("add_library(musacad_dwg", host)

    def test_libredwg_is_pinned_and_host_friendly(self):
        cmake = source("app/src/main/cpp/CMakeLists.txt")
        for key in ("LIBREDWG_LIBONLY", "LIBREDWG_DISABLE_WRITE",
                    "LIBREDWG_DISABLE_JSON", "DISABLE_WERROR", "ENABLE_LTO"):
            self.assertIn(key, cmake)
        self.assertIn('target_link_libraries(${MUSA_TARGET} PRIVATE redwg m)', cmake)
        pinned = "34f02f54b9aacb5708c1d3d2070efb3e4b2d8c43"
        for path in (".github/workflows/android.yml",
                     ".github/workflows/native-cad-regression.yml"):
            self.assertIn(pinned, source(path), path)

    def test_android_ndk_and_abis_are_not_reconfigured(self):
        gradle = source("app/build.gradle")
        self.assertIn('ndkVersion "27.2.12479018"', gradle)
        self.assertIn('version "3.22.1"', gradle)
        self.assertRegex(gradle, r'abiFilters\s+"arm64-v8a",\s*"x86_64"')

    def test_native_regressions_run_in_existing_android_gate(self):
        android = source(".github/workflows/android.yml")
        self.assertIn("Build and smoke-test native converter", android)
        self.assertIn("test_native_build_contract.py", android)
        self.assertIn("tests/native_color_test.c", android)
        self.assertIn("third_party/libredwg/test/test-data/2000/TS1.dwg", android)
        apk_step = android.split("- name: Build approved installable MusaCAD APK", 1)[1]
        apk_step = apk_step.split("\n      - name:", 1)[0]
        self.assertIn("github.event_name == 'workflow_dispatch'", apk_step)
        self.assertIn("inputs.release_approved == true", apk_step)

    def test_dedicated_workflow_cannot_ship_an_apk(self):
        workflow = source(".github/workflows/native-cad-regression.yml")
        self.assertIn("pull_request:", workflow)
        self.assertIn("workflow_dispatch:", workflow)
        self.assertIn("Debug", workflow)
        self.assertIn("Release", workflow)
        self.assertIn("native-dxf-regression.py", workflow)
        self.assertNotIn("assembleDirect", workflow)
        self.assertNotIn("wrangler deploy", workflow)
        self.assertNotIn("upload-artifact", workflow)


if __name__ == "__main__":
    unittest.main()
