"""Deterministic source diff admission policy for all-platform common CAD motor."""
import importlib.util
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location(
    "core_migration_diff", ROOT / "tests/check_core_migration_diff.py")
module = importlib.util.module_from_spec(spec)
spec.loader.exec_module(module)


class EngineDiffGateTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        import json
        cls.inventory = json.loads(
            (ROOT / "docs/UNIFIED_CAD_ENGINE_INVENTORY.json").read_text(encoding="utf-8"))

    def test_java_only_cad_calculation_patch_is_rejected(self):
        changed = ["app/src/main/java/com/musa/cad/CadRevolveShell.java",
                   "tests/CadRevolveShellTest.java"]
        self.assertEqual(len(module.violations(changed, self.inventory)), 2)

    def test_cad_math_change_must_add_portable_c_implementation_and_test(self):
        changed = ["app/src/main/java/com/musa/cad/DxfParser.java",
                   "core/native/src/musacad_core.c"]
        self.assertEqual(len(module.violations(changed, self.inventory)), 1)
        changed += ["core/native/tests/core_api_test.c"]
        self.assertEqual(module.violations(changed, self.inventory), [])

    def test_android_view_and_bridge_changes_are_platform_adapters(self):
        changed = ["app/src/main/java/com/musa/cad/Mesh3dActivity.java",
                   "app/src/main/java/com/musa/cad/CadView.java",
                   "app/src/main/java/com/musa/cad/MainActivity.java"]
        self.assertEqual(module.violations(changed, self.inventory), [])

    def test_new_native_test_alone_is_not_an_engine_duplicate(self):
        self.assertEqual(module.violations(
            ["core/native/tests/core_mesh_topology_test.c"], self.inventory), [])


if __name__ == "__main__":
    unittest.main()
