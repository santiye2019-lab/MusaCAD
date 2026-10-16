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
        self.assertIn("target_link_libraries(${MUSA_TARGET} PRIVATE redwg m musacad_core)", adapter)
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
