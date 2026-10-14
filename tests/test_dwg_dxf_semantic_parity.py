"""Strict future-AC1032 DXF-level semantic acceptance regression.

These are synthetic DXFs, NOT evidence of a working AC1032 DWG encoder.
"""
import sys
import tempfile
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
from dwg_dxf_semantic_parity import compare, fingerprint, DxfParityError


def pair(code, value):
    return f"{code}\n{value}\n"


def drawing(entities, blocks="", tables=""):
    pieces = []
    if tables:
        pieces.extend([pair(0, "SECTION"), pair(2, "TABLES"), tables,
                       pair(0, "ENDSEC")])
    if blocks:
        pieces.extend([pair(0, "SECTION"), pair(2, "BLOCKS"), blocks,
                       pair(0, "ENDSEC")])
    pieces.extend([pair(0, "SECTION"), pair(2, "ENTITIES"), entities,
                   pair(0, "ENDSEC"), pair(0, "EOF")])
    return "".join(pieces)


def line(handle="A1", x="10.0", y="20", color="7", layer="0"):
    return "".join([
        pair(0, "LINE"), pair(5, handle), pair(8, layer), pair(62, color),
        pair(10, x), pair(20, y), pair(11, "45"), pair(21, "50")
    ])


def block(name="PUMP"):
    return (pair(0, "BLOCK") + pair(2, name) + pair(70, 0) +
            pair(0, "ENDBLK"))


def insert(name="PUMP"):
    return (pair(0, "INSERT") + pair(2, name) + pair(8, "MECHANICAL") +
            pair(10, 5) + pair(20, 6))


class DxfSemanticParityTest(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory()
        self.addCleanup(self.tmp.cleanup)
        self.before = Path(self.tmp.name) / "approved.dxf"
        self.after = Path(self.tmp.name) / "reopened.dxf"

    def check(self, original, reopened, expected):
        self.before.write_text(original, encoding="utf-8")
        self.after.write_text(reopened, encoding="utf-8")
        actual = compare(self.before, self.after)
        self.assertIs(actual["pass"], expected, actual)
        return actual

    def test_renumbered_numeric_text_but_same_geometry_passes(self):
        orig = drawing(line(x="10.0", y="20.000"))
        output = drawing(line(x="10.00000", y="2.0e1"))
        self.check(orig, output, True)

    def test_renumbered_entities_keep_order_independent(self):
        a, b = line("A1"), line("A2", x="99", color="1")
        self.check(drawing(a + b), drawing(b + a), True)

    def test_internal_geometry_drift_same_type_and_count_fails(self):
        result = self.check(drawing(line()), drawing(line(x="10.125")), False)
        self.assertEqual(result["missing_records"], 1)
        self.assertEqual(result["extra_records"], 1)

    def test_color_attribute_and_layer_loss_detected(self):
        self.check(drawing(line()), drawing(line(color="1")), False)
        self.check(drawing(line()), drawing(line(layer="PIPES")), False)
        self.check(drawing(line()), drawing(line() + pair(420, 0x123456)), False)

    def test_handle_drift_detected(self):
        self.check(drawing(line("A1")), drawing(line("A2")), False)

    def test_text_loss_detected_without_exposing_text(self):
        source = drawing(pair(0, "TEXT") + pair(5, "2") +
                         pair(1, "Mechanical equipment") + pair(10, 2))
        reopened = source.replace("Mechanical equipment", "Wrong equipment")
        result = self.check(source, reopened, False)
        self.assertNotIn("Mechanical equipment", str(result))

    def test_blocks_xref_and_inserts_are_not_just_entity_counts(self):
        orig = drawing(insert(), blocks=block())
        self.check(orig, drawing(insert("VALVE"), blocks=block()), False)
        self.check(orig, drawing(insert(), blocks=block("VALVE")), False)
        self.check(orig, orig, True)

    def test_layer_table_inheritance_must_match(self):
        table = pair(0, "TABLE") + pair(2, "LAYER")
        record = pair(0, "LAYER") + pair(2, "PIPES") + pair(62, 7)
        end = pair(0, "ENDTAB")
        src = drawing(line(), tables=table + record + end)
        modified = drawing(line(), tables=table +
                           record.replace(pair(62, 7), pair(62, 1)) + end)
        self.check(src, modified, False)

    def test_per_vertex_bulge_and_width_must_match(self):
        base = (pair(0, "LWPOLYLINE") + pair(5, "3") + pair(90, 2) +
                pair(10, 0) + pair(20, 0) + pair(42, "0.25") +
                pair(40, 1) + pair(10, 5) + pair(20, 5))
        self.check(drawing(base), drawing(base.replace("0.25", "0.5")), False)

    def test_unknown_acis_and_proxy_objects_fail_closed(self):
        for unsupported in ("3DSOLID", "BODY", "REGION", "ACAD_PROXY_ENTITY"):
            data = drawing(pair(0, unsupported) + pair(5, "2") +
                           pair(1, "opaque payload"))
            result = self.check(data, data, False)
            self.assertEqual(result["unverifiable_entities"], 2)

    def test_corrupted_and_unclosed_dxf_refused(self):
        good = drawing(line())
        cases = (good[:-6], good + pair(0, "POINT"),
                 good.replace(pair(0, "ENDSEC"), "", 1),
                 good.replace(pair(10, "10.0"), pair(10, "NaN")))
        for case in cases:
            with self.subTest(case=case[-25:]):
                self.after.write_text(case, encoding="utf-8")
                with self.assertRaises(DxfParityError):
                    fingerprint(self.after)

    def test_missing_or_empty_dxf_refused(self):
        with self.assertRaises(DxfParityError):
            fingerprint(self.before)
        self.before.write_text("", encoding="utf-8")
        with self.assertRaises(DxfParityError):
            fingerprint(self.before)


if __name__ == "__main__":
    unittest.main()
