package com.musa.cad;

import java.text.Normalizer;
import java.util.*;
import java.util.regex.*;

/**
 * Conservative structural drawing <-> calculation-report comparison.
 *
 * This class compares only facts that can be recovered from visible CAD text
 * and extracted report text. It does not calculate structural capacity,
 * seismic performance or regulatory compliance.
 */
public final class MusaAiStructuralCalculation {
    public enum Status { MATCH, MISMATCH, REPORT_ONLY, DRAWING_ONLY, UNVERIFIED }

    public static final class Check {
        public final String field,drawingValue,reportValue;
        public final Status status;
        Check(String field,String drawingValue,String reportValue,Status status){
            this.field=field;this.drawingValue=drawingValue;this.reportValue=reportValue;this.status=status;
        }
    }

    public static final class Result {
        public final String reportName,text;
        public final List<Check> checks;
        public final int matched,mismatched,reportOnly,drawingOnly,unverified;
        private Result(String reportName,String text,List<Check>checks){
            this.reportName=clean(reportName);this.text=text;
            this.checks=Collections.unmodifiableList(new ArrayList<>(checks));
            int a=0,b=0,c=0,d=0,e=0;
            for(Check x:checks)switch(x.status){
                case MATCH:a++;break;case MISMATCH:b++;break;case REPORT_ONLY:c++;break;
                case DRAWING_ONLY:d++;break;default:e++;
            }
            matched=a;mismatched=b;reportOnly=c;drawingOnly=d;unverified=e;
        }
    }

    private static final Pattern CONCRETE=Pattern.compile("(?i)(?<![A-Z0-9])C\\s*\\d{2,3}(?![A-Z0-9])");
    private static final Pattern STEEL=Pattern.compile("(?i)(?<![A-Z0-9])(?:B|S)\\s*\\d{3}[A-Z]?(?![A-Z0-9])");
    private static final Pattern SECTION=Pattern.compile("(?i)(?<!\\d)\\d{2,4}\\s*[x×/]\\s*\\d{2,4}(?!\\d)");
    private static final Pattern REBAR=Pattern.compile("(?i)(?:Ø|Φ|ø|\\bfi\\s*)\\s*\\d{1,2}");
    private static final Pattern LEVEL=Pattern.compile("[+-]?\\s*\\d{1,3}[\\.,]\\d{1,3}");
    private static final Pattern ELEMENT=Pattern.compile("(?i)\\b(?:K|C|S|P|B)\\s*[-_.]?\\s*\\d{1,4}[A-Z]?\\b");

