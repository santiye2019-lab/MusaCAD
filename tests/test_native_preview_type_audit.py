"""Synthetic no-private-DWG tests for native entity omission accounting."""
import re
import tempfile
import unittest
from pathlib import Path

from native_preview_type_audit import audit_types, CONDITIONAL_NATIVE_ENTITY_TYPES
from private_dwg_read_audit import inventory_dxf


def entity(name, *groups):
    return "0\n" + name + "\n" + "".join(
        str(k) + "\n" + str(v) + "\n" for k, v in groups
    )


class NativePreviewTypeAuditTest(unittest.TestCase):
    def test_known_unhandled_are_disclosed_without_false_rendering_claims(self):
        result = audit_types(
            {"LINE": 3, "LWPOLYLINE": 2, "3DFACE": 1,
             "3DSOLID": 4, "ACAD_PROXY_ENTITY": 2, "HATCH": 3,
             "VERTEX": 5, "SEQEND": 1},
            {"INSERT": 2, "REGION": 1, "BODY": 2})
        self.assertEqual(result["native_preview_unsupported_entity_count"], 9)
        self.assertEqual(result["native_preview_unsupported_entity_types"],
                         {"3DSOLID": 4, "ACAD_PROXY_ENTITY": 2,
                          "BODY": 2, "REGION": 1})
        self.assertEqual(result["native_preview_conditional_entity_count"], 11)
        self.assertEqual(result["native_preview_structural_record_count"], 6)
        self.assertEqual(result["native_preview_completeness"],
                         "unsupported_types_detected")
        self.assertIn("NOT independently verified",
                      result["native_preview_type_scope"])

    def test_conditional_never_claims_accurate_native_pixels(self):
        result = audit_types({"LINE": 1, "SPLINE": 2, "DIMENSION": 3}, {})
        self.assertEqual(result["native_preview_unsupported_entity_count"], 0)
        self.assertEqual(result["native_preview_completeness"],
                         "type_dispatch_only_unverified")
        self.assertNotIn("complete", result["native_preview_completeness"])

    def test_unknown_classes_are_counted_without_leaking_names(self):
        result = audit_types(
            {"CUSTOM_BUILDING_FINISH": 2, "SECRET USER LAYER": 3,
             "": 1, "3DFACE": 1},
            {"3DSOLID": 5, "CUSTOM_BUILDING_FINISH": 1})
        self.assertEqual(result["native_preview_unsupported_entity_types"],
                         {"3DSOLID": 5, "OTHER_UNVERIFIED_ENTITY": 7})
        self.assertEqual(result["native_preview_conditional_entity_types"],
                         {"3DFACE": 1})
        report = str(result)
        self.assertNotIn("SECRET", report)
        self.assertNotIn("CUSTOM_BUILDING", report)

    def test_bad_inventory_rejected(self):
        for invalid in (None, [], {"LINE": -1}, {"LINE": "2"},
                        {"LINE": True}, {None: 1}):
            with self.subTest(invalid=invalid):
                with self.assertRaises(ValueError):
                    audit_types(invalid, {})

    def test_native_c_type_dispatch_inventory_is_not_stale(self):
        scene = Path("app/src/main/cpp/native_scene.c").read_text(encoding="utf-8")
        start = scene.index("    SceneColor color=entity_color(obj,byBlock,layerColor);")
        end = scene.index("    if(nested_leaf){", start)
        dispatched = set()
        for name in re.findall(r"case DWG_TYPE_([A-Z0-9_]+)\s*:", scene[start:end]):
            if name.startswith("DIMENSION_") or name in (
                    "ARC_DIMENSION", "LARGE_RADIAL_DIMENSION"):
                dispatched.add("DIMENSION")
            elif name.startswith("POLYLINE_"):
                dispatched.add("POLYLINE")
            elif name == "_3DFACE":
                dispatched.add("3DFACE")
            else:
                dispatched.add(name)
        self.assertEqual(dispatched, CONDITIONAL_NATIVE_ENTITY_TYPES,
                         "Update preview type audit when native C dispatch changes")

    def test_native_first_paint_warns_not_all_entity_types_verified(self):
        activity = Path("app/src/main/java/com/musa/cad/MainActivity.java"
                        ).read_text(encoding="utf-8")
        self.assertIn("Renkler ve tüm nesne türleri henüz doğrulanmadı.",
                      activity)

    def test_actual_synthetic_dxf_inventory_integration(self):
        dxf = (
            entity("SECTION", (2, "BLOCKS")) +
            entity("BLOCK", (2, "SYMBOL"), (70, 0)) +
            entity("3DSOLID", (8, "0")) +
            entity("3DFACE", (8, "0")) +
            entity("ENDBLK") + entity("ENDSEC") +
            entity("SECTION", (2, "ENTITIES")) +
            entity("LINE", (8, "0")) +
            entity("ACAD_PROXY_ENTITY", (8, "0")) +
            entity("SPLINE", (8, "0")) +
            entity("ENDSEC") + entity("EOF")
        )
        with tempfile.TemporaryDirectory(prefix="musa-preview-audit-") as td:
            src = Path(td) / "generated.dxf"
            src.write_text(dxf, encoding="ascii")
            result = inventory_dxf(src)
        self.assertEqual(result["native_preview_unsupported_entity_types"],
                         {"3DSOLID": 1, "ACAD_PROXY_ENTITY": 1})
        self.assertEqual(result["native_preview_conditional_entity_count"], 3)
        self.assertEqual(result["model_entity_count"], 3)
        self.assertEqual(result["block_geometry_entity_count"], 2)
        self.assertEqual(result["native_preview_completeness"],
                         "unsupported_types_detected")


if __name__ == "__main__":
    unittest.main()
