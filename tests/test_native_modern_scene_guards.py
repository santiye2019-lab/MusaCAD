"""Guard modern native DWG preview from null payloads and partial display.

Source-level checks catch accidental reordering before native host builds.
The pinned LibreDWG smoke suite supplies the actual compilation/fixture gate.
"""
import re
import unittest
from pathlib import Path

SRC = Path("app/src/main/cpp/native_scene.c")
VIEW = Path("app/src/main/java/com/musa/cad/CadView.java")


def branch(source, entity):
    start = source.index("case DWG_TYPE_" + entity + ":{")
    following = source.find("\n        case DWG_TYPE_", start + 1)
    return source[start:following if following >= 0 else len(source)]


class ModernSceneGuardTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.native = SRC.read_text(encoding="utf-8")
        cls.view = VIEW.read_text(encoding="utf-8")

    def test_multileader_checks_outer_and_nested_buffers(self):
        body = branch(self.native, "MULTILEADER")
        self.assertLess(body.index("if(ctx->num_leaders>0&&!ctx->leaders)"),
                        body.index("ctx->leaders[li]"))
        self.assertLess(body.index("if(node->num_lines>0&&!node->lines)"),
                        body.index("node->lines[lj]"))
        self.assertIn("s->truncated=1;break;", body)
        self.assertIn("if(s->truncated)break;", body)

    def test_hatch_never_dereferences_missing_boundary(self):
        body = branch(self.native, "HATCH")
        guard = body.index("(path->flag&2)?!path->polyline_paths:!path->segs")
        use = body.index("path->polyline_paths[i].point")
        self.assertLess(guard, use)
        self.assertIn("s->truncated=1;break;", body)

    def test_partial_native_first_frame_warns_user(self):
        # A partially parsed scene must not silently look like the full DWG.
        branch_start = self.view.index("}else if(nativeDrawing!=null){")
        next_branch = self.view.index("}else if(drawing!=null)", branch_start)
        body = self.view[branch_start:next_branch]
        self.assertIn("if(nativeDrawing.truncated)", body)
        self.assertIn("DWG hızlı önizleme kısmi", body)
        self.assertIn("nativeDrawing.draw(c,imageMatrix,fastNavigation)", body)


if __name__ == "__main__":
    unittest.main()
