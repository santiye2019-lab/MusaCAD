"""Optional source-DWG-bound, *ordered* polyface vertex/face audit.

Modelled DXF POLYLINE flag 64 followed by coordinate VERTEX flag 192,
face VERTEX flag 128 and SEQEND. Signed face indices encode which edge is
invisible. This is a strict subset, not ACIS/B-Rep or full manifold proof.
Never echo proprietary handles, coordinates or block names in the report.
"""
from collections import Counter
from decimal import Decimal, InvalidOperation

from dwg_entity_identity_audit import DwgEntityIdentityAudit

_MAX_MESHES = 50000
_MAX_COMPONENTS = 1000000
_MAX_PER_MESH = 65535
_COORD_LIMIT = Decimal("1e12")


def _handle(value):
    return DwgEntityIdentityAudit._handle(value)


def _numeric(value):
    if not isinstance(value, str) or not 1 <= len(value) <= 128:
        raise ValueError("invalid private polyface numeric data")
    try:
        result = Decimal(value)
    except InvalidOperation:
        raise ValueError("invalid private polyface number") from None
    if not result.is_finite() or abs(result) > _COORD_LIMIT:
        raise ValueError("nonfinite or unbounded polyface position")
    return result.normalize() if result else Decimal(0)


def _integer(value):
    if isinstance(value, bool):
        raise ValueError("invalid polyface index")
    if isinstance(value, int):
        number = value
    elif isinstance(value, str) and value and len(value) <= 10:
        if not (value.lstrip("-").isdigit() and
                (not value.startswith("+"))):
            raise ValueError("invalid polyface index")
        number = int(value)
    else:
        raise ValueError("invalid polyface index")
    if not -65535 <= number <= 65535:
        raise ValueError("polyface index out of bounds")
    return number


def _face_indices(values, vertices):
    if not isinstance(values, (tuple, list)) or not 3 <= len(values) <= 4:
        raise ValueError("polyface face requires 3 or 4 signed indices")
    indices = tuple(_integer(val) for val in values)
    # A zero is only permissible as trailing optional fourth field.
    if 0 in indices:
        if len(indices) != 4 or indices[3] != 0:
            raise ValueError("polyface face has a missing internal vertex")
        indices = indices[:3]
    if len(set(abs(n) for n in indices)) != len(indices) or any(
        n == 0 or abs(n) > vertices for n in indices
    ):
        raise ValueError("polyface face invalid, reused or out-of-range vertex")
    return indices


def _edge_counts(faces):
    incidence = Counter()
    directions = Counter()
    for _, face in faces:
        for i, index in enumerate(face):
            u, v = abs(index), abs(face[(i+1) % len(face)])
            edge = (min(u, v), max(u, v))
            incidence[edge] += 1
            directions[(u, v)] += 1
    shared = sum(v == 2 for v in incidence.values())
    boundary = sum(v == 1 for v in incidence.values())
    nonmanifold = sum(v > 2 for v in incidence.values())
    bad_winding = sum(
        amount == 2 and (
            directions[(u, v)] != 1 or directions[(v, u)] != 1)
        for (u, v), amount in incidence.items()
    )
    return shared, boundary, nonmanifold, bad_winding


