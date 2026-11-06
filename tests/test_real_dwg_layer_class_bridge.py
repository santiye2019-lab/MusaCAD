"""Source-scoped native OT51/OT505 bridge contract tests; no render admission."""
import copy
import unittest

from real_dwg_layer_class_bridge import EvidenceError, summarize, require_minimum_evidence


def sample(n=2, *, material=True):
    return {
        "signature": "AC1024",
        "source_modified": False,
        "source_unchanged": True,
        "geometry_verified": False,
        "rendering_verified": False,
        "ac1024_compatibility_hypothesis": {
            "sampled_layer_eed_terminal_candidate_verified": n,
            "sampled_layer_eed_terminal_candidate_matched": 3*n,
            "sampled_layer_eed_terminal_candidate_attempts": n,
            "sampled_layer_eed_terminal_triplet_attempts": n,
            "sampled_layer_eed_terminal_triplet_verified": n,
            "sampled_layer_eed_terminal_triplet_nonzero_fragment": 0,
            "sampled_layer_eed_terminal_candidate_absolute": 3*n,
            "sampled_layer_eed_terminal_candidate_unmatched": 0,
            "sampled_layer_eed_terminal_fragment_attempts": n,
            "sampled_layer_eed_terminal_fragment_verified": n,
            "sampled_layer_eed_terminal_fragment_bits_2": n,
            "sampled_layer_eed_terminal_fragment_bits_4": 0,
            "sampled_layer_eed_terminal_fragment_zero": n,
            "sampled_layer_eed_terminal_fragment_nonzero": 0,
            "sampled_layer_eed_terminal_fragment_prefix_four_tokens": n,
            "sampled_layer_eed_terminal_fragment_2bit_pattern_mask": 1 if n else 0,
            "sampled_layer_eed_terminal_fragment_4bit_pattern_mask": 0,
            "sampled_layer_eed_terminal_fragment_start_mod8_mask": 64 if n else 0,
            "sampled_layer_eed_terminal_fragment_end_mod8_mask": 1 if n else 0,
            "sampled_layer_eed_terminal_candidate_ot80_placeholder": n,
            "sampled_layer_eed_terminal_candidate_ot57_ltype": n,
            "sampled_layer_eed_terminal_candidate_dynamic_class_unresolved": n,
            "sampled_layer_eed_terminal_candidate_ordinal_2_matched": n,
            "sampled_layer_eed_terminal_candidate_ordinal_3_matched": n,
            "sampled_layer_eed_terminal_candidate_ordinal_4_matched": n,
            "sampled_layer_eed_terminal_candidate_ordinal_2_raw_ot_consensus": 81 if n else 0,
            "sampled_layer_eed_terminal_candidate_ordinal_3_raw_ot_consensus": 506 if n else 0,
            "sampled_layer_eed_terminal_candidate_ordinal_4_raw_ot_consensus": 58 if n else 0,
            "source_classes_section_verified": True,
            "source_classes_extracted": True,
            "source_classes_r2010_numeric_verified": True,
            "source_classes_r2010_505_dxf_identifier_verified": material,
            "source_classes_r2010_505_dxf_identifier": "MATERIAL" if material else "",
            "source_classes_r2010_ot505_present": material,
            "source_classes_r2010_ot505_object": material,
            "source_classes_r2010_tu_verified": material,
        },
    }


def stage(*files):
    return {
        "files_checked": len(files),
        "files": list(files),
        "no_geometry_layers_blocks_colors_writing_claim": True,
    }


