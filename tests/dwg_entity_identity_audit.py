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
# Covered XYZ and 3DFACE vertices are compared, but this is not full CAD fidelity:
# excluded are thickness, XREF, ACIS, block transforms, true rendering, and
# viewport appearance beyond the fields listed below.
_CORE_GEOMETRY = {
    "LINE": ((10, 20, 11, 21), {30: "0", 31: "0"}),
    "POINT": ((10, 20), {30: "0", 210: "0", 220: "0", 230: "1"}),
    "CIRCLE": ((10, 20, 40), {30: "0", 210: "0", 220: "0", 230: "1"}),
    "ARC": ((10, 20, 40, 50, 51),
            {30: "0", 210: "0", 220: "0", 230: "1"}),
    # WCS center XYZ and relative major-axis vector XYZ, minor/major
    # ratio, and start/end parameters in RADIANS (not ARC degrees).
    # Requiring explicit arc parameters prevents a missing/incorrect
    # full-ellipse arc definition from being marked independently equal.
    "ELLIPSE": ((10, 20, 11, 21, 40, 41, 42),
                {30: "0", 31: "0", 210: "0", 220: "0", 230: "1"}),
    # 3DFACE: first three WCS vertices required; Z defaults to zero.
    # Fourth corner is optional and then exactly repeats the third corner.
    # Group 70 contains independent edge-visibility bits 1/2/4/8.
    "3DFACE": ((10, 20, 11, 21, 12, 22),
               {30: "0", 31: "0", 32: "0", 70: "0"}),
    # Ordered repeated numeric group codes require a distinct source record
    # form: a reference list of [group-code, value] pairs, not a dict.
    "SPLINE": ((), {}),
    "LWPOLYLINE": ((), {}),
}
_LWPOLYLINE_GROUPS = frozenset((
    90, 70, 38, 39, 43, 210, 220, 230,
    10, 20, 40, 41, 42, 91,
))
_LWPOLYLINE_HEADER_GROUPS = frozenset((90, 70, 38, 39, 43, 210, 220, 230))
_LWPOLYLINE_VERTEX_GROUPS = frozenset((10, 20, 40, 41, 42, 91))
_LWPOLYLINE_MAX_PAIRS = 25_000
_SPLINE_GROUPS = frozenset((
    70, 71, 72, 73, 74, 40, 41,
    10, 20, 30, 11, 21, 31,
    12, 22, 32, 13, 23, 33, 42, 43, 44, 210, 220, 230,
))
_SPLINE_SCALARS = frozenset((70, 71, 72, 73, 74, 42, 43, 44, 210, 220, 230))
_SPLINE_MAX_PAIRS = 25_000

_HEX_HANDLE = re.compile(r"[0-9A-Fa-f]{1,32}\Z")
_SPACES = {"ENTITIES": "model", "BLOCKS": "block"}
_SKIP = {"SECTION", "ENDSEC", "EOF", "BLOCK", "ENDBLK", "TABLE",
         "ENDTAB"}
_MAX_OBJECTS = 2_000_000
# DXF raw per-entity style codes only; layer-level effective color/line type,
# print styles and viewport overrides still require independent CAD rendering.
_STYLE_CODES = frozenset((6, 48, 60, 62, 370, 420, 430, 440))
_STYLE_DEFAULTS = {6: "BYLAYER", 48: "1", 60: "0", 62: "256", 370: "-1"}


