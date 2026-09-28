package com.musa.cad;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collections;
import java.util.HashMap;
import java.util.HashSet;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Locale;
import java.util.Map;
import java.util.Set;

/**
 * Lightweight ACIS SAT B-Rep reader for MusaCAD 3D.
 *
 * It intentionally focuses on display tessellation instead of editing ACIS:
 * - topology: face / loop / coedge / edge / vertex / point
 * - curves: straight-curve and ellipse-curve
 * - surfaces: plane-surface and common cone-surface (cylinder/cone side loft)
 *
 * Unsupported analytic/spline faces keep their real ACIS boundary edges and are
 * reported as incomplete. This is safer than inventing a filled surface.
 */
public final class AcisSatMesh {
    private static final int MAX_NODES=1_000_000;
    private static final int MAX_POINTS=2_000_000;
    private static final int MAX_TRIANGLES=4_000_000;
    private static final int ELLIPSE_STEPS=36;
    private static final double EPS=1e-7;

    public static final class Result {
        public final float[] xyz;
        public final int[] triangles;
        public final int[] edges;
        public final boolean complete;
        public final int faces;
        public final int tessellatedFaces;
        public final int unsupportedFaces;

        Result(float[] xyz,int[] triangles,int[] edges,boolean complete,int faces,int tessellatedFaces,int unsupportedFaces){
            this.xyz=xyz;this.triangles=triangles;this.edges=edges;this.complete=complete;
            this.faces=faces;this.tessellatedFaces=tessellatedFaces;this.unsupportedFaces=unsupportedFaces;
        }
        public boolean hasGeometry(){return xyz.length>=3&&(triangles.length>=3||edges.length>=2);}
    }

    private static final Result EMPTY=new Result(new float[0],new int[0],new int[0],false,0,0,0);

    /** Parses the group 1 / group 3 ACIS payload carried by DXF 3DSOLID/BODY/REGION. */
    public static Result parseDxfChunks(List<String> chunks){
        if(chunks==null||chunks.isEmpty())return EMPTY;
        StringBuilder raw=new StringBuilder();
        for(String chunk:chunks){
            if(chunk==null||chunk.isEmpty())continue;
            if(raw.length()>0)raw.append(' ');
            raw.append(chunk);
        }
        if(raw.length()==0)return EMPTY;
        String joined=raw.toString().replace("^ ","^");
        String decoded=decodeAutocadSat(joined);
        String sat=score(decoded)>score(joined)?decoded:joined;
        try{return parseSat(sat);}catch(RuntimeException ignored){return EMPTY;}
    }

    /** AutoCAD's legacy DXF SAT encryption is symmetric: printable byte -> 159-byte. */
    static String decodeAutocadSat(String value){
        StringBuilder out=new StringBuilder(value.length());
        for(int i=0;i<value.length();i++){
            char c=value.charAt(i);
            if(c<=32||c>=159)out.append(c);else out.append((char)(159-c));
        }
        return out.toString();
    }

    private static int score(String value){
        String lower=value.toLowerCase(Locale.ROOT);
        int score=0;
        String[] needles={"body ","face ","loop ","coedge ","edge ","vertex ","point ",
            "plane-surface","cone-surface","sphere-surface","torus-surface","end-of-acis"};
        for(String needle:needles)if(lower.contains(needle))score+=needle.contains("surface")?3:1;
        return score;
    }

