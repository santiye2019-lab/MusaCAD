"""Unit guard for false AC1032 parity claims and truncated scene acceptance."""
import copy
import unittest
from modern_dwg_acceptance import failures


def baseline():
    digest = "a" * 64
    return {
        "source_is_read_only": True,
        "files": [{
            "filename": "Dynblocks.dwg",
            "acad_signature": "AC1032",
            "dwg_release": "R2018",
            "sha256_before": digest,
            "sha256_after": digest,
            "source_unchanged": True,
            "status": "decoded_dxf_inventory",
            "native_scene_status": 0,
            "native_scene_truncated": False,
            "dxf_bytes": 1000,
            "model_entity_count": 20,
            "block_geometry_entity_count": 50,
            "block_definition_count": 5,
            "block_inserts": 2,
            "converted_dxf_color_sources": {"bylayer": 68, "truecolor": 2},
            "conversion_has_quality_warnings": False,
        }],
    }


class ModernDwgAcceptanceTest(unittest.TestCase):
    def test_valid_read_smoke_only(self):
        self.assertEqual(failures(baseline()), [])

    def test_older_file_fails_even_if_decoded(self):
        doc = baseline()
        doc["files"][0]["acad_signature"] = "AC1015"
        doc["files"][0]["dwg_release"] = "R2000"
        self.assertTrue(failures(doc))

    def test_truncated_native_scene_fails(self):
        doc = baseline()
        doc["files"][0]["native_scene_status"] = 1
        doc["files"][0]["native_scene_truncated"] = True
        self.assertTrue(failures(doc))

    def test_decode_warnings_do_not_falsely_prove_losslessness(self):
        doc = baseline()
        doc["files"][0]["conversion_has_quality_warnings"] = True
        # Still a limited read smoke: print warnings and require future parity checks.
        self.assertEqual(failures(doc), [])

    def test_missing_blocks_or_inserts_fails_specific_fixture(self):
        for name in ("block_definition_count", "block_inserts"):
            doc = baseline()
            doc["files"][0][name] = 0
            self.assertTrue(failures(doc), name)

    def test_source_mutation_fails(self):
        doc = baseline()
        doc["files"][0]["sha256_after"] = "b" * 64
        doc["files"][0]["source_unchanged"] = True
        self.assertTrue(failures(doc))

    def test_fake_success_without_preview_fails(self):
        doc = baseline()
        doc["files"][0]["native_scene_status"] = None
        self.assertTrue(failures(doc))

    def test_incomplete_color_census_fails(self):
        doc = baseline()
        doc["files"][0]["converted_dxf_color_sources"]["bylayer"] = 67
        self.assertTrue(failures(doc))
        del doc["files"][0]["converted_dxf_color_sources"]
        self.assertTrue(failures(doc))

    def test_wrong_fixture_or_extra_file_fails(self):
        doc = baseline()
        doc["files"][0]["filename"] = "fake.dwg"
        self.assertTrue(failures(doc))
        doc = baseline()
        doc["files"].append(copy.deepcopy(doc["files"][0]))
        self.assertTrue(failures(doc))


if __name__ == "__main__":
    unittest.main()
