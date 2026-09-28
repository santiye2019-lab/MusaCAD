package com.musa.cad;

import java.text.DecimalFormat;
import java.text.DecimalFormatSymbols;
import java.util.*;

/**
 * Keşif / BOQ bridge for MusaCAD AI.
 *
 * Uploaded XLSX/CSV documents are converted to a common row model and can be
 * compared with a conservative drawing-derived takeoff. Matching is advisory:
 * no contractual or engineering-compliance decision is made automatically.
 */
public final class MusaAiBoq {
    public enum Metric { LENGTH, AREA, COUNT, UNKNOWN }

    public static final class Row {
        public final String code,description,unit,source;
        public final double quantity;
        public final Metric metric;
        public Row(String code,String description,String unit,double quantity,String source){
            this.code=clean(code);this.description=clean(description);this.unit=clean(unit);
            this.quantity=quantity;this.source=clean(source);this.metric=metricFromUnit(this.unit);
        }
        public String label(){return code.isEmpty()?description:code+" • "+description;}
    }

    public static final class Model {
        public final String name;
        public final List<Row> rows;
        public final List<String> warnings;
        private Model(String name,Collection<Row>rows,Collection<String>warnings){
            this.name=clean(name).isEmpty()?"Keşif":clean(name);
            this.rows=Collections.unmodifiableList(new ArrayList<>(rows));
            this.warnings=Collections.unmodifiableList(new ArrayList<>(warnings));
        }
        public boolean isEmpty(){return rows.isEmpty();}
    }

    public static final class Comparison {
        public final String text;
        public final int compared,different,unmatchedBoq,projectOnly;
        private Comparison(String text,int compared,int different,int unmatchedBoq,int projectOnly){
            this.text=text;this.compared=compared;this.different=different;
            this.unmatchedBoq=unmatchedBoq;this.projectOnly=projectOnly;
        }
    }

    private static final class Header {
        int row,code=-1,description=-1,unit=-1,quantity=-1;
        boolean usable(){return description>0&&unit>0&&quantity>0;}
    }

    private static final class Candidate {
        final Row row;final int index;final double score;
        Candidate(Row row,int index,double score){this.row=row;this.index=index;this.score=score;}
    }

    private MusaAiBoq(){}

    public static Model of(String name,Collection<Row>rows){
        return new Model(name,rows==null?Collections.emptyList():rows,Collections.emptyList());
    }

    public static Model parse(String name,CadDocumentSupport.Kind kind,String extracted){
        ArrayList<Row>rows=new ArrayList<>();ArrayList<String>warnings=new ArrayList<>();
        if(extracted==null||extracted.trim().isEmpty()){
            warnings.add("Belge metni boş veya okunamadı.");
            return new Model(name,rows,warnings);
        }
        List<CadSpreadsheetLayout.Sheet>sheets;
        if(kind==CadDocumentSupport.Kind.XLSX)sheets=CadSpreadsheetLayout.parseExtractedXlsx(extracted);
        else if(kind==CadDocumentSupport.Kind.CSV)sheets=CadSpreadsheetLayout.parseCsv(extracted);
        else if(kind==CadDocumentSupport.Kind.DOCX||kind==CadDocumentSupport.Kind.TEXT)
            sheets=CadSpreadsheetLayout.parseCsv(extracted);
        else{
            warnings.add("Bu belge biçimi satır bazlı keşif karşılaştırmasına uygun değil.");
            return new Model(name,rows,warnings);
        }
        for(CadSpreadsheetLayout.Sheet sheet:sheets)parseSheet(sheet,rows,warnings);
        if(rows.isEmpty())warnings.add("Poz / açıklama / birim / miktar başlıklarıyla okunabilir keşif satırı bulunamadı.");
        return new Model(name,rows,warnings);
    }