    public static Result compare(MusaAiDrawingIndex drawing,String reportName,String reportText){
        Facts dw=factsFromDrawing(drawing);
        Facts rp=factsFromText(reportText);
        ArrayList<Check>checks=new ArrayList<>();
        compareSet(checks,"Beton sınıfı",dw.concrete,rp.concrete,true);
        compareSet(checks,"Donatı çeliği sınıfı",dw.steel,rp.steel,true);
        compareSet(checks,"Kesit / eleman ebadı",dw.sections,rp.sections,false);
        compareSet(checks,"Donatı çapı",dw.rebars,rp.rebars,false);
        compareSet(checks,"Kot / seviye",dw.levels,rp.levels,false);
        compareSet(checks,"Eleman etiketi",dw.elements,rp.elements,false);

        compareMention(checks,"Temel sistemi",dw.foundation,rp.foundation);
        compareMention(checks,"Perde sistemi",dw.wall,rp.wall);
        compareMention(checks,"Kolon",dw.column,rp.column);
        compareMention(checks,"Kiriş",dw.beam,rp.beam);

        StringBuilder out=new StringBuilder();
        out.append("MUSACAD AI\nSTATİK PROJE ↔ HESAP RAPORU KARŞILAŞTIRMASI");
        out.append("\n========================================");
        out.append("\nHesap raporu: ").append(clean(reportName).isEmpty()?"Hesap raporu":clean(reportName));
        out.append("\n\n1. KARŞILAŞTIRMA ÖZETİ");
        int match=0,mismatch=0,ro=0,do_=0,uv=0;
        for(Check c:checks)switch(c.status){
            case MATCH:match++;break;case MISMATCH:mismatch++;break;case REPORT_ONLY:ro++;break;
            case DRAWING_ONLY:do_++;break;default:uv++;
        }
        out.append("\n• Uyumlu veri grubu: ").append(match);
        out.append("\n• Uyuşmazlık adayı: ").append(mismatch);
        out.append("\n• Yalnız hesap raporunda: ").append(ro);
        out.append("\n• Yalnız çizimde: ").append(do_);
        out.append("\n• Doğrulanamayan: ").append(uv);

        out.append("\n\n2. VERİ GRUBU BAZLI KONTROL");
        for(Check c:checks){
            out.append("\n\n• ").append(c.field).append(" — ").append(label(c.status));
            out.append("\n  Çizim: ").append(c.drawingValue);
            out.append("\n  Hesap: ").append(c.reportValue);
        }

        out.append("\n\n3. HESAP RAPORUNDA ARANAN İLAVE BAŞLIKLAR");
        appendPresence(out,reportText,"Zemin / geoteknik veri","zemin","geoteknik","soil");
        appendPresence(out,reportText,"Deprem / spektrum parametreleri","deprem","spektrum","sds","sd1");
        appendPresence(out,reportText,"Yük ve yük kombinasyonları","yuk kombinasyon","yük kombinasyon","load combination","g+q");
        appendPresence(out,reportText,"Modal / periyot bilgisi","modal","periyot","period","mod ");
        appendPresence(out,reportText,"Taşıyıcı sistem / R-D katsayıları","tasiyici sistem","taşıyıcı sistem","davranis katsay","davranış katsay"," r="," d=");

        out.append("\n\n4. SINIRLAR");
        out.append("\n• Eşleştirme, görünür DWG/DXF metinleri ile belgeden çıkarılabilen metin verisine dayanır.");
        out.append("\n• Aynı değerlerin iki belgede bulunması, eleman bazında birebir hesap uygunluğu veya statik güvenlik onayı anlamına gelmez.");
        out.append("\n• Eleman kuvvetleri, donatı yeterliliği, deplasman, düzensizlik, temel/zemin yeterliliği ve deprem performansı için hesap modelinin yapılandırılmış verileri ayrıca incelenmelidir.");
        out.append("\n• Uyuşmazlık adayları proje müellifi/kontrol mühendisi tarafından kaynak pafta ve hesap çıktısından doğrulanmalıdır.");
        return new Result(reportName,out.toString(),checks);
    }

    private static final class Facts{
        final LinkedHashSet<String> concrete=new LinkedHashSet<>(),steel=new LinkedHashSet<>(),
            sections=new LinkedHashSet<>(),rebars=new LinkedHashSet<>(),levels=new LinkedHashSet<>(),
            elements=new LinkedHashSet<>();
        boolean foundation,wall,column,beam;
    }

    private static Facts factsFromDrawing(MusaAiDrawingIndex index){
        Facts f=new Facts();if(index==null)return f;
        for(MusaAiDrawingIndex.Item item:index.items()){
            if(item==null)continue;
            String raw=(item.layer+" "+item.text).trim();
            String q=norm(raw);
            boolean structural=MusaAiDiscipline.classify(raw)==MusaAiDiscipline.STRUCTURAL||
                containsAny(q,"kolon","kiris","perde","temel","radye","kazik","donati","betonarme");
            if(!structural)continue;
            collect(raw,CONCRETE,f.concrete,20);collect(raw,STEEL,f.steel,20);
            collect(raw,SECTION,f.sections,80);collect(raw,REBAR,f.rebars,40);
            if(containsAny(q,"kot","seviye","level","elevation"))collect(raw,LEVEL,f.levels,40);
            collect(raw,ELEMENT,f.elements,80);
            f.foundation|=containsAny(q,"temel","radye","kazik","foundation","pile");
            f.wall|=containsAny(q,"perde","shear wall");
            f.column|=containsAny(q,"kolon","column");
            f.beam|=containsAny(q,"kiris","beam");
        }
        return f;
    }

