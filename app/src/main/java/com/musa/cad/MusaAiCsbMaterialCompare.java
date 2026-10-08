package com.musa.cad;

import java.text.DecimalFormat;
import java.text.DecimalFormatSymbols;
import java.util.*;
import java.util.regex.*;

/**
 * Auditable, conservative comparison between an uploaded material/BOQ list
 * and source-linked sanitary quantities measured from the active DWG.
 *
 * Matching is intentionally NOT a fuzzy search: network + material + DN and
 * unit must all agree for pipe rows; equipment name + count unit for fixtures.
 * Ambiguous entries cannot be claimed as differences or priced automatically.
 */
public final class MusaAiCsbMaterialCompare {
    public static final class Result {
        public final int matched,differences,ambiguous,unmatchedSchedule,unlistedDrawing;
        public final String report;
        private Result(int matched,int differences,int ambiguous,
                       int unmatchedSchedule,int unlistedDrawing,String report){
            this.matched=matched;this.differences=differences;
            this.ambiguous=ambiguous;this.unmatchedSchedule=unmatchedSchedule;
            this.unlistedDrawing=unlistedDrawing;this.report=report;
        }
    }

    private static final Pattern DN=Pattern.compile("(?:^|\\s)(?:dn|cap|capi|ø)\\s*(\\d{2,4})(?=$|\\s)");
    private static final int MAX_ITEMS=1000;
    private static final int MAX_LINES=45;

    public static Result compare(MusaAiBoq.Model list,MusaAiCsbEstimate.Result drawing){
        if(list==null||list.rows.isEmpty()){
            return new Result(0,0,0,0,drawing==null?0:drawing.rows.size(),
                "\n\nMALZEME LİSTESİ – DWG METRAJ KARŞILAŞTIRMASI\n"+
                "Proje malzeme/keşif cetveli yüklenmedi. “Keşif yükle” ile Excel/CSV cetveli ilişkilendirin.\n"+
                "DWG'deki tekil notlar onaylı malzeme listesi sayılmaz.");
        }
        if(drawing==null||drawing.rows.isEmpty()){
            return new Result(0,0,0,list.rows.size(),0,
                "\n\nMALZEME LİSTESİ – DWG METRAJ KARŞILAŞTIRMASI\n"+
                "Güvenilir DWG uzunluk/adet kanıtı bulunamadı. Yüklenen cetvele ait miktarlar doğrulanamadı.");
        }
        int matched=0,differ=0,ambiguous=0,unmatched=0,reported=0;
        boolean[] used=new boolean[drawing.rows.size()];
        StringBuilder details=new StringBuilder();
        int scanned=0;
        for(MusaAiBoq.Row entry:list.rows){
            if(entry==null)continue;
            if(++scanned>MAX_ITEMS){ambiguous++;break;}
            int selected=-1,candidates=0;
            for(int i=0;i<drawing.rows.size();i++){
                if(matches(entry,drawing.rows.get(i))){
                    candidates++;
                    selected=i;
                }
            }
            if(candidates!=1){
                if(candidates==0)unmatched++;
                else ambiguous++;
                if(reported++<MAX_LINES)details.append("\n• ").append(label(entry))
                    .append(candidates==0?" — DWG kalemi kesin eşleşmedi.":" — Birden fazla DWG adayı; belirsiz.");
                continue;
            }
            MusaAiCsbEstimate.Row project=drawing.rows.get(selected);
            if(used[selected]){
                ambiguous++;
                if(reported++<MAX_LINES)details.append("\n• ").append(label(entry))
                    .append(" — Aynı DWG kalemi birden çok cetvel satırıyla eşleşiyor.");
                continue;
            }
            used[selected]=true;
            matched++;
            double diff=project.quantity-entry.quantity;
            double pct=Math.abs(entry.quantity)>1e-9?
                Math.abs(diff)*100d/Math.abs(entry.quantity):
                (Math.abs(diff)<=1e-9?0d:100d);
            if(pct>1d)differ++;
            if(reported++<MAX_LINES){
                details.append("\n• ").append(label(entry))
                    .append(" • Cetvel ").append(fmt(entry.quantity)).append(" ").append(entry.unit)
                    .append(" / DWG ").append(fmt(project.quantity)).append(" ").append(project.unit)
                    .append(" • fark ").append(fmt(diff)).append(" ").append(project.unit)
                    .append(" (").append(fmt(pct)).append("%)");
                if(!project.sourceIds.isEmpty())
                    details.append(" [kaynak ").append(project.sourceIds.get(0)).append("]");
                if(pct>1d)details.append(" • KONTROL ET");
            }
        }
        int unlisted=0;for(boolean x:used)if(!x)unlisted++;
        StringBuilder report=new StringBuilder("\n\n========== MALZEME CETVELİ – DWG METRAJ KARŞILAŞTIRMASI ==========");
        report.append("\nCetvel: ").append(list.name)
            .append("\nTekil kalem eşleşmesi: ").append(matched)
            .append("\n>%1 miktar farkı: ").append(differ)
            .append("\nBirden fazla eşleşme/tekrar: ").append(ambiguous)
            .append("\nCetvelde olup DWG ile eşleşmeyen: ").append(unmatched)
            .append("\nDWG metrajında olup cetvelde eşleşmeyen: ").append(unlisted)
            .append(details);
        if(scanned>MAX_ITEMS)report.append("\n• İlk 1000 cetvel satırı incelendi; tüm cetvel kapsamı KISMİ.");
        report.append("\nÖNEMLİ: %1 yalnız farkı görünür kılma eşiğidir; sözleşmesel veya idari tolerans değildir.");
        report.append(" Çap, malzeme, tesisat türü veya ölçü birimi belirsiz kalemler zorla eşleştirilmez.");
        report.append(" Şemalardaki plan tekrarları ve eksik boru bağlantı parçaları ayrıca kontrol edilmelidir.");
        return new Result(matched,differ,ambiguous,unmatched,unlisted,report.toString());
    }

