"""Local-only DWG/DXF object identity integrity and source-manifest parity.

The decoded DXF is NOT the authoritative DWG. Capture entity handles (group 5),
layers (group 8), type and model/paperspace/block identity without returning their values
in diagnostics. An independent DWG reader may supply a source manifest to
compare in memory; summaries report counts only, never handles/layers/text.
"""
from __future__ import annotations

from collections import Counter
from decimal import Decimal
from dwg_dxf_semantic_parity import _normalized
import re

# Limited geometry comparison for independently supplied ORIGINAL DWG data.
# Not full CAD fidelity: excludes elevations, colors, thickness, XREF, ACIS,
# block transforms and viewport appearance beyond the fields listed below.
_CORE_GEOMETRY = {
    "LINE": ((10, 20, 11, 21), {30: "0", 31: "0"}),
    "POINT": ((10, 20), {30: "0", 210: "0", 220: "0", 230: "1"}),
    "CIRCLE": ((10, 20, 40), {30: "0", 210: "0", 220: "0", 230: "1"}),
    "ARC": ((10, 20, 40, 50, 51),
            {30: "0", 210: "0", 220: "0", 230: "1"}),
}

_HEX_HANDLE = re.compile(r"[0-9A-Fa-f]{1,32}\Z")
_SPACES = {"ENTITIES": "model", "BLOCKS": "block"}
_SKIP = {"SECTION", "ENDSEC", "EOF", "BLOCK", "ENDBLK", "TABLE",
         "ENDTAB"}
_MAX_OBJECTS = 2_000_000


