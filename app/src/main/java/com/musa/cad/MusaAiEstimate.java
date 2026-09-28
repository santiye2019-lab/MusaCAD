package com.musa.cad;

import java.util.*;
import java.util.regex.*;

/** Estimate/takeoff model, parser, drawing-derived draft generator and comparator. */
public final class MusaAiEstimate {
    public static final class Item {
        public final String poz,description,unit,source;
        public final double quantity,unitPrice,total;
        public final MusaAiDiscipline discipline;
        public Item(String poz,String description,String unit,double quantity,double unitPrice,double total,MusaAiDiscipline discipline,String source){
            this.poz=clean(poz);this.description=clean(description);this.unit=clean(unit);
            this.quantity=finite(quantity);this.unitPrice=finite(unitPrice);
            this.total=Double.isFinite(total)?total:(Double.isFinite(this.quantity)&&Double.isFinite(this.unitPrice)?this.quantity*this.unitPrice:Double.NaN);
            this.discipline=discipline==null?MusaAiDiscipline.UNKNOWN:discipline;this.source=clean(source);
        }
    }

    public static final class Document {
        public final String sourceName,kind;
        public final boolean draft;
        public final List<Item> items;
        public Document(String sourceName,String kind,boolean draft,Collection<Item>items){
            this.sourceName=clean(sourceName);this.kind=clean(kind);this.draft=draft;
            this.items=Collections.unmodifiableList(new ArrayList<>(items==null?Collections.emptyList():items));
        }
        public int size(){return items.size();}
    }

    public enum Status { MATCHED,QUANTITY_DIFF,UNIT_MISMATCH,MISSING_IN_ESTIMATE,EXTRA_IN_ESTIMATE,REVIEW }

    public static final class Difference {
        public final Status status;
        public final Item projectItem,estimateItem;
        public final double quantityDelta,percentDelta,matchScore;
        Difference(Status status,Item p,Item e,double delta,double percent,double score){
            this.status=status;this.projectItem=p;this.estimateItem=e;this.quantityDelta=delta;this.percentDelta=percent;this.matchScore=score;
        }
    }

    public static final class CompareResult {
        public final List<Difference> differences;
        public final String text;
        public final int matched,quantityDiff,unitMismatch,missing,extra,review;
        CompareResult(List<Difference>d,String text,int matched,int quantityDiff,int unitMismatch,int missing,int extra,int review){
            this.differences=Collections.unmodifiableList(new ArrayList<>(d));this.text=text;
            this.matched=matched;this.quantityDiff=quantityDiff;this.unitMismatch=unitMismatch;this.missing=missing;this.extra=extra;this.review=review;
        }
    }

    private static final Pattern CELL=Pattern.compile("^([A-Z]+)(\\d+)\\s*\\t(.*)$");
    private static final Pattern NUM=Pattern.compile("[-+]?\\d[\\d .,'’]*(?:[.,]\\d+)?");

    public static boolean isLoadCommand(String raw){
        String q=MusaAiDrawingIndex.normalize(raw);
        return q.contains("kesif yukle")||q.contains("kesfi yukle")||q.contains("kesif dosyasi ac")||q.contains("kesif ice aktar");
    }
    public static boolean isBuildCommand(String raw){
        String q=MusaAiDrawingIndex.normalize(raw);
        return q.contains("projeden kesif")||q.contains("kesif olustur")||q.contains("taslak kesif")||q.contains("metrajdan kesif");
    }
    public static boolean isCompareCommand(String raw){
        String q=MusaAiDrawingIndex.normalize(raw);
        return q.contains("kesif karsilastir")||q.contains("kesfi karsilastir")||q.contains("proje kesif uyum")||q.contains("proje kesif kontrol");
    }

    /** Parse text extracted from XLSX/CSV/TXT/DOCX by OfficeTextExtractor. */
    public static Document parseExtracted(String sourceName,String kind,String text){
        String value=text==null?"":text.replace("\r\n","\n").replace('\r','\n');
        List<List<String>> rows=looksLikeSpreadsheetCells(value)?spreadsheetRows(value):delimitedRows(value);
        ArrayList<Item> items=parseRows(rows,sourceName);
        return new Document(sourceName,kind,false,items);
    }

