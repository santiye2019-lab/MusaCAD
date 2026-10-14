"""Conservative read-only model of native DWG preview entity coverage.

Uses ONLY entity types from MusaCAD's decoded ASCII DXF inventory. The native
first-paint C dispatcher may still refuse a handled entity because of invalid
geometry, nested transforms, budgets, missing fonts or absent external assets.
A handled type is NOT proof that all its instances reached the screen.

The source DWG requires an independent CAD reader for losslessness claims.
No per-object coordinates, layer names, handles or drawing text are emitted.
"""
from collections import Counter
import re

# Mirrors top-level dispatch CASES in native_scene.c emit_object, using DXF
# entity names. Each case remains only CONDITIONALLY renderable.
CONDITIONAL_NATIVE_ENTITY_TYPES = frozenset({
    "INSERT", "MINSERT", "DIMENSION", "LINE", "CIRCLE", "ARC",
    "POINT", "RAY", "XLINE", "MLINE", "TEXT", "ATTRIB", "ATTDEF",
    "MTEXT", "ELLIPSE", "LWPOLYLINE", "SPLINE", "SOLID", "TRACE",
    "LEADER", "MULTILEADER", "IMAGE", "OLE2FRAME", "WIPEOUT",
    "HATCH", "3DFACE",
    # DXF POLYLINE requires header flags to distinguish 2D/3D/mixed meshes.
    # Not a promise that polygon meshes/polyfaces are drawable.
    "POLYLINE",
})

# Entity-list record children; not independently visible drawing primitives.
STRUCTURAL_DXF_RECORD_TYPES = frozenset({"VERTEX", "SEQEND"})

# These are known entity classes with no direct native_scene.c drawing case.
KNOWN_NON_NATIVE_TYPES = frozenset({
    "3DSOLID", "BODY", "REGION", "ACAD_PROXY_ENTITY", "ACAD_TABLE",
    "MESH", "HELIX", "LIGHT", "SUN", "CAMERA", "GEOPOSITIONMARKER",
    "UNDERLAY", "PDFUNDERLAY", "DGNUNDERLAY", "DWFUNDERLAY",
    "ACAD_PROXY_OBJECT", "OLEFRAME", "SHAPE", "TOLERANCE",
    "ARCALIGNEDTEXT", "MULTIPOLYGON", "VIEWPORT",
})

SAFE_NAME = re.compile(r"[A-Z][A-Z0-9_]{0,63}\Z")


def _checked_counter(data):
    if not isinstance(data, dict):
        raise ValueError("missing decoded DXF type inventory")
    checked = Counter()
    for name, count in data.items():
        if not isinstance(name, str) or type(count) is not int or count < 0:
            raise ValueError("invalid decoded DXF type/count")
        # No user-supplied or proprietary type text leaks from diagnostics.
        label = name if SAFE_NAME.fullmatch(name) else "UNRECOGNIZED_ENTITY"
        checked[label] += count
    return checked


def audit_types(model_entity_types, block_geometry_types):
    """Classify decoded entity counts without certifying native scene fidelity."""
    model = _checked_counter(model_entity_types)
    block = _checked_counter(block_geometry_types)
    combined = model + block
    conditional = Counter()
    structural = Counter()
    omitted = Counter()
    for name, count in combined.items():
        if name in STRUCTURAL_DXF_RECORD_TYPES:
            structural[name] += count
        elif name in CONDITIONAL_NATIVE_ENTITY_TYPES:
            conditional[name] += count
        elif name in KNOWN_NON_NATIVE_TYPES:
            omitted[name] += count
        else:
            # Group unknown/proprietary classes to avoid leaking names. They
            # still block any claim of a complete native preview.
            omitted["OTHER_UNVERIFIED_ENTITY"] += count
    missing = sum(omitted.values())
    return {
        "native_preview_unsupported_entity_count": missing,
        "native_preview_unsupported_entity_types": dict(sorted(omitted.items())),
        "native_preview_conditional_entity_count": sum(conditional.values()),
        "native_preview_conditional_entity_types": dict(sorted(conditional.items())),
        "native_preview_structural_record_count": sum(structural.values()),
        "native_preview_type_scope": (
            "Decoded DXF ENTITIES/BLOCKS types only; BLOCK instances are not "
            "expanded; layer visibility, source-DWG class omissions, colors, "
            "entity payloads and screen pixels are NOT independently verified"
        ),
        "native_preview_completeness": (
            "unsupported_types_detected" if missing else
            "type_dispatch_only_unverified"
        ),
    }
