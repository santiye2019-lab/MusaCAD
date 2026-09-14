from pathlib import Path

p=Path('app/src/main/java/com/musa/cad/DxfParser.java')
s=p.read_text()

def rep(old,new):
    global s
    c=s.count(old)
    if c!=1:
        raise SystemExit(f'expected one match, got {c}: {old[:140]!r}')
    s=s.replace(old,new,1)

old_label='''    private static final class Label implements Entity {
        final float x,y,height,angle; final String[] rows;
        Label(float x,float y,float h,float angle,String text){
            this.x=x;this.y=y;this.height=h;this.angle=angle;rows=text.split("\\n",-1);
        }
        private Path shape(){
            Paint p=new Paint(Paint.ANTI_ALIAS_FLAG);p.setTextSize(height);
            Path shape=new Path();
            for(int i=0;i<rows.length;i++){
                Path line=new Path();p.getTextPath(rows[i],0,rows[i].length(),0,i*height*1.3f,line);shape.addPath(line);
            }
            Matrix placement=new Matrix();placement.setScale(1,-1);placement.postRotate(angle);placement.postTranslate(x,y);
            shape.transform(placement);return shape;
        }
        public void bounds(RectF b){RectF r=new RectF();shape().computeBounds(r,true);add(b,r.left,r.top);add(b,r.right,r.bottom);}
        public void draw(Canvas c,Paint p,Matrix m){
            Path path=shape();path.transform(m);p.setStyle(Paint.Style.FILL);c.drawPath(path,p);p.setStyle(Paint.Style.STROKE);
        }
    }
'''
new_label='''    private static final class Label implements Entity {
        final float x,y,height,angle,widthFactor,oblique,boxWidth,targetWidth;
        final String text,fontFamily;final int hAlign,vAlign,attachment;
        final boolean mtext,backwards,upsideDown,aligned;
        private Path cached;
        Label(float x,float y,float h,float angle,String text){
            this(x,y,h,angle,text,1,0,"sans-serif",0,0,false,1,0,false,false,0,false);
        }
        Label(float x,float y,float h,float angle,String text,float widthFactor,float oblique,String fontFamily,
              int hAlign,int vAlign,boolean mtext,int attachment,float boxWidth,boolean backwards,boolean upsideDown,
              float targetWidth,boolean aligned){
            this.x=x;this.y=y;this.height=Math.max(.01f,h);this.angle=angle;this.text=text==null?"":text;
            this.widthFactor=Math.max(.01f,Math.abs(widthFactor));this.oblique=Float.isFinite(oblique)?oblique:0;
            this.fontFamily=fontFamily==null||fontFamily.isEmpty()?"sans-serif":fontFamily;this.hAlign=hAlign;this.vAlign=vAlign;
            this.mtext=mtext;this.attachment=attachment;this.boxWidth=Math.max(0,boxWidth);this.backwards=backwards;this.upsideDown=upsideDown;
            this.targetWidth=Math.max(0,targetWidth);this.aligned=aligned;
        }
        private ArrayList<String> rows(Paint paint){
            ArrayList<String> result=new ArrayList<>();String[] explicit=text.split("\\n",-1);
            if(!mtext||boxWidth<=0){Collections.addAll(result,explicit);return result;}
            float limit=boxWidth/Math.max(.01f,widthFactor);
            for(String source:explicit){
                if(source.isEmpty()){result.add("");continue;}
                StringBuilder row=new StringBuilder();
                for(String word:source.split(" ",-1)){
                    String candidate=row.length()==0?word:row+" "+word;
                    if(row.length()>0&&paint.measureText(candidate)>limit){result.add(row.toString());row.setLength(0);row.append(word);}
                    else{if(row.length()>0)row.append(' ');row.append(word);}
                }
                result.add(row.toString());
            }
            return result;
        }
        private Path buildShape(){
            Paint paint=new Paint(Paint.ANTI_ALIAS_FLAG);paint.setTextSize(height);paint.setTypeface(Typeface.create(fontFamily,Typeface.NORMAL));
            ArrayList<String> rows=rows(paint);Path shape=new Path();
            for(int i=0;i<rows.size();i++){
                String row=rows.get(i);if(row.isEmpty())continue;Path line=new Path();
                paint.getTextPath(row,0,row.length(),0,i*height*1.3f,line);shape.addPath(line);
            }
            if(shape.isEmpty())return shape;
            Matrix glyph=new Matrix();float sx=widthFactor*(backwards?-1f:1f),sy=upsideDown?1f:-1f;
            glyph.setScale(sx,sy);if(Math.abs(oblique)>1e-4)glyph.postSkew((float)Math.tan(Math.toRadians(oblique)),0);shape.transform(glyph);
            RectF r=new RectF();shape.computeBounds(r,true);
            if(targetWidth>0&&r.width()>1e-6){float fit=targetWidth/r.width();Matrix f=new Matrix();f.setScale(fit,aligned?fit:1f);shape.transform(f);shape.computeBounds(r,true);}
            float dx,dy;
            if(targetWidth>0){dx=-r.left;dy=0;}
            else if(mtext){
                double hx=DxfTextLayout.mtextHorizontal(attachment);int vy=DxfTextLayout.mtextVertical(attachment);
                dx=(float)-(r.left+hx*r.width());dy=vy==0?-r.bottom:vy==1?-r.centerY():-r.top;
            }else{
                double hx=DxfTextLayout.textHorizontal(hAlign);int vy=DxfTextLayout.textVertical(vAlign);
                dx=(float)-(r.left+hx*r.width());dy=vy==0?0:vy==1?-r.top:vy==2?-r.centerY():-r.bottom;
            }
            Matrix placement=new Matrix();placement.setTranslate(dx,dy);placement.postRotate(angle);placement.postTranslate(x,y);shape.transform(placement);
            return shape;
        }
        private Path shape(){if(cached==null)cached=buildShape();return new Path(cached);}
        public void bounds(RectF b){RectF r=new RectF();shape().computeBounds(r,true);if(!r.isEmpty()){add(b,r.left,r.top);add(b,r.right,r.bottom);}}
        public void draw(Canvas c,Paint p,Matrix m){
            Path path=shape();path.transform(m);Paint.Style old=p.getStyle();PathEffect effect=p.getPathEffect();
            p.setPathEffect(null);p.setStyle(Paint.Style.FILL);c.drawPath(path,p);p.setStyle(old);p.setPathEffect(effect);
        }
    }
'''
rep(old_label,new_label)