    public static Model generate(MusaAiDrawingIndex index,String name){
        ArrayList<Row>rows=new ArrayList<>();ArrayList<String>warnings=new ArrayList<>();
        if(index==null){warnings.add("Aktif çizim indeksi yok.");return new Model(name,rows,warnings);}
        LinkedHashMap<String,double[]>byLayer=new LinkedHashMap<>();
        for(MusaAiDrawingIndex.Item item:index.items()){
            if(item==null)continue;
            String layer=clean(item.layer);if(layer.isEmpty())layer="0";
            double[]s=byLayer.computeIfAbsent(layer,k->new double[3]);
            s[0]+=1d;if(item.hasLength())s[1]+=item.length;if(item.hasArea())s[2]+=item.area;
        }
        String linear=clean(index.unitName);
        String area=linear.isEmpty()?"":linear+"²";
        for(Map.Entry<String,double[]>e:byLayer.entrySet()){
            String layer=e.getKey();double[]s=e.getValue();
            if(s[1]>1e-9)rows.add(new Row("",layer+" • uzunluk",linear,s[1],"Çizim katmanı: "+layer));
            if(s[2]>1e-9)rows.add(new Row("",layer+" • alan",area,s[2],"Çizim katmanı: "+layer));
            rows.add(new Row("",layer+" • adet","adet",s[0],"Çizim katmanı: "+layer));
        }
        if(linear.isEmpty())warnings.add("Çizim birimi bilinmiyor; uzunluk/alan miktarları karşılaştırılırken birim dönüşümü yapılamaz.");
        warnings.add("Otomatik keşif katman bazlıdır; iş kalemi/poz eşleştirmesi kullanıcı veya proje standardına göre doğrulanmalıdır.");
        return new Model(name,rows,warnings);
    }

    public static Comparison compare(Model boq,Model project){
        if(boq==null||boq.rows.isEmpty())return new Comparison("Yüklü keşif satırı yok.",0,0,0,project==null?0:project.rows.size());
        if(project==null||project.rows.isEmpty())return new Comparison("Çizimden üretilebilen keşif satırı yok.",0,0,boq.rows.size(),0);

        boolean[]used=new boolean[project.rows.size()];
        int compared=0,different=0,unmatched=0;
        StringBuilder detail=new StringBuilder();
        for(Row b:boq.rows){
            Candidate best=bestCandidate(b,project.rows,used);
            if(best==null||best.score<0.38d){
                unmatched++;
                appendLine(detail,"• Eşleşmeyen keşif: "+b.label()+" • "+number(b.quantity)+" "+b.unit);
                continue;
            }
            used[best.index]=true;compared++;
            Double projectQty=convert(best.row.quantity,best.row.unit,b.unit);
            if(projectQty==null){
                different++;
                appendLine(detail,"• Birim incelemesi: "+b.label()+" • keşif "+number(b.quantity)+" "+b.unit+
                    " • proje "+number(best.row.quantity)+" "+best.row.unit);
                continue;
            }
            double diff=projectQty-b.quantity;
            double pct=Math.abs(b.quantity)>1e-9?Math.abs(diff/b.quantity)*100d:(Math.abs(projectQty)<=1e-9?0d:100d);
            if(pct>1.0d)different++;
            appendLine(detail,"• "+b.label()+" • keşif "+number(b.quantity)+" "+b.unit+
                " • proje "+number(projectQty)+" "+b.unit+" • fark "+signed(diff)+" ("+number(pct)+"%)"+
                (pct<=1.0d?" • fark ≤ %1":" • incele"));
        }
        int projectOnly=0;for(boolean u:used)if(!u)projectOnly++;

        StringBuilder out=new StringBuilder();
        out.append("PROJE – KEŞİF KARŞILAŞTIRMASI");
        out.append("\n• Keşif: ").append(boq.name);
        out.append("\n• Eşleştirilen satır: ").append(compared);
        out.append("\n• %1'den büyük fark / birim incelemesi: ").append(different);
        out.append("\n• Keşifte olup projeyle eşleşmeyen: ").append(unmatched);
        out.append("\n• Projede olup keşifle eşleşmeyen otomatik satır: ").append(projectOnly);
        if(detail.length()>0)out.append("\n\nİlk satırlar\n").append(detail);
        out.append("\n\nNot: Eşleştirme açıklama/katman metni ve birim türü üzerinden otomatik yapılır. %1 yalnız raporlama eşiğidir; sözleşme veya kabul toleransı değildir.");
        return new Comparison(out.toString(),compared,different,unmatched,projectOnly);
    }

