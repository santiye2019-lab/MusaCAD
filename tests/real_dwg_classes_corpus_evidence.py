#!/usr/bin/env python3
"""Aggregate verified AC1024 numeric class metadata, without class-name claims.

This is a read-only report validator for the existing real DWG stage matrix.
Class numbers are drawing-local; numerical agreement across files is NOT a
semantic match and is never admitted as render or class-name evidence.
"""
from __future__ import annotations

import argparse
import json
from pathlib import Path


class InvalidEvidence(ValueError):
    pass


def boolean(value: object, label: str) -> bool:
    if type(value) is not bool:
        raise InvalidEvidence("invalid_bool:" + label)
    return value


def bounded_integer(value: object, label: str, maximum: int) -> int:
    if type(value) is not int or not 0 <= value <= maximum:
        raise InvalidEvidence("invalid_integer:" + label)
    return value


def summarize(stage: dict) -> dict:
    if not isinstance(stage, dict) or not isinstance(stage.get("files"), list):
        raise InvalidEvidence("invalid_stage_root")
    if stage.get("no_geometry_layers_blocks_colors_writing_claim") is not True:
        raise InvalidEvidence("source_stage_must_not_claim_geometry")
    files = stage["files"]
    if bounded_integer(stage.get("files_checked"), "files_checked", 100000) != len(files):
        raise InvalidEvidence("file_count_mismatch")

    ac1024 = 0
    numeric = 0
    observed505 = 0
    entities505 = 0
    objects505 = 0
    numerical_classes = 0
    unsupported = 0
    string_spans = 0
    extended_lengths = 0
    for record in files:
        if not isinstance(record, dict):
            raise InvalidEvidence("invalid_file_record")
        signature = record.get("signature")
        if not isinstance(signature, str):
            raise InvalidEvidence("missing_signature")
        if signature != "AC1024":
            continue
        ac1024 += 1
        if boolean(record.get("source_unchanged"), "source_unchanged") is not True:
            raise InvalidEvidence("source_modified")
        if boolean(record.get("geometry_verified"), "geometry_verified") or \
                boolean(record.get("rendering_verified"), "rendering_verified") or \
                boolean(record.get("source_modified"), "source_modified"):
            raise InvalidEvidence("unsupported_output_claim")
        hint = record.get("ac1024_compatibility_hypothesis")
        if not isinstance(hint, dict):
            raise InvalidEvidence("missing_ac1024_hint")
        authenticated = boolean(hint.get("source_classes_section_verified"), "section_verified")
        extracted = boolean(hint.get("source_classes_extracted"), "extracted")
        bitcount = boolean(hint.get("source_classes_r2010_bitcount_with_extra"), "bitcount")
        numeric_ok = boolean(hint.get("source_classes_r2010_numeric_verified"), "numeric")
        has505 = boolean(hint.get("source_classes_r2010_ot505_present"), "505_present")
        entity505 = boolean(hint.get("source_classes_r2010_ot505_entity"), "505_entity")
        object505 = boolean(hint.get("source_classes_r2010_ot505_object"), "505_object")
        count = bounded_integer(hint.get("source_classes_r2010_numeric_count"), "class_count", 4096)
        if numeric_ok:
            if not authenticated or not extracted or not bitcount or not count:
                raise InvalidEvidence("numeric_without_source_provenance")
            maximum = bounded_integer(
                hint.get("source_classes_r2010_max_class_with_extra"),
                "maximum_class_number", 5000)
            if maximum < 500 or count > maximum - 499:
                raise InvalidEvidence("invalid_class_number_range")
            numeric += 1
            numerical_classes += count
        else:
            unsupported += 1
            if count:
                raise InvalidEvidence("count_without_numeric_proof")
        span_ok = boolean(hint.get("source_classes_r2010_string_span_bounded"),
                          "string_span_bounded")
        span_bits = bounded_integer(hint.get("source_classes_r2010_string_bits"),
                                    "string_span_bits", 8 * 1024 * 1024)
        span_wide = boolean(hint.get("source_classes_r2010_string_wide_length"),
                            "string_span_wide")
        if span_ok:
            raw_bits = bounded_integer(
                hint.get("source_classes_r2010_bits_with_extra"),
                "section_bit_count", 8 * 1024 * 1024 * 8)
            if (not authenticated or not extracted or not bitcount
                    or not 0 < span_bits < raw_bits - 32):
                raise InvalidEvidence("span_without_source_provenance")
            string_spans += 1
            extended_lengths += int(span_wide)
        elif span_bits or span_wide:
            raise InvalidEvidence("span_values_without_proof")
        if has505:
            if not numeric_ok or (entity505 == object505):
                raise InvalidEvidence("505_without_unique_kind")
            observed505 += 1
            entities505 += int(entity505)
            objects505 += int(object505)
        elif entity505 or object505:
            raise InvalidEvidence("kind_without_505")

    return {
        "title": "MusaCAD verified AC1024 local numeric-class census",
        "source_files_checked": len(files),
        "ac1024_files_checked": ac1024,
        "ac1024_numeric_class_files_verified": numeric,
        "ac1024_numeric_class_files_unsupported": unsupported,
        "verified_numeric_class_record_total": numerical_classes,
        "bounded_class_string_span_files": string_spans,
        "extended_string_length_word_files": extended_lengths,
        "class505_number_present_files": observed505,
        "class505_object_kind_files": objects505,
        "class505_entity_kind_files": entities505,
        "class505_kind_varies_across_files": bool(objects505 and entities505),
        "original_class_names_decoded": False,
        "cross_file_class_identity_claim": False,
        "geometry_verified": False,
        "rendering_verified": False,
        "source_modified": False,
        "evidence_scope": "drawing-local numeric OT; no TU names, material or layer semantics",
    }


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--stage-report", type=Path, required=True)
    parser.add_argument("--report", type=Path, required=True)
    args = parser.parse_args()
    stage = json.loads(args.stage_report.read_text(encoding="utf-8"))
    result = summarize(stage)
    args.report.write_text(
        json.dumps(result, ensure_ascii=False, indent=2, sort_keys=True) + "\n",
        encoding="utf-8")
    print(
        "AC1024 verified numeric class files: "
        f"{result['ac1024_numeric_class_files_verified']}; "
        f"OT505 object={result['class505_object_kind_files']} "
        f"entity={result['class505_entity_kind_files']}; "
        "class names NOT decoded")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