rep('''        DxfLineTypes.Table lineTypes=DxfLineTypes.parse(lines);
        Map<String,Integer> layerColors=layerTable.colors;''','''        DxfLineTypes.Table lineTypes=DxfLineTypes.parse(lines);
        DxfTextStyles.Table textStyles=DxfTextStyles.parse(lines);
        Map<String,Integer> layerColors=layerTable.colors;''')

rep('''                entity=parse(item.record.type,lines,item.record.from,item.record.to);
                if(entity==null){skipped++;continue;}''','''                entity=isTextType(item.record.type)?parseTextEntity(item.record.type,lines,item.record.from,item.record.to,textStyles):
                    parse(item.record.type,lines,item.record.from,item.record.to);
                if(entity==null){skipped++;continue;}''')

rep('''        final DxfLineTypes.Table lineTypes=new DxfLineTypes.Table();
        final ArrayList<StreamNode> roots=new ArrayList<>();''','''        final DxfLineTypes.Table lineTypes=new DxfLineTypes.Table();
        final DxfTextStyles.Table textStyles=new DxfTextStyles.Table();
        final ArrayList<StreamNode> roots=new ArrayList<>();''')

rep('''        return code==1||code==2||code==3||code==6||code==8||code==9||code==62||code==370||code==420||''','''        return code==1||code==2||code==3||code==4||code==6||code==7||code==8||code==9||code==62||code==370||code==420||''')

anchor='''        if("TABLES".equals(c.section)&&"LTYPE".equals(type)){
            ArrayList<Double> values=new ArrayList<>();
            for(int i=0;i+1<r.tags.size();i+=2)if(intOf(r.tags.get(i))==49)values.add(r.numberAt(i,0));
            c.lineTypes.add(r.text(2,DxfStyle.CONTINUOUS),values);return;
        }
        ArrayList<StreamNode> target=streamTarget(c);if(target==null)return;'''
