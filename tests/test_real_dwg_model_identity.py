"""Real-DWG fixture comparator must reject same-count but wrong-identity data."""
import tempfile
import unittest
from pathlib import Path

from real_dwg_model_identity import (
    normalize, parse_native_export, parse_dxf, compare_records,
    validate_expected_coverage, compare_three_way, compare_geometry,
)


def simple_dxf(records):
    return ("0\nSECTION\n2\nENTITIES\n" + records +
            "0\nENDSEC\n0\nEOF\n")


class RealDwgIdentityTest(unittest.TestCase):
    def test_different_handle_same_entity_count_is_a_failure(self):
        native = {"A": "LINE", "B": "LINE"}
        target = {"A": "LINE", "C": "LINE"}
        with self.assertRaisesRegex(ValueError, "mismatch"):
            compare_records(native, target)

    def test_three_way_rejects_synchronized_loss_of_original_entity(self):
        initial = {"A1": "LINE", "B2": "3DFACE"}
        # Reader and DXF re-export agree but BOTH dropped source face.
        native = {"A1": "LINE"}
        exported = {"A1": "LINE"}
        self.assertEqual(compare_records(native, exported), 1)
        with self.assertRaisesRegex(ValueError, "initial DXF to binary"):
            compare_three_way(initial, native, exported)
        self.assertEqual(compare_three_way(initial, dict(initial), dict(initial)), 2)

    def test_three_way_rejects_changed_handle_with_equal_counts(self):
        original = {"A1": "LINE", "B2": "3DFACE"}
        wrong = {"A1": "LINE", "C2": "3DFACE"}
        with self.assertRaisesRegex(ValueError, "initial DXF"):
            compare_three_way(original, wrong, wrong)
        with self.assertRaisesRegex(ValueError, "mismatch"):
            compare_three_way(original, original, wrong)


    def test_three_way_geometry_catches_changed_line_endpoint(self):
        """Handles/types stay equal but a real engineering coordinate moves."""
        with tempfile.TemporaryDirectory() as t:
            p = Path(t) / "original.dxf"
            p.write_text(simple_dxf(
                "0\\nLINE\\n5\\nA1\\n10\\n1.25\\n20\\n2.5\\n30\\n0\\n"
                "11\\n3.75\\n21\\n4.5\\n31\\n5\\n"), encoding="utf-8")
            initial, source_geom = parse_dxf(p, with_geometry=True)
            p.write_text(simple_dxf(
                "0\\nLINE\\n5\\nA1\\n10\\n1.25\\n20\\n2.5\\n30\\n0\\n"
                "11\\n3.7501\\n21\\n4.5\\n31\\n5\\n"), encoding="utf-8")
            exported, changed_geom = parse_dxf(p, with_geometry=True)
            self.assertEqual(initial, exported)
            with self.assertRaisesRegex(ValueError, "coordinates differ"):
                compare_geometry(source_geom, changed_geom)

    def test_three_way_geometry_accepts_roundoff_and_triangle_default(self):
        with tempfile.TemporaryDirectory() as t:
            p = Path(t) / "face.dxf"
            p.write_text(simple_dxf(
                "0\\n3DFACE\\n5\\nB2\\n10\\n0\\n20\\n0\\n30\\n0\\n"
                "11\\n2\\n21\\n0\\n31\\n0\\n12\\n0\\n22\\n3\\n32\\n4\\n"),
                encoding="utf-8")
            identities, source = parse_dxf(p, with_geometry=True)
            self.assertEqual(identities, {"B2": "3DFACE"})
            p.write_text(simple_dxf(
                "0\\n3DFACE\\n5\\nB2\\n10\\n0\\n20\\n0\\n30\\n0\\n"
                "11\\n2\\n21\\n0\\n31\\n0\\n12\\n0\\n22\\n3\\n32\\n4\\n"
                "13\\n0\\n23\\n3\\n33\\n4.0000000001\\n"),
                encoding="utf-8")
            _, rounded = parse_dxf(p, with_geometry=True)
            self.assertEqual(compare_geometry(source, rounded), 1)

    def test_three_way_geometry_rejects_nonfinite_duplicate_or_missing_xy(self):
        with tempfile.TemporaryDirectory() as t:
            p = Path(t) / "malformed.dxf"
            for rec in (
                "0\\nLINE\\n5\\nA1\\n10\\nNaN\\n20\\n1\\n11\\n2\\n21\\n3\\n",
                "0\\nLINE\\n5\\nA1\\n10\\n0\\n10\\n1\\n20\\n1\\n11\\n2\\n21\\n3\\n",
                "0\\n3DFACE\\n5\\nA1\\n10\\n0\\n20\\n0\\n11\\n2\\n21\\n0\\n12\\n1\\n22\\n1\\n13\\n1\\n",
            ):
                p.write_text(simple_dxf(rec), encoding="utf-8")
                with self.assertRaises(ValueError):
                    parse_dxf(p, with_geometry=True)

    def test_fixture_coverage_requires_real_3d_faces(self):
        native = {"A": "LINE", "B": "3DFACE", "C": "3DFACE"}
        self.assertEqual(validate_expected_coverage(native, 3, 2), 3)
        with self.assertRaisesRegex(ValueError, "coverage too small"):
            validate_expected_coverage(native, 4, 2)
        with self.assertRaisesRegex(ValueError, "required 3DFACE"):
            validate_expected_coverage(native, 3, 3)
        with self.assertRaises(ValueError):
            validate_expected_coverage(native, 0, 0)

    def test_wrong_type_is_a_failure_and_no_empty_pass(self):
        with self.assertRaises(ValueError):
            compare_records({"A": "LINE"}, {"A": "3DFACE"})
        for a, b in (({}, {}), ({"A": "LINE"}, {}), ({}, {"A": "LINE"})):
            with self.assertRaises(ValueError):
                compare_records(a, b)
        self.assertEqual(compare_records({"A": "LINE"}, {"A": "LINE"}), 1)

    def test_original_model_handles_are_unsigned_and_unique(self):
        with tempfile.TemporaryDirectory() as t:
            p = Path(t) / "native.txt"
            p.write_text("F000000000000001\tLINE\n000A1\t3DFACE\n",
                         encoding="ascii")
            self.assertEqual(parse_native_export(p),
                             {"F000000000000001": "LINE", "A1": "3DFACE"})
            for corrupt in (
                "A\tLINE\n0A\tLINE\n",
                "0\tLINE\n",
                "XYZ\tLINE\n",
                "A\tMYSTERY\n",
                "A\tLINE\nA\tLINE\n",
            ):
                p.write_text(corrupt, encoding="ascii")
                with self.assertRaises(ValueError):
                    parse_native_export(p)
        self.assertEqual(normalize("f000000000000001"), "F000000000000001")

    def test_dxf_direct_model_handle_and_type(self):
        with tempfile.TemporaryDirectory() as t:
            p = Path(t) / "intermediate.dxf"
            p.write_text(simple_dxf(
                "0\nLINE\n5\n000A\n10\n0\n20\n0\n11\n1\n21\n0\n" +
                "0\n3DFACE\n5\nFF\n10\n0\n20\n0\n" +
                "0\nLINE\n5\nE0\n67\n1\n10\n0\n20\n0\n"),
                encoding="utf-8")
            self.assertEqual(parse_dxf(p), {"A": "LINE", "FF": "3DFACE"})
            p.write_text(simple_dxf(
                "0\nLINE\n5\nA\n" +
                "0\nLINE\n5\n000a\n"), encoding="utf-8")
            with self.assertRaises(ValueError):
                parse_dxf(p)
            p.write_text(simple_dxf("0\nLINE\n10\n0\n"), encoding="utf-8")
            with self.assertRaises(ValueError):
                parse_dxf(p)


    def test_header_can_contain_more_than_one_code_2_value(self):
        with tempfile.TemporaryDirectory() as t:
            p = Path(t) / "header_fixture.dxf"
            p.write_text(
                "0\nSECTION\n2\nHEADER\n9\n$CUSTOM\n2\nextra-value\n"
                "9\n$SECOND\n2\nextra-value-2\n0\nENDSEC\n"
                "0\nSECTION\n2\nENTITIES\n"
                "0\nLINE\n5\nA1\n10\n0\n20\n0\n"
                "11\n2\n21\n1\n0\nENDSEC\n0\nEOF\n",
                encoding="utf-8"
            )
            self.assertEqual(parse_dxf(p), {"A1": "LINE"})
            p.write_text("0\nSECTION\n9\n$BAD\n2\nHEADER\n"
                         "0\nENDSEC\n0\nEOF\n", encoding="utf-8")
            with self.assertRaises(ValueError):
                parse_dxf(p)


if __name__ == "__main__":
    unittest.main()
