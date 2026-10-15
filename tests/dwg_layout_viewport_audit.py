"""Conservative independent DWG LAYOUT / paperspace VIEWPORT metadata audit.

Only runs when a separately collected, source-hash-bound reference is supplied.
Does not assert that the actual paper-space render, plot output, or dynamic
viewport clipping is correct. Reports aggregated counts without source names.
"""
from decimal import Decimal, InvalidOperation
import re

_LIMIT = 250_000
_HEX = re.compile(r"[0-9a-fA-F]{1,32}\\Z")
_INT = re.compile(r"[+-]?[0-9]+\\Z")
_LAYOUT_NUMERIC = frozenset((10, 20, 11, 21, 40, 41))
_LAYOUT_INTEGER = frozenset((70, 71))
_LAYOUT_HANDLES = frozenset((330, 340))
_VIEW_NUMERIC = frozenset((10, 20, 30, 40, 41, 12, 22, 45, 51,
                           16, 26, 36, 17, 27, 37))
_VIEW_INTEGER = frozenset((68, 69, 90))
_VIEW_FROZEN = 331


class DwgLayoutViewportAudit:
    def __init__(self):
        self.layouts = {}
        self.viewports = {}
        self.invalid_layouts = 0
        self.invalid_viewports = 0
        self.duplicate_layouts = 0
        self.duplicate_viewports = 0

    @staticmethod
    def _text(value):
        if (not isinstance(value, str) or not value or len(value) > 512 or
                any(ord(c) < 32 or ord(c) == 127 for c in value)):
            raise ValueError("invalid layout identifier")
        return value

    @staticmethod
    def _handle(value):
        if not isinstance(value, str) or not _HEX.fullmatch(value):
            raise ValueError("invalid DWG handle")
        number = int(value, 16)
        if number == 0:
            raise ValueError("zero DWG handle")
        return format(number, "X")

    @staticmethod
    def _once(pairs, code):
        found = [value for key, value in pairs if key == code]
        if len(found) > 1:
            raise ValueError("duplicate required field")
        return found[0] if found else None

    @staticmethod
    def _number(value):
        if not isinstance(value, str) or not 0 < len(value) <= 128:
            raise ValueError("missing numeric data")
        try:
            number = Decimal(value)
        except InvalidOperation as exc:
            raise ValueError("invalid decimal") from exc
        if not number.is_finite():
            raise ValueError("nonfinite layout/viewport geometry")
        return number

    @staticmethod
    def _integer(value):
        if not isinstance(value, str) or len(value) > 128 or not _INT.fullmatch(value):
            raise ValueError("invalid layout flag or viewport number")
        number = int(value)
        if not -(2 ** 31) <= number < 2 ** 32:
            raise ValueError("out-of-range layout flag or viewport number")
        return number

    @classmethod
    def _fields(cls, pairs, decimals, integers, handles, frozen=False):
        fields = {}
        frozen_handles = []
        for code, value in pairs:
            if code == _VIEW_FROZEN and frozen:
                frozen_handles.append(cls._handle(value))
            elif code in decimals or code in integers or code in handles:
                if code in fields:
                    raise ValueError("duplicate layout/viewport property")
                if code in decimals:
                    fields[code] = cls._number(value)
                elif code in integers:
                    fields[code] = cls._integer(value)
                else:
                    fields[code] = cls._handle(value)
        if frozen:
            if len(frozen_handles) != len(set(frozen_handles)):
                raise ValueError("duplicate frozen layer")
            if frozen_handles:
                fields[_VIEW_FROZEN] = tuple(sorted(frozen_handles))
        return tuple(sorted(fields.items()))

    @classmethod
    def _layout(cls, name, fields):
        name = cls._text(name)
        return (name, cls._fields(fields, _LAYOUT_NUMERIC,
                                  _LAYOUT_INTEGER, _LAYOUT_HANDLES))

    @classmethod
    def _viewport(cls, layout, layer, space, fields):
        layout, layer = cls._text(layout), cls._text(layer)
        if space != "paperspace":
            raise ValueError("VIEWPORT entity is not in paperspace")
        decoded = cls._fields(fields, _VIEW_NUMERIC, _VIEW_INTEGER,
                              frozenset(), frozen=True)
        props = dict(decoded)
        if not all(k in props for k in (10, 20, 40, 41)):
            raise ValueError("VIEWPORT missing paper position/size")
        if props[40] <= 0 or props[41] <= 0:
            raise ValueError("VIEWPORT paper dimensions nonpositive")
        return (layout, layer, space, decoded)

    @staticmethod
    def _source_pairs(groups, allowed):
        if not isinstance(groups, dict) or len(groups) > len(allowed) + 50000:
            raise ValueError("invalid source layout/viewport fields")
        result = []
        frozen = groups.get(str(_VIEW_FROZEN))
        for key, value in groups.items():
            if (not isinstance(key, str) or not key.isdecimal() or
                    str(int(key)) != key):
                raise ValueError("invalid DXF group code")
            code = int(key)
            if code not in allowed:
                raise ValueError("unsupported source layout/viewport property")
            if code == _VIEW_FROZEN:
                if not isinstance(value, list) or len(value) > 50000:
                    raise ValueError("invalid frozen layer list")
                result.extend((code, item) for item in value)
            else:
                if not isinstance(value, str):
                    raise ValueError("invalid source field value")
                result.append((code, value))
        return result

    def observe(self, section, kind, fields):
        if section == "OBJECTS" and kind == "LAYOUT":
            try:
                handle = self._handle(self._once(fields, 5))
                name = self._text(self._once(fields, 1))
                if handle in self.layouts or any(n == name for n, _ in self.layouts.values()):
                    self.duplicate_layouts += 1
                    return
                if len(self.layouts) >= _LIMIT:
                    raise ValueError("too many LAYOUT objects")
                self.layouts[handle] = self._layout(name, fields)
            except ValueError:
                self.invalid_layouts += 1
        elif section == "ENTITIES" and kind == "VIEWPORT":
            try:
                handle = self._handle(self._once(fields, 5))
                if handle in self.viewports:
                    self.duplicate_viewports += 1
                    return
                if len(self.viewports) >= _LIMIT:
                    raise ValueError("too many VIEWPORT entities")
                space_flag = self._once(fields, 67)
                if space_flag not in (None, "1"):
                    raise ValueError("invalid paper-space flag")
                self.viewports[handle] = self._viewport(
                    self._once(fields, 410), self._once(fields, 8),
                    "paperspace", fields)
            except ValueError:
                self.invalid_viewports += 1

    def compare_independent(self, source_layouts, source_viewports):
        if (not isinstance(source_layouts, list) or
                not isinstance(source_viewports, list) or
                len(source_layouts) > _LIMIT or
                len(source_viewports) > _LIMIT or
                not (source_layouts or source_viewports)):
            raise ValueError("empty/oversized source layout and viewport inventory")
        layouts = {}
        names = set()
        for item in source_layouts:
            if not isinstance(item, dict) or set(item) != {"handle", "name", "groups"}:
                raise ValueError("invalid independent layout record")
            handle = self._handle(item["handle"])
            name = self._text(item["name"])
            if handle in layouts or name in names:
                raise ValueError("duplicate independent layout record")
            names.add(name)
            layouts[handle] = self._layout(name, self._source_pairs(
                item["groups"], _LAYOUT_NUMERIC | _LAYOUT_INTEGER | _LAYOUT_HANDLES))
        viewports = {}
        for item in source_viewports:
            if not isinstance(item, dict) or set(item) != {
                    "handle", "layout", "layer", "space", "groups"}:
                raise ValueError("invalid independent viewport record")
            handle = self._handle(item["handle"])
            if handle in viewports:
                raise ValueError("duplicate independent viewport")
            viewports[handle] = self._viewport(
                item["layout"], item["layer"], item["space"],
                self._source_pairs(item["groups"],
                                   _VIEW_NUMERIC | _VIEW_INTEGER | {_VIEW_FROZEN}))
        dl, sl = set(self.layouts), set(layouts)
        dv, sv = set(self.viewports), set(viewports)
        changed_layouts = sum(self.layouts[h] != layouts[h] for h in dl & sl)
        changed_viewports = sum(self.viewports[h] != viewports[h] for h in dv & sv)
        decoded_names = {r[0] for r in self.layouts.values()}
        unresolved = sum(r[0] not in decoded_names for r in self.viewports.values())
        passed = not any((
            self.invalid_layouts, self.invalid_viewports,
            self.duplicate_layouts, self.duplicate_viewports,
            dl ^ sl, dv ^ sv, changed_layouts, changed_viewports, unresolved))
        return {
            "independent_layout_viewport_result": (
                "layout_viewport_metadata_parity_only" if passed else
                "layout_viewport_metadata_mismatch"),
            "independent_layouts_checked": len(dl & sl),
            "independent_viewports_checked": len(dv & sv),
            "independent_layouts_missing_source": len(dl - sl),
            "independent_layouts_missing_decoded": len(sl - dl),
            "independent_viewports_missing_source": len(dv - sv),
            "independent_viewports_missing_decoded": len(sv - dv),
            "independent_layout_property_mismatches": changed_layouts,
            "independent_viewport_property_mismatches": changed_viewports,
            "independent_unresolved_viewport_layouts": unresolved,
            "independent_invalid_decoded_layouts": self.invalid_layouts,
            "independent_invalid_decoded_viewports": self.invalid_viewports,
            "independent_duplicate_decoded_layouts": self.duplicate_layouts,
            "independent_duplicate_decoded_viewports": self.duplicate_viewports,
            "independent_layout_viewport_scope": (
                "LAYOUT name/handle/selected plot fields and VIEWPORT "
                "paper position/size/view vectors/status/frozen-layer handles; "
                "not rendered paper-space, plotting or XREF resolution."),
        }
