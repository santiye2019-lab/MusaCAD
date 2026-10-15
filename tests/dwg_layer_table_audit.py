"""Opt-in count-only original-DWG LAYER table parity. No rendering claims."""
import re
_CODES=frozenset((6,62,70,290,370,420,440))
_DEFAULTS={6:"CONTINUOUS",62:"7",70:"0",290:"1",370:"-1"}
_INTEGER=re.compile(r"-?(?:0|[1-9]\d*)\Z")
_LIMIT=250000

class DwgLayerTableAudit:
    def __init__(self):
        self.layers={}
        self.invalid_decoded=0
        self.duplicate_names=0

    @staticmethod
    def _name(name):
        if (not isinstance(name,str) or not name or len(name)>256 or
                any(ord(c)<32 or ord(c)==127 for c in name)):
            raise ValueError("invalid private layer name")
        return name

    @classmethod
    def _fields(cls,fields,reference=False):
        if reference:
            if not isinstance(fields,dict) or len(fields)>len(_CODES):
                raise ValueError("invalid reference layer style")
            pairs=[]
            for key,value in fields.items():
                if not isinstance(key,str) or not key.isdecimal() or str(int(key))!=key:
                    raise ValueError("invalid original layer group code")
                code=int(key)
                if code not in _CODES:
                    raise ValueError("unsupported original layer field")
                pairs.append((code,value))
        else:
            pairs=[(code,value) for code,value in fields if code in _CODES]
        values={}
        for code,value in pairs:
            if code in values or not isinstance(value,str) or not value or len(value)>256:
                raise ValueError("duplicate or invalid layer group value")
            if code==6:
                if any(ord(c)<32 or ord(c)==127 for c in value):
                    raise ValueError("invalid linetype")
                values[code]=value
                continue
            if not _INTEGER.fullmatch(value):
                raise ValueError("invalid numeric layer field")
            val=int(value)
            if code==62 and (val==0 or abs(val)>255):
                raise ValueError("invalid signed layer ACI")
            if code==70 and not 0<=val<=65535:
                raise ValueError("invalid layer flags")
            if code==290 and val not in (0,1):
                raise ValueError("invalid layer plot switch")
            if code==370 and not -3<=val<=211:
                raise ValueError("invalid lineweight")
            if code==420 and not 0<=val<=0xFFFFFF:
                raise ValueError("invalid layer RGB")
            if code==440 and not 0<=val<=0xFFFFFFFF:
                raise ValueError("invalid transparency")
            values[code]=str(val)
        return tuple((code,values.get(code,_DEFAULTS.get(code))) for code in sorted(_CODES))

    def observe(self,fields):
        try:
            names=[value for code,value in fields if code==2]
            if len(names)!=1:
                raise ValueError("missing/duplicate decoded layer name")
            name=self._name(names[0])
            if name in self.layers:
                self.duplicate_names+=1
                return
            if len(self.layers)>=_LIMIT:
                raise ValueError("layer count exceeded")
            self.layers[name]=self._fields(fields)
        except ValueError:
            self.invalid_decoded+=1

    def compare_independent(self,records):
        if not isinstance(records,list) or not records or len(records)>_LIMIT:
            raise ValueError("empty or oversized source layer inventory")
        refs={}
        for record in records:
            if not isinstance(record,dict) or set(record)!={"name","groups"}:
                raise ValueError("invalid independent layer record")
            name=self._name(record["name"])
            if name in refs:
                raise ValueError("duplicate independent source layer")
            refs[name]=self._fields(record["groups"],reference=True)
        decoded=set(self.layers);expected=set(refs)
        common=decoded&expected
        changed=sum(self.layers[n]!=refs[n] for n in common)
        passed=(not self.invalid_decoded and not self.duplicate_names and
                not decoded^expected and not changed)
        return {
            "independent_layer_table_result":("layer_table_metadata_parity_only"
                if passed else "layer_table_metadata_mismatch"),
            "independent_layer_table_checked":len(common),
            "independent_layer_table_missing_source":len(decoded-expected),
            "independent_layer_table_unexpected_source":len(expected-decoded),
            "independent_layer_table_changed":changed,
            "independent_layer_table_invalid_decoded":self.invalid_decoded,
            "independent_layer_table_duplicate_decoded":self.duplicate_names,
            "independent_layer_table_scope":(
                "TABLES/LAYER groups 6/62/70/290/370/420/440 only; "
                "not native effective colors, XREF overrides or screen pixels.")
        }
