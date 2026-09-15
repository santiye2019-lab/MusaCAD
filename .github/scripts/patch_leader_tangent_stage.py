from pathlib import Path
p=Path('app/src/main/java/com/musa/cad/DxfParser.java');s=p.read_text()
old='''        boolean spline=((int)fv(a,from,to,72,0f))==1;ArrayList<PointF> points=packedPoints(DxfLeader.path(xs,ys,spline));if(points.size()<2)points=raw;\n'''
new='''        boolean spline=((int)fv(a,from,to,72,0f))==1;double endTx=Double.NaN,endTy=Double.NaN;\n        if(spline&&has(a,from,to,211)&&has(a,from,to,221)){endTx=f(a,from,to,211);endTy=f(a,from,to,221);if(((int)fv(a,from,to,74,0f))==0){endTx=-endTx;endTy=-endTy;}}\n        ArrayList<PointF> points=packedPoints(DxfLeader.path(xs,ys,spline,endTx,endTy));if(points.size()<2)points=raw;\n'''
if s.count(old)!=1: raise SystemExit(f'leader path marker count {s.count(old)}')
s=s.replace(old,new,1)
old_keep='''            code==330||code==331||code==340||code==370||code==410||code==420||code==440||\n            (code>=10&&code<=59)||(code>=70&&code<=79)||(code>=90&&code<=99)||code==210||code==220||code==230;\n'''
new_keep='''            code==330||code==331||code==340||code==370||code==410||code==420||code==440||\n            (code>=10&&code<=59)||(code>=70&&code<=79)||(code>=90&&code<=99)||\n            (code>=210&&code<=213)||(code>=220&&code<=223)||(code>=230&&code<=233);\n'''
if s.count(old_keep)!=1: raise SystemExit(f'keepStreamCode marker count {s.count(old_keep)}')
s=s.replace(old_keep,new_keep,1)
p.write_text(s)
