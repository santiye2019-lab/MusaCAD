from pathlib import Path

p=Path('app/src/main/java/com/musa/cad/DxfParser.java')
s=p.read_text()

def rep(old,new):
    global s
    c=s.count(old)
    if c!=1:
        raise SystemExit(f'expected one match, got {c}: {old[:100]!r}')
    s=s.replace(old,new,1)

rep('''        final String name,layer,lineType;final int aci,trueColor,lineWeight;
        final double x,y,z,sx,sy,rotation,ex,ey,ez,lineTypeScale;
        final int columns,rows;
        StreamInsert(StreamRecord r)throws IOException{
            name=key(r.text(2,""));layer=r.text(8,"0");
            aci=r.integer(62,DxfColor.BYLAYER);trueColor=r.trueColor();
            lineType=r.text(6,DxfStyle.BYLAYER);lineWeight=r.integer(370,DxfStyle.LW_BYLAYER);lineTypeScale=DxfStyle.saneScale(r.number(48,1));
            x=r.number(10,0);y=r.number(20,0);z=r.number(30,0);
            sx=r.number(41,1);sy=r.number(42,1);rotation=r.number(50,0);
            columns=(int)r.number(70,1);rows=(int)r.number(71,1);
            ex=r.number(210,0);ey=r.number(220,0);ez=r.number(230,1);
        }''','''        final String name,layer,lineType;final int aci,trueColor,lineWeight;
        final double x,y,z,sx,sy,sz,rotation,ex,ey,ez,lineTypeScale,columnSpacing,rowSpacing;
        final int columns,rows;
        StreamInsert(StreamRecord r)throws IOException{
            name=key(r.text(2,""));layer=r.text(8,"0");
            aci=r.integer(62,DxfColor.BYLAYER);trueColor=r.trueColor();
            lineType=r.text(6,DxfStyle.BYLAYER);lineWeight=r.integer(370,DxfStyle.LW_BYLAYER);lineTypeScale=DxfStyle.saneScale(r.number(48,1));
            x=r.number(10,0);y=r.number(20,0);z=r.number(30,0);
            sx=r.number(41,1);sy=r.number(42,1);sz=r.number(43,1);rotation=r.number(50,0);
            columns=(int)r.number(70,1);rows=(int)r.number(71,1);
            columnSpacing=r.number(44,0);rowSpacing=r.number(45,0);
            ex=r.number(210,0);ey=r.number(220,0);ez=r.number(230,1);
        }''')

rep('''                if(block==null||stack.contains(dimension.name)||stack.size()>=32||block.bz!=0||(block.flags&12)!=0||!block.xref.isEmpty()){''','''                if(block==null||stack.contains(dimension.name)||stack.size()>=32||(block.flags&12)!=0||!block.xref.isEmpty()){''')

rep('''            StreamInsert insert=(StreamInsert)node;String layer="0".equals(insert.layer)?parentLayer:insert.layer;
            StreamBlock block=blocks.get(insert.name);
            if(block==null||stack.contains(insert.name)||stack.size()>=32||insert.columns!=1||insert.rows!=1||insert.z!=0||
                insert.ex!=0||insert.ey!=0||insert.ez!=1||block.bz!=0||(block.flags&12)!=0||!block.xref.isEmpty()||insert.sx==0||insert.sy==0){
                context.skipped++;continue;
            }
            DxfColor.Ref insertColor=DxfColor.resolve(insert.aci,insert.trueColor,layer,byBlockColor);
            String insertType=DxfStyle.resolveLineType(insert.lineType,byBlockLineType);
            int insertWeight=DxfStyle.resolveLineWeight(insert.lineWeight,byBlockLineWeight);
            DxfBlocks.Transform local=DxfBlocks.Transform.insert(block.bx,block.by,insert.sx,insert.sy,insert.rotation,insert.x,insert.y);
            DxfBlocks.Transform transform=parent.thenLocal(local);stack.add(insert.name);
            expandStream(block.members,transform,layer,insertColor,insertType,insertWeight,blocks,stack,entities,layers,visibleLayers,counter,context);
            stack.remove(insert.name);''','''            StreamInsert insert=(StreamInsert)node;String layer="0".equals(insert.layer)?parentLayer:insert.layer;
            StreamBlock block=blocks.get(insert.name);
            if(block==null||stack.contains(insert.name)||stack.size()>=32||(block.flags&12)!=0||!block.xref.isEmpty()||
                insert.sx==0||insert.sy==0||insert.sz==0||insert.columns<1||insert.rows<1||insert.columns>1000||insert.rows>1000||
                (long)insert.columns*insert.rows>10000L){
                context.skipped++;continue;
            }
            DxfColor.Ref insertColor=DxfColor.resolve(insert.aci,insert.trueColor,layer,byBlockColor);
            String insertType=DxfStyle.resolveLineType(insert.lineType,byBlockLineType);
            int insertWeight=DxfStyle.resolveLineWeight(insert.lineWeight,byBlockLineWeight);
            stack.add(insert.name);
            try{
                for(int row=0;row<insert.rows;row++)for(int column=0;column<insert.columns;column++){
                    final DxfBlocks.Transform local;
                    try{
                        local=DxfBlocks.Transform.insert(block.bx,block.by,block.bz,insert.sx,insert.sy,insert.sz,insert.rotation,
                            insert.x,insert.y,insert.z,insert.ex,insert.ey,insert.ez,column*insert.columnSpacing,row*insert.rowSpacing);
                    }catch(IllegalArgumentException invalid){context.skipped++;continue;}
                    DxfBlocks.Transform transform=parent.thenLocal(local);
                    expandStream(block.members,transform,layer,insertColor,insertType,insertWeight,blocks,stack,entities,layers,visibleLayers,counter,context);
                }
            }finally{stack.remove(insert.name);}''')

p.write_text(s)
