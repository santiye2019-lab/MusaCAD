"""Synthetic tests for local-only original-DWG handle/layer parity audit."""
import tempfile
import unittest
from pathlib import Path

from dwg_entity_identity_audit import DwgEntityIdentityAudit
from private_dwg_read_audit import inventory_dxf


def record(kind, *pairs):
    return "0\n" + kind + "\n" + "".join(
        f"{key}\n{value}\n" for key,value in pairs)


class DwgEntityIdentityAuditTest(unittest.TestCase):
    def test_duplicate_and_invalid_handles_are_counted_not_published(self):
        audit = DwgEntityIdentityAudit()
        audit.observe("ENTITIES", "LINE", [(5, "0a"), (8, "EQUIP SECRET")])
        audit.observe("ENTITIES", "CIRCLE", [(5, "A"), (8, "OTHER SECRET")])
        audit.observe("BLOCKS", "3DSOLID", [(5, "ZZ"), (8, "SENSITIVE")])
        audit.observe("BLOCKS", "LINE", [(8, "SENSITIVE")])
        audit.observe("ENTITIES", "ARC", [(5, "11"), (8, "")])
        audit.observe("ENTITIES", "POINT",
                      [(5, "12"), (5, "13"), (8, "0"), (8, "CUSTOM")])
        audit.observe("BLOCKS", "BLOCK", [(2, "PRIVATE")])
        s = audit.summary()
        self.assertEqual(s["decoded_identity_entity_records"], 6)
        self.assertEqual(s["decoded_identity_unique_handles"], 3)
        self.assertEqual(s["decoded_identity_duplicate_handles"], 1)
        self.assertEqual(s["decoded_identity_invalid_handles"], 1)
        self.assertEqual(s["decoded_identity_missing_handles"], 1)
        self.assertEqual(s["decoded_identity_missing_layers"], 1)
        self.assertEqual(s["decoded_identity_duplicate_handle_fields"], 1)
        self.assertEqual(s["decoded_identity_duplicate_layer_fields"], 1)
        self.assertEqual(s["decoded_identity_status"],
                         "decoded_records_have_identity_issues")
        self.assertNotIn("SECRET", str(s))
        self.assertNotIn("SENSITIVE", str(s))
        self.assertNotIn("PRIVATE", str(s))

    def test_independent_reference_exact_metadata_parity(self):
        audit = DwgEntityIdentityAudit()
        audit.observe("ENTITIES", "LINE", [(5, "A"), (8, "HVAC")])
        audit.observe("BLOCKS", "REGION", [(5, "00BF"), (8, "FIRE")])
        source = [
            {"handle":"0a", "space":"model", "type":"LINE", "layer":"HVAC"},
            {"handle":"bf", "space":"block", "type":"REGION", "layer":"FIRE"},
        ]
        comparison = audit.compare_independent(source)
        self.assertEqual(comparison["independent_identity_result"],
                         "identity_metadata_parity_only")
        self.assertEqual(comparison["independent_missing_in_converted"], 0)
        self.assertEqual(comparison["independent_unexpected_in_converted"], 0)
        self.assertNotIn("HVAC", str(comparison))
        self.assertNotIn("FIRE", str(comparison))
        self.assertNotIn("BF", str(comparison))

    def test_source_object_loss_and_semantic_drift(self):
        audit = DwgEntityIdentityAudit()
        audit.observe("ENTITIES", "CIRCLE", [(5, "A"), (8, "CHANGED")])
        audit.observe("BLOCKS", "LINE", [(5, "B"), (8, "0")])
        source = [
            {"handle":"A", "space":"block", "type":"LINE", "layer":"ORIGINAL"},
            {"handle":"C", "space":"model", "type":"SPLINE", "layer":"0"},
        ]
        comparison = audit.compare_independent(source)
        self.assertEqual(comparison["independent_identity_result"],
                         "identity_metadata_mismatch")
        self.assertEqual(comparison["independent_missing_in_converted"], 1)
        self.assertEqual(comparison["independent_unexpected_in_converted"], 1)
        self.assertEqual(comparison["independent_space_mismatches"], 1)
        self.assertEqual(comparison["independent_type_mismatches"], 1)
        self.assertEqual(comparison["independent_layer_mismatches"], 1)

    def test_invalid_or_spoofed_reference_rejected(self):
        audit = DwgEntityIdentityAudit()
        for invalid in (
            None, {}, "FILE.dwg", [{"handle":"XYZ","space":"model","type":"LINE","layer":"0"}],
            [{"handle":"A","space":"model","type":"LINE","layer":"0"},
             {"handle":"000A","space":"model","type":"LINE","layer":"0"}],
            [{"handle":"A","space":"paperspace","type":"LINE","layer":"0"}],
            [{"handle":"A","space":"model","type":"LINE","layer":""}],
            [{"handle":"A","space":"model","type":"LINE","layer":"0","x":123}],
        ):
            with self.subTest(invalid=invalid):
                with self.assertRaises(ValueError):
                    audit.compare_independent(invalid)

    def test_missing_decoded_layer_cannot_pass_even_if_reference_matches(self):
        audit = DwgEntityIdentityAudit()
        audit.observe("ENTITIES", "LINE", [(5, "1")])
        result = audit.compare_independent(
            [{"handle":"1","space":"model","type":"LINE","layer":"0"}])
        self.assertEqual(result["independent_identity_result"],
                         "identity_metadata_mismatch")
        self.assertEqual(result["independent_layer_mismatches"], 1)

    def test_synthetic_dxf_groups_5_8_audited_without_details(self):
        source = (
            record("SECTION",(2,"BLOCKS")) +
            record("BLOCK",(2,"PRIVATE-ROOM")) +
            record("3DSOLID",(5,"000A"),(8,"CONFIDENTIAL ROOM")) +
            record("ENDBLK") + record("ENDSEC") +
            record("SECTION",(2,"ENTITIES")) +
            record("LINE",(5,"B"),(8,"PROJECT PIPE")) +
            record("CIRCLE",(5,"B"),(8,"DIFFERENT LAYER")) +
            record("ARC",(5,"BAD_HANDLE!"),(8,"SECRET")) +
            record("ENDSEC") + record("EOF")
        )
        with tempfile.TemporaryDirectory(prefix="musacad-identity-") as tmp:
            path = Path(tmp) / "generated.dxf"
            path.write_text(source,encoding="ascii")
            result = inventory_dxf(path)
        self.assertEqual(result["decoded_identity_entity_records"], 4)
        self.assertEqual(result["decoded_identity_unique_handles"], 2)
        self.assertEqual(result["decoded_identity_duplicate_handles"], 1)
        self.assertEqual(result["decoded_identity_invalid_handles"], 1)
        self.assertEqual(result["native_preview_unsupported_entity_types"],
                         {"3DSOLID": 1})
        exposed = {k:v for k,v in result.items()
                   if k.startswith("decoded_identity_")}
        self.assertNotIn("CONFIDENTIAL", str(exposed))
        self.assertNotIn("PIPE", str(exposed))
        self.assertNotIn("000A", str(exposed))


if __name__ == "__main__":
    unittest.main()
