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

    def test_malformed_polyline_stops_before_remaining_vertices(self):
        # A bad source coordinate must stop the expensive vertex walk; do
        # not keep mapping millions of points after the scene is rejected.
        for kind in ("LWPOLYLINE", "POLYLINE_2D", "POLYLINE_3D",
                     "SPLINE", "HATCH"):
            body = branch(self.native, kind)
            self.assertIn("&&!s->truncated", body, kind)
        spline = branch(self.native, "SPLINE")
        # The previous control-/fit-point walk was intentionally replaced by
        # true NURBS sampling. Preserve abort-on-truncation without requiring
        # the old incorrect angular geometry to be reintroduced.
        self.assertIn("i<n&&!s->truncated;i++)", spline)
        self.assertIn("j<=MUSA_CORE_NURBS_SAMPLES_PER_KNOT_SPAN&&!s->truncated", spline)
        self.assertIn("musa_core_nurbs_evaluate_validated(points,n,e->knots,degree,u,&x,&y)", spline)
        self.assertIn("musa_core_nurbs_validate(points,n,e->knots", spline)
        self.assertIn("if(!musa_core_nurbs_evaluate_validated", spline)
        self.assertLess(spline.index("musa_core_nurbs_validate(points,n,e->knots"),
                        spline.index("musa_core_nurbs_evaluate_validated(points,n"))
        self.assertIn("s->truncated=1;break;", spline)
        self.assertNotIn("emit_poly_point(s,parent,e->fit_pts", spline)
        self.assertNotIn("emit_poly_point(s,parent,e->ctrl_pts", spline)
        self.assertNotIn("if(!finite2(ax,ay)){ax=0;ay=0;}", self.native)

    def test_fit_only_spline_never_invents_a_control_polygon(self):
        spline = branch(self.native, "SPLINE")
        self.assertIn("degree==1&&!e->closed_b", spline)
        self.assertIn("e->num_fit_pts==2&&e->fit_pts", spline)
        self.assertIn("emit_line(s,color,parent,e->fit_pts[0].x", spline)
        # A fit-point spline of unknown higher degree must be flagged as
        # partial rather than turned into angular or incorrect geometry.
        self.assertIn("s->truncated=1;break;", spline)
        self.assertNotIn("emit_poly_point(s,parent,e->fit_pts", spline)

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
