from pathlib import Path

p=Path('app/src/main/java/com/musa/cad/DxfParser.java')
s=p.read_text()

anchor='''    private static final class Composite implements Entity {'''
if s.count(anchor)!=1:
    raise SystemExit('Composite anchor mismatch')
if 'private static final class Hatch implements Entity' in s:
    raise SystemExit('HATCH renderer already present')

classes=r'''    private static final class HatchLoop {
        final int flags;final ArrayList<PointF> points;
        HatchLoop(int flags,ArrayList<PointF> points){this.flags=flags;this.points=points;}
    }

    /** Filled/patterned HATCH clipped to its independent boundary loops. */
    private static final class Hatch implements Entity {
        final ArrayList<HatchLoop> loops;final boolean solid;final int style;
        final ArrayList<DxfHatchPattern.Line> pattern;
        Hatch(ArrayList<HatchLoop> loops,boolean solid,int style,ArrayList<DxfHatchPattern.Line> pattern){
            this.loops=loops;this.solid=solid;this.style=style;this.pattern=pattern;
        }
        private boolean preferred(HatchLoop loop){
            if(style==2)return (loop.flags&1)!=0;              // IGNORE: external boundary only.
            if(style==1)return (loop.flags&(1|16))!=0;       // OUTER: external + outermost islands.
            return true;                                     // NORMAL: even/odd all loops.
        }
        private int preferredCount(){int n=0;for(HatchLoop l:loops)if(preferred(l))n++;return n;}
        private Path boundary(Matrix m){
            boolean fallback=preferredCount()==0;Path path=new Path();path.setFillType(Path.FillType.EVEN_ODD);
            for(HatchLoop loop:loops){
                if(!fallback&&!preferred(loop))continue;if(loop.points.size()<2)continue;
                PointF first=loop.points.get(0);path.moveTo(first.x,first.y);
                for(int i=1;i<loop.points.size();i++){PointF q=loop.points.get(i);path.lineTo(q.x,q.y);}path.close();
            }
            path.transform(m);return path;
        }
        private RectF localBounds(){
            boolean fallback=preferredCount()==0;RectF b=new RectF(Float.MAX_VALUE,Float.MAX_VALUE,-Float.MAX_VALUE,-Float.MAX_VALUE);
            for(HatchLoop loop:loops){if(!fallback&&!preferred(loop))continue;for(PointF q:loop.points)add(b,q.x,q.y);}
            return b;
        }
        public void bounds(RectF b){for(HatchLoop loop:loops)for(PointF q:loop.points)add(b,q.x,q.y);}
        public void draw(Canvas c,Paint p,Matrix m){
            if(loops.isEmpty())return;Path clip=boundary(m);
            Paint.Style oldStyle=p.getStyle();PathEffect oldEffect=p.getPathEffect();
            if(solid){
                p.setPathEffect(null);p.setStyle(Paint.Style.FILL);c.drawPath(clip,p);
                p.setStyle(oldStyle);p.setPathEffect(oldEffect);return;
            }
            if(pattern.isEmpty()){
                p.setPathEffect(null);p.setStyle(Paint.Style.STROKE);c.drawPath(clip,p);
                p.setStyle(oldStyle);p.setPathEffect(oldEffect);return;
            }
            RectF box=localBounds();if(box.left>box.right||box.top>box.bottom)return;
            int save=c.save();c.clipPath(clip);p.setPathEffect(null);p.setStyle(Paint.Style.STROKE);
            int segments=0;final int segmentLimit=25000;
            outer:for(DxfHatchPattern.Line line:pattern){
                double dx=line.dx(),dy=line.dy();if(!Double.isFinite(dx)||!Double.isFinite(dy))continue;
                int[] range=DxfHatchPattern.familyRange(line,box.left,box.top,box.right,box.bottom);
                long families=(long)range[1]-range[0]+1L;if(families<=0)continue;
                float[] vectors={(float)dx,(float)dy,(float)line.offsetX,(float)line.offsetY};m.mapVectors(vectors);
                double dirPx=Math.hypot(vectors[0],vectors[1]);
                double spacingPx=dirPx<1e-9?0:Math.abs(vectors[0]*vectors[3]-vectors[1]*vectors[2])/dirPx;
                int step=1;if(spacingPx>0&&spacingPx<1.2)step=Math.max(step,(int)Math.ceil(1.2/spacingPx));
                if(families/step>4000L)step=Math.max(step,(int)Math.ceil(families/4000d));
                double minT=Double.POSITIVE_INFINITY,maxT=Double.NEGATIVE_INFINITY;
                double[] cx={box.left,box.right,box.right,box.left},cy={box.top,box.top,box.bottom,box.bottom};
                for(int q=0;q<4;q++){double t=cx[q]*dx+cy[q]*dy;minT=Math.min(minT,t);maxT=Math.max(maxT,t);}
                Path lines=new Path();double cycle=DxfHatchPattern.dashCycle(line);
                for(long kk=range[0];kk<=range[1];kk+=step){
                    double px=line.baseX+kk*line.offsetX,py=line.baseY+kk*line.offsetY;
                    double baseT=px*dx+py*dy,t0=minT-baseT-1,t1=maxT-baseT+1;
                    if(line.dashes.length==0||cycle<1e-12){
                        lines.moveTo((float)(px+dx*t0),(float)(py+dy*t0));lines.lineTo((float)(px+dx*t1),(float)(py+dy*t1));
                        if(++segments>=segmentLimit)break outer;continue;
                    }
                    double cycleStart=Math.floor(t0/cycle)*cycle;
                    for(double cs=cycleStart;cs<=t1+cycle;cs+=cycle){
                        double pos=cs;
                        for(double raw:line.dashes){
                            double len=Math.abs(raw);
                            if(len<1e-12){
                                if(pos>=t0&&pos<=t1){double dot=dirPx>1e-9?.8/dirPx:.01;
                                    lines.moveTo((float)(px+dx*(pos-dot*.5)),(float)(py+dy*(pos-dot*.5)));
                                    lines.lineTo((float)(px+dx*(pos+dot*.5)),(float)(py+dy*(pos+dot*.5)));
                                    if(++segments>=segmentLimit)break outer;}
                            }else if(raw>0){
                                double a=Math.max(pos,t0),b=Math.min(pos+len,t1);
                                if(b>a){lines.moveTo((float)(px+dx*a),(float)(py+dy*a));lines.lineTo((float)(px+dx*b),(float)(py+dy*b));
                                    if(++segments>=segmentLimit)break outer;}
                            }
                            pos+=len;
                        }
                    }
                }
                lines.transform(m);c.drawPath(lines,p);
            }
            c.restoreToCount(save);p.setStyle(oldStyle);p.setPathEffect(oldEffect);
        }
    }

'''
s=s.replace(anchor,classes+anchor,1)

