"""Source-bound parent/ordered children/SEQEND audit regressions.

Synthetic DXF fixtures exercise the verifier, NOT real independent DWG proof.
"""
import copy
import tempfile
import unittest
from pathlib import Path
from unittest.mock import patch, Mock

from dwg_polyline_chain_audit import DwgPolylineChainAudit
from private_dwg_read_audit import inventory_dxf, convert_one


def record(name, *fields):
    return "0\n" + name + "\n" + "".join(
        f"{code}\n{value}\n" for code, value in fields
    )


def chain_records():
    return [
        {"handle":"AA", "space":"model", "vertices":["AB","AC"], "seqend":"AD"},
        {"handle":"BA", "space":"block", "vertices":["BB"], "seqend":"BC"},
    ]


class DwgPolylineChainAuditTests(unittest.TestCase):
    def drawing(self, vertices=None, end=True, owner="AA", interrupt=False,
                duplicate=False):
        vertices = vertices if vertices is not None else ["AB", "AC"]
        body = record("SECTION", (2, "ENTITIES"))
        body += record("POLYLINE", (5, "AA"), (8, "PRIVATE_PIPE"), (70, "8"))
        for handle in vertices:
            body += record("VERTEX", (5, handle), (8, "PRIVATE_PIPE"),
                           (330, owner), (70, "32"), (10, "1"),
                           (20, "2"), (30, "3"))
        if duplicate:
            body += record("VERTEX", (5, vertices[0]), (8, "PRIVATE_PIPE"),
                           (330, owner), (10, "4"), (20, "5"))
        if interrupt:
            body += record("LINE", (5, "DE"), (8, "PRIVATE_PIPE"),
                           (10, "0"), (20, "0"))
        if end:
            body += record("SEQEND", (5, "AD"), (330, "AA"), (8, "PRIVATE_PIPE"))
        body += record("ENDSEC")
        body += record("SECTION", (2, "BLOCKS"))
        body += record("BLOCK", (2, "MY_PRIVATE_BLOCK"))
        body += record("POLYLINE", (5, "BA"), (8, "PRIVATE_PIPE"), (70, "64"))
        body += record("VERTEX", (5, "BB"), (8, "PRIVATE_PIPE"),
                       (330, "BA"), (70, "128"), (71, "1"), (72, "-2"))
        body += record("SEQEND", (5, "BC"), (330, "BA"), (8, "PRIVATE_PIPE"))
        body += record("ENDBLK")
        body += record("ENDSEC") + record("EOF")
        return body

    def inspect(self, drawing, source=None):
        with tempfile.TemporaryDirectory() as work:
            output = Path(work) / "fixture.dxf"
            output.write_text(drawing, encoding="utf-8")
            audit = DwgPolylineChainAudit()
            inventory_dxf(output, polyline_collector=audit)
            return audit.compare_independent(
                chain_records() if source is None else source)

    def test_parent_order_and_termination_match_with_no_sensitive_data(self):
        good = self.inspect(self.drawing())
        self.assertEqual(good["independent_polyline_chain_result"],
                         "record_order_subset_parity_only")
        self.assertEqual(good["independent_polyline_chain_checked"], 2)
        self.assertEqual(good["independent_polyline_chain_malformed_decoded"], 0)
        self.assertNotIn("MY_PRIVATE", str(good))
        self.assertNotIn("PRIVATE_PIPE", str(good))
        self.assertNotIn("'AA'", str(good))
        self.assertNotIn("'AB'", str(good))

    def test_swapped_child_order_with_identical_vertex_counts_fails(self):
        changed = self.inspect(self.drawing(vertices=["AC","AB"]))
        self.assertEqual(changed["independent_polyline_chain_mismatches"], 1)
        self.assertEqual(changed["independent_polyline_chain_result"],
                         "polyline_chain_mismatch")

    def test_source_manifest_order_change_fails(self):
        reference = chain_records()
        reference[0]["vertices"] = ["AC", "AB"]
        changed = self.inspect(self.drawing(), reference)
        self.assertEqual(changed["independent_polyline_chain_mismatches"], 1)

    def test_missing_wrong_or_unmatched_seqend_fails(self):
        for drawing in (
            self.drawing(end=False),
            self.drawing(owner="FF"),
            self.drawing(interrupt=True),
            self.drawing(duplicate=True),
        ):
            with self.subTest(scenario=drawing[:90]):
                result = self.inspect(drawing)
                self.assertGreater(result["independent_polyline_chain_malformed_decoded"], 0)
                self.assertEqual(result["independent_polyline_chain_result"],
                                 "polyline_chain_mismatch")

    def test_missing_and_surplus_source_chains_are_distinct(self):
        result = self.inspect(self.drawing(), chain_records()[:1])
        self.assertEqual(result["independent_polyline_chain_missing_source"], 1)
        reference = chain_records()
        reference.append({"handle":"CA", "space":"model", "vertices":["CB"],
                          "seqend":"CC"})
        result = self.inspect(self.drawing(), reference)
        self.assertEqual(result["independent_polyline_chain_unexpected_source"], 1)

    def test_duplicate_shared_invalid_original_reference_rejected(self):
        reference = chain_records()
        variants = []
        same_parent=copy.deepcopy(reference)
        same_parent[1]["handle"]="AA"
        variants.append(same_parent)
        same_child=copy.deepcopy(reference)
        same_child[1]["vertices"]=["AB"]
        variants.append(same_child)
        missing_end=copy.deepcopy(reference)
        missing_end[1]["seqend"]="0"
        variants.append(missing_end)
        wrong_case=copy.deepcopy(reference)
        wrong_case[0]["space"]="layout_private"
        variants.append(wrong_case)
        no_end=copy.deepcopy(reference)
        del no_end[0]["seqend"]
        variants.append(no_end)
        no_vertices=copy.deepcopy(reference)
        no_vertices[0]["vertices"]=[]
        variants.append(no_vertices)
        for records in variants:
            with self.subTest(records=records), self.assertRaises(ValueError):
                self.inspect(self.drawing(), records)

    def test_opt_in_converter_sets_fail_closed_status(self):
        drawing = self.drawing()
        with tempfile.TemporaryDirectory() as work:
            root=Path(work)
            source=root/"read_only.dwg"
            source.write_bytes(b"AC1032 synthetic only")
            converter=root/"mock"
            converter.write_text("#!/bin/sh\nexit 0\n", encoding="utf-8")
            converter.chmod(0o700)

            def converted(*args, **kwargs):
                Path(args[0][2]).write_text(drawing, encoding="utf-8")
                kwargs["stderr"].write(
                    b"MusaCAD native scene: status=0 primitives=2 floats=18\n")
                return Mock(returncode=0)

            with patch("private_dwg_read_audit.subprocess.run",side_effect=converted):
                with tempfile.TemporaryDirectory(dir=root) as temp:
                    ok=convert_one(source,converter,Path(temp),5,128,
                                   independent_polyline_chains=chain_records())
                faulty=chain_records()
                faulty[0]["vertices"]=["AC","AB"]
                with tempfile.TemporaryDirectory(dir=root) as temp:
                    bad=convert_one(source,converter,Path(temp),5,128,
                                    independent_polyline_chains=faulty)
            self.assertEqual(ok["status"],"decoded_dxf_inventory")
            self.assertEqual(ok["independent_polyline_chain_result"],
                             "record_order_subset_parity_only")
            self.assertEqual(bad["status"],
                             "decoded_dxf_inventory_polyline_chain_mismatch")
            self.assertEqual(bad["independent_polyline_chain_mismatches"],1)
            self.assertNotIn("MY_PRIVATE_BLOCK",str(ok))
            self.assertNotIn("PRIVATE_PIPE",str(bad))


if __name__ == "__main__":
    unittest.main()
