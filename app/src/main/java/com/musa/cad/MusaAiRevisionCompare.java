package com.musa.cad;

import java.util.*;

/** Deterministic offline comparison of two MusaCAD drawing indexes. */
public final class MusaAiRevisionCompare {
    public enum Mode { FULL, ADDED, REMOVED, CHANGED }

    public static final class Result {
        public final boolean matched;
        public final String text;
        public final int unchanged,added,removed,changed;
        public final List<Integer> sourceIds;
        private Result(boolean matched,String text,int unchanged,int added,int removed,int changed,Collection<Integer>sourceIds){
            this.matched=matched;this.text=text==null?"":text;
            this.unchanged=Math.max(0,unchanged);this.added=Math.max(0,added);
            this.removed=Math.max(0,removed);this.changed=Math.max(0,changed);
            this.sourceIds=Collections.unmodifiableList(new ArrayList<>(sourceIds));
        }
        public static Result none(){return new Result(false,"",0,0,0,0,Collections.emptyList());}
    }

    private static final class Pair {
        final MusaAiDrawingIndex.Item oldItem,newItem;
        Pair(MusaAiDrawingIndex.Item oldItem,MusaAiDrawingIndex.Item newItem){this.oldItem=oldItem;this.newItem=newItem;}
    }
    private static final class Diff {
        final ArrayList<Pair> unchanged=new ArrayList<>(),changed=new ArrayList<>();
        final ArrayList<MusaAiDrawingIndex.Item> added=new ArrayList<>(),removed=new ArrayList<>();
    }

    public static boolean asksComparison(String raw){
        String q=MusaAiDrawingIndex.normalize(raw);
        if(q.isEmpty())return false;
        return q.contains("revizyon")||q.contains("karsilastir")||q.contains("farklari")||
            q.contains("fark ne")||q.contains("degisiklikleri")||q.contains("eklenenleri")||
            q.contains("silinenleri")||q.contains("kaldirilanlari")||q.contains("degisenleri");
    }

    public static Result compare(MusaAiDrawingIndex baseline,MusaAiDrawingIndex current,
                                 String raw,String baselineName,String currentName){
        if(baseline==null||current==null||!asksComparison(raw))return Result.none();
        Mode mode=mode(raw);
        Diff d=diff(baseline,current);
        LinkedHashSet<Integer>ids=new LinkedHashSet<>();
        if(mode==Mode.FULL||mode==Mode.ADDED)for(MusaAiDrawingIndex.Item item:d.added)if(item.sourceId>=0)ids.add(item.sourceId);
        if(mode==Mode.FULL||mode==Mode.CHANGED)for(Pair p:d.changed)if(p.newItem.sourceId>=0)ids.add(p.newItem.sourceId);

        String base=cleanName(baselineName,"Referans");
        String now=cleanName(currentName,"Güncel");
        StringBuilder out=new StringBuilder();
        out.append("AI revizyon karşılaştırması");
        out.append("\nReferans: ").append(base).append(" → Güncel: ").append(now);

        if(mode==Mode.ADDED){
            out.append("\n• Eklenen nesne: ").append(d.added.size());
            appendTypeSummary(out,d.added);
        }else if(mode==Mode.REMOVED){
            out.append("\n• Silinen nesne: ").append(d.removed.size());
            appendTypeSummary(out,d.removed);
            out.append("\nSilinen nesneler güncel çizimde bulunmadığı için doğrudan vurgulanamaz.");
        }else if(mode==Mode.CHANGED){
            out.append("\n• Değişen / taşınan nesne: ").append(d.changed.size());
            appendChangedTypeSummary(out,d.changed);
        }else{
            out.append("\n• Değişmeden kalan: ").append(d.unchanged.size());
            out.append("\n• Eklenen: ").append(d.added.size());
            out.append("\n• Silinen: ").append(d.removed.size());
            out.append("\n• Değişen / taşınan: ").append(d.changed.size());
            appendLayerDelta(out,baseline,current);
            appendCompactTypeDelta(out,d);
        }

        if(!sameUnit(baseline.unitName,current.unitName)){
            out.append("\n⚠ Çizim birimleri farklı görünüyor: ")
               .append(unit(baseline)).append(" → ").append(unit(current));
        }
        if(!sameLayout(baseline.layout,current.layout)){
            out.append("\n⚠ Layoutlar farklı: ").append(baseline.layout).append(" → ").append(current.layout);
        }
        out.append("\nNot: Eşleşme geometri, katman, metin ve yakın konum bilgisiyle yapılır; büyük koordinat kaydırmaları bazı nesneleri eklenen/silinen olarak gösterebilir.");

        return new Result(true,out.toString(),d.unchanged.size(),d.added.size(),d.removed.size(),d.changed.size(),ids);
    }