    public static Document fromDrawing(MusaAiDrawingIndex index,String sourceName){
        if(index==null)return new Document(sourceName,"DWG/DXF",true,Collections.emptyList());
        LinkedHashMap<String,Accumulator>groups=new LinkedHashMap<>();
        for(MusaAiDrawingIndex.Item item:index.items()){
            if(item==null)continue;
            String layer=clean(item.layer);if(layer.isEmpty())layer="0";
            MusaAiDiscipline d=MusaAiDiscipline.classify(layer+" "+item.text);
            String key=d.name()+"|"+layer;
            Accumulator a=groups.get(key);
            if(a==null){a=new Accumulator(d,layer,index.unitName);groups.put(key,a);}
            a.add(item);
        }
        ArrayList<Item>out=new ArrayList<>();
        for(Accumulator a:groups.values()){
            if(a.count==0)continue;
            String unit;double qty;
            if(a.lengthCount>0){unit=blank(a.unitName,"çizim birimi");qty=a.length;}
            else if(a.areaCount>0){unit=blank(a.unitName,"çizim birimi")+"²";qty=a.area;}
            else {unit="adet";qty=a.count;}
            out.add(new Item("","Katman: "+a.layer,unit,qty,Double.NaN,Double.NaN,a.discipline,"DWG/DXF metrajı"));
        }
        out.sort((a,b)->{
            int d=a.discipline.label.compareToIgnoreCase(b.discipline.label);return d!=0?d:a.description.compareToIgnoreCase(b.description);
        });
        return new Document(sourceName,"DWG/DXF TASLAK",true,out);
    }

    public static CompareResult compare(Document project,Document estimate){
        List<Item>p=project==null?Collections.emptyList():project.items;
        List<Item>e=estimate==null?Collections.emptyList():estimate.items;
        boolean[]used=new boolean[e.size()];ArrayList<Difference>diffs=new ArrayList<>();
        int matched=0,qd=0,um=0,missing=0,extra=0,review=0;
        for(Item pi:p){
            int best=-1;double score=0;
            for(int i=0;i<e.size();i++){
                if(used[i])continue;double s=score(pi,e.get(i));if(s>score){score=s;best=i;}
            }
            if(best<0||score<0.34d){
                diffs.add(new Difference(Status.MISSING_IN_ESTIMATE,pi,null,Double.NaN,Double.NaN,score));missing++;continue;
            }
            used[best]=true;Item ei=e.get(best);
            if(!unitCompatible(pi.unit,ei.unit)){
                diffs.add(new Difference(Status.UNIT_MISMATCH,pi,ei,Double.NaN,Double.NaN,score));um++;continue;
            }
            if(!Double.isFinite(pi.quantity)||!Double.isFinite(ei.quantity)){
                diffs.add(new Difference(Status.REVIEW,pi,ei,Double.NaN,Double.NaN,score));review++;continue;
            }
            double delta=pi.quantity-ei.quantity;
            double percent=Math.abs(ei.quantity)>1e-9?delta/ei.quantity*100d:(Math.abs(delta)<1e-9?0d:Double.POSITIVE_INFINITY);
            if(Math.abs(percent)>2d){
                diffs.add(new Difference(Status.QUANTITY_DIFF,pi,ei,delta,percent,score));qd++;
            }else{
                diffs.add(new Difference(Status.MATCHED,pi,ei,delta,percent,score));matched++;
            }
        }
        for(int i=0;i<e.size();i++)if(!used[i]){diffs.add(new Difference(Status.EXTRA_IN_ESTIMATE,null,e.get(i),Double.NaN,Double.NaN,0));extra++;}
        StringBuilder out=new StringBuilder("MUSACAD AI • PROJE–KEŞİF UYGUNLUK KONTROLÜ");
        out.append("\nProje/taslak: ").append(project==null?"yok":blank(project.sourceName,"aktif proje"));
        out.append("\nYüklenen keşif: ").append(estimate==null?"yok":blank(estimate.sourceName,"keşif"));
        out.append("\n\n• Uyumlu: ").append(matched);
        out.append("\n• Miktar farkı: ").append(qd);
        out.append("\n• Birim/teknik uyumsuzluk: ").append(um);
        out.append("\n• Projede var, keşifte yok: ").append(missing);
        out.append("\n• Keşifte var, projede eşleşmedi: ").append(extra);
        out.append("\n• İnceleme gerekli: ").append(review);
        int shown=0;
        for(Difference d:diffs){
            if(d.status==Status.MATCHED||shown>=30)continue;shown++;
            out.append("\n\n").append(shown).append(". ").append(label(d.status));
            Item base=d.projectItem!=null?d.projectItem:d.estimateItem;
            out.append("\n").append(base==null?"":base.description);
            if(d.projectItem!=null&&d.estimateItem!=null){
                out.append("\nProje: ").append(number(d.projectItem.quantity)).append(" ").append(d.projectItem.unit);
                out.append(" • Keşif: ").append(number(d.estimateItem.quantity)).append(" ").append(d.estimateItem.unit);
                if(d.status==Status.QUANTITY_DIFF)out.append("\nFark: ").append(number(d.quantityDelta)).append(" • ").append(number(d.percentDelta)).append("%");
            }
        }
        if(diffs.size()>30)out.append("\n\n… kalan kalemler detaylı rapora bırakıldı.");
        out.append("\n\nNot: Eşleştirme poz numarası/açıklama/birim ve çizim katmanı metadatasına göre otomatik ön kontroldür; ihale keşfi veya hakediş onayı değildir.");
        return new CompareResult(diffs,out.toString(),matched,qd,um,missing,extra,review);
    }

