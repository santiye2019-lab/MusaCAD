"""Shared C11 affine geometry must be the only native DWG BLOCK/MINSERT transform."""
from pathlib import Path
import unittest

ROOT=Path(__file__).resolve().parents[1]
SCENE=ROOT/"app/src/main/cpp/native_scene.c"
HEADER=ROOT/"core/native/include/musacad_affine.h"

class SharedAffineWiringTests(unittest.TestCase):
    def test_native_scene_delegates_instead_of_duplicating(self):
        scene=SCENE.read_text(encoding="utf-8")
        core=HEADER.read_text(encoding="utf-8")
        self.assertIn('#include "musacad_affine.h"',scene)
        self.assertIn("typedef MusaCoreAffine2 Affine2;",scene)
        self.assertIn("return musa_core_affine_multiply(p,q);",scene)
        self.assertIn("musa_core_affine_map(m,x,y,ox,oy);",scene)
        self.assertIn("return musa_core_affine_identity();",scene)
        self.assertNotIn("r.a=p.a*q.a+p.c*q.b",scene)
        self.assertIn("r.a=p.a*q.a+p.c*q.b",core)
        self.assertIn("musa_core_affine_inverse(",core)
        self.assertIn("Affine2 combined=multiply2(parent,local);",scene)
        self.assertIn("map2(parent,",scene)

if __name__ == "__main__":
    unittest.main()
