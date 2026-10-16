"""Real-DWG fixture comparator must reject same-count but wrong-identity data."""
import tempfile
import unittest
from pathlib import Path

from real_dwg_model_identity import (
    normalize, parse_native_export, parse_dxf, compare_records,
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