    static Result parseSat(String sat){
        if(sat==null||sat.trim().isEmpty())return EMPTY;
        Map<Integer,Node> nodes=parseNodes(sat);
        if(nodes.isEmpty())return EMPTY;
        Transform3 transform=findTransform(nodes);
        MeshBuilder mesh=new MeshBuilder(transform);
        List<Node> faces=nodesByType(nodes,"face");
        int tessellated=0,unsupported=0;
        for(Node face:faces){
            List<Node> loops=loopsForFace(nodes,face.id);
            List<LoopPath> paths=new ArrayList<>();
            for(Node loop:loops){
                LoopPath path=buildLoop(nodes,loop);
                if(path.points.size()>=2){
                    paths.add(path);
                    mesh.addClosedEdges(path.points);
                }
            }
            Node surface=firstRefBySuffix(face,nodes,"-surface");
            boolean ok=false;
            if(surface!=null&&paths.size()==1&&paths.get(0).points.size()>=3&&"plane-surface".equals(surface.type)){
                ok=mesh.triangulatePlanar(paths.get(0).points);
            }else if(surface!=null&&paths.size()==2&&"cone-surface".equals(surface.type)){
                ok=mesh.loft(paths.get(0).points,paths.get(1).points);
            }
            if(ok)tessellated++;else unsupported++;
        }

        // Wire bodies / partially supported topology can have ACIS edges without faces.
        if(faces.isEmpty()){
            Set<Integer> seen=new HashSet<>();
            for(Node edge:nodesByType(nodes,"edge")){
                EdgePath p=sampleEdge(nodes,edge,false);
                if(p.points.size()>=2&&seen.add(edge.id))mesh.addOpenEdges(p.points);
            }
        }
        if(mesh.pointCount()==0)return EMPTY;
        boolean complete=!faces.isEmpty()&&tessellated==faces.size();
        return new Result(mesh.xyz(),mesh.triangles(),mesh.edges(),complete,faces.size(),tessellated,unsupported);
    }

    private static Map<Integer,Node> parseNodes(String sat){
        String[] records=sat.split("#");
        Map<Integer,Node> out=new LinkedHashMap<>();
        int implicit=0;
        boolean first=true;
        for(String record:records){
            String text=record.trim();
            if(text.isEmpty())continue;
            if(text.toLowerCase(Locale.ROOT).startsWith("end-of-acis"))break;
            if(first){text=firstRecord(text);first=false;if(text.isEmpty())continue;}
            String[] tokens=text.split("\\s+");
            if(tokens.length==0)continue;
            int at=0,id=implicit;
            if(tokens[0].matches("-\\d+")){
                try{id=-Integer.parseInt(tokens[0]);at=1;}catch(NumberFormatException ignored){continue;}
            }
            if(at>=tokens.length)continue;
            String type=tokens[at].toLowerCase(Locale.ROOT);
            if(!looksLikeType(type))continue;
            if(out.size()>=MAX_NODES)throw new IllegalArgumentException("ACIS node limit");
            String[] body=Arrays.copyOfRange(tokens,at+1,tokens.length);
            out.put(id,new Node(id,type,body));
            implicit++;
        }
        return out;
    }

    private static String firstRecord(String text){
        String lower=text.toLowerCase(Locale.ROOT);
        String[] starts={"body ","asmheader ","-0 body ","-0 asmheader ","point ","-0 point "};
        int best=-1;
        for(String needle:starts){
            int p=lower.indexOf(needle);
            if(p>=0&&(best<0||p<best))best=p;
        }
        if(best>=0)return text.substring(best).trim();
        int line=text.lastIndexOf('\n');
        return line>=0?text.substring(line+1).trim():text;
    }

    private static boolean looksLikeType(String value){
        if(value.isEmpty())return false;
        char c=value.charAt(0);
        return Character.isLetter(c)||c=='_';
    }

    private static final class Node {
        final int id;final String type;final String[] tokens;final int[] refs;
        Node(int id,String type,String[] tokens){
            this.id=id;this.type=type;this.tokens=tokens;
            Ints r=new Ints();
            for(String token:tokens)if(token.startsWith("$")&&token.length()>1){
                try{int v=Integer.parseInt(token.substring(1));if(v>=0)r.add(v);}catch(NumberFormatException ignored){}
            }
            refs=r.toArray();
        }
        boolean hasWord(String word){
            for(String token:tokens)if(word.equalsIgnoreCase(token))return true;
            return false;
        }
        double[] numbers(){
            Doubles d=new Doubles();
            for(String token:tokens){
                if(token.startsWith("$"))continue;
                try{double v=Double.parseDouble(token);if(Double.isFinite(v))d.add(v);}catch(NumberFormatException ignored){}
            }
            return d.toArray();
        }
    }

    private static List<Node> nodesByType(Map<Integer,Node> nodes,String type){
        List<Node> out=new ArrayList<>();
        for(Node n:nodes.values())if(type.equals(n.type))out.add(n);
        return out;
    }