start=s.index('    private static Entity parseHatch(')
end=s.index('    private static Entity parseWipeout(',start)
new_parse=r'''    /** Parse independent HATCH loops, then render SOLID fill or explicit DXF pattern definitions. */
    private static Entity parseHatch(List<String>a,int from,int to){
        int pathCount=(int)fv(a,from,to,91,0f);if(pathCount<=0)return null;
        int i=from;ArrayList<HatchLoop> loops=new ArrayList<>();
        for(int pathIndex=0;pathIndex<pathCount;pathIndex++){
            while(i+1<to&&intOf(a.get(i))!=92)i+=2;
            if(i+1>=to)break;
            int flags=(int)floatOf(a.get(i+1));i+=2;
            if((flags&2)!=0){
                boolean closed=true;int vertexCount=-1;
                while(i+1<to){int code=intOf(a.get(i));if(code==73)closed=((int)floatOf(a.get(i+1)))!=0;
                    if(code==93){vertexCount=(int)floatOf(a.get(i+1));i+=2;break;}if(code==92)break;i+=2;}
                ArrayList<PointF> points=new ArrayList<>();ArrayList<Double> bulges=new ArrayList<>();
                for(int v=0;v<vertexCount&&i+1<to;v++){
                    while(i+1<to&&intOf(a.get(i))!=10){int code=intOf(a.get(i));if(code==92||code==97)break;i+=2;}
                    if(i+1>=to||intOf(a.get(i))!=10)break;float x=floatOf(a.get(i+1));i+=2;
                    while(i+1<to&&intOf(a.get(i))!=20){int code=intOf(a.get(i));if(code==92||code==97||code==10)break;i+=2;}
                    if(i+1>=to||intOf(a.get(i))!=20)break;float y=floatOf(a.get(i+1));i+=2;double bulge=0d;
                    if(i+1<to&&intOf(a.get(i))==42){bulge=floatOf(a.get(i+1));i+=2;}
                    if(Float.isFinite(x)&&Float.isFinite(y)){points.add(new PointF(x,y));bulges.add(bulge);}
                }
                if(points.size()>=2){Poly sampled=(Poly)bulgedPoly(points,bulges,closed);loops.add(new HatchLoop(flags,new ArrayList<>(sampled.pts)));}
            }else{
                int edgeCount=-1;
                while(i+1<to){int code=intOf(a.get(i));if(code==93){edgeCount=(int)floatOf(a.get(i+1));i+=2;break;}if(code==92)break;i+=2;}
                ArrayList<PointF> loop=new ArrayList<>();
                for(int edge=0;edge<edgeCount&&i+1<to;edge++){
                    while(i+1<to&&intOf(a.get(i))!=72){int code=intOf(a.get(i));if(code==92||code==97)break;i+=2;}
                    if(i+1>=to||intOf(a.get(i))!=72)break;int edgeType=(int)floatOf(a.get(i+1));int edgeFrom=i+2;i=edgeFrom;
                    while(i+1<to){int code=intOf(a.get(i));if(code==72||code==92||code==97)break;i+=2;}
                    appendHatchEdge(loop,hatchEdgePoints(edgeType,a,edgeFrom,i));
                }
                if(loop.size()>=2)loops.add(new HatchLoop(flags,loop));
            }
        }
        if(loops.isEmpty())return null;
        boolean solid=((int)fv(a,from,to,70,0f))!=0;int style=(int)fv(a,from,to,75,0f);
        ArrayList<DxfHatchPattern.Line> pattern=solid?new ArrayList<>():parseHatchPatternLines(a,i,to);
        return new Hatch(loops,solid,style,pattern);
    }

    private static void appendHatchEdge(ArrayList<PointF> loop,ArrayList<PointF> edge){
        if(edge==null||edge.isEmpty())return;if(loop.isEmpty()){loop.addAll(edge);return;}
        PointF last=loop.get(loop.size()-1),first=edge.get(0);int start=Math.hypot(last.x-first.x,last.y-first.y)<1e-4?1:0;
        for(int i=start;i<edge.size();i++)loop.add(edge.get(i));
    }

    private static ArrayList<PointF> hatchEdgePoints(int type,List<String>a,int from,int to){
        ArrayList<PointF> result=new ArrayList<>();
        if(type==1&&has(a,from,to,10)&&has(a,from,to,20)&&has(a,from,to,11)&&has(a,from,to,21)){
            result.add(new PointF(f(a,from,to,10),f(a,from,to,20)));result.add(new PointF(f(a,from,to,11),f(a,from,to,21)));return result;
        }
        if(type==2&&has(a,from,to,10)&&has(a,from,to,20)&&has(a,from,to,40)){
            double start=f(a,from,to,50),end=f(a,from,to,51);boolean ccw=((int)fv(a,from,to,73,1f))!=0;
            double sweep=end-start;if(ccw){while(sweep<=0)sweep+=360;}else{while(sweep>=0)sweep-=360;}
            int n=Math.max(8,Math.min(128,(int)Math.ceil(Math.abs(sweep)/5d)));double cx=f(a,from,to,10),cy=f(a,from,to,20),r=Math.abs(f(a,from,to,40));
            for(int k=0;k<=n;k++){double q=Math.toRadians(start+sweep*k/n);result.add(new PointF((float)(cx+r*Math.cos(q)),(float)(cy+r*Math.sin(q))));}return result;
        }
        if(type==3&&has(a,from,to,10)&&has(a,from,to,20)&&has(a,from,to,11)&&has(a,from,to,21)){
            double start=f(a,from,to,50),end=f(a,from,to,51);boolean ccw=((int)fv(a,from,to,73,1f))!=0;
            double sweep=end-start;if(ccw){while(sweep<=0)sweep+=Math.PI*2;}else{while(sweep>=0)sweep-=Math.PI*2;}
            int n=Math.max(12,Math.min(128,(int)Math.ceil(Math.abs(sweep)*24/Math.PI)));
            double cx=f(a,from,to,10),cy=f(a,from,to,20),mx=f(a,from,to,11),my=f(a,from,to,21),ratio=Math.abs(fv(a,from,to,40,1f));
            for(int k=0;k<=n;k++){double q=start+sweep*k/n,co=Math.cos(q),si=Math.sin(q);result.add(new PointF((float)(cx+mx*co-my*ratio*si),(float)(cy+my*co+mx*ratio*si)));}return result;
        }
        if(type==4){
            int degree=(int)fv(a,from,to,94,3f);ArrayList<PointF> controls=repeatedPoints(a,from,to,10,20);
            if(controls.size()<2)return result;double[] xs=new double[controls.size()],ys=new double[controls.size()];
            for(int k=0;k<controls.size();k++){xs[k]=controls.get(k).x;ys[k]=controls.get(k).y;}
            double[] knots=repeatedValues(a,from,to,40),weights=repeatedValues(a,from,to,42);if(weights.length!=controls.size())weights=null;
            return packedPoints(DxfCurves.sampleNurbs(degree,knots,weights,xs,ys));
        }
        return result;
    }

    private static ArrayList<DxfHatchPattern.Line> parseHatchPatternLines(List<String>a,int from,int to){
        ArrayList<DxfHatchPattern.Line> lines=new ArrayList<>();int marker=-1,count=0;
        for(int i=Math.max(0,from);i+1<to;i+=2)if(intOf(a.get(i))==78){marker=i;count=(int)floatOf(a.get(i+1));break;}
        if(marker<0||count<=0)return lines;int i=marker+2;
        for(int n=0;n<count;n++){
            while(i+1<to&&intOf(a.get(i))!=53){if(intOf(a.get(i))==98)return lines;i+=2;}
            if(i+1>=to)break;double angle=floatOf(a.get(i+1));int lineFrom=i+2;i=lineFrom;
            while(i+1<to&&intOf(a.get(i))!=53&&intOf(a.get(i))!=98)i+=2;int lineTo=i;
            double bx=fv(a,lineFrom,lineTo,43,0f),by=fv(a,lineFrom,lineTo,44,0f),ox=fv(a,lineFrom,lineTo,45,0f),oy=fv(a,lineFrom,lineTo,46,0f);
            int dashCount=(int)fv(a,lineFrom,lineTo,79,0f);double[] raw=repeatedValues(a,lineFrom,lineTo,49);
            if(dashCount>=0&&raw.length>dashCount)raw=Arrays.copyOf(raw,dashCount);
            if(Double.isFinite(angle)&&Double.isFinite(bx)&&Double.isFinite(by)&&Double.isFinite(ox)&&Double.isFinite(oy))
                lines.add(new DxfHatchPattern.Line(angle,bx,by,ox,oy,raw));
        }
        return lines;
    }

'''
s=s[:start]+new_parse+s[end:]
p.write_text(s)
