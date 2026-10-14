"""Independent original-DWG BLOCK/XREF and INSERT identity/transform audit.

Optional private local source manifest ONLY. Captures metadata, not evaluated
dynamic blocks, loaded XREF geometry, rendered layout or on-screen fidelity.
Never reports confidential paths, block names, handles or coordinates.
"""
from decimal import Decimal, InvalidOperation
import re
from dwg_entity_identity_audit import DwgEntityIdentityAudit

_LIMIT=250_000
_HEX=re.compile(r"[0-9A-Fa-f]{1,32}\Z")
_INTEGER=re.compile(r"[+-]?\d+\Z")
_FLOAT_GROUPS=(10,20,30,41,42,43,50)
_DEFAULT_INSERT={30:"0",41:"1",42:"1",43:"1",50:"0"}

class DwgBlockReferenceAudit:
    def __init__(self):
        self.blocks={}
        self.inserts={}
        self.invalid_blocks=0
        self.invalid_inserts=0
        self.duplicate_blocks=0
        self.duplicate_inserts=0

    @staticmethod
    def _text(value):
        if (not isinstance(value,str) or not value or len(value)>512
                or any(ord(char)<32 or ord(char)==127 for char in value)):
            raise ValueError("invalid identifier or reference text")
        return value

    @staticmethod
    def _once(pairs,code):
        vals=[v for k,v in pairs if k==code]
        if len(vals)>1:
            raise ValueError("duplicate DXF field")
        return vals[0] if vals else None

    @staticmethod
    def _decimal(value):
        if not isinstance(value,str) or not value or len(value)>128:
            raise ValueError("missing or invalid decimal value")
        try:
            number=Decimal(value)
        except InvalidOperation as exc:
            raise ValueError("invalid floating number") from exc
        if not number.is_finite():
            raise ValueError("nonfinite drawing transform")
        return number

    @classmethod
    def _block(cls,name,pairs):
        name=cls._text(name)
        flags=cls._once(pairs,70)
        flags="0" if flags is None else flags
        if not isinstance(flags,str) or not _INTEGER.fullmatch(flags):
            raise ValueError("invalid block flags")
        integer=int(flags)
        if not 0<=integer<=255:
            raise ValueError("invalid block flags bits")
        path=cls._once(pairs,1)
        if path is None:
            path=""
        if not isinstance(path,str) or len(path)>4096 or any(ord(c)<32 for c in path):
            raise ValueError("invalid XREF path")
        if integer&12 and not path:
            raise ValueError("XREF lacks recorded external source path")
        origin=[]
        for code in (10,20,30):
            value=cls._once(pairs,code)
            origin.append(cls._decimal(value if value is not None else "0"))
        return (integer,path,tuple(origin))

    @classmethod
    def _insert(cls,handle,name,layer,space,layout,pairs):
        name=cls._text(name)
        layer=cls._text(layer)
        if space not in ("model","paperspace","block"):
            raise ValueError("invalid INSERT space")
        if layout is not None:
            layout=cls._text(layout)
        groups={}
        for code,value in pairs:
            if code in _FLOAT_GROUPS:
                if code in groups:
                    raise ValueError("duplicate INSERT transform field")
                groups[code]=cls._decimal(value)
        if 10 not in groups or 20 not in groups:
            raise ValueError("missing INSERT insertion position")
        for code in _FLOAT_GROUPS:
            if code not in groups:
                if code in _DEFAULT_INSERT:
                    groups[code]=cls._decimal(_DEFAULT_INSERT[code])
                else:
                    raise ValueError("incomplete INSERT coordinates")
        if any(groups[code]==0 for code in (41,42,43)):
            raise ValueError("zero INSERT scale invalid")
        return (name,layer,space,layout,tuple((c,groups[c]) for c in _FLOAT_GROUPS))

    @classmethod
    def _handle(cls,raw):
        if not isinstance(raw,str) or not _HEX.fullmatch(raw):
            raise ValueError("invalid INSERT handle")
        parsed=int(raw,16)
        if parsed==0:
            raise ValueError("zero INSERT handle")
        return format(parsed,"X")

    def observe(self,section,kind,fields):
        if section=="BLOCKS" and kind=="BLOCK":
            try:
                name=self._once(fields,2)
                if len(self.blocks)>=_LIMIT:
                    raise ValueError("too many BLOCK records")
                if name in self.blocks:
                    self.duplicate_blocks+=1
                    return
                self.blocks[self._text(name)]=self._block(name,fields)
            except ValueError:
                self.invalid_blocks+=1
        elif kind=="INSERT" and section in ("BLOCKS","ENTITIES"):
            try:
                handle=self._handle(self._once(fields,5))
                if handle in self.inserts:
                    self.duplicate_inserts+=1
                    return
                if len(self.inserts)>=_LIMIT:
                    raise ValueError("too many INSERT records")
                space,valid=DwgEntityIdentityAudit._space(section,fields)
                if not valid:
                    raise ValueError("invalid model/paper classification")
                layout=self._once(fields,410)
                self.inserts[handle]=self._insert(
                    handle,self._once(fields,2),self._once(fields,8),
                    space,layout,fields)
            except ValueError:
                self.invalid_inserts+=1

    def compare_independent(self,block_records,insert_records):
        if (not isinstance(block_records,list) or not isinstance(insert_records,list)
                or len(block_records)>_LIMIT or len(insert_records)>_LIMIT
                or not (block_records or insert_records)):
            raise ValueError("empty/oversized independent BLOCK/INSERT records")
        blocks={}
        for r in block_records:
            if not isinstance(r,dict) or set(r)!={"name","groups"}:
                raise ValueError("invalid source block entry")
            name=self._text(r["name"])
            if name in blocks:
                raise ValueError("duplicate original block")
            if not isinstance(r["groups"],dict):
                raise ValueError("invalid source block fields")
            blocks[name]=self._block(name,self._reference_pairs(r["groups"],(1,10,20,30,70)))
        inserts={}
        for r in insert_records:
            if not isinstance(r,dict) or set(r)!={"handle","name","layer","space","layout","groups"}:
                raise ValueError("invalid source INSERT record")
            handle=self._handle(r["handle"])
            if handle in inserts:
                raise ValueError("duplicate original INSERT")
            inserts[handle]=self._insert(handle,r["name"],r["layer"],r["space"],r["layout"],
                self._reference_pairs(r["groups"],_FLOAT_GROUPS))
        a,b=set(self.blocks),set(blocks)
        ia,ib=set(self.inserts),set(inserts)
        changed_blocks=sum(self.blocks[name]!=blocks[name] for name in a&b)
        changed_inserts=sum(self.inserts[handle]!=inserts[handle] for handle in ia&ib)
        unresolved=sum(record[0] not in self.blocks for record in self.inserts.values())
        okay=(not self.invalid_blocks and not self.invalid_inserts and
              not self.duplicate_blocks and not self.duplicate_inserts and
              not a^b and not ia^ib and not changed_blocks and
              not changed_inserts and not unresolved)
        return {
            "independent_block_reference_result":(
                "block_reference_metadata_parity_only" if okay else "block_reference_metadata_mismatch"),
            "independent_block_records_checked":len(a&b),
            "independent_insert_records_checked":len(ia&ib),
            "independent_block_records_missing_source":len(a-b),
            "independent_block_records_missing_decoded":len(b-a),
            "independent_insert_records_missing_source":len(ia-ib),
            "independent_insert_records_missing_decoded":len(ib-ia),
            "independent_block_definition_mismatches":changed_blocks,
            "independent_insert_transform_mismatches":changed_inserts,
            "independent_insert_unresolved_blocks":unresolved,
            "independent_invalid_decoded_blocks":self.invalid_blocks,
            "independent_invalid_decoded_inserts":self.invalid_inserts,
            "independent_duplicate_decoded_blocks":self.duplicate_blocks,
            "independent_duplicate_decoded_inserts":self.duplicate_inserts,
            "independent_block_reference_scope":(
                "BLOCK flags/XREF source path/origin and INSERT handle/name/layer/"
                "space/layout/2D-3D position/XYZ scale/rotation only. "
                "No XREF resolution, dynamic block evaluation, or screen parity."),
        }

    @staticmethod
    def _reference_pairs(obj,allowed):
        if not isinstance(obj,dict) or len(obj)>len(allowed):
            raise ValueError("invalid original field map")
        pairs=[]
        for key,value in obj.items():
            if not isinstance(key,str) or not key.isdecimal() or str(int(key))!=key:
                raise ValueError("malformed group key")
            code=int(key)
            if code not in allowed or not isinstance(value,str):
                raise ValueError("unknown/invalid source group")
            pairs.append((code,value))
        return pairs
