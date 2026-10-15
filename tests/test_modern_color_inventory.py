"""Synthetic DXF group-code audit: modern color and 2D/3D object coverage."""
import tempfile
import unittest
from pathlib import Path
from private_dwg_read_audit import inventory_dxf


def dxf(contents: str) -> dict:
    # Only generated geometry here; never use any private municipal project.
    with tempfile.TemporaryDirectory(prefix="musa-modern-color-") as root:
        file = Path(root) / "synthetic.dxf"
        file.write_text(contents, encoding="ascii")
        return inventory_dxf(file)


def record(name, *pairs):
    pairs = list(pairs)
    return "0\n" + name + "\n" + "".join(
        str(code) + "\n" + str(value) + "\n" for code, value in pairs
    )


class ModernColorInventoryTest(unittest.TestCase):
    def test_model_and_block_color_sources(self):
        contents = (
            record("SECTION", (2, "BLOCKS")) +
            record("BLOCK", (2, "A"), (70, 0)) +
            record("INSERT", (8, "0"), (62, 0)) +
            record("3DSOLID", (8, "0"), (420, 0x12A0EF), (62, 256)) +
            record("ENDBLK") + record("ENDSEC") +
            record("SECTION", (2, "ENTITIES")) +
            record("LINE", (8, "0"), (62, 3)) +
            record("CIRCLE", (8, "0"), (62, 256)) +
            record("HATCH", (8, "0")) +
            record("MTEXT", (8, "0"), (420, 255)) +
            record("ACAD_PROXY_ENTITY", (8, "0"), (62, -7)) +
            record("ENDSEC") + record("EOF")
        )
        inventory = dxf(contents)
        self.assertEqual(
            inventory["converted_dxf_color_sources"],
            {"byblock": 1, "bylayer": 2, "explicit_aci": 2, "truecolor": 2}
        )
        self.assertEqual(inventory["block_geometry_entity_count"], 2)
        self.assertEqual(inventory["model_entity_count"], 5)
        self.assertEqual(inventory["converted_dxf_advanced_entity_counts"], {
            "3DSOLID": 1, "ACAD_PROXY_ENTITY": 1,
            "HATCH": 1, "INSERT": 1, "MTEXT": 1
        })

    def test_invalid_color_is_not_silently_classified_as_correct(self):
        contents = (
            record("SECTION", (2, "ENTITIES")) +
            record("LINE", (8, "0"), (420, 99999999), (62, 2)) +
            record("CIRCLE", (8, "0"), (62, "invalid")) +
            record("ENDSEC") + record("EOF")
        )
        inventory = dxf(contents)
        self.assertEqual(inventory["converted_dxf_color_sources"],
                         {"invalid_aci": 1, "invalid_truecolor": 1})
        self.assertEqual(inventory["model_entity_count"], 2)

    def test_missing_eof_fails_closed(self):
        with self.assertRaises(ValueError):
            dxf(record("SECTION", (2, "ENTITIES")) +
                record("POINT", (8, "0")) + record("ENDSEC"))


if __name__ == "__main__":
    unittest.main()
