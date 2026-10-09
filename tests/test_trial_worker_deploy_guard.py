"""Safety/regression tests for a Cloudflare license Worker code-only deployment."""
import contextlib
import io
import json
import tempfile
import unittest
from pathlib import Path

from scripts.trial_worker_deploy_guard import GuardError, run, settings_bindings

DEVICE = "MC-12345678-90ABCDEF-12345678"  # Fake test-only ID
D1 = "12345678-1234-1234-1234-123456789012"


def payload():
    return {
        "success": True,
        "result": {"bindings": [
            {"type": "plain_text", "name": "MUSACAD_DEVELOPER_DEVICE_IDS", "text": DEVICE},
            {"type": "plain_text", "name": "MUSACAD_PACKAGE_NAME", "text": "com.musa.cad"},
            {"type": "plain_text", "name": "MUSACAD_PLAY_ALLOW_LEGACY_UNBOUND", "text": "false"},
            {"type": "secret_text", "name": "MUSACAD_TRIAL_PRIVATE_KEY_PEM"},
            {"type": "d1", "name": "DB", "database_id": D1},
        ]}
    }


class DeployGuardTests(unittest.TestCase):
    def test_existing_bindings_are_accepted(self):
        result, db = settings_bindings(payload())
        self.assertEqual(db, D1)
        self.assertEqual(len(result), 5)

    def test_creates_minimal_wranger_config_without_personal_ids(self):
        with tempfile.TemporaryDirectory() as folder:
            root = Path(folder)
            settings = root / "settings.json"
            output = root / "wrangler.toml"
            settings.write_text(json.dumps(payload()))
            console = io.StringIO()
            with contextlib.redirect_stdout(console):
                run(["--settings", str(settings), "--output", str(output)])
            written = output.read_text()
            self.assertIn('name = "musacad-trial-api"', written)
            self.assertIn(D1, written)
            self.assertNotIn(DEVICE, written)
            self.assertNotIn(DEVICE, console.getvalue())

    def test_blocks_missing_dev_allowlist(self):
        v = payload()
        v["result"]["bindings"] = [b for b in v["result"]["bindings"]
                                   if b["name"] != "MUSACAD_DEVELOPER_DEVICE_IDS"]
        with self.assertRaises(GuardError):
            settings_bindings(v)

    def test_blocks_invalid_allowlist(self):
        v = payload()
        v["result"]["bindings"][0]["text"] = "not-a-validated-device"
        with self.assertRaises(GuardError):
            settings_bindings(v)

    def test_blocks_missing_private_key_binding(self):
        v = payload()
        v["result"]["bindings"] = [b for b in v["result"]["bindings"]
                                   if b["name"] != "MUSACAD_TRIAL_PRIVATE_KEY_PEM"]
        with self.assertRaises(GuardError):
            settings_bindings(v)

    def test_blocks_unknown_binding_type(self):
        v = payload()
        v["result"]["bindings"].append({"name": "ASSETS", "type": "assets"})
        with self.assertRaises(GuardError):
            settings_bindings(v)

    def test_blocks_second_d1_binding(self):
        v = payload()
        v["result"]["bindings"].append({"name": "OTHER", "type": "d1", "database_id": D1})
        with self.assertRaises(GuardError):
            settings_bindings(v)

    def test_blocks_d1_change_after_deploy(self):
        with tempfile.TemporaryDirectory() as folder:
            root = Path(folder)
            before, after = root / "before.json", root / "after.json"
            before.write_text(json.dumps(payload()))
            changed = payload()
            changed["result"]["bindings"][-1]["database_id"] = "23456789-1234-1234-1234-123456789012"
            after.write_text(json.dumps(changed))
            with self.assertRaises(GuardError):
                run(["--settings", str(before), "--compare", str(after)])

    def test_blocks_device_list_overwrite_after_deploy(self):
        with tempfile.TemporaryDirectory() as folder:
            root = Path(folder)
            before, after = root / "before.json", root / "after.json"
            before.write_text(json.dumps(payload()))
            changed = payload()
            changed["result"]["bindings"][0]["text"] = "MC-ABCDEF12-34567890-ABCDEF12"
            after.write_text(json.dumps(changed))
            with self.assertRaises(GuardError):
                run(["--settings", str(before), "--compare", str(after)])

    def test_preserved_config_passes_comparison_without_leaking_value(self):
        with tempfile.TemporaryDirectory() as folder:
            root = Path(folder)
            before, after = root / "before.json", root / "after.json"
            before.write_text(json.dumps(payload()))
            after.write_text(json.dumps(payload()))
            console = io.StringIO()
            with contextlib.redirect_stdout(console):
                run(["--settings", str(before), "--compare", str(after)])
            self.assertIn("PASS", console.getvalue())
            self.assertNotIn(DEVICE, console.getvalue())


if __name__ == "__main__":
    unittest.main()
