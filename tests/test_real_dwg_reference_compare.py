"""Test read-only observable reference inventory; no GPL code or drawings."""
import json
from pathlib import Path
import tempfile
import unittest

from real_dwg_reference_compare import observable_inventory, reference_read


class ObservableReferenceTest(unittest.TestCase):
    def test_counts_only_explicitly_identified_entity_and_table_types(self):
        source = {
            "HEADER": {"$ACADVER": "AC1032"},
            "OBJECTS": [
                {"entity": "LINE", "handle": "private", "start": [1, 2, 3]},
                {"entity": "LINE", "text": "never-exported"},
                {"entity": "INSERT", "name": "secret-block"},
                {"object": "LAYER", "name": "secret-layer"},
                {"object": "BLOCK_HEADER", "name": "secret-block"},
                {"type": 29, "handle": "AA"},
                {"entity": "LINE/UNSAFE"},
            ],
        }
        inventory = observable_inventory(source)
        self.assertEqual(inventory["reference_typed_entity_count"], 3)
        self.assertEqual(inventory["reference_typed_nonentity_count"], 2)
        self.assertEqual(inventory["reference_unclassified_count"], 2)
        self.assertEqual(inventory["reference_entities_by_type"], {"INSERT": 1, "LINE": 2})
        self.assertEqual(inventory["reference_layer_record_count"], 1)
        self.assertEqual(inventory["reference_block_header_record_count"], 1)
        dump = json.dumps(inventory)
        for secret in ("private", "secret-layer", "secret-block", "never-exported"):
            self.assertNotIn(secret, dump)

    def test_schema_missing_must_not_look_like_zero_entities(self):
        for invalid in ([], {}, {"objects": []}, {"OBJECTS": "[]" },
                        {"OBJECTS": [{"type": 4, "name": "unknown"}]}):
            with self.subTest(invalid=invalid):
                with self.assertRaises(ValueError):
                    observable_inventory(invalid)

    def test_bounded_cli_success_and_nonzero_are_distinct(self):
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            source = root / "fake.dwg"
            source.write_bytes(b"AC1032")
            reader = root / "fake-reader"
            reader.write_text(
                "#!/usr/bin/env python3\n"
                "import json\n"
                "print(json.dumps({'OBJECTS':[{'entity':'LINE'},"
                "{'object':'LAYER'}]}))\n",
                encoding="utf-8",
            )
            reader.chmod(0o755)
            observed = reference_read(reader, source, 10)
            self.assertEqual(observed["reference_status"], "libredwg_typed_inventory_only")
            self.assertEqual(observed["reference_typed_entity_count"], 1)
            reader.write_text("#!/usr/bin/env python3\nimport sys\nsys.exit(9)\n",
                              encoding="utf-8")
            failed = reference_read(reader, source, 10)
            self.assertEqual(failed["reference_status"], "libredwg_rejected_or_failed")
            self.assertEqual(failed["reference_exit_code"], 9)


if __name__ == "__main__":
    unittest.main()
