"""Test read-only observable reference inventory; no GPL code or drawings."""
import json
from io import BytesIO
from pathlib import Path
import tempfile
import unittest

from real_dwg_reference_compare import (ac1024_candidate_alignment, observable_inventory, observable_inventory_stream, reference_read)


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


    def test_streaming_schema_parity_and_nested_spoofing(self):
        document = {
            "HEADER": {"irrelevant": [{"entity": "FAKE"}]},
            "OBJECTS": [
                {"entity": "LINE", "details": {"entity": "FAKE", "object": "LAYER"}},
                {"object": "LAYER"},
                {"entity": "INSERT", "name": "never-export-me"},
                {"type": 17, "details": {"entity": "FAKE"}},
                "unclassified",
            ],
        }
        expected = observable_inventory(document)
        actual = observable_inventory_stream(BytesIO(json.dumps(document).encode()))
        self.assertEqual(actual, expected)
        self.assertNotIn("never-export-me", json.dumps(actual))
        for invalid in ({}, {"OBJECTS": []}, {"OBJECTS": "not-an-array"}):
            with self.subTest(invalid=invalid):
                with self.assertRaises(ValueError):
                    observable_inventory_stream(BytesIO(json.dumps(invalid).encode()))

    def test_streaming_counts_beyond_original_128_mib_json_cap(self):
        # Stream >128 MiB of disposable non-CAD metadata, keeping each token
        # 64 KiB; do not build a huge JSON object or allocate huge RAM.
        with tempfile.TemporaryFile() as output:
            output.write(b'{"HEADER":{"padding":[')
            chunk = b'"' + (b'x' * 65536) + b'",'
            for _ in range(2048):
                output.write(chunk)
            output.write(b'"" ]},"OBJECTS":[{"entity":"LINE"},{"object":"LAYER"}]}')
            self.assertGreater(output.tell(), 128 * 1024 * 1024)
            output.seek(0)
            result = observable_inventory_stream(output)
        self.assertEqual(result["reference_typed_entity_count"], 1)
        self.assertEqual(result["reference_layer_record_count"], 1)
        self.assertEqual(result["reference_unclassified_count"], 0)


    def test_malformed_pretty_json_uses_valid_minjson_fallback(self):
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            source = root / "fake.dwg"
            source.write_bytes(b"AC1024")
            reader = root / "fake-reader"
            reader.write_text(
                "#!/usr/bin/env python3\n"
                "import sys\n"
                "if sys.argv[2] == 'JSON':\n"
                "    print('{ malformed reference')\n"
                "else:\n"
                "    print('{\"OBJECTS\":[{\"entity\":\"LINE\"}]}')\n",
                encoding="utf-8",
            )
            reader.chmod(0o755)
            observed = reference_read(reader, source, 10)
            self.assertEqual(observed["reference_status"], "libredwg_typed_inventory_only")
            self.assertEqual(observed["reference_output_mode"], "minJSON")
            self.assertEqual(observed["reference_typed_entity_count"], 1)

    def test_both_malformed_json_variants_fail_without_false_green(self):
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            source = root / "fake.dwg"
            source.write_bytes(b"AC1024")
            reader = root / "fake-reader"
            reader.write_text("#!/usr/bin/env python3\nprint('{ malformed')\n",
                              encoding="utf-8")
            reader.chmod(0o755)
            failure = reference_read(reader, source, 10)
            self.assertEqual(failure["reference_status"], "libredwg_unusable_json")
            self.assertEqual(len(failure["reference_attempts"]), 2)

    def test_direct_host_probe_survives_both_broken_json_exporters(self):
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            source = root / "fake.dwg"
            source.write_bytes(b"AC1024")
            reader = root / "bad-reader"
            reader.write_text("#!/usr/bin/env python3\nprint('{ malformed')\n",
                              encoding="utf-8")
            reader.chmod(0o755)
            host = root / "host-types"
            host.write_text(
                "#!/usr/bin/env python3\n"
                "print('{\"DECODER_WARNING_MASK\":4,\"OBJECTS\":['"
                "'{\"entity\":\"3DSOLID\"},{\"entity\":\"LINE\"},"
                "{\"object\":\"LAYER\"}]}')\n",
                encoding="utf-8",
            )
            host.chmod(0o755)
            result = reference_read(reader, source, 10, host)
            self.assertEqual(result["reference_status"], "libredwg_typed_inventory_only")
            self.assertEqual(result["reference_output_mode"], "libredwg_host_direct_types")
            self.assertTrue(result["reference_json_exporter_failed"])
            self.assertEqual(result["reference_decoder_warning_mask"], 4)
            self.assertEqual(result["reference_typed_entity_count"], 2)
            self.assertEqual(result["reference_layer_record_count"], 1)


    def test_ac1024_candidate_count_match_is_not_entity_parity(self):
        reference = {
            "reference_status": "libredwg_typed_inventory_only",
            "reference_total_object_records": 145279,
            "reference_entities_by_type": {
                "ARC": 4360, "CIRCLE": 9543,
                "LINE": 44941, "LWPOLYLINE": 13987,
            },
        }
        native = {
            "signature": "AC1024",
            "ac1024_compatibility_hypothesis": {
                "candidate_handle_index": True,
                "candidate_handle_count": 145279,
                "legacy_frame_probe_completed": True,
                "legacy_frame_crc_ok": 145279,
                "legacy_frame_crc_failed": 0,
                "legacy_raw_type_17_arc": 4360,
                "legacy_raw_type_18_circle": 9543,
                "legacy_raw_type_19_line": 44941,
                "legacy_raw_type_77_lwpolyline": 13987,
            }
        }
        report = ac1024_candidate_alignment(reference, native)
        self.assertEqual(report["state"],
                         "CANDIDATE_HANDLE_COUNT_EQUAL_NOT_ENTITY_PARITY")
        self.assertEqual(report["entity_parity"], "NOT_MEASURABLE")
        self.assertFalse(report["type_parity_verified"])
        self.assertFalse(report["layer_owner_color_parity_verified"])
        self.assertTrue(report["legacy_raw_code_count_correlations"]["19"]["count_equal_only"])
        native["ac1024_compatibility_hypothesis"]["legacy_raw_type_19_line"] = 12
        mismatch = ac1024_candidate_alignment(reference, native)
        self.assertFalse(mismatch["legacy_raw_code_count_correlations"]["19"]["count_equal_only"])
        native["ac1024_compatibility_hypothesis"]["candidate_handle_count"] = 17
        self.assertEqual(ac1024_candidate_alignment(reference, native)["state"],
                         "CANDIDATE_HANDLE_COUNT_MISMATCH")
        native["ac1024_compatibility_hypothesis"]["candidate_handle_index"] = False
        self.assertEqual(ac1024_candidate_alignment(reference, native)["state"],
                         "FIRSTPARTY_CANDIDATE_UNAVAILABLE")
        native["signature"] = "AC1032"
        self.assertEqual(ac1024_candidate_alignment(reference, native)["state"],
                         "NOT_APPLICABLE")

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
