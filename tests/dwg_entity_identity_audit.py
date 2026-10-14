"""Local-only DWG/DXF object identity integrity and source-manifest parity.

The decoded DXF is NOT the authoritative DWG. Capture entity handles (group 5),
layers (group 8), type and model/block ownership without returning their values
in diagnostics. An independent DWG reader may supply a source manifest to
compare in memory; summaries report counts only, never handles/layers/text.
"""
from __future__ import annotations

from collections import Counter
import re

_HEX_HANDLE = re.compile(r"[0-9A-Fa-f]{1,32}\Z")
_SPACES = {"ENTITIES": "model", "BLOCKS": "block"}
_SKIP = {"SECTION", "ENDSEC", "EOF", "BLOCK", "ENDBLK", "TABLE",
         "ENDTAB"}
_MAX_OBJECTS = 2_000_000


class DwgEntityIdentityAudit:
    def __init__(self):
        self.entities = {}
        self.total = 0
        self.missing_handle = 0
        self.invalid_handle = 0
        self.duplicate_handle = 0
        self.missing_layer = 0
        self.duplicate_handle_field = 0
        self.duplicate_layer_field = 0

    @staticmethod
    def _one(fields, code):
        return [value for key, value in fields if key == code]

    @staticmethod
    def _handle(value):
        if not isinstance(value, str) or not _HEX_HANDLE.fullmatch(value):
            return None
        numeric = int(value, 16)
        return format(numeric, "X") if numeric else None

    def observe(self, section, kind, fields):
        if section not in _SPACES or kind in _SKIP:
            return
        self.total += 1
        if self.total > _MAX_OBJECTS:
            raise ValueError("object identity audit safety cap exceeded")
        handles = self._one(fields, 5)
        layers = self._one(fields, 8)
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
        self.entities[handle] = (_SPACES[section], kind,
                                 layers[0] if layers else None)

    def summary(self):
        issues = (self.missing_handle + self.invalid_handle +
                  self.duplicate_handle + self.missing_layer +
                  self.duplicate_handle_field + self.duplicate_layer_field)
        return {
            "decoded_identity_entity_records": self.total,
            "decoded_identity_unique_handles": len(self.entities),
            "decoded_identity_missing_handles": self.missing_handle,
            "decoded_identity_invalid_handles": self.invalid_handle,
            "decoded_identity_duplicate_handles": self.duplicate_handle,
            "decoded_identity_missing_layers": self.missing_layer,
            "decoded_identity_duplicate_handle_fields": self.duplicate_handle_field,
            "decoded_identity_duplicate_layer_fields": self.duplicate_layer_field,
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
                entry["space"] not in ("model", "block") or
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
                "Only source handle, entity type, layer, model/block. No "
                "coordinates, colors, text, viewport pixels, or ACIS fidelity."
            ),
        }