    public static Model filterByDiscipline(Model model,MusaAiDisciplineControl.Discipline discipline){
        if(model==null||discipline==null)return new Model("Keşif",Collections.emptyList(),Collections.singletonList("Disiplin filtresi uygulanamadı."));
        ArrayList<Row>rows=new ArrayList<>();
        for(Row row:model.rows){
            String hay=row.code+" "+row.description+" "+row.source;
            if(MusaAiDisciplineControl.classifyText(hay).contains(discipline))rows.add(row);
        }
        ArrayList<String>warnings=new ArrayList<>();
        if(rows.isEmpty())warnings.add(discipline.label+" için otomatik sınıflandırılan keşif satırı bulunamadı.");
        return new Model(model.name+" • "+discipline.label,rows,warnings);
    }

    public static String disciplineCompatibility(Model boq,Model project,Collection<MusaAiDisciplineControl.Discipline>requested){
        if(requested==null||requested.isEmpty())return "";
        StringBuilder out=new StringBuilder("DİSİPLİN BAZLI PROJE – KEŞİF UYUM ÖZETİ");
        int shown=0;
        for(MusaAiDisciplineControl.Discipline d:requested){
            Model b=filterByDiscipline(boq,d),p=filterByDiscipline(project,d);
            if(b.rows.isEmpty()&&p.rows.isEmpty())continue;
            shown++;
            Comparison c=compare(b,p);
            out.append("\n• ").append(d.label)
               .append(": keşif ").append(b.rows.size())
               .append(" satır • proje ").append(p.rows.size())
               .append(" satır • eşleşen ").append(c.compared)
               .append(" • fark/inceleme ").append(c.different)
               .append(" • keşifte eşleşmeyen ").append(c.unmatchedBoq)
               .append(" • projede eşleşmeyen ").append(c.projectOnly);
        }
        if(shown==0)out.append("\n• Seçilen disiplinlerde otomatik sınıflandırılabilen proje/keşif satırı bulunamadı.");
        out.append("\nNot: Disiplin gruplaması poz açıklaması, katman adı ve kaynak metnindeki anahtar kelimelerle otomatik yapılır; sınıflandırılmamış satırlar ayrıca genel keşif karşılaştırmasında incelenmelidir.");
        return out.toString();
    }

    public static String summary(Model model){
        if(model==null)return "Keşif yüklenmedi.";
        StringBuilder out=new StringBuilder();
        out.append(model.name).append("\n• Okunan keşif satırı: ").append(model.rows.size());
        EnumMap<Metric,Integer>counts=new EnumMap<>(Metric.class);
        for(Row r:model.rows)counts.put(r.metric,counts.getOrDefault(r.metric,0)+1);
        if(!counts.isEmpty()){
            out.append("\n• Uzunluk: ").append(counts.getOrDefault(Metric.LENGTH,0));
            out.append(" • Alan: ").append(counts.getOrDefault(Metric.AREA,0));
            out.append(" • Adet: ").append(counts.getOrDefault(Metric.COUNT,0));
            out.append(" • Belirsiz: ").append(counts.getOrDefault(Metric.UNKNOWN,0));
        }
        if(!model.warnings.isEmpty()){
            out.append("\nUyarılar:");
            for(String w:model.warnings)out.append("\n• ").append(w);
        }
        int shown=0;
        for(Row r:model.rows){
            if(shown++>=6){out.append("\n• …");break;}
            out.append("\n• ").append(r.label()).append(" = ").append(number(r.quantity)).append(" ").append(r.unit);
        }
        return out.toString();
    }

