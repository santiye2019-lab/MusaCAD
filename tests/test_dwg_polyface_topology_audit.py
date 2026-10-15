"""Polyface source-bound numeric and topology subset regression.

No real private DWG is embedded here. Synthetic converted DXF + independent
manifest show verifier behavior, NOT original-DWG or AutoCAD parity.
"""
import copy
import tempfile
import unittest
from pathlib import Path
from unittest.mock import Mock, patch

from dwg_polyface_topology_audit import DwgPolyfaceTopologyAudit
from private_dwg_read_audit import inventory_dxf, convert_one


def rec(kind, *pairs):
    return "0\n" + kind + "\n" + "".join(f"{k}\n{v}\n" for k, v in pairs)


def source_mesh():
    return [{
        "handle": "A0", "space": "model",
        "vertices": [
            {"handle": "A1", "xyz": ["0", "0", "0"]},
            {"handle": "A2", "xyz": ["1", "0", "0"]},
            {"handle": "A3", "xyz": ["1", "1", "0"]},
            {"handle": "A4", "xyz": ["0", "1", "0"]},
        ],
        "faces": [
            {"handle": "A5", "indices": [1, 2, -3]},
            {"handle": "A6", "indices": [1, 3, 4]},
        ],
        "seqend": "A7",
    }]


def drawing(points=None, faces=None, count_vertices="4", count_faces="2",
            close=True, face_owner="A0", additional="", broken_flag=None):
    pts = points if points is not None else [
        ("A1", "0", "0", "0"), ("A2", "1", "0", "0"),
        ("A3", "1", "1", "0"), ("A4", "0", "1", "0")
    ]
    triangles = faces if faces is not None else [
        ("A5", (1, 2, -3)), ("A6", (1, 3, 4))
    ]
    dxf = rec("SECTION", (2, "ENTITIES"))
    dxf += rec("POLYLINE", (5, "A0"), (8, "PRIVATE_SOLID"),
               (70, "64"), (71, count_vertices), (72, count_faces))
    for h, x, y, z in pts:
        dxf += rec("VERTEX", (5, h), (330, "A0"), (8, "PRIVATE_SOLID"),
                   (70, "192"), (10, x), (20, y), (30, z))
    for h, indices in triangles:
        fields = [(5, h), (330, face_owner), (8, "PRIVATE_SOLID"),
                  (70, broken_flag if broken_flag is not None else "128")]
        fields += [(71+i, str(n)) for i, n in enumerate(indices)]
        dxf += rec("VERTEX", *fields)
    dxf += additional
    if close:
        dxf += rec("SEQEND", (5, "A7"), (330, "A0"), (8, "PRIVATE_SOLID"))
    dxf += rec("ENDSEC") + rec("EOF")
    return dxf


def verify_dxf(dxf, reference=None):
    with tempfile.TemporaryDirectory() as tmp:
        path = Path(tmp) / "synthetic.dxf"
        path.write_text(dxf, encoding="utf-8")
        audit = DwgPolyfaceTopologyAudit()
        inventory_dxf(path, polyface_collector=audit)
        return audit.compare_independent(source_mesh() if reference is None else reference)