    public static String summary(Document doc){
        if(doc==null)return "Keşif yüklenmedi.";
        EnumMap<MusaAiDiscipline,Integer>counts=new EnumMap<>(MusaAiDiscipline.class);
        for(Item i:doc.items)counts.put(i.discipline,counts.getOrDefault(i.discipline,0)+1);
        StringBuilder b=new StringBuilder((doc.draft?"Taslak keşif":"Yüklenen keşif")+" • "+blank(doc.sourceName,"adsız"));
        b.append("\nKalem: ").append(doc.items.size());
        for(MusaAiDiscipline d:MusaAiDiscipline.engineering())if(counts.getOrDefault(d,0)>0)b.append("\n• ").append(d.label).append(": ").append(counts.get(d));
        if(counts.getOrDefault(MusaAiDiscipline.UNKNOWN,0)>0)b.append("\n• Sınıflandırılmamış: ").append(counts.get(MusaAiDiscipline.UNKNOWN));
        return b.toString();
    }

    private static final class Accumulator{
        final MusaAiDiscipline discipline;final String layer,unitName;int count,lengthCount,areaCount;double length,area;
        Accumulator(MusaAiDiscipline d,String layer,String unitName){discipline=d;this.layer=layer;this.unitName=clean(unitName);}
        void add(MusaAiDrawingIndex.Item i){count++;if(i.hasLength()){length+=i.length;lengthCount++;}if(i.hasArea()){area+=i.area;areaCount++;}}
    }

