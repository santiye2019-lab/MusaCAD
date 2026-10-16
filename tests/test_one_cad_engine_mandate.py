"""Mandatory migration-inventory and single-engine architecture checks.

Does not mislabel legacy Java engines as ported, or C compile success as iPhone app.
"""
import json
import re
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[1]
MANIFEST = ROOT / "docs/UNIFIED_CAD_ENGINE_INVENTORY.json"
JAVA = ROOT / "app/src/main/java/com/musa/cad"
CORE = ROOT / "core/native"


class OneCadEngineMandateTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.data = json.loads(MANIFEST.read_text(encoding="utf-8"))

    def test_owner_instruction_persists_in_repo_for_future_work(self):
        agents = (ROOT / "AGENTS.md").read_text(encoding="utf-8")
        policy = (ROOT / "docs/UNIFIED_CAD_ENGINE_POLICY.md").read_text(encoding="utf-8")
        for expected in ["Android", "Windows", "iOS", "C ABI", "ALL CAD", "If ANY gate fails"]:
            self.assertIn(expected, agents)
        for expected in ["EXTRUDE", "SWEEP", "REVOLVE", "DWG", "DXF", "undo/redo"]:
            self.assertIn(expected, policy)
        self.assertIn("No APK", agents)

    def test_unmigrated_modules_not_misrepresented_as_complete(self):
        data = self.data
        self.assertFalse(data["all_cad_engine_functions_shared"])
        self.assertFalse(data["all_platform_apps_complete"])
        self.assertEqual(data["certified_autocad_2026_families"], 0)
        self.assertEqual(data["certified_family_denominator"], 51)
        entries = data["java_modules"]
        self.assertEqual(data["java_candidate_count"], len(entries))
        self.assertEqual(len({r["path"] for r in entries}), len(entries))
        for row in entries:
            self.assertFalse(row["migrated_verified"], row["path"])
            self.assertIn(row["classification"],
                          {"needs_core_audit", "platform_adapter_candidate"})
        for critical in ["DxfWriter.java", "DxfParser.java", "CadPrismBrep.java",
                         "CadRevolveShell.java", "CadUcsFrame.java", "CadGizmoMath.java"]:
            item = next((x for x in entries if x["path"].endswith("/" + critical)), None)
            self.assertIsNotNone(item, critical)
            self.assertEqual(item["classification"], "needs_core_audit")

    def test_no_new_untracked_java_cad_class_without_adding_explicit_review(self):
        manifest = self.data
        prefixes = tuple(manifest["candidate_detection_prefixes"])
        actual = {
            "app/src/main/java/com/musa/cad/" + path.name
            for path in JAVA.glob("*.java") if path.name.startswith(prefixes)
        }
        tracked = {row["path"] for row in manifest["java_modules"]}
        self.assertEqual(tracked, actual,
                         "Every added/removed CAD-related Java class must be explicitly audited")
        self.assertGreaterEqual(len(tracked), 100)

    def test_core_remains_portable_and_c_abi_stable(self):
        api = (CORE / "include/musacad_core.h").read_text(encoding="utf-8")
        android = (ROOT / "app/src/main/cpp/CMakeLists.txt").read_text(encoding="utf-8")
        self.assertIn("MUSA_CORE_ABI_VERSION 1u", api)
        self.assertIn("musacad_core", android)
        self.assertIn("target_link_libraries(${MUSA_TARGET} PRIVATE redwg m musacad_core)", android)
        forbidden = re.compile(
            r'^\s*#\s*include\s*[<"](?:jni\.h|android/|windows\.h|UIKit/|Foundation/|Cocoa/)',
            re.I | re.M
        )
        for path in list((CORE / "src").glob("*.c")) + list((CORE / "include").glob("*.h")):
            with self.subTest(source=path.name):
                self.assertIsNone(forbidden.search(path.read_text(encoding="utf-8")))

    def test_mandate_and_cross_platform_gates_cannot_disappear(self):
        portability = (ROOT / ".github/workflows/cad-crossplatform-core.yml").read_text(encoding="utf-8")
        gate = (ROOT / ".github/workflows/cad-one-core-mandate.yml").read_text(encoding="utf-8")
        self.assertIn("os: [ubuntu-latest, windows-latest, macos-latest]", portability)
        self.assertIn("iphoneos clang -std=c11 -arch arm64", portability)
        self.assertIn("test_one_cad_engine_mandate.py", gate)
        self.assertIn("'app/src/main/java/com/musa/cad/**'", gate)
        self.assertIn("'core/native/**'", gate)
        self.assertIn("'AGENTS.md'", gate)
        self.assertNotIn("assembleDirectDebug", gate)
        self.assertNotIn("release_approved: true", gate)


if __name__ == "__main__":
    unittest.main()
