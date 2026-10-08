package com.musa.cad;

import java.net.URI;
import java.nio.charset.StandardCharsets;
import java.text.DecimalFormat;
import java.text.DecimalFormatSymbols;
import java.util.*;

/**
 * Imports a MANUALLY CURATED correspondence CSV from an official ÇŞİDB YFK
 * price source. Raw YFK PDFs/XLS files do not natively contain the MusaCAD
 * itemKey and must be explicitly matched and reviewed before import.
 *
 * Format (UTF-8 semicolon):
 * item_key;poz;aciklama;birim;birim_fiyat;donem;kaynak;fiyat_kapsami
 * ...
 * BORU|Pis su | PVC | DN100;XX.YY;...;m;123,45;2026-10;https://yfk.csb.gov.tr/...;malzeme+montaj
 *
 * A parse result is NEVER trusted pricing. Caller must separately ask for
 * explicit user approval. The import itself cannot validate the PDF or the
 * official price position; the approval is an auditable user assertion only.
 */
public final class MusaAiCsbPriceCsv {
    public static final class Draft {
        public final List<MusaAiCsbEstimate.Rate> rates;
        public final List<String> warnings;
        public final String period,origin;
        public final String summary;
        private Draft(List<MusaAiCsbEstimate.Rate> rates,List<String> warnings,String period,String origin){
            this.rates=Collections.unmodifiableList(new ArrayList<>(rates));
            this.warnings=Collections.unmodifiableList(new ArrayList<>(warnings));
            this.period=period;this.origin=origin;
            StringBuilder summary=new StringBuilder("ÇŞİDB POZ / BİRİM FİYAT EŞLEŞTİRME CSV ÖN İNCELEMESİ");
            summary.append("\nDönem: ").append(period)
                .append("\nPoz ve teknik tarife adayları: ").append(rates.size())
                .append("\nGeçersiz/atlanmış satır uyarısı: ").append(warnings.size())
                .append("\nKaynak: ").append(origin);
            for(int i=0;i<Math.min(6,rates.size());i++){
                MusaAiCsbEstimate.Rate r=rates.get(i);
                summary.append("\n• ").append(r.code).append(" – ").append(r.description)
                    .append(" = ").append(format(r.installedUnitPrice))
                    .append(" TL/").append(r.unit);
            }
            for(int i=0;i<Math.min(6,warnings.size());i++)
                summary.append("\nUYARI: ").append(warnings.get(i));
            summary.append("\nDosya onaylanana kadar hiçbir birim fiyat kullanılmaz.")
                .append("\nOnay, poz tarifinin ve fiyat döneminin resmî kaynakla karşılaştırılması sorumluluğunu kaldırmaz.");
            this.summary=summary.toString();
        }
        /** Only explicit user confirmation makes source-tracked rates available to the estimator. */
        public List<MusaAiCsbEstimate.Rate> approve(){
            ArrayList<MusaAiCsbEstimate.Rate> out=new ArrayList<>();
            for(MusaAiCsbEstimate.Rate r:rates){
                out.add(new MusaAiCsbEstimate.Rate(
                    r.itemKey,r.code,r.description,r.unit,r.installedUnitPrice,
                    r.period,r.source,true));
            }
            return Collections.unmodifiableList(out);
        }
    }

    private static final int MAX_BYTES=350_000,MAX_LINES=2000,MAX_COLUMN=1500;
    private static final String[] HEADER={
        "item key","poz","aciklama","birim","birim fiyat","donem","kaynak","fiyat kapsami"
    };

