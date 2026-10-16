"""Android Back must cancel an unsaved CAD project close, not destroy its cache."""
from pathlib import Path
import unittest

SOURCE = Path("app/src/main/java/com/musa/cad/MainActivity.java")


class UnsavedProjectBackSafety(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.src = SOURCE.read_text(encoding="utf-8")

    def test_project_aware_navigation_keeps_close_confirmation(self):
        source = self.src
        start = source.index("private void handleBackNavigation(){")
        end = source.index("@Override protected void onNewIntent(", start)
        handler = source[start:end]
        self.assertIn("requestCloseProject(currentProject);", handler)
        self.assertNotIn("closeProjectNow(", handler)

    def test_back_on_confirmation_does_not_close_or_erase_recovery(self):
        source = self.src
        start = source.index("projectCloseDialog.setOnKeyListener(")
        end = source.index("projectCloseDialog.setOnDismissListener(", start)
        handler = source[start:end]
        self.assertIn("KeyEvent.KEYCODE_BACK", handler)
        self.assertIn("event.getAction()==KeyEvent.ACTION_UP", handler)
        self.assertIn("dialog.dismiss()", handler)
        self.assertNotIn("closeProjectNow(", handler)
        self.assertNotIn("clearRecovery(", handler)
        self.assertNotIn("projects.remove(", handler)

    def test_discard_requires_named_explicit_action(self):
        source = self.src
        begin = source.index("private void requestCloseProject(")
        end = source.index("private void closeProjectNow(", begin)
        method = source[begin:end]
        self.assertIn('setNegativeButton("İPTAL",null)', method)
        self.assertIn('setNeutralButton("KAYDETMEDEN KAPAT"', method)
        self.assertIn('setPositiveButton("KAYDET VE KAPAT"', method)
        self.assertIn("if(dirty){", method)
        self.assertIn("Telefonun Geri tuşu işlemi iptal eder.", method)
        self.assertNotIn("İkinci kez geri tuşuna basarsanız proje", method)
        self.assertIn("projectCloseDialog.setOnDismissListener(", method)


if __name__ == "__main__":
    unittest.main()