    private static boolean looksLikeSpreadsheetCells(String text){
        int n=0;for(String line:text.split("\n"))if(CELL.matcher(line.trim()).matches()&&++n>=3)return true;return false;
    }
    private static List<List<String>> spreadsheetRows(String text){
        TreeMap<Integer,TreeMap<Integer,String>> rows=new TreeMap<>();
        for(String raw:text.split("\n")){
            Matcher m=CELL.matcher(raw.trim());if(!m.matches())continue;
            int row=parseInt(m.group(2));int col=columnIndex(m.group(1));
            rows.computeIfAbsent(row,k->new TreeMap<>()).put(col,m.group(3).trim());
        }
        ArrayList<List<String>>out=new ArrayList<>();
        for(TreeMap<Integer,String>r:rows.values()){
            int max=r.isEmpty()?0:r.lastKey();ArrayList<String>cells=new ArrayList<>();
            for(int c=0;c<=max;c++)cells.add(r.getOrDefault(c,""));out.add(cells);
        }
        return out;
    }
    private static List<List<String>> delimitedRows(String text){
        ArrayList<List<String>>out=new ArrayList<>();
        for(String raw:text.split("\n")){
            String line=raw.trim();if(line.isEmpty()||line.startsWith("— Sayfa"))continue;
            char sep=line.indexOf(';')>=0?';':(line.indexOf('\t')>=0?'\t':',');
            String[]parts=splitCsv(line,sep);if(parts.length<2)continue;
            ArrayList<String>r=new ArrayList<>();for(String p:parts)r.add(p.trim());out.add(r);
        }
        return out;
    }
    private static ArrayList<Item> parseRows(List<List<String>>rows,String source){
        ArrayList<Item>out=new ArrayList<>();if(rows.isEmpty())return out;
        int header=-1,poz=-1,desc=-1,unit=-1,qty=-1,price=-1,total=-1,discipline=-1;
        for(int r=0;r<Math.min(rows.size(),25);r++){
            List<String>cells=rows.get(r);int hits=0;
            for(int c=0;c<cells.size();c++){
                String n=MusaAiDrawingIndex.normalize(cells.get(c));
                if(isAny(n,"poz","poz no","poz numarasi","is kalemi no")){poz=c;hits++;}
                else if(isAny(n,"tanim","aciklama","imalat","malzeme","is kalemi","imalat tanimi")){desc=c;hits++;}
                else if(isAny(n,"birim")){unit=c;hits++;}
                else if(isAny(n,"miktar","metraj","miktari")){qty=c;hits++;}
                else if(isAny(n,"birim fiyat","fiyat")){price=c;hits++;}
                else if(isAny(n,"toplam","tutar","bedel")){total=c;hits++;}
                else if(isAny(n,"disiplin","bran s","brans")){discipline=c;hits++;}
            }
            if(hits>=2&&(desc>=0||poz>=0)){header=r;break;}
            poz=desc=unit=qty=price=total=discipline=-1;
        }
        if(header<0){
            for(List<String>cells:rows){
                if(cells.size()<3)continue;
                String p=get(cells,0),d=get(cells,1),u=get(cells,2);double q=parseNumber(get(cells,3));
                if(d.isEmpty()||!Double.isFinite(q))continue;
                out.add(new Item(p,d,u,q,parseNumber(get(cells,4)),parseNumber(get(cells,5)),MusaAiDiscipline.classify(d),source));
            }
            return out;
        }
        for(int r=header+1;r<rows.size();r++){
            List<String>cells=rows.get(r);String p=get(cells,poz),d=get(cells,desc),u=get(cells,unit);
            if(d.isEmpty()&&p.isEmpty())continue;
            double q=parseNumber(get(cells,qty)),pr=parseNumber(get(cells,price)),t=parseNumber(get(cells,total));
            if(!Double.isFinite(q)&&!Double.isFinite(pr)&&!Double.isFinite(t)&&d.length()<3)continue;
            MusaAiDiscipline disc=discipline>=0?MusaAiDiscipline.classify(get(cells,discipline)):MusaAiDiscipline.classify(d+" "+p);
            out.add(new Item(p,d.isEmpty()?p:d,u,q,pr,t,disc,source));
        }
        return out;
    }