    private static List<Node> loopsForFace(Map<Integer,Node> nodes,int faceId){
        List<Node> out=new ArrayList<>();
        for(Node n:nodes.values())if("loop".equals(n.type)&&contains(n.refs,faceId))out.add(n);
        if(!out.isEmpty())return out;
        Node face=nodes.get(faceId);
        if(face!=null)for(int ref:face.refs){Node n=nodes.get(ref);if(n!=null&&"loop".equals(n.type))out.add(n);}
        return out;
    }

    private static Node firstRefByType(Node node,Map<Integer,Node> nodes,String type){
        for(int ref:node.refs){Node n=nodes.get(ref);if(n!=null&&type.equals(n.type))return n;}
        return null;
    }

    private static Node firstRefBySuffix(Node node,Map<Integer,Node> nodes,String suffix){
        for(int ref:node.refs){Node n=nodes.get(ref);if(n!=null&&n.type.endsWith(suffix))return n;}
        return null;
    }

    private static List<Node> refsByType(Node node,Map<Integer,Node> nodes,String type){
        List<Node> out=new ArrayList<>();
        for(int ref:node.refs){Node n=nodes.get(ref);if(n!=null&&type.equals(n.type))out.add(n);}
        return out;
    }

    private static boolean contains(int[] values,int value){for(int v:values)if(v==value)return true;return false;}

    private static LoopPath buildLoop(Map<Integer,Node> nodes,Node loop){
        Node start=firstRefByType(loop,nodes,"coedge");
        if(start==null){
            for(Node n:nodes.values())if("coedge".equals(n.type)&&contains(n.refs,loop.id)){start=n;break;}
        }
        if(start==null)return new LoopPath(Collections.emptyList(),false);
        List<Vec> points=new ArrayList<>();
        Set<Integer> seen=new HashSet<>();
        Node current=start;boolean exact=true;
        while(current!=null&&seen.add(current.id)&&seen.size()<100000){
            Node edge=firstRefByType(current,nodes,"edge");
            if(edge==null){exact=false;break;}
            boolean reversed=current.hasWord("reversed");
            EdgePath path=sampleEdge(nodes,edge,reversed);
            exact&=path.exact;
            appendPath(points,path.points);
            Node next=null;
            for(int ref:current.refs){
                Node candidate=nodes.get(ref);
                if(candidate!=null&&"coedge".equals(candidate.type)&&contains(candidate.refs,loop.id)){
                    next=candidate;break;
                }
            }
            current=next;
            if(current!=null&&current.id==start.id)break;
        }
        removeClosingDuplicate(points);
        return new LoopPath(points,exact&&points.size()>=3);
    }

    private static EdgePath sampleEdge(Map<Integer,Node> nodes,Node edge,boolean coedgeReversed){
        List<Node> vertices=refsByType(edge,nodes,"vertex");
        if(vertices.size()<2)return new EdgePath(Collections.emptyList(),false);
        Vec a=vertexPoint(vertices.get(0),nodes),b=vertexPoint(vertices.get(1),nodes);
        if(a==null||b==null)return new EdgePath(Collections.emptyList(),false);
        boolean curveReversed=edge.hasWord("reversed");
        boolean reverse=coedgeReversed;
        Vec start=reverse?b:a,end=reverse?a:b;
        Node curve=firstRefBySuffix(edge,nodes,"-curve");
        if(curve==null)return new EdgePath(Arrays.asList(start,end),false);
        if("straight-curve".equals(curve.type))return new EdgePath(Arrays.asList(start,end),true);
        if("ellipse-curve".equals(curve.type)){
            List<Vec> sampled=sampleEllipse(curve,start,end,reverse^curveReversed);
            if(sampled.size()>=2)return new EdgePath(sampled,true);
        }
        return new EdgePath(Arrays.asList(start,end),false);
    }

    private static Vec vertexPoint(Node vertex,Map<Integer,Node> nodes){
        Node point=firstRefByType(vertex,nodes,"point");
        if(point==null)return null;
        double[] n=point.numbers();if(n.length<3)return null;
        return new Vec(n[n.length-3],n[n.length-2],n[n.length-1]);
    }

