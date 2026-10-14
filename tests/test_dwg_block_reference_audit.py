"""Original-DWG BLOCK/XREF INSERT topology metadata regression."""
import hashlib,json,sys,tempfile,unittest
from pathlib import Path
from unittest.mock import Mock,patch
from dwg_block_reference_audit import DwgBlockReferenceAudit
from private_dwg_read_audit import convert_one,inventory_dxf,main

def record(kind,*fields):
    return "0\n"+kind+"\n"+"".join(f"{k}\n{v}\n" for k,v in fields)

def source_blocks():
    return [
        {"name":"External-Secret","groups":{"70":"4","1":"C:\\PRIVATE\\external.dwg","10":"0","20":"0"}},
        {"name":"Local","groups":{"70":"0"}},
    ]

def source_inserts():
    return [
        {"handle":"A","name":"External-Secret","layer":"DO_NOT_REPORT",
         "space":"paperspace","layout":"Layout-Private",
         "groups":{"10":"5","20":"6","30":"0","41":"1","42":"-2","43":"1","50":"45"}},
        {"handle":"B","name":"Local","layer":"0","space":"block","layout":None,
         "groups":{"10":"0","20":"2"}},
    ]

class BlockReferenceAuditTest(unittest.TestCase):
    def make_audit(self):
        audit=DwgBlockReferenceAudit()
        audit.observe("BLOCKS","BLOCK",[(2,"External-Secret"),(70,"4"),
            (1,"C:\\PRIVATE\\external.dwg"),(10,"0"),(20,"0")])
        audit.observe("BLOCKS","BLOCK",[(2,"Local"),(70,"0")])
        audit.observe("ENTITIES","INSERT",[
            (5,"0a"),(2,"External-Secret"),(8,"DO_NOT_REPORT"),
            (67,"1"),(410,"Layout-Private"),(10,"5.0"),(20,"6"),
            (41,"1"),(42,"-2"),(43,"1"),(50,"45")])
        audit.observe("BLOCKS","INSERT",[
            (5,"B"),(2,"Local"),(8,"0"),(10,"0"),(20,"2")])
        return audit

    def test_exact_xref_path_transform_and_layout_parity_no_data_leak(self):
        a=self.make_audit()
        passed=a.compare_independent(source_blocks(),source_inserts())
        self.assertEqual(passed["independent_block_reference_result"],
                         "block_reference_metadata_parity_only")
        self.assertEqual(passed["independent_block_records_checked"],2)
        self.assertEqual(passed["independent_insert_records_checked"],2)
        for hidden in ("PRIVATE","External-Secret","DO_NOT_REPORT","Layout-Private","0a"):
            self.assertNotIn(hidden,str(passed))
        changed=source_blocks()
        changed[0]["groups"]["1"]="C:\\PRIVATE\\DIFFERENT.dwg"
        result=a.compare_independent(changed,source_inserts())
        self.assertEqual(result["independent_block_definition_mismatches"],1)
        changed=source_inserts()
        changed[0]["groups"]["42"]="-3"
        self.assertEqual(a.compare_independent(source_blocks(),changed)[
            "independent_insert_transform_mismatches"],1)
        changed=source_inserts()
        changed[0]["layout"]="Different Layout"
        self.assertEqual(a.compare_independent(source_blocks(),changed)[
            "independent_insert_transform_mismatches"],1)

    def test_missing_extra_invalid_and_duplicate_fail_closed(self):
        audit=self.make_audit()
        self.assertEqual(audit.compare_independent(source_blocks()[:1],source_inserts()[:1])[
            "independent_block_reference_result"],"block_reference_metadata_mismatch")
        with self.assertRaises(ValueError):
            audit.compare_independent(source_blocks(),source_inserts()+source_inserts()[:1])
        for b in (
            {"name":"External-Secret","groups":{"70":"4"}},
            {"name":"External-Secret","groups":{"70":"not-a-number"}},
            {"name":"External-Secret","groups":{"70":"4","1":"BAD\nFILE"}},
        ):
            with self.subTest(b=b):
                with self.assertRaises(ValueError):
                    audit.compare_independent([b],source_inserts())
        changed=source_inserts()
        changed[0]["groups"]["41"]="0"
        with self.assertRaises(ValueError):
            audit.compare_independent(source_blocks(),changed)
        broken=DwgBlockReferenceAudit()
        broken.observe("BLOCKS","BLOCK",[(2,"NotXref"),(70,"0")])
        broken.observe("ENTITIES","INSERT",[(5,"A"),(2,"MISSING"),(8,"0"),(10,"0"),(20,"0")])
        diff=broken.compare_independent([{"name":"NotXref","groups":{}}],
             [{"handle":"A","name":"MISSING","layer":"0","space":"model","layout":None,
               "groups":{"10":"0","20":"0"}}])
        self.assertEqual(diff["independent_insert_unresolved_blocks"],1)
        self.assertEqual(diff["independent_block_reference_result"],
                         "block_reference_metadata_mismatch")
        dup=DwgBlockReferenceAudit()
        dup.observe("BLOCKS","BLOCK",[(2,"Local"),(70,"0")])
        dup.observe("BLOCKS","BLOCK",[(2,"Local"),(70,"0")])
        self.assertEqual(dup.duplicate_blocks,1)

    def test_private_dxf_wiring_and_explicit_status(self):
        dxf=(record("SECTION",(2,"BLOCKS"))+
             record("BLOCK",(2,"Local"),(70,"0"))+
             record("INSERT",(5,"B"),(2,"Local"),(8,"0"),(10,"0"),(20,"2"))+
             record("ENDBLK")+record("ENDSEC")+
             record("SECTION",(2,"ENTITIES"))+
             record("LINE",(5,"C"),(8,"0"),(10,"0"),(20,"0"),(11,"1"),(21,"2"))+
             record("ENDSEC")+record("EOF"))
        with tempfile.TemporaryDirectory() as td:
            root=Path(td)
            converted=root/"source.dxf"
            converted.write_text(dxf,encoding="utf-8")
            block_audit=DwgBlockReferenceAudit()
            inventory=inventory_dxf(converted,block_collector=block_audit)
            self.assertEqual(inventory["block_definition_count"],1)
            self.assertEqual(block_audit.compare_independent(
                [{"name":"Local","groups":{"70":"0"}}],[source_inserts()[1]])[
                    "independent_block_reference_result"],"block_reference_metadata_parity_only")
            original=root/"source.dwg"
            original.write_bytes(b"AC1032 private test fixture")
            converter=root/"converter"
            converter.write_text("#!/bin/sh\nexit 0\n",encoding="ascii")
            converter.chmod(0o700)
            def write_mock(args,**kwargs):
                Path(args[2]).write_text(dxf,encoding="utf-8")
                kwargs["stderr"].write(b"MusaCAD native scene: status=0 primitives=2 floats=10\n")
                return Mock(returncode=0)
            with patch("private_dwg_read_audit.subprocess.run",side_effect=write_mock):
                with tempfile.TemporaryDirectory(dir=root) as tmp:
                    good=convert_one(original,converter,Path(tmp),5,128,
                        independent_blocks=[{"name":"Local","groups":{"70":"0"}}],
                        independent_inserts=[source_inserts()[1]])
                with tempfile.TemporaryDirectory(dir=root) as tmp:
                    bad=convert_one(original,converter,Path(tmp),5,128,
                        independent_blocks=[{"name":"Local","groups":{"70":"0"}}],
                        independent_inserts=[dict(source_inserts()[1],layout="PRIVATE")])
            self.assertEqual(good["status"],"decoded_dxf_inventory")
            self.assertEqual(bad["status"],"decoded_dxf_inventory_block_reference_mismatch")
            self.assertEqual(original.read_bytes(),b"AC1032 private test fixture")

    def test_manifest_optional_both_lists_or_fail_closed(self):
        with tempfile.TemporaryDirectory() as td:
            root=Path(td);original=root/"orig.dwg"
            original.write_bytes(b"AC1032 manifest fixture")
            digest=hashlib.sha256(original.read_bytes()).hexdigest()
            folder=root/"refs";folder.mkdir()
            exe=root/"converter"
            exe.write_text("#!/bin/sh\nexit 0\n",encoding="ascii");exe.chmod(0o700)
            filename=folder/(digest+".json")
            refs={"source_sha256":digest,"source_kind":"independent_dwg_reader",
                  "records":[],"block_records":source_blocks(),
                  "insert_records":source_inserts()}
            filename.write_text(json.dumps(refs),encoding="utf-8")
            cmd=["private_dwg_read_audit.py","--converter",str(exe),
                 "--independent-manifest-dir",str(folder),
                 "--report",str(root/"report.json"),str(original)]
            with patch.object(sys,"argv",cmd),patch(
                    "private_dwg_read_audit.convert_one",
                    return_value={"status":"decoded_dxf_inventory"}) as mocked:
                self.assertEqual(main(),0)
                self.assertEqual(mocked.call_args.kwargs["independent_blocks"],source_blocks())
                self.assertEqual(mocked.call_args.kwargs["independent_inserts"],source_inserts())
            refs.pop("insert_records")
            filename.write_text(json.dumps(refs),encoding="utf-8")
            with patch.object(sys,"argv",cmd),patch("private_dwg_read_audit.convert_one") as mocked:
                self.assertEqual(main(),1)
                mocked.assert_not_called()

if __name__=="__main__":
    unittest.main()