    private static Facts factsFromText(String text){
        Facts f=new Facts();String raw=text==null?"":text;String q=norm(raw);
        collect(raw,CONCRETE,f.concrete,20);collect(raw,STEEL,f.steel,20);
        collect(raw,SECTION,f.sections,120);collect(raw,REBAR,f.rebars,60);
        collect(raw,LEVEL,f.levels,60);collect(raw,ELEMENT,f.elements,120);
        f.foundation=containsAny(q,"temel","radye","kazik","foundation","pile");
        f.wall=containsAny(q,"perde","shear wall");
        f.column=containsAny(q,"kolon","column");
        f.beam=containsAny(q,"kiris","beam");
        return f;
    }

    private static void compareSet(List<Check>out,String field,Set<String>dw,Set<String>rp,boolean strict){
        String a=show(dw),b=show(rp);
        if(dw.isEmpty()&&rp.isEmpty()){out.add(new Check(field,a,b,Status.UNVERIFIED));return;}
        if(dw.isEmpty()){out.add(new Check(field,a,b,Status.REPORT_ONLY));return;}
        if(rp.isEmpty()){out.add(new Check(field,a,b,Status.DRAWING_ONLY));return;}
        LinkedHashSet<String>common=new LinkedHashSet<>(dw);common.retainAll(rp);
        if(strict){
            out.add(new Check(field,a,b,dw.equals(rp)?Status.MATCH:Status.MISMATCH));
        }else{
            out.add(new Check(field,a,b,common.isEmpty()?Status.MISMATCH:Status.MATCH));
        }
    }

    private static void compareMention(List<Check>out,String field,boolean dw,boolean rp){
        Status s=dw&&rp?Status.MATCH:dw?Status.DRAWING_ONLY:rp?Status.REPORT_ONLY:Status.UNVERIFIED;
        out.add(new Check(field,dw?"var":"bulunamadı",rp?"var":"bulunamadı",s));
    }

    private static void appendPresence(StringBuilder out,String raw,String title,String...terms){
        String q=norm(raw);boolean found=containsAny(q,terms);
        out.append("\n• ").append(title).append(": ").append(found?"metin ipucu bulundu":"otomatik metin taramasında bulunamadı");
    }

    private static void collect(String raw,Pattern p,Set<String>out,int max){
        Matcher m=p.matcher(raw==null?"":raw);
        while(m.find()&&out.size()<max){
            String v=m.group().replaceAll("\\s+","").toUpperCase(new Locale("tr","TR"));
            if(!v.isEmpty())out.add(v);
        }
    }

    private static String show(Set<String>s){
        if(s.isEmpty())return "bulunamadı";
        StringBuilder b=new StringBuilder();int n=0;
        for(String v:s){if(n++>=12){b.append(", …");break;}if(b.length()>0)b.append(", ");b.append(v);}
        return b.toString();
    }
    private static String label(Status s){
        switch(s){
            case MATCH:return "UYUMLU";
            case MISMATCH:return "UYUŞMAZLIK ADAYI";
            case REPORT_ONLY:return "YALNIZ HESAP RAPORUNDA";
            case DRAWING_ONLY:return "YALNIZ ÇİZİMDE";
            default:return "DOĞRULANAMADI";
        }
    }
    private static boolean containsAny(String q,String...terms){
        for(String t:terms)if(q.contains(norm(t)))return true;return false;
    }
    private static String norm(String s){
        if(s==null)return "";
        String x=s.toLowerCase(new Locale("tr","TR")).replace('ı','i').replace('ğ','g').replace('ü','u').replace('ş','s').replace('ö','o').replace('ç','c');
        return Normalizer.normalize(x,Normalizer.Form.NFD).replaceAll("\\p{M}+","").replaceAll("[^a-z0-9+]+"," ").trim().replaceAll("\\s+"," ");
    }
    private static String clean(String s){return s==null?"":s.trim();}
    private MusaAiStructuralCalculation(){}
}