    private static boolean matches(MusaAiBoq.Row schedule,MusaAiCsbEstimate.Row project){
        if(!Double.isFinite(schedule.quantity)||schedule.quantity<0d)return false;
        if(project==null||!Double.isFinite(project.quantity)||project.quantity<0d)return false;
        String desc=norm(schedule.description),value=norm(project.description);
        if(desc.isEmpty()||value.isEmpty())return false;
        if("BORU".equals(project.type)){
            if(!isMeter(schedule.unit))return false;
            int projectDn=diameter(value),scheduleDn=diameter(desc);
            if(projectDn<=0||scheduleDn!=projectDn)return false;
            String mat=material(value);
            if(mat.isEmpty()||!mat.equals(material(desc)))return false;
            String network=network(value);
            return !network.isEmpty()&&network.equals(network(desc));
        }
        if("CIHAZ".equals(project.type)){
            if(!isCount(schedule.unit))return false;
            // Only exact component words, not compound/substring guesses.
            return fixture(value).equals(fixture(desc))&&!fixture(value).isEmpty();
        }
        return false;
    }

    private static int diameter(String s){
        Matcher m=DN.matcher(s);
        if(!m.find())return -1;
        try{return Integer.parseInt(m.group(1));}catch(NumberFormatException e){return -1;}
    }
    private static String material(String s){
        if(has(s,"pprc","ppr","polipropilen"))return "pprc";
        if(has(s,"pvc","upvc"))return "pvc";
        if(has(s,"hdpe","pe100","polietilen"))return "pe";
        if(has(s,"celik","galvaniz"))return "celik";
        if(has(s,"bakir","copper"))return "bakir";
        return "";
    }
    private static String network(String s){
        if(has(s,"yagmur suyu","yagmursuyu"))return "yagmur";
        if(has(s,"pis su","pissu","atik su","atiksu","kanalizasyon"))return "pis";
        if(has(s,"sicak su"))return "sicak";
        if(has(s,"sirkulasyon"))return "sirkulasyon";
        if(has(s,"temiz su","temizsu","soguk su","kullanma suyu"))return "temiz";
        return "";
    }
    private static String fixture(String s){
        for(String word:new String[]{"lavabo","klozet","pisuvar","suzgec","evye",
            "batarya","hidrofor","boyler","vana"}){
            if(has(s,word))return word;
        }
        return "";
    }
    private static boolean has(String hay,String... needles){
        String padded=" "+hay+" ";
        for(String x:needles){
            String n=norm(x);
            if(padded.contains(" "+n+" "))return true;
        }
        return false;
    }
    private static boolean isMeter(String s){
        String n=norm(s);return n.equals("m")||n.equals("metre");
    }
    private static boolean isCount(String s){
        String n=norm(s);return n.equals("ad")||n.equals("adet");
    }
    private static String norm(String s){return MusaAiDrawingIndex.normalize(s);}
    private static String label(MusaAiBoq.Row x){
        return x.code.isEmpty()?x.description:x.code+" • "+x.description;
    }
    private static String fmt(double x){
        return new DecimalFormat("#,##0.##",
            DecimalFormatSymbols.getInstance(new Locale("tr","TR"))).format(x);
    }
    private MusaAiCsbMaterialCompare(){}
}