replacement='''        if("TABLES".equals(c.section)&&"LTYPE".equals(type)){
            ArrayList<Double> values=new ArrayList<>();
            for(int i=0;i+1<r.tags.size();i+=2)if(intOf(r.tags.get(i))==49)values.add(r.numberAt(i,0));
            c.lineTypes.add(r.text(2,DxfStyle.CONTINUOUS),values);return;
        }
        if("TABLES".equals(c.section)&&"STYLE".equals(type)){
            c.textStyles.add(r.text(2,DxfTextStyles.STANDARD),r.text(3,""),r.text(4,""),r.number(40,0),r.number(41,1),r.number(50,0),
                r.integer(70,0),r.integer(71,0));return;
        }
        ArrayList<StreamNode> target=streamTarget(c);if(target==null)return;'''
rep(anchor,replacement)

rep('''        Entity entity=parse(type,r.tags,0,r.tags.size());
        if(entity==null)c.skipped++;else{
            entity=projectOcs(type,entity,r.tags,0,r.tags.size());''','''        Entity entity=isTextType(type)?parseTextEntity(type,r.tags,0,r.tags.size(),c.textStyles):parse(type,r.tags,0,r.tags.size());
        if(entity==null)c.skipped++;else{
            entity=projectOcs(type,entity,r.tags,0,r.tags.size());''')

helper_anchor='''    private static Entity projectOcs(String type,Entity entity,List<String>a,int from,int to)throws IOException{'''
helpers='''    private static boolean isTextType(String type){return "TEXT".equals(type)||"MTEXT".equals(type)||"ATTRIB".equals(type)||"ATTDEF".equals(type);}

    private static Entity parseTextEntity(String type,List<String>a,int from,int to,DxfTextStyles.Table styles){
        boolean mtext="MTEXT".equals(type);StringBuilder raw=new StringBuilder();
        for(int i=from;i+1<to;i+=2){int code=intOf(a.get(i));if(code==1||(mtext&&code==3))raw.append(a.get(i+1));}
        String plain=DxfText.plain(raw.toString());if(plain.trim().isEmpty())return null;
        DxfTextStyles.Entry style=styles.get(str(a,from,to,7,DxfTextStyles.STANDARD));
        float height=fv(a,from,to,40,0f);if(height<=0)height=(float)(style.fixedHeight>0?style.fixedHeight:1d);
        float width=(float)style.widthFactor;if(!mtext)width*=Math.max(.01f,fv(a,from,to,41,1f));
        float oblique=has(a,from,to,51)?f(a,from,to,51):(float)style.oblique;
        boolean backwards=false,upsideDown=false;int hAlign=0,vAlign=0,attachment=1;float boxWidth=0,targetWidth=0;
        float x=f(a,from,to,10),y=f(a,from,to,20),angle=0;boolean aligned=false;
        if(mtext){
            attachment=(int)fv(a,from,to,71,1f);boxWidth=Math.max(0,fv(a,from,to,41,0f));
            if(has(a,from,to,11)&&has(a,from,to,21))angle=(float)Math.toDegrees(Math.atan2(f(a,from,to,21),f(a,from,to,11)));
            else if(has(a,from,to,50))angle=(float)Math.toDegrees(f(a,from,to,50));
        }else{
            hAlign=(int)fv(a,from,to,72,0f);vAlign=(int)fv(a,from,to,73,0f);angle=fv(a,from,to,50,0f);
            int generation=has(a,from,to,71)?(int)fv(a,from,to,71,0f):style.generationFlags;
            backwards=(generation&2)!=0;upsideDown=(generation&4)!=0;
            if(DxfTextLayout.isAlignedOrFit(hAlign)&&has(a,from,to,11)&&has(a,from,to,21)){
                float x2=f(a,from,to,11),y2=f(a,from,to,21),dx=x2-x,dy=y2-y;targetWidth=(float)Math.hypot(dx,dy);
                if(targetWidth>1e-6){angle=(float)Math.toDegrees(Math.atan2(dy,dx));aligned=hAlign==3;hAlign=0;vAlign=0;}
            }else if(DxfTextLayout.usesAlignmentPoint(hAlign,vAlign)&&has(a,from,to,11)&&has(a,from,to,21)){
                x=f(a,from,to,11);y=f(a,from,to,21);
            }
        }
        return new Label(x,y,height,angle,plain,width,oblique,style.familyHint(),hAlign,vAlign,mtext,attachment,boxWidth,
            backwards,upsideDown,targetWidth,aligned);
    }

'''
if s.count(helper_anchor)!=1:raise SystemExit('projectOcs anchor mismatch')
s=s.replace(helper_anchor,helpers+helper_anchor,1)

p.write_text(s)
