"""Fail-closed source contracts for MusaCAD's single portable CAD math engine.

This test guards source topology and CI triggers, not iPhone/Windows UI acceptance.
"""
from pathlib import Path
import re
import unittest

ROOT = Path(__file__).resolve().parents[1]
CORE = ROOT / "core/native"
CMAKE = CORE / "CMakeLists.txt"
ANDROID_CMAKE = ROOT / "app/src/main/cpp/CMakeLists.txt"
CROSS = ROOT / ".github/workflows/cad-crossplatform-core.yml"


class SharedCorePortabilityContract(unittest.TestCase):
    def test_single_c11_target_is_linked_into_android(self):
        core = CMAKE.read_text(encoding="utf-8")
        adapter = ANDROID_CMAKE.read_text(encoding="utf-8")
        self.assertIn("add_library(musacad_core STATIC", core)
        self.assertIn("target_compile_features(musacad_core PUBLIC c_std_11)", core)
        self.assertIn('add_subdirectory("${CMAKE_CURRENT_LIST_DIR}/../../../../core/native"', adapter)
        android_target = adapter.split("if(ANDROID)", 1)[1].split("return()", 1)[0]
        android_target = "\n".join(line.split("#", 1)[0] for line in android_target.splitlines())
        self.assertIn("target_link_libraries(musacad_dwg PRIVATE m musacad_core)", android_target)
        self.assertIn("firstparty_dwg_unavailable_jni.c", android_target)
        self.assertNotIn("redwg", android_target.lower())
        self.assertNotIn("dwg_bridge.c", android_target)
        self.assertNotIn("native_scene.c", android_target)
        abi = (CORE / "include/musacad_core.h").read_text(encoding="utf-8")
        self.assertIn("#define MUSA_CORE_ABI_VERSION 1u", abi)
        self.assertIn('extern "C"', abi)
        self.assertIn("unsigned int musa_core_abi_version(void);", abi)

    def test_core_never_imports_platform_sdk_headers(self):
        banned = re.compile(
            r'^\s*#\s*include\s*[<"](?:jni\.h|android/|android\.h|windows\.h|'
            r'winbase\.h|UIKit/|Foundation/|Cocoa/|objc/|swift/|OpenGL/|GLES/)',
            re.IGNORECASE | re.MULTILINE
        )
        inputs = list((CORE / "src").rglob("*.c")) + list((CORE / "include").rglob("*.h"))
        self.assertGreaterEqual(len(inputs), 12)
        for path in inputs:
            with self.subTest(file=str(path.relative_to(ROOT))):
                self.assertIsNone(banned.search(path.read_text(encoding="utf-8")))

    def test_ios_checks_every_registered_c_unit_and_three_host_operating_systems(self):
        workflow = CROSS.read_text(encoding="utf-8")
        self.assertIn("os: [ubuntu-latest, windows-latest, macos-latest]", workflow)
        self.assertIn("ctest --test-dir build/shared-core", workflow)
        self.assertIn("ctest --test-dir build/shared-affine", workflow)
        self.assertIn("xcrun --sdk iphoneos clang -std=c11 -arch arm64", workflow)
        self.assertIn('if set(sources) != tracked:', workflow)
        self.assertIn('while IFS= read -r source;', workflow)
        for path in [
            "'core/native/**'",
            "'app/src/main/java/com/musa/cad/**'",
            "'app/src/main/cpp/CMakeLists.txt'",
            "'tests/**'",
        ]:
            with self.subTest(trigger=path):
                self.assertIn(path, workflow)
        self.assertNotIn("assembleDirectDebug", workflow)
        self.assertNotIn("release_approved: true", workflow)

    def test_android_and_ios_share_one_native_core_build_contract(self):
        workflow = CROSS.read_text(encoding="utf-8")
        script = (ROOT / "tests/run_native_crossbuild.sh").read_text(encoding="utf-8")
        self.assertIn("run_native_crossbuild.sh android", workflow)
        self.assertIn("run_native_crossbuild.sh ios-archive", workflow)
        self.assertIn("if: runner.os == 'Linux'", workflow)
        self.assertIn("if: runner.os == 'macOS'", workflow)
        for required in [
            "android.toolchain.cmake", "arm64-v8a x86_64",
            "-DANDROID_ABI=", "-DANDROID_PLATFORM=android-24",
            "-DBUILD_TESTING=OFF", "libmusacad_core.a",
            "musa_bits_read", "musa_core_mesh_mass_verified_f32",
            "iphoneos ar", "iphoneos nm",
        ]:
            with self.subTest(required=required):
                self.assertIn(required, script)
        self.assertNotIn("assembleDirectDebug", workflow)
        self.assertNotIn("release_approved: true", workflow)

    def test_apple_ios_simulator_core_swift_abi_is_required(self):
        workflow = CROSS.read_text(encoding="utf-8")
        script = (ROOT / "tests/run_native_crossbuild.sh").read_text(encoding="utf-8")
        swift = (ROOT / "platforms/ios/MusaCADCoreAdapter.swift").read_text(encoding="utf-8")
        module = (ROOT / "platforms/ios/include/module.modulemap").read_text(encoding="utf-8")
        bridge = (ROOT / "platforms/ios/include/MusaCADCoreBridge.h").read_text(encoding="utf-8")
        link = (ROOT / "tests/apple_core_abi_link_smoke.c").read_text(encoding="utf-8")

        # Apple builds must exercise the real shared engine, not just a
        # fake/dummy ABI or a source-file subset. No iOS app success is implied.
        for required in [
            "'platforms/ios/**'", "ios-simulator-abi:", "runs-on: macos-latest",
            "run_native_crossbuild.sh ios-simulator",
            "swiftc -target arm64-apple-ios15.0",
            "MusaCADCoreAdapter.swift",
        ]:
            with self.subTest(workflow=required):
                self.assertIn(required, workflow)
        for required in [
            "ios-simulator)", "iphonesimulator --show-sdk-path",
            "arm64-apple-ios15.0-simulator", "set(sources) != tracked",
            "core/native/CMakeLists.txt", "iphonesimulator clang",
            "iphonesimulator ar", "iphonesimulator nm", "symbols.txt",
            "apple_core_abi_link_smoke.c", "-typecheck",
            "musa_dwg_ac1024_line_fields_probe",
        ]:
            with self.subTest(script=required):
                self.assertIn(required, script)
        self.assertIn("module MusaCADCore", module)
        self.assertIn('header "MusaCADCoreBridge.h"', module)
        self.assertIn('#include "musacad_core.h"', bridge)
        self.assertIn("import MusaCADCore", swift)
        self.assertIn("musa_core_abi_version()", swift)
        self.assertIn("MUSA_CORE_ABI_VERSION", swift)
        self.assertIn("musa_dwg_ac1024_line_fields_probe", link)
        for forbidden in ["libredwg", "dwgread", "xcodebuild archive", "assembleDirectDebug"]:
            with self.subTest(forbidden=forbidden):
                self.assertNotIn(forbidden, script.lower())

    def test_shared_core_sources_are_explicitly_declared(self):
        cmake = CMAKE.read_text(encoding="utf-8")
        match = re.search(r"add_library\(musacad_core STATIC\s+(.*?)\)", cmake, re.S)
        self.assertIsNotNone(match)
        declared = re.findall(r"src/[A-Za-z0-9_]+\.c", match.group(1))
        on_disk = {
            str(path.relative_to(CORE))
            for path in (CORE / "src").glob("*.c")
        }
        self.assertEqual(set(declared), on_disk, "unregistered portable C code")
        self.assertEqual(len(declared), len(set(declared)), "duplicate C unit")


if __name__ == "__main__":
    unittest.main()