class DwgEntityIdentityAudit:
    def __init__(self, geometry_enabled=False):
        self.geometry_enabled = bool(geometry_enabled)
        self.core_geometry = {} if geometry_enabled else None
        self.invalid_core_geometry = 0
        self.entities = {}
        self.total = 0
        self.missing_handle = 0
        self.invalid_handle = 0
        self.duplicate_handle = 0
        self.missing_layer = 0
        self.duplicate_handle_field = 0
        self.duplicate_layer_field = 0
        self.invalid_space_fields = 0

    @staticmethod
    def _one(fields, code):
        return [value for key, value in fields if key == code]

    @staticmethod
    def _handle(value):
        if not isinstance(value, str) or not _HEX_HANDLE.fullmatch(value):
            return None
        numeric = int(value, 16)
        return format(numeric, "X") if numeric else None

    @staticmethod
    def _primitive_geometry(kind, groups, reference=False):
        """Canonical exact numeric subset; never return values in diagnostics.

        Reference records use a mapping of decimal DXF group-code strings to
        numeric strings. Source DXF records are ordered (group-code, value)
        pairs. Missing optional Z and OCS normals use DWG/DXF documented
        defaults. Fail on missing/duplicate/malformed covered numeric fields.
        """
        spec = _CORE_GEOMETRY.get(kind)
        if spec is None:
            raise ValueError("unsupported independent geometry type")
        mandatory, defaults = spec
        allowed = set(mandatory) | set(defaults)
        if reference:
            if not isinstance(groups, dict) or len(groups) > len(allowed):
                raise ValueError("invalid reference geometry field mapping")
            pairs = []
            for key, value in groups.items():
                if not isinstance(key, str) or not key.isdecimal():
                    raise ValueError("invalid reference geometry group code")
                code = int(key)
                if key != str(code) or code not in allowed:
                    raise ValueError("unknown reference geometry field")
                pairs.append((code, value))
        else:
            pairs = [(code, value) for code, value in groups if code in allowed]
        fields = {}
        for code, value in pairs:
            if code in fields or not isinstance(value, str) or len(value) > 128:
                raise ValueError("invalid or duplicate geometry coordinate")
            fields[code] = value
        if any(code not in fields for code in mandatory):
            raise ValueError("incomplete primitive geometry")
        canonical = []
        for code in sorted(allowed):
            raw = fields.get(code, defaults.get(code))
            value = _normalized(code, raw)
            if code == 40 and Decimal(value) <= 0:
                raise ValueError("nonpositive primitive radius")
            canonical.append((code, value))
        return tuple(canonical)

    @classmethod
    def _space(cls, section, fields):
        """Classify ENTITIES layout space without publishing private sheet names.

        DXF 67 is the paper-space flag; 410 is the layout name. BLOCKS
        records represent block definition content, not paper-space instances.
        Contradictory, duplicate or invalid space metadata fails closed.
        """
        if section == "BLOCKS":
            return "block", True
        flags = cls._one(fields, 67)
        layouts = cls._one(fields, 410)
        if len(flags) > 1 or len(layouts) > 1:
            return "invalid", False
        by_flag = None
        if flags:
            by_flag = {"0": "model", "1": "paperspace"}.get(flags[0])
            if by_flag is None:
                return "invalid", False
        by_layout = None
        if layouts:
            if not layouts[0]:
                return "invalid", False
            by_layout = ("model" if layouts[0].casefold() == "model"
                         else "paperspace")
        if by_flag and by_layout and by_flag != by_layout:
            return "invalid", False
        return by_flag or by_layout or "model", True

    def observe(self, section, kind, fields):
        if section not in _SPACES or kind in _SKIP:
            return
        self.total += 1
        if self.total > _MAX_OBJECTS:
            raise ValueError("object identity audit safety cap exceeded")
        handles = self._one(fields, 5)
        layers = self._one(fields, 8)
        space, valid_space = self._space(section, fields)
        if not valid_space:
            self.invalid_space_fields += 1
        if len(handles) > 1:
            self.duplicate_handle_field += 1
        if len(layers) > 1:
            self.duplicate_layer_field += 1
        if not layers or not layers[0]:
            self.missing_layer += 1
        if not handles:
            self.missing_handle += 1
            return
        handle = self._handle(handles[0])
        if handle is None:
            self.invalid_handle += 1
            return
        if handle in self.entities:
            self.duplicate_handle += 1
            return
        self.entities[handle] = (space, kind,
                                 layers[0] if layers else None)
        if self.geometry_enabled and kind in _CORE_GEOMETRY:
            try:
                self.core_geometry[handle] = self._primitive_geometry(kind, fields)
            except ValueError:
                self.core_geometry[handle] = None
                self.invalid_core_geometry += 1

    def summary(self):
        issues = (self.missing_handle + self.invalid_handle +
                  self.duplicate_handle + self.missing_layer +
                  self.duplicate_handle_field + self.duplicate_layer_field +
                  self.invalid_space_fields)
        return {
            "decoded_identity_entity_records": self.total,
            "decoded_identity_unique_handles": len(self.entities),
            "decoded_identity_missing_handles": self.missing_handle,
            "decoded_identity_invalid_handles": self.invalid_handle,
            "decoded_identity_duplicate_handles": self.duplicate_handle,
            "decoded_identity_missing_layers": self.missing_layer,
            "decoded_identity_duplicate_handle_fields": self.duplicate_handle_field,
            "decoded_identity_duplicate_layer_fields": self.duplicate_layer_field,
            "decoded_identity_invalid_space_fields": self.invalid_space_fields,
            "decoded_identity_status": (
                "decoded_records_have_identity_issues" if issues else
                "decoded_identity_well_formed_source_unverified"),
            "decoded_identity_scope": (
                "Converted DXF ENTITIES and BLOCKS only. Source DWG handles, "
                "classes, XREF graph and actual screen pixels are NOT verified."
            ),
        }

    def compare_independent(self, reference_records):
        """Compare with a separately acquired DWG handle/type/layer manifest.

        Caller must verify the reference's provenance and source DWG SHA-256.
        A manifest derived from our converted DXF is NOT independent evidence.
        """
        if not isinstance(reference_records, list) or len(reference_records) > _MAX_OBJECTS:
            raise ValueError("invalid independent CAD manifest")
        reference = {}
        for entry in reference_records:
            if not isinstance(entry, dict) or set(entry) != {
                "handle", "space", "type", "layer"}:
                raise ValueError("invalid independent CAD entry")
            handle = self._handle(entry["handle"])
            if (handle is None or handle in reference or
                entry["space"] not in ("model", "paperspace", "block") or
                not isinstance(entry["type"], str) or not entry["type"] or
                not isinstance(entry["layer"], str) or not entry["layer"]):
                raise ValueError("invalid independent CAD handle/type/layer")
            reference[handle] = (entry["space"], entry["type"], entry["layer"])
        common = self.entities.keys() & reference.keys()
        missing = len(reference.keys() - self.entities.keys())
        extra = len(self.entities.keys() - reference.keys())
        space_drift = sum(self.entities[h][0] != reference[h][0] for h in common)
        type_drift = sum(self.entities[h][1] != reference[h][1] for h in common)
        layer_drift = sum(self.entities[h][2] != reference[h][2] for h in common)
        decoded_issues = self.summary()["decoded_identity_status"] != (
            "decoded_identity_well_formed_source_unverified")
        parity = (not decoded_issues and not missing and not extra and
                  not space_drift and not type_drift and not layer_drift and
                  self.total == len(reference))
        return {
            "independent_expected_records": len(reference),
            "independent_missing_in_converted": missing,
            "independent_unexpected_in_converted": extra,
            "independent_space_mismatches": space_drift,
            "independent_type_mismatches": type_drift,
            "independent_layer_mismatches": layer_drift,
            "independent_identity_result": (
                "identity_metadata_parity_only" if parity else
                "identity_metadata_mismatch"),
            "independent_identity_scope": (
                "Only source handle, entity type, layer, model/paperspace/block. No "
                "coordinates, colors, text, viewport pixels, or ACIS fidelity."
            ),
        }

    def compare_independent_geometry(self, reference_records):
        """Opt-in exact primitive parameter comparison with independent reader.

        This verifies only explicitly supported numeric DXF groups. It never
        certifies unrelated classes, whole-drawing quality or DWG Save As.
        """
        if not self.geometry_enabled or not isinstance(reference_records, list):
            raise ValueError("independent geometry audit is not enabled")
        if not reference_records or len(reference_records) > _MAX_OBJECTS:
            raise ValueError("empty or oversized independent geometry records")
        reference = {}
        for entry in reference_records:
            if not isinstance(entry, dict) or set(entry) != {"handle", "type", "groups"}:
                raise ValueError("invalid independent geometry entry")
            handle = self._handle(entry["handle"])
            kind = entry["type"]
            if (handle is None or handle in reference or
                    not isinstance(kind, str) or kind not in _CORE_GEOMETRY):
                raise ValueError("invalid independent geometry identity")
            reference[handle] = (kind, self._primitive_geometry(
                kind, entry["groups"], reference=True))
        expected = set(self.core_geometry)
        supplied = set(reference)
        missing = len(expected - supplied)
        extra = len(supplied - expected)
        drift = sum(self.core_geometry[handle] is None or
                    self.entities[handle][1] != reference[handle][0] or
                    self.core_geometry[handle] != reference[handle][1]
                    for handle in expected & supplied)
        # Metadata-level mismatch still fails independently in the caller.
        # A subset proof must never be labeled full geometric equivalence.
        passed = (not missing and not extra and not drift and
                  not self.invalid_core_geometry)
        return {
            "independent_geometry_result": (
                "primitive_subset_parity_only" if passed else
                "primitive_subset_mismatch"),
            "independent_geometry_checked": len(expected & supplied),
            "independent_geometry_expected_primitives": len(expected),
            "independent_geometry_missing_reference": missing,
            "independent_geometry_unexpected_reference": extra,
            "independent_geometry_parameter_mismatches": drift,
            "independent_geometry_invalid_decoded": self.invalid_core_geometry,
            "independent_geometry_scope": (
                "LINE endpoints, POINT position/normal, CIRCLE/ARC center/radius/"
                "angles/normal only. No tolerance, full DWG geometry, color, "
                "XREF, block transforms, 3DSOLID, or rendering assurance."
            ),
        }