    private static void parseSheet(CadSpreadsheetLayout.Sheet sheet,List<Row>out,List<String>warnings){
        if(sheet==null||sheet.isEmpty())return;
        Header h=findHeader(sheet);
        if(!h.usable()){
            warnings.add(sheet.title+": keşif başlıkları algılanamadı.");
            return;
        }
        for(int row=h.row+1;row<=sheet.rows;row++){
            String desc=sheet.valueAt(row,h.description);
            String unit=sheet.valueAt(row,h.unit);
            String qty=sheet.valueAt(row,h.quantity);
            String code=h.code>0?sheet.valueAt(row,h.code):"";
            if(clean(desc).isEmpty()&&clean(code).isEmpty())continue;
            Double value=parseNumber(qty);if(value==null)continue;
            out.add(new Row(code,desc,unit,value,sheet.title+" • satır "+row));
            if(out.size()>=5000){warnings.add("Keşif 5000 satır sınırında kesildi.");return;}
        }
    }

    private static Header findHeader(CadSpreadsheetLayout.Sheet sheet){
        Header best=new Header();
        int max=Math.min(sheet.rows,25);
        for(int row=1;row<=max;row++){
            Header h=new Header();h.row=row;
            for(int col=1;col<=sheet.columns;col++){
                String n=MusaAiDrawingIndex.normalize(sheet.valueAt(row,col));
                if(n.isEmpty())continue;
                if(h.code<0&&contains(n,"poz","poz no","kod","is kalemi no","sira no"))h.code=col;
                if(h.description<0&&contains(n,"aciklama","tanim","is kalemi","imalat","malzeme","description"))h.description=col;
                if(h.unit<0&&contains(n,"birim","unit"))h.unit=col;
                if(h.quantity<0&&contains(n,"miktar","metraj","quantity","qty","toplam miktar"))h.quantity=col;
            }
            if(h.usable())return h;
            int score=(h.description>0?1:0)+(h.unit>0?1:0)+(h.quantity>0?1:0)+(h.code>0?1:0);
            int bestScore=(best.description>0?1:0)+(best.unit>0?1:0)+(best.quantity>0?1:0)+(best.code>0?1:0);
            if(score>bestScore)best=h;
        }
        return best;
    }

    private static Candidate bestCandidate(Row boq,List<Row>project,boolean[]used){
        Candidate best=null;
        for(int i=0;i<project.size();i++){
            if(used[i])continue;Row p=project.get(i);
            if(boq.metric!=Metric.UNKNOWN&&p.metric!=Metric.UNKNOWN&&boq.metric!=p.metric)continue;
            double score=textScore(boq.description,p.description);
            if(!boq.code.isEmpty())score=Math.max(score,textScore(boq.code+" "+boq.description,p.description));
            if(boq.metric==p.metric&&boq.metric!=Metric.UNKNOWN)score+=0.12d;
            if(best==null||score>best.score)best=new Candidate(p,i,score);
        }
        return best;
    }

    private static double textScore(String a,String b){
        String na=stripMetricWords(MusaAiDrawingIndex.normalize(a));
        String nb=stripMetricWords(MusaAiDrawingIndex.normalize(b));
        if(na.isEmpty()||nb.isEmpty())return 0d;
        if(na.equals(nb))return 1d;
        double bonus=(na.contains(nb)||nb.contains(na))?0.28d:0d;
        Set<String>aa=tokens(na),bb=tokens(nb);if(aa.isEmpty()||bb.isEmpty())return bonus;
        int inter=0;for(String t:aa)if(bb.contains(t))inter++;
        int union=aa.size()+bb.size()-inter;
        return Math.min(1d,bonus+(union==0?0d:(double)inter/union));
    }

    private static String stripMetricWords(String s){
        return (" "+s+" ").replace(" uzunluk "," ").replace(" alani "," ").replace(" alan "," ")
            .replace(" adet "," ").replace(" miktar "," ").replace(" metraj "," ").trim().replaceAll("\\s+"," ");
    }