class LayerClassBridgeTests(unittest.TestCase):
    def test_supported_source_scoped_joins(self):
        out = summarize(stage(sample(2), sample(4), sample(4), sample(2), sample(2)))
        self.assertEqual(out["layer_samples_with_three_crc_backed_targets"], 14)
        self.assertEqual(out["layer_samples_local_ot505_material_class_proven"], 14)
        self.assertEqual(out["drawings_with_local_material_class_join"], 5)
        self.assertEqual(out["verified_raw_target_records"], 42)
        self.assertEqual(out["first_party_native_type_triplets_verified"], 14)
        self.assertTrue(out["fixed_placeholder_ot80_identity_confirmed"])
        self.assertTrue(out["fixed_ltype_ot57_identity_confirmed"])
        self.assertEqual(out["terminal_fragment_2_bit_samples"], 14)
        self.assertEqual(out["terminal_fragment_undecoded_samples"], 14)
        self.assertEqual(out["terminal_fragment_2bit_pattern_values_seen"], [0])
        self.assertEqual(out["terminal_fragment_4bit_pattern_values_seen"], [])
        self.assertFalse(out["h_tail_fully_decoded"])
        self.assertFalse(out["material_property_link_semantics_proven"])
        self.assertFalse(out["ot57_semantics_proven"])
        self.assertFalse(out["layer_visibility_parity_proven"])

    def test_505_is_not_assumed_shared_across_drawings(self):
        out = summarize(stage(sample(2), sample(4, material=False)))
        self.assertEqual(out["layer_samples_with_three_crc_backed_targets"], 6)
        self.assertEqual(out["layer_samples_local_ot505_material_class_proven"], 2)
        self.assertEqual(out["layer_samples_with_unresolved_target_class"], 4)

    def test_no_success_without_three_same_source_target_roles(self):
        bad = sample(4)
        h = bad["ac1024_compatibility_hypothesis"]
        h["sampled_layer_eed_terminal_candidate_ordinal_3_raw_ot_consensus"] = 57
        h["sampled_layer_eed_terminal_triplet_verified"] = 0
        out = summarize(stage(bad))
        self.assertEqual(out["layer_samples_with_three_crc_backed_targets"], 0)
        self.assertEqual(out["layer_samples_with_unresolved_target_class"], 4)

    def test_reject_fabricated_total_or_missing_original_crc(self):
        for field, value in [
            ("sampled_layer_eed_terminal_candidate_matched", 0),
            ("source_classes_section_verified", False),
            ("source_classes_extracted", False),
            ("source_classes_r2010_numeric_verified", False),
        ]:
            bad = sample(2)
            bad["ac1024_compatibility_hypothesis"][field] = value
            with self.subTest(field=field), self.assertRaises(EvidenceError):
                summarize(stage(bad))

    def test_reject_claims_when_source_changed(self):
        for field, value in [
            ("source_modified", True),
            ("source_unchanged", False),
            ("geometry_verified", True),
            ("rendering_verified", True),
        ]:
            bad = sample()
            bad[field] = value
            with self.subTest(field=field), self.assertRaises(EvidenceError):
                summarize(stage(bad))

    def test_nonzero_terminal_fragment_is_not_painted_or_dismissed(self):
        item = sample(2)
        h = item["ac1024_compatibility_hypothesis"]
        h["sampled_layer_eed_terminal_fragment_zero"] = 0
        h["sampled_layer_eed_terminal_fragment_nonzero"] = 2
        h["sampled_layer_eed_terminal_triplet_nonzero_fragment"] = 2
        h["sampled_layer_eed_terminal_fragment_bits_2"] = 0
        h["sampled_layer_eed_terminal_fragment_bits_4"] = 2
        h["sampled_layer_eed_terminal_fragment_2bit_pattern_mask"] = 0
        h["sampled_layer_eed_terminal_fragment_4bit_pattern_mask"] = 1 << 5
        h["sampled_layer_eed_terminal_fragment_start_mod8_mask"] = 1 << 4
        observed = summarize(stage(item))
        self.assertEqual(observed["terminal_fragment_nonzero_samples"], 2)
        self.assertEqual(observed["terminal_fragment_4_bit_samples"], 2)
        self.assertEqual(observed["terminal_fragment_undecoded_samples"], 2)
        self.assertFalse(observed["layer_visibility_parity_proven"])
        self.assertFalse(observed["rendering_verified"])
        self.assertFalse(observed["h_tail_fully_decoded"])

    def test_corrupt_tail_shape_and_excess_target_counts_refused(self):
        for field, value in (
            ("sampled_layer_eed_terminal_fragment_verified", 0),
            ("sampled_layer_eed_terminal_fragment_bits_2", 1),
            ("sampled_layer_eed_terminal_fragment_zero", 3),
            ("sampled_layer_eed_terminal_fragment_prefix_four_tokens", 0),
            ("sampled_layer_eed_terminal_candidate_absolute", 7),
            ("sampled_layer_eed_terminal_candidate_matched", 7),
            ("sampled_layer_eed_terminal_candidate_ot80_placeholder", 3),
            ("sampled_layer_eed_terminal_candidate_unmatched", 1),
            ("sampled_layer_eed_terminal_triplet_attempts", 1),
            ("sampled_layer_eed_terminal_triplet_verified", 1),
            ("sampled_layer_eed_terminal_triplet_nonzero_fragment", 1),
        ):
            item = sample(2)
            item["ac1024_compatibility_hypothesis"][field] = value
            with self.subTest(field=field), self.assertRaises(EvidenceError):
                summarize(stage(item))

    def test_real_corpus_minimum_coverage_gate(self):
        out = summarize(stage(sample(2), sample(4), sample(4), sample(2), sample(2)))
        require_minimum_evidence(out, 14, 5)
        for native_min, drawings_min in ((15, 5), (14, 6), (1, 6)):
            with self.subTest(native=native_min, drawings=drawings_min):
                with self.assertRaises(EvidenceError):
                    require_minimum_evidence(out, native_min, drawings_min)
        for threshold in (-1, 2.5, True):
            with self.subTest(threshold=threshold), self.assertRaises(EvidenceError):
                require_minimum_evidence(out, threshold, 0)
        only_fourteen = summarize(stage(sample(2), sample(4), sample(4),
                                        sample(4, material=False)))
        with self.assertRaises(EvidenceError):
            require_minimum_evidence(only_fourteen, 14, 5)

    def test_pattern_without_original_suffix_is_rejected(self):
        for key, value in (
            ("sampled_layer_eed_terminal_fragment_2bit_pattern_mask", 0),
            ("sampled_layer_eed_terminal_fragment_4bit_pattern_mask", 2),
            ("sampled_layer_eed_terminal_fragment_start_mod8_mask", 0),
            ("sampled_layer_eed_terminal_fragment_end_mod8_mask", 0),
        ):
            invalid = sample(2)
            invalid["ac1024_compatibility_hypothesis"][key] = value
            with self.subTest(key=key), self.assertRaises(EvidenceError):
                summarize(stage(invalid))

    def test_missing_provenance_is_rejected(self):
        bad = sample()
        bad["ac1024_compatibility_hypothesis"].pop(
            "sampled_layer_eed_terminal_candidate_ordinal_4_matched")
        with self.assertRaises(EvidenceError):
            summarize(stage(bad))
        with self.assertRaises(EvidenceError):
            summarize({"files_checked": 2, "files": [sample()],
                       "no_geometry_layers_blocks_colors_writing_claim": True})


if __name__ == "__main__":
    unittest.main()