    private static Diff diff(MusaAiDrawingIndex oldIndex,MusaAiDrawingIndex newIndex){
        ArrayList<MusaAiDrawingIndex.Item>oldItems=new ArrayList<>(oldIndex.items());
        ArrayList<MusaAiDrawingIndex.Item>newItems=new ArrayList<>(newIndex.items());
        boolean[]oldUsed=new boolean[oldItems.size()],newUsed=new boolean[newItems.size()];
        Diff d=new Diff();

        HashMap<String,ArrayDeque<Integer>>exact=new HashMap<>();
        for(int i=0;i<oldItems.size();i++)exact.computeIfAbsent(exactKey(oldItems.get(i)),k->new ArrayDeque<>()).add(i);
        for(int j=0;j<newItems.size();j++){
            ArrayDeque<Integer>q=exact.get(exactKey(newItems.get(j)));
            while(q!=null&&!q.isEmpty()&&oldUsed[q.peekFirst()])q.removeFirst();
            if(q==null||q.isEmpty())continue;
            int i=q.removeFirst();oldUsed[i]=true;newUsed[j]=true;
            d.unchanged.add(new Pair(oldItems.get(i),newItems.get(j)));
        }

        HashMap<String,ArrayDeque<Integer>>layerless=new HashMap<>();
        for(int i=0;i<oldItems.size();i++)if(!oldUsed[i])layerless.computeIfAbsent(layerlessKey(oldItems.get(i)),k->new ArrayDeque<>()).add(i);
        for(int j=0;j<newItems.size();j++){
            if(newUsed[j])continue;
            String key=layerlessKey(newItems.get(j));if(key.isEmpty())continue;
            ArrayDeque<Integer>q=layerless.get(key);
            while(q!=null&&!q.isEmpty()&&oldUsed[q.peekFirst()])q.removeFirst();
            if(q==null||q.isEmpty())continue;
            int i=q.removeFirst();oldUsed[i]=true;newUsed[j]=true;
            d.changed.add(new Pair(oldItems.get(i),newItems.get(j)));
        }

        double scale=drawingScale(oldItems,newItems);
        for(int j=0;j<newItems.size();j++){
            if(newUsed[j])continue;
            MusaAiDrawingIndex.Item n=newItems.get(j);
            int best=-1;double bestCost=Double.POSITIVE_INFINITY;
            for(int i=0;i<oldItems.size();i++){
                if(oldUsed[i])continue;
                MusaAiDrawingIndex.Item o=oldItems.get(i);
                double cost=similarityCost(o,n,scale);
                if(cost<bestCost){bestCost=cost;best=i;}
            }
            if(best>=0&&bestCost<=1d){
                oldUsed[best]=true;newUsed[j]=true;
                d.changed.add(new Pair(oldItems.get(best),n));
            }
        }

        for(int j=0;j<newItems.size();j++)if(!newUsed[j])d.added.add(newItems.get(j));
        for(int i=0;i<oldItems.size();i++)if(!oldUsed[i])d.removed.add(oldItems.get(i));
        return d;
    }

