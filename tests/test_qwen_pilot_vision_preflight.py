import base64
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parent.parent


class QwenVisionPreflightGuardTest(unittest.TestCase):
    def test_synthetic_image_is_real_jpeg_with_no_user_data(self):
        fixture = ROOT / "tests/fixtures/qwen-vision-smoke-jpeg.b64"
        raw = base64.b64decode(fixture.read_text().strip(), validate=True)
        self.assertTrue(raw.startswith(b"\xff\xd8"))
        self.assertTrue(raw.endswith(b"\xff\xd9"))
        self.assertLess(len(raw), 12000)
        self.assertGreater(len(raw), 150)

    def test_signed_pilot_requires_explicit_test_apk_checkbox_and_live_vision(self):
        workflow = (ROOT / ".github/workflows/qwen-isolated-pilot-24h.yml").read_text()
        self.assertIn("name: Live Qwen synthetic-image readiness probe", workflow)
        self.assertIn("data:image/jpeg;base64,", workflow)
        self.assertIn("qwen-vision-smoke-jpeg.b64", workflow)
        validator = (ROOT / "scripts/validate_qwen_vision_result.py").read_text()
        self.assertIn("Qwen synthetic-image readiness PASS", validator)
        self.assertIn("python3 scripts/validate_qwen_vision_result.py", workflow)
        self.assertIn("no user drawing", workflow)
        self.assertIn("if [ \"$status\" != 200 ]; then", workflow)
        self.assertIn("no deployment", workflow)
        self.assertNotIn("inputs.release_approved == true", workflow)
        self.assertNotIn("Final acceptance complete", workflow)
        self.assertIn("I approve creating a signed 24h TEST APK only", workflow)
        self.assertIn("python3 scripts/validate_qwen_vision_result.py", workflow)
        self.assertIn("inputs.build_pilot_apk == true", workflow)
        self.assertIn("inputs.operation == 'deploy'", workflow)
        self.assertIn('if: inputs.operation == \'deploy\' && inputs.build_pilot_apk == true', workflow)
        self.assertIn("MUSACAD_PILOT_DEVELOPER_ONLY = \"true\"", workflow)
        self.assertIn("MUSACAD_PILOT_EXPIRES_AT_MS", workflow)
        # No image/CAD data is embedded in CI output or sent to third-party logs.
        self.assertNotIn('echo "$CLOUDFLARE_AI_API_TOKEN"', workflow)

    def test_same_three_batch_grid_is_supported_at_both_ends(self):
        sweep = (ROOT / "app/src/main/java/com/musa/cad/MusaAiVisualSweepPlan.java").read_text()
        worker = (ROOT / "server/ai-worker/src/index.js").read_text()
        self.assertIn("TILES_PER_BATCH=3", sweep)
        self.assertIn("expectedThreeFirst", worker)
        self.assertIn("expectedLegacyFirst", worker)
        self.assertIn("threeTileProtocol", worker)
        self.assertIn("legacyProtocol", worker)
        self.assertIn("if (!threeTileProtocol && !legacyProtocol) return null;", worker)


if __name__ == "__main__":
    unittest.main()
