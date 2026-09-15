from pathlib import Path
p=Path('app/src/main/java/com/musa/cad/DxfParser.java');s=p.read_text()

def repl(old,new,count=1):
    global s
    n=s.count(old)
    if n!=count: raise SystemExit(f'expected {count}, got {n}: {old[:220]!r}')
    s=s.replace(old,new,count)

# Large-file streaming must retain the nested MULTILEADER structural markers.
repl('''            code==330||code==331||code==340||code==370||code==410||code==420||code==440||
            (code>=10&&code<=59)||(code>=70&&code<=79)||(code>=90&&code<=99)||
            (code>=210&&code<=213)||(code>=220&&code<=223)||(code>=230&&code<=233);
''','''            code==330||code==331||code==340||code==370||code==410||code==420||code==440||
            (code>=10&&code<=59)||(code>=70&&code<=79)||(code>=90&&code<=99)||(code>=300&&code<=305)||
            (code>=210&&code<=213)||(code>=220&&code<=223)||(code>=230&&code<=233);
''')

repl('''    private static Entity parse(String type,List<String>a,int from,int to,DxfPointStyle.Style pointStyle,DxfDimStyles.Table dimStyles){
        if("POINT".equals(type))return new Marker(f(a,from,to,10),f(a,from,to,20),pointStyle);
        if("LEADER".equals(type))return leaderEntity(a,from,to,dimStyles);
        return parse(type,a,from,to);
    }
''','''    private static Entity parse(String type,List<String>a,int from,int to,DxfPointStyle.Style pointStyle,DxfDimStyles.Table dimStyles){
        if("POINT".equals(type))return new Marker(f(a,from,to,10),f(a,from,to,20),pointStyle);
        if("LEADER".equals(type))return leaderEntity(a,from,to,dimStyles);
        if("MULTILEADER".equals(type))return mLeaderEntity(a,from,to);
        return parse(type,a,from,to);
    }

    private static Entity mLeaderEntity(List<String>a,int from,int to){
        DxfMLeader.Data data=DxfMLeader.parse(a.subList(Math.max(0,from),Math.min(a.size(),to)));ArrayList<Entity> items=new ArrayList<>();
        String text=DxfText.plain(data.text);if(!text.trim().isEmpty()&&Double.isFinite(data.textX)&&Double.isFinite(data.textY)){
            float height=(float)(Double.isFinite(data.textHeight)&&data.textHeight>1e-9?data.textHeight:1d);
            items.add(new Label((float)data.textX,(float)data.textY,height,0,text));
        }
        for(DxfMLeader.Leader leader:data.leaders){
            ArrayList<PointF> points=packedPoints(leader.points);if(points.size()<2)continue;ArrayList<PointF> arrow=new ArrayList<>();
            PointF tip=points.get(points.size()-1),next=points.get(points.size()-2);double adjacent=Math.hypot(next.x-tip.x,next.y-tip.y);
            if(adjacent>1e-9)arrow=packedPoints(DxfLeader.arrow(tip.x,tip.y,next.x,next.y,leader.arrowSize));
            items.add(new LeaderEntity(points,arrow));
        }
        if(items.isEmpty())return null;return items.size()==1?items.get(0):new Composite(items);
    }
''')

p.write_text(s)
