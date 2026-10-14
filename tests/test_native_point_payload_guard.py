"""Fail the build if native POINT dereferences its payload before guarding it.

This is a source contract, not a substitute for corrupt-DWG sanitizer runs.
"""
import re
import unittest
from pathlib import Path

SCENE = Path("app/src/main/cpp/native_scene.c")


class NativePointPayloadGuardTest(unittest.TestCase):
    def test_point_payload_checked_before_coordinate_access(self):
        source = SCENE.read_text(encoding="utf-8")
        match = re.search(
            r"case\s+DWG_TYPE_POINT\s*:\s*\{(?P<body>.*?)\n\s*\}",
            source,
            re.DOTALL,
        )
        self.assertIsNotNone(match, "native POINT emission branch missing")
        body = match.group("body")
        declaration = body.find("Dwg_Entity_POINT *e=")
        guard = body.find("if(!e)break;")
        self.assertGreaterEqual(declaration, 0)
        self.assertGreater(guard, declaration, "missing null payload guard")
        accesses = [m.start() for m in re.finditer(r"\be->(?:x|y|z|extrusion)\b", body)]
        self.assertTrue(accesses, "POINT branch no longer decodes coordinates")
        self.assertTrue(all(guard < pos for pos in accesses),
                        "POINT coordinates dereferenced before payload guard")


if __name__ == "__main__":
    unittest.main()