class DwgEntityIdentityAudit:
    def __init__(self, geometry_enabled=False, style_enabled=False):
        self.style_enabled = bool(style_enabled)
        self.core_style = {} if style_enabled else None
        self.invalid_core_style = 0
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
    def _spline_geometry(groups, reference=False):
        """Source-bound, exact ordered DXF parameter subset for SPLINE.

        The external independent reader must export an ordered list of
        numeric group-code/value pairs (no private handles/layers/text), NOT
        a mapping: knots, weights and control/fit XYZ recur. This compares
        definitions, not mathematically evaluated curves or source DWG pixels.
        """
        if reference:
            if not isinstance(groups, list):
                raise ValueError("ordered SPLINE reference pairs required")
            pairs = []
            for item in groups:
                if (not isinstance(item, list) or len(item) != 2 or
                        not isinstance(item[0], str) or not item[0].isdecimal() or
                        str(int(item[0])) != item[0] or
                        int(item[0]) not in _SPLINE_GROUPS):
                    raise ValueError("invalid source SPLINE numeric group")
                pairs.append((int(item[0]), item[1]))
        else:
            pairs = [(code, raw) for code, raw in groups if code in _SPLINE_GROUPS]
        if not pairs or len(pairs) > _SPLINE_MAX_PAIRS:
            raise ValueError("empty or oversized SPLINE fields")
        normalized = []
        scalars = {}
        knots, weights, controls, fits = [], [], [], []
        current_control, current_fit = None, None
        for code, raw in pairs:
            if not isinstance(raw, str) or not 1 <= len(raw) <= 128:
                raise ValueError("invalid SPLINE numeric value")
            value = _normalized(code, raw)
            if code in _SPLINE_SCALARS:
                if code in scalars:
                    raise ValueError("duplicate SPLINE scalar")
                scalars[code] = value
            elif code == 40:
                knots.append(Decimal(value))
                if len(knots) > 4104:
                    raise ValueError("too many SPLINE knots")
            elif code == 41:
                weights.append(Decimal(value))
                if len(weights) > 4096:
                    raise ValueError("too many SPLINE weights")
            elif code in (10, 20, 30):
                if code == 10:
                    if current_control is not None and 20 not in current_control:
                        raise ValueError("incomplete SPLINE control vertex")
                    current_control = {10:value}
                    controls.append(current_control)
                elif current_control is None or code in current_control or (
                        code == 30 and 20 not in current_control):
                    raise ValueError("invalid SPLINE control field order")
                else:
                    current_control[code] = value
                if len(controls) > 4096:
                    raise ValueError("too many SPLINE control vertices")
            elif code in (11, 21, 31):
                if code == 11:
                    if current_fit is not None and 21 not in current_fit:
                        raise ValueError("incomplete SPLINE fit vertex")
                    current_fit = {11:value}
                    fits.append(current_fit)
                elif current_fit is None or code in current_fit or (
                        code == 31 and 21 not in current_fit):
                    raise ValueError("invalid SPLINE fit field order")
                else:
                    current_fit[code] = value
                if len(fits) > 4096:
                    raise ValueError("too many SPLINE fit vertices")
            normalized.append((code, value))
        if (71 not in scalars or 72 not in scalars or 73 not in scalars or
                current_control is not None and 20 not in current_control or
                current_fit is not None and 21 not in current_fit):
            raise ValueError("SPLINE missing degree/count/XYZ data")
        degree = int(scalars[71])
        if degree < 1 or degree > 32:
            raise ValueError("SPLINE degree outside audit bounds")
        if (int(scalars[72]) != len(knots) or
                int(scalars[73]) != len(controls) or
                int(scalars.get(74, str(len(fits)))) != len(fits) or
                weights and len(weights) != len(controls) or
                any(weight <= 0 for weight in weights) or
                any(knots[i] > knots[i+1] for i in range(len(knots)-1))):
            raise ValueError("SPLINE declared count, knot or weight invalid")
        if controls:
            if len(controls) < degree+1 or len(knots) != len(controls)+degree+1:
                raise ValueError("SPLINE knot/control relation invalid")
        elif knots or weights or len(fits) < 2:
            raise ValueError("SPLINE missing fit-only or control definition")
        if int(scalars.get(70, "0")) < 0:
            raise ValueError("invalid SPLINE flag")
        return tuple(normalized)

    @staticmethod
    def _lwpolyline_geometry(groups, reference=False):
        """Audit the *ordered* LWPOLYLINE numeric geometry parameter stream.

        10/20 XY vertices and their optional 40/41 widths, 42 signed bulges,
        and 91 vertex IDs must remain attached to the same vertex. Reference
        pairs come from an independent reader of the original DWG, not from
        the converted DXF. Preserve precision, signs, repeated group order.
        """
        if reference:
            if not isinstance(groups, list):
                raise ValueError("ordered LWPOLYLINE reference pairs required")
            pairs = []
            for item in groups:
                if (not isinstance(item, list) or len(item) != 2 or
                    not isinstance(item[0], str) or not item[0].isdecimal() or
                    str(int(item[0])) != item[0] or
                    int(item[0]) not in _LWPOLYLINE_GROUPS):
                    raise ValueError("invalid independent LWPOLYLINE group")
                pairs.append((int(item[0]), item[1]))
        else:
            pairs = [(code, raw) for code, raw in groups
                     if code in _LWPOLYLINE_GROUPS]
        if not pairs or len(pairs) > _LWPOLYLINE_MAX_PAIRS:
            raise ValueError("empty or oversized LWPOLYLINE fields")
        normalized = []
        scalars = {}
        vertices = []
        current = None
        for code, raw in pairs:
            if not isinstance(raw, str) or not 1 <= len(raw) <= 128:
                raise ValueError("invalid LWPOLYLINE numeric field")
            value = _normalized(code, raw)
            if code in _LWPOLYLINE_HEADER_GROUPS:
                if code in scalars:
                    raise ValueError("duplicate LWPOLYLINE header field")
                scalars[code] = value
            elif code == 10:
                if current is not None and 20 not in current:
                    raise ValueError("LWPOLYLINE missing Y coordinate")
                if len(vertices) >= 4096:
                    raise ValueError("LWPOLYLINE vertex count exceeds audit cap")
                current = {10: value}
                vertices.append(current)
            elif code in _LWPOLYLINE_VERTEX_GROUPS:
                if current is None or code in current or (
                        code != 20 and 20 not in current):
                    raise ValueError("invalid LWPOLYLINE vertex field order")
                current[code] = value
                if code in (40, 41) and Decimal(value) < 0:
                    raise ValueError("LWPOLYLINE negative vertex width")
                if code == 91 and int(value) < 0:
                    raise ValueError("LWPOLYLINE invalid vertex ID")
            normalized.append((code, value))
        if (90 not in scalars or current is None or 20 not in current or
                int(scalars[90]) != len(vertices) or
                not 1 <= len(vertices) <= 4096):
            raise ValueError("LWPOLYLINE declared vertex count mismatch")
        if 70 in scalars and int(scalars[70]) & ~129:
            raise ValueError("unsupported LWPOLYLINE flags")
        if (43 in scalars and Decimal(scalars[43]) < 0):
            raise ValueError("LWPOLYLINE negative constant width")
        normal = tuple(Decimal(scalars.get(code, default))
                       for code, default in ((210, "0"), (220, "0"), (230, "1")))
        if normal == (0, 0, 0):
            raise ValueError("LWPOLYLINE invalid OCS normal")
        return tuple(normalized)

    @staticmethod
    def _primitive_geometry(kind, groups, reference=False):
        """Canonical exact numeric subset; never return values in diagnostics.

        Reference records use a mapping of decimal DXF group-code strings to
        numeric strings. Source DXF records are ordered (group-code, value)
        pairs. Missing optional Z and OCS normals use DWG/DXF documented
        defaults. Fail on missing/duplicate/malformed covered numeric fields.
        """
        if kind == "SPLINE":
            return DwgEntityIdentityAudit._spline_geometry(groups, reference)
        if kind == "LWPOLYLINE":
            return DwgEntityIdentityAudit._lwpolyline_geometry(groups, reference)
        spec = _CORE_GEOMETRY.get(kind)
        if spec is None:
            raise ValueError("unsupported independent geometry type")
        mandatory, defaults = spec
        allowed = set(mandatory) | set(defaults)
        if kind == "3DFACE":
            allowed.update((13, 23, 33))
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
        if kind == "3DFACE":
            # No fourth corner denotes a triangle: 4th vertex = 3rd.
            # A standalone Z or a partially specified XY corner cannot
            # safely define the fourth vertex and must fail closed.
            if (13 in fields) != (23 in fields) or (33 in fields and 13 not in fields):
                raise ValueError("partial 3DFACE fourth corner")
            if 13 not in fields:
                fields[13], fields[23] = fields[12], fields[22]
                fields[33] = fields.get(32, "0")
            else:
                fields.setdefault(33, "0")
        canonical = []
        for code in sorted(allowed):
            raw = fields.get(code, defaults.get(code))
            value = _normalized(code, raw)
            if code == 40 and Decimal(value) <= 0:
                raise ValueError("nonpositive primitive radius/ellipse axis ratio")
            if kind == "ELLIPSE" and code == 40 and Decimal(value) > 1:
                raise ValueError("ELLIPSE minor/major ratio exceeds one")
            if kind == "3DFACE" and code == 70 and not 0 <= int(value) <= 15:
                raise ValueError("invalid 3DFACE invisible edge bits")
            canonical.append((code, value))
        if kind == "ELLIPSE":
            values = dict(canonical)
            axis = tuple(Decimal(values[c]) for c in (11, 21, 31))
            normal = tuple(Decimal(values[c]) for c in (210, 220, 230))
            axis_size = max(abs(x) for x in axis)
            normal_size = max(abs(x) for x in normal)
            if not axis_size or not normal_size:
                raise ValueError("ELLIPSE major axis or plane normal is zero")
            # The CAD major axis is in the plane normal's perpendicular
            # plane. Reject a clearly non-coplanar axis, without rounding
            # high-precision coordinates to binary floats in the audit.
            orthogonality = sum(x*y for x,y in zip(axis, normal))
            if abs(orthogonality) > (axis_size*normal_size*Decimal("0.00000001")):
                raise ValueError("ELLIPSE major axis outside extrusion plane")
        return tuple(canonical)

    @staticmethod
    def _entity_style(fields, reference=False):
        """Compare explicit DXF entity appearance fields, never rendered pixels.

        Missing color/linetype/lineweight values use DXF ByLayer defaults;
        presence/absence of TrueColor and transparency remains distinguishable.
        No hidden style strings or values leave the count-only report.
        """
        if reference:
            if not isinstance(fields, dict) or len(fields) > len(_STYLE_CODES):
                raise ValueError("invalid independent entity style")
            pairs = []
            for key, value in fields.items():
                if not isinstance(key, str) or not key.isdecimal():
                    raise ValueError("invalid style group key")
                code = int(key)
                if str(code) != key or code not in _STYLE_CODES:
                    raise ValueError("unsupported independent style group")
                pairs.append((code, value))
        else:
            pairs = [(code, value) for code, value in fields
                     if code in _STYLE_CODES]
        values = {}
        for code, raw in pairs:
            if (code in values or not isinstance(raw, str) or
                    not raw or len(raw) > 256):
                raise ValueError("invalid or duplicate entity style field")
            if code in (6, 430):
                if any(ord(character) < 32 for character in raw):
                    raise ValueError("invalid style name")
                val = raw.upper() if code == 6 and raw.upper() in (
                    "BYLAYER", "BYBLOCK", "CONTINUOUS") else raw
            else:
                val = _normalized(code, raw)
                if code == 62 and not -255 <= int(val) <= 256:
                    raise ValueError("invalid ACI color")
                if code == 420 and not 0 <= int(val) <= 0xFFFFFF:
                    raise ValueError("invalid TrueColor")
                if code == 440 and not 0 <= int(val) <= 0xFFFFFFFF:
                    raise ValueError("invalid transparency")
                if code == 60 and int(val) not in (0, 1):
                    raise ValueError("invalid visibility")
                if code == 370 and not -3 <= int(val) <= 211:
                    raise ValueError("invalid lineweight")
                if code == 48 and Decimal(val) <= 0:
                    raise ValueError("invalid linetype scale")
            values[code] = val
        return tuple((code, values.get(code, _STYLE_DEFAULTS.get(code)))
                     for code in sorted(_STYLE_CODES))

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
        if self.style_enabled:
            try:
                self.core_style[handle] = self._entity_style(fields)
            except ValueError:
                self.core_style[handle] = None
                self.invalid_core_style += 1
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
                "LINE XYZ endpoints, POINT position/normal, CIRCLE/ARC center/"
                "radius/angles/normal, ELLIPSE WCS center/axis/ratio/radian "
                "parameters/normal, 3DFACE XYZ corners/hidden edges, and "
                "ordered SPLINE knot/control/fit/weight and LWPOLYLINE vertex/"
                "signed-bulge/width/OCS parameter groups only. "
                "Exact parameter strings after numeric normalization; no "
                "evaluated curve or full DWG geometry, color, "
                "XREF, block transforms, 3DSOLID, or rendering assurance."
            ),
        }

    def compare_independent_style(self, reference_records):
        """Opt-in exact raw entity style parity; not resolved layer/render color."""
        if not self.style_enabled or not isinstance(reference_records, list):
            raise ValueError("independent style audit not enabled")
        if not reference_records or len(reference_records) > _MAX_OBJECTS:
            raise ValueError("missing or oversized independent style manifest")
        references = {}
        for entry in reference_records:
            if not isinstance(entry, dict) or set(entry) != {"handle", "groups"}:
                raise ValueError("invalid independent style entry")
            handle = self._handle(entry["handle"])
            if handle is None or handle in references:
                raise ValueError("invalid or duplicate source style handle")
            references[handle] = self._entity_style(entry["groups"], reference=True)
        expected = set(self.core_style)
        actual = set(references)
        mismatch = sum(
            self.core_style[handle] is None or
            self.core_style[handle] != references[handle]
            for handle in expected & actual
        )
        passed = (not (expected - actual) and not (actual - expected)
                  and not mismatch and not self.invalid_core_style)
        return {
            "independent_style_result": (
                "entity_style_subset_parity_only" if passed else
                "entity_style_subset_mismatch"),
            "independent_style_checked": len(expected & actual),
            "independent_style_missing_reference": len(expected - actual),
            "independent_style_unexpected_reference": len(actual - expected),
            "independent_style_field_mismatches": mismatch,
            "independent_style_invalid_decoded": self.invalid_core_style,
            "independent_style_scope": (
                "Raw entity group 6/48/60/62/370/420/430/440 only. "
                "Does NOT verify layer tables, effective ByLayer/ByBlock "
                "inheritance, viewport overrides, plot styles or screen colors."
            ),
        }