    private static double score(Item a,Item b){
        if(a==null||b==null)return 0;
        if(!a.poz.isEmpty()&&!b.poz.isEmpty()&&MusaAiDrawingIndex.normalize(a.poz).equals(MusaAiDrawingIndex.normalize(b.poz)))return 1d;
        Set<String>ta=tokens(a.description),tb=tokens(b.description);if(ta.isEmpty()||tb.isEmpty())return 0;
        int common=0;for(String t:ta)if(tb.contains(t))common++;
        double s=common/(double)Math.max(ta.size(),tb.size());
        if(a.discipline!=MusaAiDiscipline.UNKNOWN&&a.discipline==b.discipline)s+=.18d;
        if(unitCompatible(a.unit,b.unit))s+=.08d;
        return Math.min(1d,s);
    }
    private static Set<String>tokens(String value){
        LinkedHashSet<String>out=new LinkedHashSet<>();for(String t:MusaAiDrawingIndex.normalize(value).split(" "))if(t.length()>2&&!isStop(t))out.add(t);return out;
    }
    private static boolean isStop(String t){return Arrays.asList("katman","adet","boru","hatti","tesisati","sistemi","imalati","proje").contains(t);}
    private static boolean unitCompatible(String a,String b){
        String x=normalizeUnit(a),y=normalizeUnit(b);return x.isEmpty()||y.isEmpty()||x.equals(y);
    }
    private static String normalizeUnit(String u){
        String n=MusaAiDrawingIndex.normalize(u).replace("metre","m").replace("metrekare","m2").replace("metre kare","m2");
        if(n.equals("mt")||n.equals("mtr"))n="m";if(n.equals("ad")||n.equals("adet"))n="adet";return n;
    }
    private static String label(Status s){switch(s){case QUANTITY_DIFF:return "MİKTAR FARKI";case UNIT_MISMATCH:return "BİRİM/TEKNİK UYUMSUZLUK";case MISSING_IN_ESTIMATE:return "PROJEDE VAR – KEŞİFTE YOK";case EXTRA_IN_ESTIMATE:return "KEŞİFTE VAR – PROJEDE EŞLEŞMEDİ";case REVIEW:return "İNCELEME GEREKLİ";default:return "UYUMLU";}}
    private static String[] splitCsv(String line,char sep){
        ArrayList<String>out=new ArrayList<>();StringBuilder b=new StringBuilder();boolean quote=false;
        for(int i=0;i<line.length();i++){char c=line.charAt(i);if(c=='"'){if(quote&&i+1<line.length()&&line.charAt(i+1)=='"'){b.append('"');i++;}else quote=!quote;}else if(c==sep&&!quote){out.add(b.toString());b.setLength(0);}else b.append(c);}out.add(b.toString());return out.toArray(new String[0]);
    }
    private static double parseNumber(String raw){
        if(raw==null)return Double.NaN;Matcher m=NUM.matcher(raw.replace("\u00A0"," "));if(!m.find())return Double.NaN;String s=m.group().replace(" ","").replace("'","").replace("’","");
        int comma=s.lastIndexOf(','),dot=s.lastIndexOf('.');
        if(comma>=0&&dot>=0){if(comma>dot)s=s.replace(".","").replace(',','.');else s=s.replace(",","");}
        else if(comma>=0)s=s.replace(".","").replace(',','.');
        try{return Double.parseDouble(s);}catch(Exception e){return Double.NaN;}
    }
    private static String get(List<String>cells,int i){return i>=0&&i<cells.size()?clean(cells.get(i)):"";}
    private static boolean isAny(String n,String...v){for(String s:v)if(n.equals(MusaAiDrawingIndex.normalize(s)))return true;return false;}
    private static int columnIndex(String letters){int n=0;for(char c:letters.toCharArray())n=n*26+(c-'A'+1);return n-1;}
    private static int parseInt(String s){try{return Integer.parseInt(s);}catch(Exception e){return 0;}}
    private static double finite(double v){return Double.isFinite(v)?v:Double.NaN;}
    private static String clean(String s){return s==null?"":s.trim();}
    private static String blank(String s,String f){return clean(s).isEmpty()?f:clean(s);}
    private static String number(double v){if(!Double.isFinite(v))return "—";return String.format(new Locale("tr","TR"),"%.3f",v).replaceAll("0+$","").replaceAll("[,.]$","");}
    private MusaAiEstimate(){}
}
