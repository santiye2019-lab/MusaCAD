"""Negative evidence-contract checks for the AC1024 numeric class census."""
import unittest

from real_dwg_classes_corpus_evidence import InvalidEvidence, summarize


def sample(*, numeric=True, has505=True, obj=True):
    return {
        "signature": "AC1024",
        "source_unchanged": True,
        "source_modified": False,
        "geometry_verified": False,
        "rendering_verified": False,
        "ac1024_compatibility_hypothesis": {
            "source_classes_section_verified": True,
            "source_classes_extracted": True,
            "source_classes_r2010_bitcount_with_extra": True,
            "source_classes_r2010_numeric_verified": numeric,
            "source_classes_r2010_bits_with_extra": 1024,
            "source_classes_r2010_string_span_bounded": False,
            "source_classes_r2010_string_bits": 0,
            "source_classes_r2010_string_wide_length": False,
            "source_classes_r2010_tu_verified": False,
            "source_classes_r2010_tu_fields": 0,
            "source_classes_r2010_tu_units": 0,
            "source_classes_r2010_tu_505_three_nonempty": False,
            "source_classes_r2010_tu_505_ascii_only": False,
            "source_classes_r2010_numeric_count": 59 if numeric else 0,
            "source_classes_r2010_max_class_with_extra": 558,
            "source_classes_r2010_ot505_present": has505,
            "source_classes_r2010_ot505_entity": has505 and not obj,
            "source_classes_r2010_ot505_object": has505 and obj,
        },
    }


def report(*files):
    return {
        "files_checked": len(files),
        "no_geometry_layers_blocks_colors_writing_claim": True,
        "files": list(files),
    }


class NumericClassCensusTest(unittest.TestCase):
    def test_evidence_stays_drawing_scoped(self):
        outcome = summarize(report(sample(), sample(), sample(obj=False)))
        self.assertEqual(outcome["ac1024_numeric_class_files_verified"], 3)
        self.assertEqual(outcome["class505_object_kind_files"], 2)
        self.assertEqual(outcome["class505_entity_kind_files"], 1)
        self.assertTrue(outcome["class505_kind_varies_across_files"])
        self.assertFalse(outcome["original_class_names_decoded"])
        self.assertFalse(outcome["cross_file_class_identity_claim"])
        self.assertFalse(outcome["rendering_verified"])

    def test_unresolved_class_is_not_invented(self):
        x = sample(numeric=False, has505=False)
        x["ac1024_compatibility_hypothesis"]["source_classes_section_verified"] = False
        x["ac1024_compatibility_hypothesis"]["source_classes_extracted"] = False
        x["ac1024_compatibility_hypothesis"]["source_classes_r2010_bitcount_with_extra"] = False
        data = summarize(report(x))
        self.assertEqual(data["ac1024_numeric_class_files_unsupported"], 1)
        self.assertEqual(data["class505_number_present_files"], 0)

    def test_no_unverified_provenance(self):
        for key in ("source_classes_section_verified", "source_classes_extracted",
                    "source_classes_r2010_bitcount_with_extra"):
            x = sample()
            x["ac1024_compatibility_hypothesis"][key] = False
            with self.subTest(key=key), self.assertRaises(InvalidEvidence):
                summarize(report(x))

    def test_reject_false_505_claims(self):
        for field, value in (("source_classes_r2010_ot505_entity", True),
                             ("source_classes_r2010_ot505_object", False),
                             ("source_classes_r2010_numeric_count", 0),
                             ("source_classes_r2010_numeric_verified", False)):
            x = sample()
            x["ac1024_compatibility_hypothesis"][field] = value
            with self.subTest(field=field), self.assertRaises(InvalidEvidence):
                summarize(report(x))

    def test_no_source_or_render_claim(self):
        for field, value in (("source_unchanged", False),
                             ("source_modified", True),
                             ("rendering_verified", True),
                             ("geometry_verified", True)):
            x = sample()
            x[field] = value
            with self.subTest(field=field), self.assertRaises(InvalidEvidence):
                summarize(report(x))

    def test_class_string_span_stays_structural(self):
        x = sample()
        h = x["ac1024_compatibility_hypothesis"]
        h["source_classes_r2010_string_span_bounded"] = True
        h["source_classes_r2010_string_bits"] = 128
        outcome = summarize(report(x))
        self.assertEqual(outcome["bounded_class_string_span_files"], 1)
        self.assertFalse(outcome["original_class_names_decoded"])
        self.assertFalse(outcome["cross_file_class_identity_claim"])
        for field, value in (
            ("source_classes_section_verified", False),
            ("source_classes_extracted", False),
            ("source_classes_r2010_bitcount_with_extra", False),
            ("source_classes_r2010_string_bits", 1024),
        ):
            bad = sample()
            b = bad["ac1024_compatibility_hypothesis"]
            b["source_classes_r2010_string_span_bounded"] = True
            b["source_classes_r2010_string_bits"] = 128
            b[field] = value
            with self.subTest(field=field), self.assertRaises(InvalidEvidence):
                summarize(report(b))

    def test_verified_utf16_tu_never_becomes_a_class_semantic_claim(self):
        x=sample()
        h=x["ac1024_compatibility_hypothesis"]
        h["source_classes_r2010_string_span_bounded"]=True
        h["source_classes_r2010_string_bits"]=1024
        h["source_classes_r2010_tu_verified"]=True
        h["source_classes_r2010_tu_fields"]=177
        h["source_classes_r2010_tu_units"]=45
        h["source_classes_r2010_tu_505_three_nonempty"]=True
        h["source_classes_r2010_tu_505_ascii_only"]=True
        outcome=summarize(report(x))
        self.assertEqual(outcome["complete_utf16_tu_class_files"],1)
        self.assertEqual(outcome["verified_utf16_tu_fields"],177)
        self.assertFalse(outcome["original_class_names_decoded"])
        self.assertFalse(outcome["cross_file_class_identity_claim"])
        for field,value in (
            ("source_classes_r2010_tu_fields",176),
            ("source_classes_r2010_tu_verified",False),
            ("source_classes_r2010_numeric_verified",False),
            ("source_classes_r2010_string_span_bounded",False),
        ):
            bad=sample()
            t=bad["ac1024_compatibility_hypothesis"]
            t.update(h)
            t[field]=value
            with self.subTest(field=field),self.assertRaises(InvalidEvidence):
                summarize(report(bad))

    def test_reject_missing_fields_and_bad_count(self):
        x = sample()
        x["ac1024_compatibility_hypothesis"].pop("source_classes_r2010_ot505_present")
        with self.assertRaises(InvalidEvidence):
            summarize(report(x))
        x = sample()
        with self.assertRaises(InvalidEvidence):
            summarize({**report(x), "files_checked": 2})


if __name__ == "__main__":
    unittest.main()
