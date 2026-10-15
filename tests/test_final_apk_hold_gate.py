"""Repository APK hold policy: never assemble a device-installable APK on push/PR.

Developer requested final explicit approval before *any* APK, including the
old isolated Qwen pilot. This is source-based CI validation and does not
supply, infer or forge a user's release authorization.
"""
import re
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
WORKFLOWS = ROOT / ".github/workflows"
MANUAL_APPROVAL = "github.event_name == 'workflow_dispatch' && inputs.release_approved == true"
FINAL_PILOT_APPROVAL = "inputs.final_apk_approved == true"

# An APK may only be built under manual workflow_dispatch plus explicit consent.
# Compilation and JUnit tests are allowed in normal CI; assemble/bundle is not.
BUILD_TASK = re.compile(r"(?:^|\s)(?:\S*gradlew?\s+\S*\s+)?"
                        r"(?::[\w-]+:)?(?:assemble|bundle)[A-Za-z0-9_-]*\b")


def apk_build_steps(text):
    """Return (job, step, nearest step/job if) for actual Gradle APK tasks."""
    job = None
    job_condition = ""
    step = None
    step_condition = ""
    for line in text.splitlines():
        if re.match(r"^  [A-Za-z][\w-]*:\s*$", line):
            job = line.strip().rstrip(":")
            job_condition = ""
            step = None
            step_condition = ""
        elif re.match(r"^    if:\s*", line) and job:
            job_condition = line.strip().split("if:", 1)[1].strip()
        elif re.match(r"^      - (?:name|uses):", line) and job:
            step = line.strip()
            step_condition = ""
        elif re.match(r"^        if:\s*", line) and job and step:
            step_condition = line.strip().split("if:", 1)[1].strip()
        elif "./gradlew" in line and BUILD_TASK.search(line):
            yield (job, step, job_condition, step_condition, line.strip())


class FinalApkHoldPolicyTests(unittest.TestCase):
    def test_all_assemble_or_bundle_jobs_require_final_manual_approval(self):
        builds = []
        for path in sorted(WORKFLOWS.glob("*.yml")):
            text = path.read_text(encoding="utf-8")
            for job, step, job_guard, step_guard, cmd in apk_build_steps(text):
                builds.append((path.name, job, step))
                guards = job_guard + " " + step_guard
                if path.name == "qwen-isolated-pilot-24h.yml":
                    self.assertIn(FINAL_PILOT_APPROVAL, guards, (path.name, cmd))
                    self.assertIn("inputs.build_pilot_apk == true", guards)
                    self.assertIn("inputs.operation == 'deploy'", guards)
                    self.assertIn("workflow_dispatch:", text)
                    self.assertIn("final_apk_approved:", text)
                    self.assertIn("default: false", text)
                else:
                    self.assertIn(MANUAL_APPROVAL, guards, (path.name, cmd))
                    self.assertIn("release_approved:", text, path.name)
                    self.assertIn("default: false", text, path.name)
        # At least the app, the two manager workflows, AI and pilot builds
        # have been audited. Catch accidentally deleting the entire audit.
        self.assertGreaterEqual(len(builds), 6, builds)

    def test_android_ci_only_compiles_and_tests_before_final_approval(self):
        android = (WORKFLOWS / "android.yml").read_text(encoding="utf-8")
        self.assertIn("compilePlayReleaseJavaWithJavac", android)
        self.assertIn("compileDirectReleaseJavaWithJavac", android)
        self.assertIn(MANUAL_APPROVAL, android)
        self.assertIn("assembleDirectDebug", android)
        self.assertNotIn("contains(github.event.head_commit.message, '[build-apk]')",
                         android)

    def test_pilot_requires_both_final_authorization_and_separate_test_consent(self):
        pilot = (WORKFLOWS / "qwen-isolated-pilot-24h.yml").read_text(
            encoding="utf-8")
        self.assertIn(
            "if: inputs.operation == 'deploy' && inputs.build_pilot_apk == true "
            "&& inputs.final_apk_approved == true", pilot)
        self.assertIn("final_apk_approved:\n"
                      '        description: "FINAL APK authorization', pilot)
        self.assertIn("        default: false", pilot)

    def test_no_release_auto_merge_trigger_or_commit_message_shortcut(self):
        for path in WORKFLOWS.glob("*.yml"):
            text = path.read_text(encoding="utf-8")
            for job, step, jguard, sguard, cmd in apk_build_steps(text):
                # A job may run for code tests or AI deployment following a
                # source push. If its APK step has a stricter own gate,
                # only that step gate can authorize Gradle APK assembly.
                effective_gate = sguard if sguard else jguard
                self.assertNotIn("head_commit.message", effective_gate,
                                 (path.name, cmd))
                self.assertNotIn("github.event_name == 'push'",
                                 effective_gate, (path.name, cmd))


if __name__ == "__main__":
    unittest.main()
