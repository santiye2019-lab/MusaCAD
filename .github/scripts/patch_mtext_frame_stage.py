from pathlib import Path
p=Path('app/src/main/java/com/musa/cad/DxfParser.java');s=p.read_text()

def repl(old,new,count=1):
    global s
    n=s.count(old)
    if n!=count: raise SystemExit(f'expected {count}, got {n}: {old[:220]!r}')
    s=s.replace(old,new,count)

repl('''        final boolean mtext,backwards,upsideDown,aligned,backgroundEnabled;\n''','''        final boolean mtext,backwards,upsideDown,aligned,backgroundEnabled,backgroundFrame;\n''')
repl('''            this(x,y,h,angle,text,1,0,"sans-serif",0,0,false,1,0,false,false,0,false,false,BACKGROUND,1.5f);\n''','''            this(x,y,h,angle,text,1,0,"sans-serif",0,0,false,1,0,false,false,0,false,false,false,BACKGROUND,1.5f);\n''')
repl('''            this(x,y,h,angle,text,widthFactor,oblique,fontFamily,hAlign,vAlign,mtext,attachment,boxWidth,backwards,upsideDown,targetWidth,aligned,false,BACKGROUND,1.5f);\n''','''            this(x,y,h,angle,text,widthFactor,oblique,fontFamily,hAlign,vAlign,mtext,attachment,boxWidth,backwards,upsideDown,targetWidth,aligned,false,false,BACKGROUND,1.5f);\n''')
repl('''              float targetWidth,boolean aligned,boolean backgroundEnabled,int backgroundColor,float backgroundScale){\n''','''              float targetWidth,boolean aligned,boolean backgroundEnabled,boolean backgroundFrame,int backgroundColor,float backgroundScale){\n''')
repl('''            this.targetWidth=Math.max(0,targetWidth);this.aligned=aligned;this.backgroundEnabled=backgroundEnabled;this.backgroundColor=backgroundColor;\n''','''            this.targetWidth=Math.max(0,targetWidth);this.aligned=aligned;this.backgroundEnabled=backgroundEnabled;this.backgroundFrame=backgroundFrame;this.backgroundColor=backgroundColor;\n''')
repl('''            if(backgroundEnabled){\n                RectF box=new RectF(r);float pad=(float)DxfMTextBackground.padding(height,backgroundScale);box.inset(-pad,-pad);\n''','''            if(backgroundEnabled||backgroundFrame){\n                RectF box=new RectF(r);float pad=(float)DxfMTextBackground.padding(height,backgroundScale);box.inset(-pad,-pad);\n''')
repl('''            p.setPathEffect(null);p.setStyle(Paint.Style.FILL);Path bg=background();if(bg!=null){bg.transform(m);p.setColor(backgroundColor);c.drawPath(bg,p);p.setColor(oldColor);}\n            c.drawPath(path,p);p.setColor(oldColor);p.setStyle(old);p.setPathEffect(effect);\n''','''            p.setPathEffect(null);Path bg=background();if(bg!=null)bg.transform(m);\n            if(bg!=null&&backgroundEnabled){p.setStyle(Paint.Style.FILL);p.setColor(backgroundColor);c.drawPath(bg,p);}\n            if(bg!=null&&backgroundFrame){p.setStyle(Paint.Style.STROKE);p.setColor(oldColor);c.drawPath(bg,p);}\n            p.setStyle(Paint.Style.FILL);p.setColor(oldColor);c.drawPath(path,p);p.setColor(oldColor);p.setStyle(old);p.setPathEffect(effect);\n''')
repl('''        boolean backgroundEnabled=false;int backgroundColor=BACKGROUND;float backgroundScale=1.5f;\n''','''        boolean backgroundEnabled=false,backgroundFrame=false;int backgroundColor=BACKGROUND;float backgroundScale=1.5f;\n''')
repl('''            int backgroundFlags=(int)fv(a,from,to,90,0f);backgroundEnabled=DxfMTextBackground.enabled(backgroundFlags);\n            if(backgroundEnabled){\n''','''            int backgroundFlags=(int)fv(a,from,to,90,0f);backgroundEnabled=DxfMTextBackground.enabled(backgroundFlags);backgroundFrame=DxfMTextBackground.frame(backgroundFlags);\n            if(backgroundEnabled){\n''')
repl('''                backgroundScale=(float)DxfMTextBackground.boxScale(fv(a,from,to,45,1.5f));\n            }\n            if(has(a,from,to,11)&&has(a,from,to,21))angle=''', '''            }\n            if(backgroundEnabled||backgroundFrame)backgroundScale=(float)DxfMTextBackground.boxScale(fv(a,from,to,45,1.5f));\n            if(has(a,from,to,11)&&has(a,from,to,21))angle=''',1)
repl('''            backwards,upsideDown,targetWidth,aligned,backgroundEnabled,backgroundColor,backgroundScale);\n''','''            backwards,upsideDown,targetWidth,aligned,backgroundEnabled,backgroundFrame,backgroundColor,backgroundScale);\n''')

p.write_text(s)
