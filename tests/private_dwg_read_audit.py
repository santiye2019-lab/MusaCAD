#!/usr/bin/env python3
"""Private, read-only real-DWG acceptance audit.

The tool NEVER uploads or modifies source DWGs. It records original hashes,
converts with the repo's native host converter in a temporary directory and
collects a DXF layer/block/entity inventory. This inventory does not prove
semantic fidelity; compare against independent CAD-export baselines separately.

Examples:
  python3 tests/private_dwg_read_audit.py --manifest-only --report manifest.json FILE.dwg
  python3 tests/private_dwg_read_audit.py --converter build/musacad_dwg_convert --report audit.json FILE.dwg
"""
from __future__ import annotations

import argparse
from collections import Counter
from native_preview_type_audit import audit_types
from dwg_entity_identity_audit import DwgEntityIdentityAudit
import hashlib
import json
import os
from pathlib import Path
import re
import subprocess
import sys
import tempfile
import time

VERSIONS = {
    "AC1015": "R2000", "AC1018": "R2004", "AC1021": "R2007",
    "AC1024": "R2010", "AC1027": "R2013", "AC1032": "R2018",
}


def sha256_file(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for chunk in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def dxf_pairs(path: Path):
    # ASCII DXF group-code/value pairs. A binary/invalid DXF fails closed.
    with path.open("r", encoding="utf-8", errors="replace", newline=None) as stream:
        while True:
            key = stream.readline()
            if not key:
                return
            value = stream.readline()
            if not value:
                raise ValueError("truncated DXF group code/value pair")
            code = key.strip()
            if not re.fullmatch(r"[-+]?\d+", code):
                raise ValueError("non-ASCII/non-DXF pair code")
            yield int(code), value.strip()


def inventory_dxf(path: Path):
    section = ""
    current = None
    fields = []
    layers = {}
    blocks = {}
    model = Counter()
    block_geometry = Counter()
    identity = DwgEntityIdentityAudit()
    # Raw decoded-DXF attributes, NOT proof that native Canvas displayed them.
    color_sources = Counter()
    advanced_entities = Counter()
    has_eof = False
    sections_seen = set()
    total_records = 0

    def finish():
        nonlocal section, total_records
        if current is None:
            return
        data = {code: val for code, val in fields}
        # Preserve every raw group-5 / group-8 field before collapsing duplicate
        # codes into the convenience map used by the color/shape inventory.
        # Never serialize identifiers or layer names into these diagnostics.
        identity.observe(section,current,fields)
        is_drawable_entity = (
            section in ("ENTITIES", "BLOCKS") and
            current not in ("SECTION", "ENDSEC", "EOF", "BLOCK",
                            "ENDBLK", "ENDTAB", "TABLE")
        )
        if is_drawable_entity:
            # DXF 420 overrides ACI 62 when valid. 0=ByBlock, 256/absent
            # =ByLayer. Record invalid colors separately, never pretend they
            # are accurate or silently classify them as indexed colors.
            rgb = data.get(420)
            aci = data.get(62)
            if rgb is not None:
                try:
                    value = int(rgb)
                except ValueError:
                    value = -1
                color_sources["truecolor" if 0 <= value <= 0xFFFFFF
                              else "invalid_truecolor"] += 1
            elif aci is None:
                color_sources["bylayer"] += 1
            else:
                try:
                    color = int(aci)
                except ValueError:
                    color = None
                if color is None:
                    color_sources["invalid_aci"] += 1
                elif color == 0:
                    color_sources["byblock"] += 1
                elif color == 256:
                    color_sources["bylayer"] += 1
                elif 1 <= abs(color) <= 255:
                    color_sources["explicit_aci"] += 1
                else:
                    color_sources["invalid_aci"] += 1
            if current in (
                "3DSOLID", "BODY", "REGION", "ACAD_PROXY_ENTITY",
                "DIMENSION", "HATCH", "MTEXT", "ACAD_TABLE",
                "INSERT", "IMAGE", "OLE2FRAME", "SPLINE",
            ):
                advanced_entities[current] += 1
        if current == "SECTION":
            section = data.get(2, "").upper()
            sections_seen.add(section)
        elif current == "ENDSEC":
            section = ""
        elif current == "LAYER" and section == "TABLES":
            name = data.get(2)
            if name:
                layers[name] = {"color_aci": data.get(62), "flags": data.get(70)}
        elif current == "BLOCK" and section == "BLOCKS":
            name = data.get(2)
            if name:
                try:
                    flag = int(data.get(70, "0"))
                except ValueError:
                    flag = 0
                blocks[name] = {"flags": flag, "xref": bool(flag & 12)}
        elif section == "ENTITIES" and current not in ("ENDTAB", "TABLE"):
            model[current] += 1
            total_records += 1
        elif section == "BLOCKS" and current not in ("BLOCK", "ENDBLK"):
            block_geometry[current] += 1
            total_records += 1

    for code, value in dxf_pairs(path):
        if code == 0:
            finish()
            current = value.upper()
            fields = []
            if current == "EOF":
                has_eof = True
        elif current is not None:
            fields.append((code, value))
    finish()
    if not has_eof or "ENTITIES" not in sections_seen:
        raise ValueError("missing DXF EOF or ENTITIES section")
    if total_records == 0:
        raise ValueError("converter emitted no model/block geometry")
    return {
        **audit_types(dict(model), dict(block_geometry)),
        **identity.summary(),
        "dxf_bytes": path.stat().st_size,
        "section_names": sorted(sections_seen),
        "layer_count": len(layers),
        "layer_names": sorted(layers),
        "block_definition_count": len(blocks),
        "xref_block_definitions": sum(1 for b in blocks.values() if b["xref"]),
        "block_names": sorted(blocks),
        "model_entity_count": sum(model.values()),
        "block_geometry_entity_count": sum(block_geometry.values()),
        "model_entity_types": dict(sorted(model.items())),
        "block_geometry_types": dict(sorted(block_geometry.items())),
        "block_inserts": model["INSERT"] + block_geometry["INSERT"],
        "model_proxy_entities": model["ACAD_PROXY_ENTITY"],
        "converted_dxf_color_sources": dict(sorted(color_sources.items())),
        "converted_dxf_advanced_entity_counts": dict(sorted(advanced_entities.items())),
        "color_audit_scope": "DXF entity codes 420/62 only; does NOT validate layer/block inheritance, viewport visibility or rendered screen pixels",
        "inventory_scope": "DWG decoded to DXF by MusaCAD host converter; not independent DWG fidelity proof",
    }


def convert_one(input_path: Path, converter: Path, temp: Path, timeout: int, limit_mb: int):
    output = temp / "converted.dxf"
    error_log = temp / "converter.stderr"
    def memory_limit():
        if os.name == "posix" and sys.platform.startswith("linux"):
            import resource
            maxbytes = limit_mb * 1024 * 1024
            resource.setrlimit(resource.RLIMIT_AS, (maxbytes, maxbytes))
    started = time.monotonic()
    try:
        with error_log.open("wb") as log:
            run = subprocess.run(
                [str(converter), str(input_path), str(output)],
                stdout=subprocess.DEVNULL, stderr=log, timeout=timeout,
                check=False, preexec_fn=memory_limit if os.name == "posix" else None,
            )
        status = run.returncode
    except subprocess.TimeoutExpired:
        return {"status": "timeout", "wall_seconds": round(time.monotonic()-started, 3)}
    except OSError as exc:
        return {"status": "launch_error", "error_type": type(exc).__name__}
    result = {"converter_returncode": status,
              "wall_seconds": round(time.monotonic()-started, 3)}
    diagnostics = error_log.read_text(encoding="utf-8", errors="replace")
    scene_match = re.search(
        r"MusaCAD native scene: status=(-?\d+) primitives=(\d+) floats=(\d+)",
        diagnostics,
    )
    if scene_match:
        result["native_scene_status"] = int(scene_match.group(1))
        result["native_scene_primitives"] = int(scene_match.group(2))
        result["native_scene_floats"] = int(scene_match.group(3))
        result["native_scene_truncated"] = result["native_scene_status"] == 1
    else:
        result["native_scene_status"] = None
        result["native_scene_truncated"] = None
    # Count-only decoder diagnostics. Never attach potentially sensitive
    # drawing coordinates, text, filenames or raw converter stderr to JSON.
    # These records contain ONLY enumerated numeric spline metadata.
    # The raw stderr stays local; coordinates/handles/text are never emitted.
    spline_diagnostics = []
    spline_pattern = re.compile(
        r"MusaCAD spline rejected: scenario=(-?\d+) degree=(-?\d+) "
        r"control=(\d+) knots=(\d+) fit=(\d+) "
        r"weighted=(\d+) rational=(\d+) "
        r"reason=(missing_knots_or_fit_only|bad_weights_knots_or_budget)"
        r"(?: ctrl_pointer=(\d+) knots_pointer=(\d+) closed=(\d+)"
        r" periodic=(\d+) begin_tangent_zero=(\d+) end_tangent_zero=(\d+))?"
    )
    for match in spline_pattern.finditer(diagnostics):
        if len(spline_diagnostics) >= 32:
            break
        scenario, degree, control, knots, fit, weighted, rational = (
            int(value) for value in match.groups()[:7])
        spline_diagnostics.append({
            "scenario": scenario, "degree": degree, "control": control,
            "knots": knots, "fit": fit, "weighted": weighted,
            "rational": rational, "reason": match.group(8),
            "ctrl_pointer": int(match.group(9)) if match.group(9) else None,
            "knots_pointer": int(match.group(10)) if match.group(10) else None,
            "closed": int(match.group(11)) if match.group(11) else None,
            "periodic": int(match.group(12)) if match.group(12) else None,
            "begin_tangent_zero": int(match.group(13)) if match.group(13) else None,
            "end_tangent_zero": int(match.group(14)) if match.group(14) else None,
        })
    result["native_spline_rejection_details"] = spline_diagnostics
    fit_diagnostics = []
    fit_failure_pattern = re.compile(
        r"MusaCAD fit spline rejected: stage=(build|scene_budget|evaluation|scene_coordinate)"
        r" fit=(\d+)"
        r"(?: duplicate_adjacent=(\d+) nonfinite_fit=(\d+)"
        r" begin_tangent_finite=(\d+) end_tangent_finite=(\d+))?"
        r"(?: begin_tangent_zero=(\d+) end_tangent_zero=(\d+))?"
    )
    for match in fit_failure_pattern.finditer(diagnostics):
        if len(fit_diagnostics) >= 32:
            break
        fit_diagnostics.append({
            "stage": match.group(1),
            "fit": int(match.group(2)),
            "duplicate_adjacent": int(match.group(3)) if match.group(3) else None,
            "nonfinite_fit": int(match.group(4)) if match.group(4) else None,
            "begin_tangent_finite": int(match.group(5)) if match.group(5) else None,
            "end_tangent_finite": int(match.group(6)) if match.group(6) else None,
            "begin_tangent_zero": int(match.group(7)) if match.group(7) else None,
            "end_tangent_zero": int(match.group(8)) if match.group(8) else None,
        })
    result["native_fit_spline_stage_diagnostics"] = fit_diagnostics
    # A zero converter exit must not hide any native SPLINE rejection.
    # Keep counts only; never expose source coordinates, names or stderr.
    result["native_spline_rejection_count"] = (
        diagnostics.count("MusaCAD spline rejected:") +
        diagnostics.count("MusaCAD fit spline rejected:")
    )

    result["native_spline_fit_only_rejections"] = diagnostics.count(
        "reason=missing_knots_or_fit_only")
    result["native_spline_invalid_weight_rejections"] = diagnostics.count(
        "reason=bad_weights_knots_or_budget")
    result["utf8_decode_warning_count"] = diagnostics.count("BAD_CONTINUATION_BYTE")
    converter_match = re.search(r"MusaCAD converter status: (-?\d+)", diagnostics)
    if converter_match:
        mask = int(converter_match.group(1))
        result["native_converter_warning_mask"] = mask
        result["value_out_of_bounds_warning"] = bool(mask & 64)
        result["unhandled_class_warning"] = bool(mask & 4)
        result["other_converter_warnings"] = bool(mask & ~(64 | 4))
    if status != 0 or not output.is_file():
        result["status"] = "decode_failed"
        return result
    try:
        result.update(inventory_dxf(output))
        if result.get("native_scene_truncated") is True:
            # The 9 Mi-float native first-paint cap clipped this drawing.
            result["status"] = "decoded_dxf_inventory_scene_truncated"
        elif result["native_scene_status"] is None:
            # Old/unknown converter cannot certify the native first-paint scene.
            result["status"] = "decoded_dxf_inventory_scene_unverified"
        elif result["native_scene_status"] != 0 or result["native_spline_rejection_count"]:
            # Fail closed even if converter exits 0 or an unexpected native
            # scene status was logged. A decoded DXF is not a complete preview.
            result["status"] = "decoded_dxf_inventory_scene_incomplete"
        else:
            result["status"] = "decoded_dxf_inventory"
        result["conversion_has_quality_warnings"] = bool(
            result.get("native_converter_warning_mask", 0)
            or result["utf8_decode_warning_count"]
        )
    except (ValueError, OSError) as exc:
        result["status"] = "invalid_converted_dxf"
        result["error_type"] = type(exc).__name__
    return result


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("drawings", type=Path, nargs="+")
    parser.add_argument("--converter", type=Path)
    parser.add_argument("--manifest-only", action="store_true")
    parser.add_argument("--report", type=Path, required=True)
    parser.add_argument("--timeout-sec", type=int, default=180)
    parser.add_argument("--max-vmem-mb", type=int, default=1536)
    args = parser.parse_args()
    if not args.manifest_only and not args.converter:
        parser.error("full audit requires --converter; use --manifest-only for signature inventory")
    if args.timeout_sec <= 0 or args.max_vmem_mb <= 0:
        parser.error("time and memory limits must be positive")
    converter = args.converter.resolve() if args.converter else None
    if converter and (not converter.is_file() or not os.access(converter, os.X_OK)):
        parser.error("host converter does not exist or is not executable")
    report = {"schema_version": 1, "privacy": "private local file analysis; no uploads",
              "source_is_read_only": True, "files": []}
    failure = False
    for source in args.drawings:
        if not source.is_file():
            parser.error("missing input drawing: " + str(source))
        source = source.resolve()
        with source.open("rb") as stream:
            signature = stream.read(6).decode("ascii", errors="replace")
        digest = sha256_file(source)
        row = {"filename": source.name, "bytes": source.stat().st_size,
               "sha256_before": digest, "acad_signature": signature,
               "dwg_release": VERSIONS.get(signature, "unsupported_or_unknown")}
        if signature not in VERSIONS:
            row["status"] = "unsupported_signature"
            failure = True
        elif args.manifest_only:
            row["status"] = "metadata_only_not_geometry_checked"
        else:
            with tempfile.TemporaryDirectory(prefix="musacad-private-dwg-") as tmp:
                row.update(convert_one(source, converter, Path(tmp),
                                       args.timeout_sec, args.max_vmem_mb))
            if row["status"] != "decoded_dxf_inventory":
                failure = True
        row["sha256_after"] = sha256_file(source)
        row["source_unchanged"] = row["sha256_before"] == row["sha256_after"]
        if not row["source_unchanged"]:
            failure = True
        report["files"].append(row)
        print(row["filename"], row["dwg_release"], row["status"],
              "source_unchanged=" + str(row["source_unchanged"]), flush=True)
    args.report.parent.mkdir(parents=True, exist_ok=True)
    args.report.write_text(json.dumps(report, indent=2, ensure_ascii=False) + "\n",
                           encoding="utf-8")
    return 1 if failure else 0


if __name__ == "__main__":
    sys.exit(main())
