"""Independent layer table regression; no source drawing upload or mutation."""
import hashlib
import json
import sys
import tempfile
import unittest
from pathlib import Path
from unittest.mock import Mock,patch
from dwg_layer_table_audit import DwgLayerTableAudit
from private_dwg_read_audit import convert_one,inventory_dxf,main

def record(kind,*fields):
    return "0\n"+kind+"\n"+"".join(f"{code}\n{value}\n" for code,value in fields)

class DwgLayerTableAuditTest(unittest.TestCase):
    def test_layer_aci_off_lock_truecolor_and_linetype_parity(self):
        audit=DwgLayerTableAudit()
        audit.observe([(2,"PRIVATE PIPE"),(62,"-5"),(70,"4"),
                       (6,"DASHED"),(420,"16711680"),(290,"0"),(370,"50")])
        audit.observe([(2,"0"),(62,"7")])
        reference=[
            {"name":"PRIVATE PIPE","groups":{
                "62":"-5","70":"4","6":"DASHED","420":"16711680",
                "290":"0","370":"50"}},
            {"name":"0","groups":{}}
        ]
        pass_result=audit.compare_independent(reference)
        self.assertEqual(pass_result["independent_layer_table_result"],
                         "layer_table_metadata_parity_only")
        self.assertEqual(pass_result["independent_layer_table_checked"],2)
        self.assertNotIn("PRIVATE",str(pass_result))
        changed=[dict(x) for x in reference]
        changed[0]=dict(reference[0],groups=dict(reference[0]["groups"],**{"420":"255"}))
        self.assertEqual(audit.compare_independent(changed)["independent_layer_table_changed"],1)
        changed[0]=dict(reference[0],groups=dict(reference[0]["groups"],**{"70":"0"}))
        self.assertEqual(audit.compare_independent(changed)["independent_layer_table_changed"],1)
        self.assertEqual(audit.compare_independent(reference[1:])[
            "independent_layer_table_missing_source"],1)

    def test_malformed_fields_and_duplicates_fail_closed(self):
        for extra in ([(62,"0")],[(62,"256")],[(420,"16777216")],
                      [(70,"-1")],[(290,"2")],[(370,"-4")],
                      [(440,"4294967296")],[(6,"")],
                      [(62,"7"),(62,"7")],[(2,"DIFFERENT")]):
            with self.subTest(extra=extra):
                audit=DwgLayerTableAudit()
                audit.observe([(2,"SECRET"),(62,"7")]+extra)
                response=audit.compare_independent([{"name":"SECRET","groups":{}}])
                self.assertEqual(response["independent_layer_table_result"],
                                 "layer_table_metadata_mismatch")
                self.assertNotIn("SECRET",str(response))
        audit=DwgLayerTableAudit()
        audit.observe([(2,"0"),(62,"7")])
        audit.observe([(2,"0"),(62,"7")])
        self.assertEqual(audit.compare_independent([{"name":"0","groups":{}}])[
            "independent_layer_table_duplicate_decoded"],1)
        for invalid in ({"420":"NaN"},{"999":"5"},{"062":"7"},{"290":1}):
            with self.subTest(invalid=invalid):
                with self.assertRaises(ValueError):
                    audit.compare_independent([{"name":"0","groups":invalid}])

    def test_dxf_integration_and_status_when_source_layer_changes(self):
        fixture=(record("SECTION",(2,"TABLES"))+
                 record("LAYER",(2,"CONFIDENTIAL"),(62,"-2"),(70,"4"))+
                 record("ENDSEC")+record("SECTION",(2,"ENTITIES"))+
                 record("LINE",(5,"A"),(8,"CONFIDENTIAL"),
                        (10,"0"),(20,"0"),(11,"2"),(21,"2"))+
                 record("ENDSEC")+record("EOF"))
        collector=DwgLayerTableAudit()
        with tempfile.TemporaryDirectory() as directory:
            root=Path(directory)
            dxf=root/"fixture.dxf"
            dxf.write_text(fixture,encoding="utf-8")
            summary=inventory_dxf(dxf,layer_collector=collector)
            self.assertEqual(summary["layer_count"],1)
            self.assertEqual(collector.compare_independent([
                {"name":"CONFIDENTIAL","groups":{"62":"-2","70":"4"}}])[
                "independent_layer_table_result"],"layer_table_metadata_parity_only")
            source=root/"original.dwg"
            source.write_bytes(b"AC1032 synthetic source")
            converter=root/"converter"
            converter.write_text("#!/bin/sh\nexit 0\n",encoding="ascii")
            converter.chmod(0o700)
            def run(argv,**kw):
                Path(argv[2]).write_text(fixture,encoding="utf-8")
                kw["stderr"].write(b"MusaCAD native scene: status=0 primitives=1 floats=5\n")
                return Mock(returncode=0)
            with patch("private_dwg_read_audit.subprocess.run",side_effect=run):
                with tempfile.TemporaryDirectory(dir=root) as temp:
                    good=convert_one(source,converter,Path(temp),5,128,
                        independent_layers=[{"name":"CONFIDENTIAL",
                            "groups":{"62":"-2","70":"4"}}])
                with tempfile.TemporaryDirectory(dir=root) as temp:
                    bad=convert_one(source,converter,Path(temp),5,128,
                        independent_layers=[{"name":"CONFIDENTIAL",
                            "groups":{"62":"-2","70":"0"}}])
            self.assertEqual(good["status"],"decoded_dxf_inventory")
            self.assertEqual(bad["status"],"decoded_dxf_inventory_layer_table_mismatch")
            self.assertEqual(bad["independent_layer_table_changed"],1)

    def test_manifest_forwards_layer_records(self):
        with tempfile.TemporaryDirectory() as directory:
            root=Path(directory)
            source=root/"src.dwg"
            source.write_bytes(b"AC1032 source manifest")
            folder=root/"refs"
            folder.mkdir()
            digest=hashlib.sha256(source.read_bytes()).hexdigest()
            refs=[{"name":"0","groups":{"62":"7"}}]
            (folder/(digest+".json")).write_text(json.dumps({
                "source_sha256":digest,"source_kind":"independent_dwg_reader",
                "records":[],"layer_records":refs}),encoding="utf-8")
            converter=root/"converter"
            converter.write_text("#!/bin/sh\nexit 0\n",encoding="ascii")
            converter.chmod(0o700)
            args=["private_dwg_read_audit.py","--converter",str(converter),
                  "--independent-manifest-dir",str(folder),
                  "--report",str(root/"report.json"),str(source)]
            with patch.object(sys,"argv",args),patch(
                    "private_dwg_read_audit.convert_one",
                    return_value={"status":"decoded_dxf_inventory"}) as called:
                self.assertEqual(main(),0)
                self.assertEqual(called.call_args.kwargs["independent_layers"],refs)

if __name__=="__main__":
    unittest.main()
