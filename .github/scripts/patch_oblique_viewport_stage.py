from pathlib import Path
p=Path('app/src/main/java/com/musa/cad/DxfParser.java');s=p.read_text()

def repl(old,new,count=1):
    global s
    n=s.count(old)
    if n!=count: raise SystemExit(f'expected {count}, got {n}: {old[:180]!r}')
    s=s.replace(old,new,count)

repl('''        return DxfViewport.of(f(a,from,to,10),f(a,from,to,20),fv(a,from,to,40,0f),fv(a,from,to,41,0f),
            f(a,from,to,12),f(a,from,to,22),fv(a,from,to,45,0f),fv(a,from,to,51,0f),
            (int)fv(a,from,to,69,0f),(int)fv(a,from,to,68,1f),
            fv(a,from,to,16,0f),fv(a,from,to,26,0f),fv(a,from,to,36,1f));
''','''        return DxfViewport.of(f(a,from,to,10),f(a,from,to,20),fv(a,from,to,40,0f),fv(a,from,to,41,0f),
            f(a,from,to,12),f(a,from,to,22),fv(a,from,to,45,0f),fv(a,from,to,51,0f),
            (int)fv(a,from,to,69,0f),(int)fv(a,from,to,68,1f),
            fv(a,from,to,16,0f),fv(a,from,to,26,0f),fv(a,from,to,36,1f),
            fv(a,from,to,17,0f),fv(a,from,to,27,0f),fv(a,from,to,37,0f));
''')

repl('''            spec=DxfViewport.of(r.number(10,0),r.number(20,0),r.number(40,0),r.number(41,0),
                r.number(12,0),r.number(22,0),r.number(45,0),r.number(51,0),r.integer(69,0),r.integer(68,1),
                r.number(16,0),r.number(26,0),r.number(36,1));
''','''            spec=DxfViewport.of(r.number(10,0),r.number(20,0),r.number(40,0),r.number(41,0),
                r.number(12,0),r.number(22,0),r.number(45,0),r.number(51,0),r.integer(69,0),r.integer(68,1),
                r.number(16,0),r.number(26,0),r.number(36,1),r.number(17,0),r.number(27,0),r.number(37,0));
''')

p.write_text(s)