    public static Draft parse(String csv){
        ArrayList<MusaAiCsbEstimate.Rate> rows=new ArrayList<>();
        ArrayList<String> warnings=new ArrayList<>();
        if(csv==null||csv.getBytes(StandardCharsets.UTF_8).length>MAX_BYTES)
            return new Draft(rows,Collections.singletonList("CSV 350 KB sınırını aşıyor veya boş."),"","");
        String[] lines=csv.replace("\uFEFF","").split("\\r?\\n",-1);
        if(lines.length<2)return new Draft(rows,Collections.singletonList("Poz CSV'si başlık ve en az bir satır içermelidir."),"","");
        if(lines.length>MAX_LINES+2)warnings.add("2000 satır sınırı uygulandı; fiyat cetveli KISMİ.");
        List<String> head=parseLine(lines[0]);
        if(head.size()!=HEADER.length)return new Draft(rows,
            Collections.singletonList("Başlıklar tam 8 sütun olmalı: item_key;poz;aciklama;birim;birim_fiyat;donem;kaynak;fiyat_kapsami"),"","");
        for(int i=0;i<HEADER.length;i++){
            if(!norm(head.get(i)).equals(HEADER[i])){
                return new Draft(rows,Collections.singletonList("CSV sütunu beklenenden farklı: "+(i+1)+". sütun."),"","");
            }
        }
        HashSet<String> seen=new HashSet<>();
        String firstPeriod="",firstSource="";
        for(int i=1;i<Math.min(lines.length,MAX_LINES+1);i++){
            if(lines[i].trim().isEmpty())continue;
            List<String> cols=parseLine(lines[i]);
            if(cols.size()!=8){warnings.add("Satır "+(i+1)+": sütun sayısı geçersiz.");continue;}
            String key=cols.get(0).trim(),code=cols.get(1).trim(),
                desc=cols.get(2).trim(),unit=cols.get(3).trim(),
                priceRaw=cols.get(4).trim(),period=cols.get(5).trim(),
                link=cols.get(6).trim(),scope=norm(cols.get(7));
            if(key.isEmpty()||key.length()>160||code.isEmpty()||code.length()>55||
                desc.isEmpty()||desc.length()>250||!period.matches("20[2-9][0-9]-(0[1-9]|1[0-2])")||
                !isOfficial(link)||!scope.equals("malzeme montaj")||
                (!unit.equals("m")&&!unit.equals("Ad"))){
                warnings.add("Satır "+(i+1)+": kaynak, dönem, kapsam, birim veya tarife eksik/hatalı.");continue;
            }
            if(firstPeriod.isEmpty()){firstPeriod=period;firstSource=link;}
            if(!period.equals(firstPeriod)){
                warnings.add("Satır "+(i+1)+": aynı içe aktarmada karışık fiyat dönemleri reddedildi.");continue;
            }
            Double price=parsePrice(priceRaw);
            if(price==null){warnings.add("Satır "+(i+1)+": fiyat sayı olarak doğrulanamadı.");continue;}
            String id=norm(key)+"|"+unit.toLowerCase(Locale.ROOT);
            if(!seen.add(id)){warnings.add("Satır "+(i+1)+": mükerrer fiyat anahtarı.");continue;}
            // The boolean remains false until the user explicitly accepts the review dialog.
            rows.add(new MusaAiCsbEstimate.Rate(key,code,desc,unit,price,period,link,false));
        }
        if(rows.isEmpty())warnings.add("Kullanılabilir eşleştirme satırı bulunamadı.");
        return new Draft(rows,warnings,firstPeriod,firstSource);
    }

    private static List<String> parseLine(String line){
        ArrayList<String> result=new ArrayList<>();
        StringBuilder word=new StringBuilder();
        boolean quotes=false;
        for(int i=0;i<line.length();i++){
            char c=line.charAt(i);
            if(c=='"'){
                if(quotes&&i+1<line.length()&&line.charAt(i+1)=='"'){
                    word.append('"');i++;
                }else quotes=!quotes;
            }else if(c==';'&&!quotes){
                result.add(word.toString());word.setLength(0);
            }else word.append(c);
            if(word.length()>MAX_COLUMN)return Collections.emptyList();
        }
        if(quotes)return Collections.emptyList();
        result.add(word.toString());
        return result;
    }

    private static Double parsePrice(String value){
        String s=value.replace("\u00A0","").replace(" ","").trim();
        if(s.isEmpty()||!s.matches("\\d[\\d.,]*"))return null;
        if(s.matches("\\d{1,3}(\\.\\d{3})+(,\\d{1,2})?"))s=s.replace(".","").replace(",",".");
        else if(s.matches("\\d{1,3}(,\\d{3})+(\\.\\d{1,2})?"))s=s.replace(",","");
        else if(s.matches("\\d+[,][0-9]{1,2}"))s=s.replace(',','.');
        else if(!s.matches("\\d+(\\.\\d{1,2})?"))return null;
        try{double v=Double.parseDouble(s);return Double.isFinite(v)&&v>=0d&&v<=100000000d?v:null;}
        catch(NumberFormatException e){return null;}
    }

    static boolean isOfficial(String link){
        try{
            URI uri=new URI(link);
            if(!"https".equalsIgnoreCase(uri.getScheme()))return false;
            String host=uri.getHost();
            if(host==null)return false;
            host=host.toLowerCase(Locale.ROOT);
            return host.equals("yfk.csb.gov.tr")||host.equals("webdosya.csb.gov.tr");
        }catch(Exception e){return false;}
    }

    private static String norm(String s){return MusaAiDrawingIndex.normalize(s);}
    private static String format(double x){
        return new DecimalFormat("#,##0.00",
            DecimalFormatSymbols.getInstance(new Locale("tr","TR"))).format(x);
    }
    private MusaAiCsbPriceCsv(){}
}