    private static double similarityCost(MusaAiDrawingIndex.Item a,MusaAiDrawingIndex.Item b,double scale){
        if(!a.type.equals(b.type))return Double.POSITIVE_INFINITY;
        String at=MusaAiDrawingIndex.normalize(a.text),bt=MusaAiDrawingIndex.normalize(b.text);
        boolean textIdentity=!at.isEmpty()||!bt.isEmpty();
        if(textIdentity&&!at.equals(bt))return Double.POSITIVE_INFINITY;

        boolean sameLayer=MusaAiDrawingIndex.normalize(a.layer).equals(MusaAiDrawingIndex.normalize(b.layer));
        if(!textIdentity&&!sameLayer)return Double.POSITIVE_INFINITY;

        double center=0d;
        if(a.hasCenter()&&b.hasCenter()){
            center=Math.hypot(a.centerX-b.centerX,a.centerY-b.centerY)/Math.max(1e-9,scale);
            double max=textIdentity?.22d:.04d;
            if(center>max)return Double.POSITIVE_INFINITY;
            center/=max;
        }else center=.35d;

        double metric=metricDifference(a,b);
        if(!textIdentity&&metric>.35d)return Double.POSITIVE_INFINITY;
        if(textIdentity&&metric>.9d)return Double.POSITIVE_INFINITY;
        double layerPenalty=sameLayer?0d:.20d;
        return center*.58d+Math.min(1d,metric)*.32d+layerPenalty;
    }

    private static double metricDifference(MusaAiDrawingIndex.Item a,MusaAiDrawingIndex.Item b){
        double total=0d;int n=0;
        if(a.hasLength()&&b.hasLength()){total+=relative(a.length,b.length);n++;}
        if(a.hasArea()&&b.hasArea()){total+=relative(a.area,b.area);n++;}
        if(a.closedKnown&&b.closedKnown){total+=a.closed==b.closed?0d:1d;n++;}
        return n==0?0d:total/n;
    }

    private static double relative(double a,double b){
        double den=Math.max(Math.max(Math.abs(a),Math.abs(b)),1e-9);
        return Math.abs(a-b)/den;
    }

    private static String exactKey(MusaAiDrawingIndex.Item i){
        return MusaAiDrawingIndex.normalize(i.layer)+"|"+layerlessKey(i);
    }
    private static String layerlessKey(MusaAiDrawingIndex.Item i){
        String g=i.geometryKey==null?"":i.geometryKey.trim();
        if(g.isEmpty()){
            if(!i.hasCenter()&&!i.hasLength()&&!i.hasArea()&&i.text.isEmpty())return "";
            g="fallback|"+q(i.centerX)+","+q(i.centerY)+"|"+q(i.length)+"|"+q(i.area);
        }
        return i.type+"|"+MusaAiDrawingIndex.normalize(i.text)+"|"+g;
    }
    private static long q(double v){return Double.isFinite(v)?Math.round(v*100000d):Long.MIN_VALUE;}

    private static double drawingScale(List<MusaAiDrawingIndex.Item>a,List<MusaAiDrawingIndex.Item>b){
        double minX=Double.POSITIVE_INFINITY,minY=Double.POSITIVE_INFINITY,maxX=Double.NEGATIVE_INFINITY,maxY=Double.NEGATIVE_INFINITY,maxMetric=1d;
        for(List<MusaAiDrawingIndex.Item>list:Arrays.asList(a,b))for(MusaAiDrawingIndex.Item i:list){
            if(i.hasCenter()){minX=Math.min(minX,i.centerX);minY=Math.min(minY,i.centerY);maxX=Math.max(maxX,i.centerX);maxY=Math.max(maxY,i.centerY);}
            if(i.hasLength())maxMetric=Math.max(maxMetric,i.length);
        }
        double diag=(Double.isFinite(minX)&&Double.isFinite(maxX))?Math.hypot(maxX-minX,maxY-minY):0d;
        return Math.max(Math.max(diag,maxMetric),1d);
    }