class DwgPolyfaceTopologyAudit:
    def __init__(self):
        self.active = None
        self.meshes = {}
        self.malformed = 0
        self.components = 0
        self._decoded_handles = set()

    @staticmethod
    def _one(fields, code):
        return [v for k, v in fields if k == code]

    def _unique(self, fields, code):
        values = self._one(fields, code)
        if len(values) > 1:
            raise ValueError("repeated polyface field")
        return values[0] if values else None

    def _start(self, section, fields):
        raw_flags = self._unique(fields, 70)
        flags = _integer(raw_flags if raw_flags is not None else "0")
        if not (flags & 64):
            self.active = None
            return
        if self.active is not None:
            self.malformed += 1
        self.active = None
        if len(self.meshes) >= _MAX_MESHES:
            raise ValueError("polyface mesh audit cap exceeded")
        parent = _handle(self._unique(fields, 5))
        space, valid = DwgEntityIdentityAudit._space(section, fields)
        if not parent or not valid or parent in self._decoded_handles:
            self.malformed += 1
            return
        self._decoded_handles.add(parent)
        self.meshes[parent] = None
        self.active = {
            "parent": parent, "space": space, "section": section,
            "vertices": [], "faces": [], "end": None,
            "expected_vertices": self._unique(fields, 71),
            "expected_faces": self._unique(fields, 72),
        }

    def observe(self, section, kind, fields):
        if section not in ("ENTITIES", "BLOCKS"):
            return
        if kind == "POLYLINE":
            try:
                if self.active is not None:
                    self.malformed += 1
                self._start(section, fields)
            except ValueError:
                self.malformed += 1
                self.active = None
            return
        if self.active is None:
            return
        if section != self.active["section"]:
            self.malformed += 1
            self.active = None
            return
        if kind not in ("VERTEX", "SEQEND"):
            self.malformed += 1
            self.active = None
            return
        try:
            handle = _handle(self._unique(fields, 5))
            owner = self._unique(fields, 330)
            if not handle or handle in self._decoded_handles or (
                owner is not None and _handle(owner) != self.active["parent"]
            ):
                raise ValueError("invalid polyface ownership or reused handle")
            self._decoded_handles.add(handle)
            if kind == "SEQEND":
                self._finish(handle)
                return
            flag = _integer(self._unique(fields, 70) or "0")
            self.components += 1
            if self.components > _MAX_COMPONENTS:
                raise ValueError("polyface component audit cap exceeded")
            if flag & 64 and flag & 128:  # coordinate vertex
                if self.active["faces"] or len(self.active["vertices"]) >= _MAX_PER_MESH:
                    raise ValueError("polyface coordinate appears after face or above cap")
                xx = self._unique(fields, 10)
                yy = self._unique(fields, 20)
                zz = self._unique(fields, 30)
                if xx is None or yy is None:
                    raise ValueError("polyface coordinate vertex lacks X/Y")
                xyz = (_numeric(xx), _numeric(yy),
                       _numeric(zz if zz is not None else "0"))
                self.active["vertices"].append((handle, xyz))
            elif flag & 128 and not flag & 64:  # face vertex
                if len(self.active["faces"]) >= _MAX_PER_MESH:
                    raise ValueError("polyface face count above cap")
                raw = [self._unique(fields, k) for k in (71, 72, 73, 74)]
                if any(x is None for x in raw[:3]):
                    raise ValueError("polyface face requires at least three indices")
                if raw[3] is None:
                    raw.pop()
                self.active["faces"].append((handle, raw))
            else:
                raise ValueError("unsupported polyface VERTEX class")
        except ValueError:
            self.malformed += 1
            self.active = None

    def _finish(self, end):
        rec = self.active
        self.active = None
        if not rec["vertices"] or not rec["faces"]:
            self.malformed += 1
            return
        n, f = len(rec["vertices"]), len(rec["faces"])
        if (rec["expected_vertices"] is not None and
                _integer(rec["expected_vertices"]) != n or
            rec["expected_faces"] is not None and
                _integer(rec["expected_faces"]) != f):
            self.malformed += 1
            return
        try:
            faces = tuple((handle, _face_indices(raw, n))
                          for handle, raw in rec["faces"])
        except ValueError:
            self.malformed += 1
            return
        self.meshes[rec["parent"]] = (
            rec["space"], tuple(rec["vertices"]), faces, end
        )

    @staticmethod
    def _reference(rec):
        if not isinstance(rec, dict) or set(rec) != {
            "handle", "space", "vertices", "faces", "seqend"
        }:
            raise ValueError("invalid independent polyface mesh record")
        parent, end = _handle(rec["handle"]), _handle(rec["seqend"])
        vertices, faces = rec["vertices"], rec["faces"]
        if (not parent or not end or parent == end or
            rec["space"] not in ("model", "paperspace", "block") or
            not isinstance(vertices, list) or not isinstance(faces, list) or
            not 1 <= len(vertices) <= _MAX_PER_MESH or
            not 1 <= len(faces) <= _MAX_PER_MESH):
            raise ValueError("invalid independent polyface header")
        used = {parent, end}
        checked_vertices = []
        for vertex in vertices:
            if not isinstance(vertex, dict) or set(vertex) != {"handle", "xyz"}:
                raise ValueError("invalid independent polyface coordinate record")
            handle = _handle(vertex["handle"])
            xyz = vertex["xyz"]
            if (not handle or handle in used or not isinstance(xyz, list) or
                    len(xyz) != 3):
                raise ValueError("invalid independent polyface point")
            used.add(handle)
            checked_vertices.append((handle, tuple(_numeric(x) for x in xyz)))
        checked_faces = []
        for face in faces:
            if not isinstance(face, dict) or set(face) != {"handle", "indices"}:
                raise ValueError("invalid independent polyface face record")
            handle = _handle(face["handle"])
            if not handle or handle in used:
                raise ValueError("polyface face handle reused")
            used.add(handle)
            checked_faces.append((handle, _face_indices(
                face["indices"], len(vertices))))
        return parent, (
            rec["space"], tuple(checked_vertices), tuple(checked_faces), end
        ), used

    def compare_independent(self, source_records):
        if (not isinstance(source_records, list) or not source_records or
                len(source_records) > _MAX_MESHES):
            raise ValueError("missing/oversized independent polyface records")
        independent = {}
        handles = set()
        total = 0
        for record in source_records:
            owner, data, used = self._reference(record)
            if owner in independent or not handles.isdisjoint(used):
                raise ValueError("overlapping independent polyface handles")
            handles.update(used)
            independent[owner] = data
            total += len(data[1]) + len(data[2])
            if total > _MAX_COMPONENTS:
                raise ValueError("independent polyface component cap exceeded")
        missing = len(self.meshes.keys() - independent.keys())
        extra = len(independent.keys() - self.meshes.keys())
        drift = sum(self.meshes[owner] != independent[owner]
                    for owner in self.meshes.keys() & independent.keys())
        malformed = self.malformed + (1 if self.active is not None else 0)
        # Count topology diagnostics; never dump vertex handles or xyz.
        counts = Counter()
        for decoded in self.meshes.values():
            if decoded is not None:
                shared, boundary, nonmanifold, winding = _edge_counts(decoded[2])
                counts.update({
                    "shared": shared, "boundary": boundary,
                    "nonmanifold": nonmanifold, "winding": winding,
                })
        passed = not (missing or extra or drift or malformed)
        return {
            "independent_polyface_result": (
                "ordered_face_subset_parity_only" if passed else
                "polyface_topology_mismatch"),
            "independent_polyface_checked": len(self.meshes.keys() & independent.keys()),
            "independent_polyface_missing_source": missing,
            "independent_polyface_extra_source": extra,
            "independent_polyface_drift": drift,
            "independent_polyface_invalid_decoded": malformed,
            "independent_polyface_shared_edges": counts["shared"],
            "independent_polyface_boundary_edges": counts["boundary"],
            "independent_polyface_nonmanifold_edges": counts["nonmanifold"],
            "independent_polyface_inconsistent_winding_edges": counts["winding"],
            "independent_polyface_scope": (
                "Source-bound ordered polyface mesh XYZ, signed face index "
                "lists, face winding and invisible-edge sign, parent/SEQEND "
                "identity. Only decoded DXF vs supplied independent DWG "
                "manifest; not a B-Rep validation or modern DWG writer proof."
            ),
        }
