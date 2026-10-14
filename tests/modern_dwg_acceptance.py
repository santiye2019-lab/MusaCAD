#!/usr/bin/env python3
"""AC1032 (AutoCAD 2018-format / AutoCAD 2026 application) read-only CI gate.

Checks the actual pinned public DWG decode inventory; it does NOT claim that
DXF conversion is lossless or that DWG Save As, 3DSOLID, dynamic-block actions,
color fidelity, or modern CAD editing work. Private drawings are never needed.
"""
import argparse
import json
from pathlib import Path


MODERN_MAGIC = "AC1032"
EXPECTED_FIXTURE = "Dynblocks.dwg"


def failures(report: dict, fixture_name: str = EXPECTED_FIXTURE) -> list[str]:
    problems = []
    if report.get("source_is_read_only") is not True:
        problems.append("audit must declare read-only source")
    files = report.get("files")
    if not isinstance(files, list) or len(files) != 1:
        return problems + ["require exactly one audited public DWG fixture"]
    row = files[0]
    if row.get("filename") != fixture_name:
        problems.append("unexpected drawing fixture name")
    if row.get("acad_signature") != MODERN_MAGIC or row.get("dwg_release") != "R2018":
        problems.append("modern AC1032/R2018 signature is not independently verified")
    if (not row.get("source_unchanged") or
            not isinstance(row.get("sha256_before"), str) or
            len(row["sha256_before"]) != 64 or
            row.get("sha256_after") != row.get("sha256_before")):
        problems.append("original DWG hash changed or is unverified")
    if row.get("status") != "decoded_dxf_inventory":
        problems.append("DWG decode, exported DXF inventory or first paint incomplete")
    if row.get("native_scene_status") != 0 or row.get("native_scene_truncated") is not False:
        problems.append("native preview must complete without silent clipping")
    if not isinstance(row.get("dxf_bytes"), int) or row["dxf_bytes"] < 100:
        problems.append("DXF export is empty or unverified")
    if not isinstance(row.get("model_entity_count"), int) or not isinstance(
            row.get("block_geometry_entity_count"), int) or (
            row["model_entity_count"] + row["block_geometry_entity_count"] <= 0):
        problems.append("converted DXF entity inventory is empty")
    # This specific upstream Dynblocks fixture exercises actual BLOCK/INSERT
    # structures; its existence does not establish edit/action fidelity.
    if not isinstance(row.get("block_definition_count"), int) or row["block_definition_count"] < 1:
        problems.append("expected modern BLOCK definitions not observed")
    if not isinstance(row.get("block_inserts"), int) or row["block_inserts"] < 1:
        problems.append("expected modern INSERT records not observed")
    return problems


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("audit_json", type=Path)
    parser.add_argument("--fixture", default=EXPECTED_FIXTURE)
    args = parser.parse_args()
    report = json.loads(args.audit_json.read_text(encoding="utf-8"))
    errors = failures(report, args.fixture)
    for error in errors:
        print("FAIL AC1032:", error)
    if errors:
        return 1
    row = report["files"][0]
    print("PASS AC1032 reader smoke: version=", row["acad_signature"],
          "source_unchanged=", row["source_unchanged"],
          "model_entities=", row["model_entity_count"],
          "block_entities=", row["block_geometry_entity_count"],
          "blocks=", row["block_definition_count"],
          "inserts=", row["block_inserts"],
          "decoder_warnings=", row.get("native_converter_warning_mask"),
          "quality_warnings=", row.get("conversion_has_quality_warnings"),
          "(NOT an AutoCAD 2026 fidelity or DWG-writing acceptance)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