class DwgPolyfaceTopologyAuditTests(unittest.TestCase):
    def test_exact_signed_face_and_winding_parity(self):
        report = verify_dxf(drawing())
        self.assertEqual(report["independent_polyface_result"],
                         "ordered_face_subset_parity_only")
        self.assertEqual(report["independent_polyface_checked"], 1)
        self.assertEqual(report["independent_polyface_shared_edges"], 1)
        self.assertEqual(report["independent_polyface_boundary_edges"], 4)
        self.assertEqual(report["independent_polyface_nonmanifold_edges"], 0)
        self.assertEqual(report["independent_polyface_inconsistent_winding_edges"], 0)
        self.assertNotIn("PRIVATE_SOLID", str(report))
        self.assertNotIn("'A0'", str(report))
        self.assertNotIn("'A5'", str(report))

    def test_vertex_position_and_face_order_drift(self):
        original = drawing()
        # Same entity/face counts: changed XYZ, invisible edge sign, face
        # orientation, winding, and order must all reject equivalence.
        alternatives = [
            drawing(points=[("A1", "0", "0", "0"),
                            ("A2", "1.0000000000000001", "0", "0"),
                            ("A3", "1", "1", "0"),
                            ("A4", "0", "1", "0")]),
            drawing(faces=[("A5", (1, 2, 3)), ("A6", (1, 3, 4))]),
            drawing(faces=[("A5", (1, 2, -3)), ("A6", (3, 1, 4))]),
            drawing(faces=[("A6", (1, 3, 4)), ("A5", (1, 2, -3))]),
        ]
        for altered in alternatives:
            with self.subTest(variant=altered[altered.find("A5")-10:altered.find("A5")+40]):
                self.assertNotEqual(original, altered)
                result = verify_dxf(altered)
                self.assertEqual(result["independent_polyface_drift"], 1)
                self.assertEqual(result["independent_polyface_result"],
                                 "polyface_topology_mismatch")
        winding = verify_dxf(alternatives[2])
        self.assertEqual(winding["independent_polyface_inconsistent_winding_edges"], 1)

    def test_invalid_faces_and_header_fail_closed(self):
        samples = [
            drawing(faces=[("A5", (1, 2, -9)), ("A6", (1, 3, 4))]),
            drawing(faces=[("A5", (1, 2, 2)), ("A6", (1, 3, 4))]),
            drawing(faces=[("A5", (1, 0, 3)), ("A6", (1, 3, 4))]),
            drawing(faces=[("A5", (1, 2, -3)), ("A6", (1, 3, 4))],
                    count_faces="3"),
            drawing(count_vertices="3"),
            drawing(close=False),
            drawing(face_owner="CA"),
            drawing(broken_flag="32"),
            drawing(additional=rec("LINE", (5, "F1"), (8, "PRIVATE"))),
        ]
        for dxf in samples:
            with self.subTest(data_len=len(dxf), tail=dxf[-65:]):
                result = verify_dxf(dxf)
                self.assertNotEqual(result["independent_polyface_result"],
                                    "ordered_face_subset_parity_only")
                self.assertGreater(result["independent_polyface_invalid_decoded"], 0)

    def test_source_manifest_rejects_wrong_indices_shared_handles_or_coords(self):
        scenarios = []
        bad = source_mesh()
        bad[0]["faces"][0]["indices"] = [1, 2, 6]
        scenarios.append(bad)
        bad = source_mesh()
        bad[0]["vertices"][1]["xyz"][0] = "NaN"
        scenarios.append(bad)
        bad = source_mesh()
        bad[0]["faces"][1]["handle"] = "A2"
        scenarios.append(bad)
        bad = source_mesh()
        bad[0]["faces"][0]["indices"] = [1, 2, 2]
        scenarios.append(bad)
        bad = source_mesh()
        bad[0]["vertices"][0]["xyz"][0] = "1e99999999999999"
        scenarios.append(bad)
        bad = source_mesh()
        bad[0]["faces"][0]["indices"] = [1, -2]
        scenarios.append(bad)
        bad = source_mesh()
        bad[0]["seqend"] = "A0"
        scenarios.append(bad)
        bad = source_mesh()
        bad[0]["vertices"][0]["xyz"][0] = 0.0
        scenarios.append(bad)
        bad = source_mesh()
        bad[0]["faces"][0]["indices"] = [1, True, 3]
        scenarios.append(bad)
        for source in scenarios:
            with self.subTest(source=str(source)[:100]), self.assertRaises(ValueError):
                verify_dxf(drawing(), source)

    def test_optional_private_converter_reports_count_only(self):
        dxf = drawing()
        with tempfile.TemporaryDirectory() as td:
            root = Path(td)
            original = root / "original-unchanged.dwg"
            original.write_bytes(b"AC1032 synthetic only")
            converter = root / "mock"
            converter.write_text("#!/bin/sh\nexit 0\n", encoding="ascii")
            converter.chmod(0o700)

            def convert(*args, **kwargs):
                Path(args[0][2]).write_text(dxf, encoding="utf-8")
                kwargs["stderr"].write(
                    b"MusaCAD native scene: status=0 primitives=1 floats=9\n"
                )
                return Mock(returncode=0)

            with patch("private_dwg_read_audit.subprocess.run", side_effect=convert):
                with tempfile.TemporaryDirectory(dir=root) as staging:
                    good = convert_one(original, converter, Path(staging),
                                       timeout=5, limit_mb=128,
                                       independent_polyface=source_mesh())
                changed = source_mesh()
                changed[0]["faces"][0]["indices"] = [1, -2, -3]
                with tempfile.TemporaryDirectory(dir=root) as staging:
                    bad = convert_one(original, converter, Path(staging),
                                      timeout=5, limit_mb=128,
                                      independent_polyface=changed)
        self.assertEqual(good["status"], "decoded_dxf_inventory")
        self.assertEqual(good["independent_polyface_result"],
                         "ordered_face_subset_parity_only")
        self.assertEqual(bad["status"],
                         "decoded_dxf_inventory_polyface_topology_mismatch")
        self.assertEqual(bad["independent_polyface_drift"], 1)
        own = {k:v for k,v in good.items() if k.startswith("independent_polyface_")}
        self.assertNotIn("PRIVATE",str(own))
        self.assertNotIn("'A0'",str(own))


if __name__ == "__main__":
    unittest.main()
