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


def drawing(entities, blocks="", tables="", header=""):
    pieces = []
    if header:
        pieces.extend([pair(0, "SECTION"), pair(2, "HEADER"), header,
                       pair(0, "ENDSEC")])
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

    def test_dwg_header_units_and_dimension_scale_are_not_silently_lost(self):
        # Identical pipe geometry in millimetres vs metres is a critical
        # engineering measurement discrepancy, even with equal layer counts.
        metadata = (pair(9, "$INSUNITS") + pair(70, 4) +
                    pair(9, "$MEASUREMENT") + pair(70, 1) +
                    pair(9, "$LUNITS") + pair(70, 2) +
                    pair(9, "$DIMSCALE") + pair(40, "100.0") +
                    pair(9, "$INSBASE") + pair(10, 0) +
                    pair(20, 0) + pair(30, 0) +
                    pair(9, "$CLAYER") + pair(8, "PIPES"))
        original = drawing(line(), header=metadata)
        self.check(original, original, True)
        for before, after in (
            (pair(70, 4), pair(70, 6)),
            (pair(40, "100.0"), pair(40, "1.0")),
            (pair(8, "PIPES"), pair(8, "MECHANICAL")),
            (pair(30, 0), pair(30, 2)),
        ):
            with self.subTest(change=after):
                result = self.check(
                    original,
                    drawing(line(), header=metadata.replace(before, after, 1)),
                    False)
                self.assertTrue(result["critical_header_mismatch"])

    def test_dwg_header_numeric_format_and_reordered_variable_groups(self):
        one = pair(9, "$DIMSCALE") + pair(40, "1.0000") + (
            pair(9, "$INSUNITS") + pair(70, "4"))
        two = pair(9, "$INSUNITS") + pair(70, "04") + (
            pair(9, "$DIMSCALE") + pair(40, "1e0"))
        self.check(drawing(line(), header=one),
                   drawing(line(), header=two), True)

    def test_dwg_header_variable_removed_or_empty_is_rejected(self):
        one = pair(9, "$INSUNITS") + pair(70, 4)
        self.check(drawing(line(), header=one),
                   drawing(line()), False)
        self.check(drawing(line(), header=one),
                   drawing(line(), header=pair(9, "$LUNITS") + pair(70, 4)),
                   False)
        self.before.write_text(
            drawing(line(), header=pair(9, "$INSUNITS")),
            encoding="utf-8")
        with self.assertRaises(DxfParityError):
            fingerprint(self.before)

    def test_no_decimal_context_rounding_can_hide_coordinate_mutation(self):
        first = "123456789012345678901234567890.0"
        second = "123456789012345678901234567891.0"
        result = self.check(drawing(line(x=first)),
                            drawing(line(x=second)), False)
        self.assertEqual(result["missing_records"], 1)
        self.assertEqual(result["extra_records"], 1)
        equivalent = "1.23456789012345678901234567890e29"
        self.check(drawing(line(x=first)),
                   drawing(line(x=equivalent)), True)

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

    def test_block_membership_cannot_change_with_same_entity_multiset(self):
        def definition(name, handle, member):
            return (pair(0, "BLOCK") + pair(5, handle) + pair(2, name) +
                    member + pair(0, "ENDBLK"))
        water_line = line(handle="A1", x="100")
        drain_line = line(handle="A2", x="200")
        before_blocks = (definition("WATER", "1A", water_line) +
                         definition("DRAIN", "1B", drain_line))
        after_blocks = (definition("WATER", "1A", drain_line) +
                        definition("DRAIN", "1B", water_line))
        # Every entity, handle, color and coordinate is unchanged. Only
        # BLOCK/ENDBLK membership changed (a dangerous false-green before).
        result = self.check(drawing(insert("WATER"), blocks=before_blocks),
                            drawing(insert("WATER"), blocks=after_blocks), False)
        self.assertEqual(result["missing_records"], 0)
        self.assertEqual(result["extra_records"], 0)
        self.assertTrue(result["structural_order_mismatch"])

    def test_reordering_whole_block_definitions_is_conservatively_refused(self):
        # Until an independent block-owner graph can prove equivalence,
        # reject rearrangements instead of silently accepting reassociation.
        a = pair(0, "BLOCK") + pair(2, "A") + line("1A") + pair(0, "ENDBLK")
        b = pair(0, "BLOCK") + pair(2, "B") + line("1B") + pair(0, "ENDBLK")
        result = self.check(drawing(line(), blocks=a + b),
                            drawing(line(), blocks=b + a), False)
        self.assertTrue(result["structural_order_mismatch"])

    def test_attached_attributes_cannot_jump_between_insert_instances(self):
        def attrib(handle, text):
            return (pair(0, "ATTRIB") + pair(5, handle) +
                    pair(1, text) + pair(8, "MECHANICAL"))
        def end(handle):
            return pair(0, "SEQEND") + pair(5, handle)
        a = insert("PUMP") + attrib("AA", "PUMP_1") + end("AC")
        b = insert("PUMP") + attrib("AB", "PUMP_2") + end("AD")
        switched = (insert("PUMP") + attrib("AB", "PUMP_2") + end("AC") +
                    insert("PUMP") + attrib("AA", "PUMP_1") + end("AD"))
        result = self.check(drawing(a+b), drawing(switched), False)
        self.assertEqual(result["missing_records"], 0)
        self.assertEqual(result["extra_records"], 0)
        self.assertTrue(result["structural_order_mismatch"])

    def test_classic_polyline_vertices_cannot_jump_between_polyline_headers(self):
        def polyline(handle):
            return pair(0, "POLYLINE") + pair(5, handle) + pair(70, 0)
        def vertex(handle, x):
            return pair(0, "VERTEX") + pair(5, handle) + pair(10, x) + pair(20, 0)
        def end(handle):
            return pair(0, "SEQEND") + pair(5, handle)
        a = polyline("1A") + vertex("2A", 5) + end("3A")
        b = polyline("1B") + vertex("2B", 50) + end("3B")
        after = (polyline("1A") + vertex("2B", 50) + end("3A") +
                 polyline("1B") + vertex("2A", 5) + end("3B"))
        result = self.check(drawing(a+b), drawing(after), False)
        self.assertEqual(result["missing_records"], 0)
        self.assertEqual(result["extra_records"], 0)
        self.assertTrue(result["structural_order_mismatch"])

    def test_structural_table_record_membership_and_section_presence(self):
        tables = (pair(0, "TABLE") + pair(2, "LAYER") +
                  pair(0, "LAYER") + pair(2, "PIPES") +
                  pair(0, "ENDTAB") +
                  pair(0, "TABLE") + pair(2, "LTYPE") +
                  pair(0, "LAYER") + pair(2, "STRUCTURE") +
                  pair(0, "ENDTAB"))
        before = drawing(line(), tables=tables)
        after = drawing(line(), tables=(
            pair(0, "TABLE") + pair(2, "LAYER") +
            pair(0, "LAYER") + pair(2, "STRUCTURE") +
            pair(0, "ENDTAB") +
            pair(0, "TABLE") + pair(2, "LTYPE") +
            pair(0, "LAYER") + pair(2, "PIPES") +
            pair(0, "ENDTAB")))
        result = self.check(before, after, False)
        self.assertEqual(result["missing_records"], 0)
        self.assertEqual(result["extra_records"], 0)
        self.assertTrue(result["structural_order_mismatch"])
        empty_blocks = (pair(0, "SECTION") + pair(2, "BLOCKS") +
                        pair(0, "ENDSEC"))
        result = self.check(drawing(line()),
                            drawing(line()).replace(
                                pair(0, "EOF"), empty_blocks + pair(0, "EOF")), False)
        self.assertTrue(result["audited_section_mismatch"])

    def test_malformed_block_group_refused_even_if_both_exports_agree(self):
        cases = (
            line("10"),  # BLOCKS geometry without any BLOCK owner
            pair(0, "ENDBLK"),  # orphan close
            pair(0, "BLOCK") + pair(2, "A") + line("10"),  # missing ENDBLK
            pair(0, "BLOCK") + pair(2, "A") +
            pair(0, "BLOCK") + pair(2, "B") + pair(0, "ENDBLK"),
        )
        for content in cases:
            with self.subTest(record=content[:60]):
                self.before.write_text(
                    drawing(line(), blocks=content), encoding="utf-8")
                with self.assertRaises(DxfParityError):
                    fingerprint(self.before)

    def test_malformed_table_group_refused_even_if_both_exports_agree(self):
        cases = (
            pair(0, "LAYER") + pair(2, "PIPES"),  # orphan row
            pair(0, "ENDTAB"),  # orphan close
            pair(0, "TABLE") + pair(2, "LAYER") +
            pair(0, "LAYER") + pair(2, "PIPES"),  # missing ENDTAB
            pair(0, "TABLE") + pair(2, "LAYER") +
            pair(0, "TABLE") + pair(2, "LTYPE") +
            pair(0, "ENDTAB"),  # nested TABLE
        )
        for content in cases:
            with self.subTest(record=content[:60]):
                self.before.write_text(
                    drawing(line(), tables=content), encoding="utf-8")
                with self.assertRaises(DxfParityError):
                    fingerprint(self.before)

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

    def test_external_xref_and_linked_image_require_asset_verification(self):
        # XREF paths can remain textually equal while referenced drawings
        # change; the DXF-only gate must NOT promise full project fidelity.
        for mask in (4, 8, 16, 32):
            xref = (pair(0, "BLOCK") + pair(2, "ARCH_XREF") +
                    pair(70, mask) + pair(1, "external/architecture.dwg") +
                    pair(0, "ENDBLK"))
            data = drawing(insert("ARCH_XREF"), blocks=xref)
            with self.subTest(block_flags=mask):
                result = self.check(data, data, False)
                self.assertEqual(result["unverifiable_entities"], 2)
        for entity in ("IMAGE", "OLE2FRAME", "PDFUNDERLAY",
                       "DGNUNDERLAY", "ACAD_CUSTOM_CLASS", "AEC_WALL"):
            data = drawing(pair(0, entity) + pair(5, "A1") +
                           pair(8, "EXTERNALS"))
            with self.subTest(entity=entity):
                result = self.check(data, data, False)
                self.assertEqual(result["unverifiable_entities"], 2)
        # Normal local blocks are still within the DXF preflight scope.
        self.check(drawing(insert(), blocks=block()),
                   drawing(insert(), blocks=block()), True)

    def test_acdsdata_section_is_unverifiable_even_if_equal(self):
        base = drawing(line())
        extra = (pair(0, "SECTION") + pair(2, "ACDSDATA") +
                 pair(0, "ACDSRECORD") + pair(1, "opaque ACIS payload") +
                 pair(0, "ENDSEC"))
        with_opaque = base.replace(pair(0, "EOF"), extra + pair(0, "EOF"))
        result = self.check(with_opaque, with_opaque, False)
        self.assertEqual(result["unverifiable_entities"], 2)

    def test_numeric_exponent_bomb_rejected(self):
        malformed = drawing(line(x="1e999999999"))
        self.before.write_text(malformed, encoding="utf-8")
        with self.assertRaises(DxfParityError):
            fingerprint(self.before)

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

    def test_no_hidden_fields_allowed_after_structural_markers(self):
        good = drawing(line())
        cases = (
            good + pair(1, "hidden data after EOF"),
            good.replace(pair(0, "EOF"),
                         pair(1, "hidden data after ENDSEC") + pair(0, "EOF")),
        )
        for malformed in cases:
            with self.subTest(tail=malformed[-50:]):
                self.before.write_text(malformed, encoding="utf-8")
                with self.assertRaises(DxfParityError):
                    fingerprint(self.before)

    def test_missing_or_empty_dxf_refused(self):
        with self.assertRaises(DxfParityError):
            fingerprint(self.before)
        self.before.write_text("", encoding="utf-8")
        with self.assertRaises(DxfParityError):
            fingerprint(self.before)


if __name__ == "__main__":
    unittest.main()