    private static List<Vec> sampleEllipse(Node curve,Vec start,Vec end,boolean reverse){
        double[] n=curve.numbers();if(n.length<10)return Collections.emptyList();
        Vec center=new Vec(n[0],n[1],n[2]);
        Vec normal=new Vec(n[3],n[4],n[5]).normalized();
        Vec major=new Vec(n[6],n[7],n[8]);
        double ratio=n[9],majorLen=major.length();
        if(majorLen<EPS||normal.length()<EPS||!Double.isFinite(ratio)||Math.abs(ratio)<EPS)return Collections.emptyList();
        Vec minor=normal.cross(major).normalized().times(majorLen*ratio);
        double a0=ellipseAngle(start,center,major,minor),a1=ellipseAngle(end,center,major,minor);
        boolean full=start.distance(end)<=Math.max(EPS,majorLen*1e-6);
        double sweep;
        if(full)sweep=reverse?-Math.PI*2:Math.PI*2;
        else{
            sweep=a1-a0;
            while(sweep<=0)sweep+=Math.PI*2;
            if(reverse)sweep-=Math.PI*2;
        }
        int steps=full?ELLIPSE_STEPS:Math.max(3,(int)Math.ceil(Math.abs(sweep)/(Math.PI*2)*ELLIPSE_STEPS));
        List<Vec> out=new ArrayList<>();
        int limit=full?steps:steps+1;
        for(int i=0;i<limit;i++){
            double t=a0+sweep*i/steps;
            out.add(center.plus(major.times(Math.cos(t))).plus(minor.times(Math.sin(t))));
        }
        if(!full){out.set(0,start);out.set(out.size()-1,end);}
        return out;
    }

    private static double ellipseAngle(Vec p,Vec c,Vec major,Vec minor){
        Vec d=p.minus(c);
        double x=d.dot(major)/Math.max(EPS,major.dot(major));
        double y=d.dot(minor)/Math.max(EPS,minor.dot(minor));
        return Math.atan2(y,x);
    }

    private static void appendPath(List<Vec> target,List<Vec> path){
        for(Vec p:path){
            if(target.isEmpty()||target.get(target.size()-1).distance(p)>EPS)target.add(p);
        }
    }

    private static void removeClosingDuplicate(List<Vec> points){
        if(points.size()>2&&points.get(0).distance(points.get(points.size()-1))<=EPS)points.remove(points.size()-1);
    }

    private static Transform3 findTransform(Map<Integer,Node> nodes){
        for(Node body:nodesByType(nodes,"body")){
            Node t=firstRefByType(body,nodes,"transform");
            if(t!=null)return Transform3.from(t);
        }
        for(Node t:nodesByType(nodes,"transform"))return Transform3.from(t);
        return Transform3.IDENTITY;
    }

    private static final class Transform3 {
        static final Transform3 IDENTITY=new Transform3(new double[]{1,0,0,0,1,0,0,0,1},new Vec(0,0,0),1);
        final double[] m;final Vec t;final double scale;
        Transform3(double[] m,Vec t,double scale){this.m=m;this.t=t;this.scale=scale;}
        static Transform3 from(Node node){
            double[] n=node.numbers();
            if(n.length<13)return IDENTITY;
            double s=n[12];if(!Double.isFinite(s)||Math.abs(s)<EPS)s=1;
            return new Transform3(Arrays.copyOfRange(n,0,9),new Vec(n[9],n[10],n[11]),s);
        }
        Vec apply(Vec p){
            double x=scale*(p.x*m[0]+p.y*m[3]+p.z*m[6])+t.x;
            double y=scale*(p.x*m[1]+p.y*m[4]+p.z*m[7])+t.y;
            double z=scale*(p.x*m[2]+p.y*m[5]+p.z*m[8])+t.z;
            return new Vec(x,y,z);
        }
    }

    private static final class MeshBuilder {
        final Transform3 transform;
        final List<Vec> points=new ArrayList<>();
        final Ints triangles=new Ints(),edges=new Ints();
        final Map<Key,Integer> index=new HashMap<>();
        final Set<Long> edgeKeys=new HashSet<>();
        MeshBuilder(Transform3 transform){this.transform=transform==null?Transform3.IDENTITY:transform;}

