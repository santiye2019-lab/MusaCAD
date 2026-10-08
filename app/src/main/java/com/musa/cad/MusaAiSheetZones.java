package com.musa.cad;

import java.util.*;

/**
 * Small, deterministic world-coordinate grid over a BOUNDED vector evidence sample.
 *
 * This is groundwork for future tiled vision. Zones describe co-location only:
 * no semantic object identification, connected pipe graph, or full-sheet audit is
 * inferred from a text label being close to another CAD object.
 */
public final class MusaAiSheetZones {
    private static final int GRID=4;
    private static final int MAX_REPORTED_ZONES=8;
    private static final int MAX_SAMPLE_TEXT=70;

    public static final class Zone {
        public final int x,y,annotations,runs,blocks;
        public final List<Integer> sourceIds;
        public final List<String> labelSamples;

        private Zone(int x,int y,int annotations,int runs,int blocks,
                     Collection<Integer> ids,Collection<String> labels){
            this.x=x;this.y=y;this.annotations=annotations;this.runs=runs;this.blocks=blocks;
            this.sourceIds=Collections.unmodifiableList(new ArrayList<>(ids));
            this.labelSamples=Collections.unmodifiableList(new ArrayList<>(labels));
        }
    }

    public static final class Result {
        public final List<Zone> zones;
        public final int positioned,withoutCoordinates;
        public final String text;

        private Result(Collection<Zone> zones,int positioned,int withoutCoordinates,String text){
            this.zones=Collections.unmodifiableList(new ArrayList<>(zones));
            this.positioned=positioned;
            this.withoutCoordinates=withoutCoordinates;
            this.text=text;
        }
    }

    private static final class MutableZone {
        final int x,y;
        int annotations,runs,blocks;
        final LinkedHashSet<Integer> ids=new LinkedHashSet<>();
        final LinkedHashSet<String> labels=new LinkedHashSet<>();
        MutableZone(int x,int y){this.x=x;this.y=y;}
        void accept(MusaAiDrawingIndex.Item item){
            String t=item.type;
            if(isText(t)){
                annotations++;
                if(!item.text.isEmpty()&&labels.size()<2){
                    String s=item.text.replace('\n',' ').replace('\r',' ').trim();
                    if(s.length()>MAX_SAMPLE_TEXT)s=s.substring(0,MAX_SAMPLE_TEXT)+"…";
                    if(!s.isEmpty())labels.add(s);
                }
            }else if(isLine(t)&&item.hasLength()){
                runs+=item.quantity;
            }else if("BLOCK".equals(t)||"INSERT".equals(t)){
                blocks+=item.quantity;
            }
            if(item.sourceId>=0&&ids.size()<12)ids.add(item.sourceId);
        }
        Zone freeze(){return new Zone(x,y,annotations,runs,blocks,ids,labels);}
    }

    public static Result analyze(MusaAiDrawingIndex index){
        if(index==null||index.items().isEmpty())
            return new Result(Collections.emptyList(),0,0,
                "\n\nPAFTA KONUM BÖLGELERİ: Koordinatlı vektör kanıtı bulunamadı.");
        double xmin=Double.POSITIVE_INFINITY,ymin=Double.POSITIVE_INFINITY;
        double xmax=Double.NEGATIVE_INFINITY,ymax=Double.NEGATIVE_INFINITY;
        int positioned=0,missing=0;

        for(MusaAiDrawingIndex.Item item:index.items()){
            if(item==null)continue;
            if(!item.hasCenter()){missing++;continue;}
            positioned++;
            xmin=Math.min(xmin,item.centerX);xmax=Math.max(xmax,item.centerX);
            ymin=Math.min(ymin,item.centerY);ymax=Math.max(ymax,item.centerY);
        }

        if(positioned==0){
            return new Result(Collections.emptyList(),0,missing,
                "\n\nPAFTA KONUM BÖLGELERİ: Örneklenen öğelerde geçerli X/Y koordinatı bulunamadı."+
                " Görsel analiz ya da tüm pafta kapsaması iddia edilemez.");
        }

        double xspan=xmax-xmin,yspan=ymax-ymin;
        // Reject non-finite extents (including extreme CAD coordinate overflow).
        if(!Double.isFinite(xspan)||!Double.isFinite(yspan))
            return new Result(Collections.emptyList(),positioned,missing,
                "\n\nPAFTA KONUM BÖLGELERİ: Koordinat aralığı geçersiz; bölgeleme atlandı.");

        MutableZone[] bins=new MutableZone[GRID*GRID];
        for(MusaAiDrawingIndex.Item item:index.items()){
            if(item==null||!item.hasCenter())continue;
            int x=bin(item.centerX,xmin,xspan);
            int y=bin(item.centerY,ymin,yspan);
            int k=y*GRID+x;
            if(bins[k]==null)bins[k]=new MutableZone(x,y);
            bins[k].accept(item);
        }
        List<Zone> zones=new ArrayList<>();
        for(MutableZone b:bins)if(b!=null)zones.add(b.freeze());
        zones.sort((a,b)->{
            int av=a.annotations+a.runs+a.blocks,bv=b.annotations+b.runs+b.blocks;
            int c=Integer.compare(bv,av);
            if(c!=0)return c;
            c=Integer.compare(a.y,b.y);return c!=0?c:Integer.compare(a.x,b.x);
        });

        StringBuilder out=new StringBuilder("\n\nPAFTA KONUM BÖLGELERİ (yalnızca vektör örneklemi)");
        out.append("\n• Koordinatı okunan: ").append(positioned)
            .append(" öğe; konumsuz: ").append(missing)
            .append("; dolu bölge: ").append(zones.size()).append("/16.");
        int shown=0;
        for(Zone z:zones){
            if(shown++>=MAX_REPORTED_ZONES)break;
            out.append("\n• X").append(z.x+1).append("/Y").append(z.y+1)
                .append(" — ").append(z.annotations).append(" yazı, ")
                .append(z.runs).append(" ölçülebilir çizgi, ")
                .append(z.blocks).append(" blok");
            if(!z.labelSamples.isEmpty())out.append("; örnek: ").append(z.labelSamples.get(0));
            if(!z.sourceIds.isEmpty())out.append(" [kaynak ").append(z.sourceIds.get(0)).append("]");
        }
        if(zones.size()>MAX_REPORTED_ZONES)out.append("\n• Diğer dolu bölgeler özet dışında.");
        out.append("\n• Bu alanlar sadece örnek öğelerin çizim koordinatlarına göre oluşturuldu.");
        out.append(" Yakınlık bağlantı, çap ataması, görsel sembol tanıma veya projenin tamamının incelendiğini kanıtlamaz.");
        return new Result(zones,positioned,missing,out.toString());
    }

    private static int bin(double point,double min,double span){
        if(span<=0d)return 0;
        double fraction=(point-min)/span;
        if(!Double.isFinite(fraction))return 0;
        return (int)Math.max(0,Math.min(GRID-1,Math.floor(fraction*GRID)));
    }
    private static boolean isText(String type){
        return "TEXT".equals(type)||"MTEXT".equals(type)||"ATTRIB".equals(type)||"ATTDEF".equals(type);
    }
    private static boolean isLine(String type){
        return "LINE".equals(type)||"POLYLINE".equals(type)||
            "LWPOLYLINE".equals(type)||"ARC".equals(type);
    }
    private MusaAiSheetZones(){}
}