    private static void appendLayerDelta(StringBuilder out,MusaAiDrawingIndex oldIndex,MusaAiDrawingIndex newIndex){
        ArrayList<String>added=new ArrayList<>(),removed=new ArrayList<>();
        for(String s:newIndex.allLayers)if(!containsIgnoreCase(oldIndex.allLayers,s))added.add(s);
        for(String s:oldIndex.allLayers)if(!containsIgnoreCase(newIndex.allLayers,s))removed.add(s);
        if(!added.isEmpty())out.append("\n• Yeni katman: ").append(joinLimited(added,6));
        if(!removed.isEmpty())out.append("\n• Kaldırılan katman: ").append(joinLimited(removed,6));
    }

    private static void appendCompactTypeDelta(StringBuilder out,Diff d){
        LinkedHashMap<String,Integer>add=counts(d.added),rem=counts(d.removed);
        if(!add.isEmpty())out.append("\n• Eklenen türler: ").append(formatCounts(add,6));
        if(!rem.isEmpty())out.append("\n• Silinen türler: ").append(formatCounts(rem,6));
        if(!d.changed.isEmpty()){
            LinkedHashMap<String,Integer>x=new LinkedHashMap<>();
            for(Pair p:d.changed)x.put(p.newItem.type,x.getOrDefault(p.newItem.type,0)+1);
            out.append("\n• Değişen türler: ").append(formatCounts(x,6));
        }
    }
    private static void appendTypeSummary(StringBuilder out,List<MusaAiDrawingIndex.Item>items){
        LinkedHashMap<String,Integer>c=counts(items);if(!c.isEmpty())out.append("\n• Türler: ").append(formatCounts(c,8));
    }
    private static void appendChangedTypeSummary(StringBuilder out,List<Pair>items){
        LinkedHashMap<String,Integer>c=new LinkedHashMap<>();
        for(Pair p:items)c.put(p.newItem.type,c.getOrDefault(p.newItem.type,0)+1);
        if(!c.isEmpty())out.append("\n• Türler: ").append(formatCounts(c,8));
    }
    private static LinkedHashMap<String,Integer>counts(List<MusaAiDrawingIndex.Item>items){
        LinkedHashMap<String,Integer>out=new LinkedHashMap<>();
        for(MusaAiDrawingIndex.Item i:items)out.put(i.type,out.getOrDefault(i.type,0)+1);
        return out;
    }
    private static String formatCounts(Map<String,Integer>counts,int limit){
        ArrayList<Map.Entry<String,Integer>>entries=new ArrayList<>(counts.entrySet());
        entries.sort((a,b)->Integer.compare(b.getValue(),a.getValue()));
        ArrayList<String>out=new ArrayList<>();for(Map.Entry<String,Integer>e:entries){out.add(e.getKey()+" "+e.getValue());if(out.size()>=limit)break;}
        return String.join(" • ",out);
    }
    private static String joinLimited(List<String>items,int limit){
        return String.join(", ",items.subList(0,Math.min(limit,items.size())))+(items.size()>limit?" …":"");
    }
    private static boolean containsIgnoreCase(Collection<String>items,String wanted){for(String s:items)if(s.equalsIgnoreCase(wanted))return true;return false;}
    private static boolean sameUnit(String a,String b){return MusaAiDrawingIndex.normalize(a).equals(MusaAiDrawingIndex.normalize(b));}
    private static boolean sameLayout(String a,String b){return a==null?b==null:a.equalsIgnoreCase(b);}
    private static String unit(MusaAiDrawingIndex i){return i.unitName==null||i.unitName.trim().isEmpty()?"belirsiz":i.unitName;}
    private static String cleanName(String name,String fallback){return name==null||name.trim().isEmpty()?fallback:name.trim();}
    private static Mode mode(String raw){
        String q=MusaAiDrawingIndex.normalize(raw);
        if(q.contains("eklenen"))return Mode.ADDED;
        if(q.contains("silinen")||q.contains("kaldirilan"))return Mode.REMOVED;
        if(q.contains("degisen")||q.contains("tasinan")||q.contains("modifiye"))return Mode.CHANGED;
        return Mode.FULL;
    }
    private MusaAiRevisionCompare(){}
}