        int pointCount(){return points.size();}
        int vertex(Vec raw){
            Vec p=transform.apply(raw);Key key=new Key(p);
            Integer existing=index.get(key);if(existing!=null)return existing;
            if(points.size()>=MAX_POINTS)throw new IllegalArgumentException("ACIS vertex limit");
            int i=points.size();points.add(p);index.put(key,i);return i;
        }
        void addEdge(Vec a,Vec b){
            int ia=vertex(a),ib=vertex(b);if(ia==ib)return;
            long key=((long)Math.min(ia,ib)<<32)|(Math.max(ia,ib)&0xffffffffL);
            if(edgeKeys.add(key)){edges.add(ia);edges.add(ib);}
        }
        void addOpenEdges(List<Vec> p){for(int i=1;i<p.size();i++)addEdge(p.get(i-1),p.get(i));}
        void addClosedEdges(List<Vec> p){
            addOpenEdges(p);if(p.size()>2)addEdge(p.get(p.size()-1),p.get(0));
        }
        boolean addTriangle(Vec a,Vec b,Vec c){
            int ia=vertex(a),ib=vertex(b),ic=vertex(c);if(ia==ib||ib==ic||ia==ic)return false;
            if(triangles.n/3>=MAX_TRIANGLES)throw new IllegalArgumentException("ACIS triangle limit");
            triangles.add(ia);triangles.add(ib);triangles.add(ic);return true;
        }

        boolean triangulatePlanar(List<Vec> polygon){
            List<Vec> p=cleanPolygon(polygon);if(p.size()<3)return false;
            Vec normal=newell(p);int axis=dominantAxis(normal);
            double area=signedArea2(p,axis);if(Math.abs(area)<EPS)return false;
            List<Integer> remaining=new ArrayList<>();for(int i=0;i<p.size();i++)remaining.add(i);
            boolean ccw=area>0;int guard=0,added=0;
            while(remaining.size()>3&&guard++<p.size()*p.size()*2){
                boolean clipped=false;
                for(int ri=0;ri<remaining.size();ri++){
                    int a=remaining.get((ri+remaining.size()-1)%remaining.size());
                    int b=remaining.get(ri);
                    int c=remaining.get((ri+1)%remaining.size());
                    if(!convex(p.get(a),p.get(b),p.get(c),axis,ccw))continue;
                    boolean inside=false;
                    for(int q:remaining)if(q!=a&&q!=b&&q!=c&&insideTriangle(p.get(q),p.get(a),p.get(b),p.get(c),axis)){inside=true;break;}
                    if(inside)continue;
                    if(addTriangle(p.get(a),p.get(b),p.get(c)))added++;
                    remaining.remove(ri);clipped=true;break;
                }
                if(!clipped)return false;
            }
            if(remaining.size()==3&&addTriangle(p.get(remaining.get(0)),p.get(remaining.get(1)),p.get(remaining.get(2))))added++;
            return added>0;
        }

        boolean loft(List<Vec> first,List<Vec> second){
            List<Vec> a=cleanPolygon(first),b=cleanPolygon(second);
            if(a.size()<3||a.size()!=b.size())return false;
            int n=a.size(),best=0;double bestCost=Double.POSITIVE_INFINITY;
            for(int shift=0;shift<n;shift++){
                double cost=0;for(int i=0;i<n;i++)cost+=a.get(i).distanceSquared(b.get((i+shift)%n));
                if(cost<bestCost){bestCost=cost;best=shift;}
            }
            boolean flip=false;
            double forward=0,reverse=0;
            for(int i=0;i<n;i++){
                forward+=a.get(i).distanceSquared(b.get((i+best)%n));
                reverse+=a.get(i).distanceSquared(b.get((best-i+n)%n));
            }
            flip=reverse<forward;int added=0;
            for(int i=0;i<n;i++){
                int j=(i+1)%n;
                Vec a0=a.get(i),a1=a.get(j);
                Vec b0=b.get(flip?(best-i+n)%n:(i+best)%n);
                Vec b1=b.get(flip?(best-j+n)%n:(j+best)%n);
                if(addTriangle(a0,a1,b1))added++;
                if(addTriangle(a0,b1,b0))added++;
            }
            return added>0;
        }

        float[] xyz(){float[] out=new float[points.size()*3];int k=0;for(Vec p:points){out[k++]=(float)p.x;out[k++]=(float)p.y;out[k++]=(float)p.z;}return out;}
        int[] triangles(){return triangles.toArray();}
        int[] edges(){return edges.toArray();}
    }

