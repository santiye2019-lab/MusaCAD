#!/usr/bin/env python3
"""Reject any third-party CAD SDK dependencies inside first-party core/native."""
import re
import unittest
from pathlib import Path

ROOT=Path(__file__).resolve().parents[1]
NATIVE=ROOT/"core"/"native"

class FirstPartyCadCoreDependencyTest(unittest.TestCase):
    def test_only_owned_sources_are_built(self):
        cmake=(NATIVE/"CMakeLists.txt").read_text(encoding="utf-8")
        target=re.search(r"add_library\(musacad_core\s+STATIC\s+(.*?)\)",cmake,re.S)
        self.assertIsNotNone(target,"First-party C ABI target missing")
        declared=re.findall(r"src/[A-Za-z0-9_]+\.c",target.group(1))
        actual=[str(p.relative_to(NATIVE)).replace("\\","/") for p in (NATIVE/"src").glob("*.c")]
        self.assertEqual(set(declared),set(actual))
        self.assertEqual(len(declared),len(set(declared)))
        self.assertIn("src/musacad_document.c",declared)
        # A future change cannot silently insert the old third-party decoder.
        active="\n".join(line.split("#",1)[0] for line in cmake.splitlines())
        self.assertNotRegex(active,r"(?i)\b(?:redwg|libredwg|opendesign|oda[_:]|teigha)\b")
        self.assertNotRegex(active,r"(?i)\b(?:add_subdirectory|fetchcontent|externalproject_add)\s*\(.*(?:third_party|libredwg)")

    def test_no_external_cad_binary_interfaces_in_shared_kernel(self):
        forbidden_headers={
            "dwg.h","dwg_api.h","out_dxf.h","bits.h",
            "oddbdatabase.h","oddbentity.h","odfilebuf.h"
        }
        for root in (NATIVE/"src",NATIVE/"include"):
            for file in list(root.glob("*.c"))+list(root.glob("*.h")):
                with self.subTest(file=str(file.relative_to(ROOT))):
                    contents=file.read_text(encoding="utf-8")
                    includes=re.findall(r"^\s*#\s*include\s*[<\"]([^>\"]+)[>\"]",contents,re.M)
                    self.assertFalse(
                        [item for item in includes
                         if Path(item.lower()).name in forbidden_headers
                         or "libredwg" in item.lower() or "teigha" in item.lower()],
                        "First-party core cannot depend on a DWG SDK header"
                    )

    def test_document_abi_is_real_and_bounded(self):
        header=(NATIVE/"include"/"musacad_document.h").read_text(encoding="utf-8")
        for operation in ("musa_document_create","musa_document_add_line",
                          "musa_document_add_circle","musa_document_translate",
                          "musa_document_delete"):
            self.assertIn(operation,header)
        cmake=(NATIVE/"CMakeLists.txt").read_text(encoding="utf-8")
        self.assertIn("musacad_core_document_test",cmake)

if __name__=="__main__":
    unittest.main()