    private static Set<String>tokens(String s){
        LinkedHashSet<String>out=new LinkedHashSet<>();
        for(String t:s.split(" ")){
            if(t.length()<2||contains(t,"ve","ile","icin","kalemi","imalat","toplam"))continue;
            out.add(t);
        }
        return out;
    }

    private static Metric metricFromUnit(String unit){
        String u=unitKey(unit);
        if(u.isEmpty())return Metric.UNKNOWN;
        if(u.equals("adet")||u.equals("ad")||u.equals("pcs")||u.equals("piece")||u.equals("ea"))return Metric.COUNT;
        if(u.equals("m2")||u.equals("cm2")||u.equals("mm2")||u.equals("metrekare"))return Metric.AREA;
        if(u.equals("m")||u.equals("metre")||u.equals("cm")||u.equals("mm"))return Metric.LENGTH;
        return Metric.UNKNOWN;
    }

    private static Double convert(double quantity,String from,String to){
        Metric fm=metricFromUnit(from),tm=metricFromUnit(to);
        if(fm==Metric.UNKNOWN||tm==Metric.UNKNOWN)return clean(from).equalsIgnoreCase(clean(to))?quantity:null;
        if(fm!=tm)return null;
        if(fm==Metric.COUNT)return quantity;
        double base=quantity*factorToBase(from,fm);
        double target=factorToBase(to,tm);return target<=0d?null:base/target;
    }

    private static double factorToBase(String unit,Metric metric){
        String u=unitKey(unit);
        if(metric==Metric.LENGTH){
            if(u.equals("mm"))return 0.001d;if(u.equals("cm"))return 0.01d;return 1d;
        }
        if(metric==Metric.AREA){
            if(u.startsWith("mm"))return 0.000001d;if(u.startsWith("cm"))return 0.0001d;return 1d;
        }
        return 1d;
    }

    private static String unitKey(String unit){
        String raw=clean(unit).toLowerCase(Locale.ROOT)
            .replace("㎡","m2").replace("²","2").replace("^2","2").replace(" ","");
        return MusaAiDrawingIndex.normalize(raw).replace(" ","");
    }

    private static Double parseNumber(String raw){
        if(raw==null)return null;String s=raw.trim().replace("\u00A0","").replace(" ","");
        if(s.isEmpty())return null;
        if(s.matches("[-+]?\\d{1,3}(\\.\\d{3})+,\\d+"))s=s.replace(".","").replace(',','.');
        else if(s.matches("[-+]?\\d{1,3}(,\\d{3})+\\.\\d+"))s=s.replace(",","");
        else if(s.indexOf(',')>=0&&s.indexOf('.')<0)s=s.replace(',','.');
        else if(s.indexOf(',')>=0&&s.indexOf('.')>=0){
            if(s.lastIndexOf(',')>s.lastIndexOf('.'))s=s.replace(".","").replace(',','.');
            else s=s.replace(",","");
        }
        s=s.replaceAll("[^0-9+\\-.]","");
        try{return s.isEmpty()?null:Double.parseDouble(s);}catch(Exception e){return null;}
    }

    private static boolean contains(String q,String...terms){for(String t:terms)if(q.equals(t)||q.contains(t))return true;return false;}
    private static String clean(String v){return v==null?"":v.trim().replaceAll("\\s+"," ");}

    private static void appendLine(StringBuilder b,String line){
        int lines=0;for(int i=0;i<b.length();i++)if(b.charAt(i)=='\n')lines++;
        if(lines>=20)return;if(b.length()>0)b.append('\n');b.append(line);
    }

    private static String signed(double v){return (v>0?"+":"")+number(v);}
    private static String number(double v){
        DecimalFormatSymbols s=DecimalFormatSymbols.getInstance(new Locale("tr","TR"));
        return new DecimalFormat("#,##0.###",s).format(v);
    }
}
