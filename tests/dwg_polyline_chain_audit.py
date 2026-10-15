"""Optional original-DWG-bound POLYLINE -> VERTEX* -> SEQEND chain audit.

This verifies ordered *record ownership*, not polyface surface connectivity or
DWG binary writer fidelity. Reports contain COUNTS ONLY, never source handles.
"""
from dwg_entity_identity_audit import DwgEntityIdentityAudit

_MAX_CHAINS = 200_000
_MAX_VERTICES = 1_000_000


class DwgPolylineChainAudit:
    def __init__(self):
        self.active = None
        self.chains = {}
        self.malformed = 0
        self.total_vertices = 0

    @staticmethod
    def _one(fields, code):
        return [value for key, value in fields if key == code]

    @staticmethod
    def _handle(raw):
        return DwgEntityIdentityAudit._handle(raw)

    def _handle_field(self, fields):
        raw = self._one(fields, 5)
        return self._handle(raw[0]) if len(raw) == 1 else None

    @staticmethod
    def _space(section, fields):
        space, good = DwgEntityIdentityAudit._space(section, fields)
        return space if good else None

    def observe(self, section, kind, fields):
        if section not in ("ENTITIES", "BLOCKS"):
            return
        if kind == "POLYLINE":
            if self.active is not None:
                self.malformed += 1  # previous chain was not terminated
                self.active = None
            if len(self.chains) >= _MAX_CHAINS:
                raise ValueError("POLYLINE chain audit capacity exceeded")
            handle = self._handle_field(fields)
            space = self._space(section, fields)
            if not handle or not space or handle in self.chains:
                self.malformed += 1
                return
            self.chains[handle] = None
            self.active = {
                "handle": handle, "section": section, "space": space,
                "vertices": [], "seqend": None,
            }
        elif kind == "VERTEX":
            if self.active is None or self.active["section"] != section:
                self.malformed += 1
                return
            vertex = self._handle_field(fields)
            owners = self._one(fields, 330)
            if (not vertex or vertex == self.active["handle"] or
                    vertex in self.active["vertices"] or
                    (owners and (len(owners) != 1 or
                                 self._handle(owners[0]) != self.active["handle"]))):
                self.malformed += 1
                return
            self.total_vertices += 1
            if self.total_vertices > _MAX_VERTICES:
                raise ValueError("POLYLINE vertex chain audit cap exceeded")
            self.active["vertices"].append(vertex)
        elif kind == "SEQEND":
            if self.active is None or self.active["section"] != section:
                self.malformed += 1
                return
            end_handle = self._handle_field(fields)
            owners = self._one(fields, 330)
            active = self.active
            self.active = None
            if (not end_handle or not active["vertices"] or
                    end_handle == active["handle"] or
                    end_handle in active["vertices"] or
                    (owners and (len(owners) != 1 or
                                 self._handle(owners[0]) != active["handle"]))):
                self.malformed += 1
                return
            active["seqend"] = end_handle
            self.chains[active["handle"]] = (
                active["space"], tuple(active["vertices"]), end_handle
            )
        elif self.active is not None:
            # A POLYLINE sequence cannot contain a LINE, new BLOCK or EOF.
            self.malformed += 1
            self.active = None

    def compare_independent(self, source_records):
        if not isinstance(source_records, list) or not source_records or len(source_records) > _MAX_CHAINS:
            raise ValueError("missing or oversized original DWG POLYLINE chain records")
        reference = {}
        references_vertex_count = 0
        used = set()
        for rec in source_records:
            if not isinstance(rec, dict) or set(rec) != {"handle", "space", "vertices", "seqend"}:
                raise ValueError("invalid original DWG POLYLINE chain record")
            handle = self._handle(rec["handle"])
            end = self._handle(rec["seqend"])
            children = rec["vertices"]
            if (not handle or not end or handle in reference or
                    rec["space"] not in ("model", "paperspace", "block") or
                    not isinstance(children, list) or not children or
                    len(children) > _MAX_VERTICES):
                raise ValueError("invalid original DWG chain owner or termination")
            vertices = []
            for child in children:
                vert = self._handle(child)
                if not vert or vert in used or vert == handle or vert == end:
                    raise ValueError("invalid or reused source vertex handle")
                used.add(vert)
                vertices.append(vert)
            if handle in used or end in used or handle in vertices or end in vertices:
                raise ValueError("overlapping source chain handles")
            used.update((handle, end))
            reference[handle] = (rec["space"], tuple(vertices), end)
            references_vertex_count += len(vertices)
            if references_vertex_count > _MAX_VERTICES:
                raise ValueError("source POLYLINE vertices exceed cap")
        missing = len(set(self.chains) - set(reference))
        unexpected = len(set(reference) - set(self.chains))
        mismatch = sum(
            self.chains[handle] != reference[handle]
            for handle in self.chains.keys() & reference.keys()
        )
        malformed = self.malformed + (1 if self.active is not None else 0)
        passed = not (missing or unexpected or mismatch or malformed)
        return {
            "independent_polyline_chain_result": (
                "record_order_subset_parity_only" if passed else
                "polyline_chain_mismatch"),
            "independent_polyline_chain_checked": len(self.chains.keys() & reference.keys()),
            "independent_polyline_chain_missing_source": missing,
            "independent_polyline_chain_unexpected_source": unexpected,
            "independent_polyline_chain_mismatches": mismatch,
            "independent_polyline_chain_malformed_decoded": malformed,
            "independent_polyline_chain_scope": (
                "Source-bound POLYLINE record parent / ordered VERTEX handles / "
                "SEQEND handle, including optional group-330 ownership check. "
                "Not geometrical edge adjacency, polyface winding, native "
                "binary DWG write/reopen, or complete AutoCAD 2026 parity."
            ),
        }