    private static List<Vec> cleanPolygon(List<Vec> input){
        List<Vec> out=new ArrayList<>();
        for(Vec p:input)if(out.isEmpty()||out.get(out.size()-1).distance(p)>EPS)out.add(p);
        removeClosingDuplicate(out);
        return out;
    }

    private static Vec newell(List<Vec> p){
        double x=0,y=0,z=0;for(int i=0;i<p.size();i++){Vec a=p.get(i),b=p.get((i+1)%p.size());x+=(a.y-b.y)*(a.z+b.z);y+=(a.z-b.z)*(a.x+b.x);z+=(a.x-b.x)*(a.y+b.y);}return new Vec(x,y,z);
    }
    private static int dominantAxis(Vec n){double x=Math.abs(n.x),y=Math.abs(n.y),z=Math.abs(n.z);return x>=y&&x>=z?0:y>=z?1:2;}
    private static double px(Vec p,int axis){return axis==0?p.y:p.x;}
    private static double py(Vec p,int axis){return axis==2?p.y:p.z;}
    private static double signedArea2(List<Vec> p,int axis){double a=0;for(int i=0;i<p.size();i++){Vec u=p.get(i),v=p.get((i+1)%p.size());a+=px(u,axis)*py(v,axis)-px(v,axis)*py(u,axis);}return a*.5;}
    private static double cross2(Vec a,Vec b,Vec c,int axis){return (px(b,axis)-px(a,axis))*(py(c,axis)-py(a,axis))-(py(b,axis)-py(a,axis))*(px(c,axis)-px(a,axis));}
    private static boolean convex(Vec a,Vec b,Vec c,int axis,boolean ccw){double v=cross2(a,b,c,axis);return ccw?v>EPS:v<-EPS;}
    private static boolean insideTriangle(Vec p,Vec a,Vec b,Vec c,int axis){
        double c1=cross2(a,b,p,axis),c2=cross2(b,c,p,axis),c3=cross2(c,a,p,axis);
        boolean neg=c1<-EPS||c2<-EPS||c3<-EPS,pos=c1>EPS||c2>EPS||c3>EPS;return !(neg&&pos);
    }

    private static final class LoopPath {final List<Vec> points;final boolean exact;LoopPath(List<Vec> p,boolean e){points=p;exact=e;}}
    private static final class EdgePath {final List<Vec> points;final boolean exact;EdgePath(List<Vec> p,boolean e){points=p;exact=e;}}
    private static final class Vec {
        final double x,y,z;Vec(double x,double y,double z){this.x=x;this.y=y;this.z=z;}
        Vec plus(Vec p){return new Vec(x+p.x,y+p.y,z+p.z);}Vec minus(Vec p){return new Vec(x-p.x,y-p.y,z-p.z);}
        Vec times(double s){return new Vec(x*s,y*s,z*s);}double dot(Vec p){return x*p.x+y*p.y+z*p.z;}
        Vec cross(Vec p){return new Vec(y*p.z-z*p.y,z*p.x-x*p.z,x*p.y-y*p.x);}
        double length(){return Math.sqrt(x*x+y*y+z*z);}Vec normalized(){double l=length();return l<EPS?new Vec(0,0,0):times(1/l);}
        double distance(Vec p){return Math.sqrt(distanceSquared(p));}double distanceSquared(Vec p){double a=x-p.x,b=y-p.y,c=z-p.z;return a*a+b*b+c*c;}
    }
    private static final class Key {
        final long x,y,z;Key(Vec p){x=q(p.x);y=q(p.y);z=q(p.z);}static long q(double v){return Math.round(v*1_000_000d);}
        @Override public int hashCode(){long h=x*73856093L^y*19349663L^z*83492791L;return (int)(h^(h>>>32));}
        @Override public boolean equals(Object o){if(!(o instanceof Key))return false;Key k=(Key)o;return x==k.x&&y==k.y&&z==k.z;}
    }
    private static final class Ints {int[] a=new int[64];int n;void add(int v){if(n==a.length)a=Arrays.copyOf(a,a.length*2);a[n++]=v;}int[] toArray(){return Arrays.copyOf(a,n);}}
    private static final class Doubles {double[] a=new double[32];int n;void add(double v){if(n==a.length)a=Arrays.copyOf(a,a.length*2);a[n++]=v;}double[] toArray(){return Arrays.copyOf(a,n);}}

    private AcisSatMesh(){}
}
