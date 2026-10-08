package com.musa.cad;

import java.text.DecimalFormat;
import java.text.DecimalFormatSymbols;
import java.util.*;
import java.util.regex.*;

/**
 * Evidence-first sanitary plumbing material takeoff + optional ÇŞİDB price bridge.
 *
 * Does NOT invent public work-item codes or prices. A price may only be applied
 * from a caller-supplied, verified, date-scoped catalog row whose exact technical
 * spec and measurement unit match the evidence. Values are advisory; DWG layers
 * and source text alone do not establish pipe centerline or installation.
 */
public final class MusaAiCsbEstimate {
    public static final class Rate {
        public final String itemKey,code,description,unit,period,source;
        public final double installedUnitPrice;
        public final boolean userVerifiedInstalledPrice;
        public Rate(String itemKey,String code,String description,String unit,
                    double installedUnitPrice,String period,String source,boolean userVerifiedInstalledPrice) {
            this.itemKey=clean(itemKey);this.code=clean(code);this.description=clean(description);
            this.unit=clean(unit);this.installedUnitPrice=installedUnitPrice;
            this.period=clean(period);this.source=clean(source);
            this.userVerifiedInstalledPrice=userVerifiedInstalledPrice;
        }
        public boolean usable() {
            return userVerifiedInstalledPrice&&!code.isEmpty()&&!period.isEmpty()&&!source.isEmpty()&&
                Double.isFinite(installedUnitPrice)&&installedUnitPrice>=0d;
        }
    }

    public static final class Row {
        public final String type,itemKey,description,unit,matchedPoz,pricePeriod,priceSource;
        public final double quantity,unitPrice,amount;
        public final List<Integer> sourceIds;
        public final boolean priced,verifiedSpecification;
        private Row(String type,String itemKey,String description,String unit,double quantity,
                    List<Integer> sourceIds,boolean verifiedSpecification,Rate rate){
            this.type=type;this.itemKey=itemKey;this.description=description;
            this.unit=unit;this.quantity=quantity;
            this.sourceIds=Collections.unmodifiableList(new ArrayList<>(sourceIds));
            this.verifiedSpecification=verifiedSpecification;
            this.priced=verifiedSpecification&&rate!=null&&rate.usable()&&
                norm(rate.itemKey).equals(norm(itemKey))&&unit.equalsIgnoreCase(rate.unit);
            this.matchedPoz=priced?rate.code:"";
            this.pricePeriod=priced?rate.period:"";
            this.priceSource=priced?rate.source:"";
            this.unitPrice=priced?rate.installedUnitPrice:Double.NaN;
            this.amount=priced?quantity*unitPrice:Double.NaN;
        }
    }

    public static final class Result {
        public final List<Row> rows;
        public final List<String> warnings;
        public final int pricedRows,unpricedRows,sourceCount;
        public final double pricedSubtotal,completeTotal;
        public final String report;
        private Result(List<Row> rows,List<String> warnings,int sourceCount,String report){
            this.rows=Collections.unmodifiableList(new ArrayList<>(rows));
            this.warnings=Collections.unmodifiableList(new ArrayList<>(warnings));
            this.sourceCount=sourceCount;
            int priced=0;double subtotal=0d;
            for(Row row:rows)if(row.priced){priced++;subtotal+=row.amount;}
            this.pricedRows=priced;
            this.unpricedRows=rows.size()-priced;
            this.pricedSubtotal=subtotal;
            this.completeTotal=unpricedRows==0&&!rows.isEmpty()?subtotal:Double.NaN;
            this.report=report;
        }
    }

    private static final class Group {
        final String type,key,description,unit;
        final boolean verifiedSpec;
        final LinkedHashSet<Integer> sourceIds=new LinkedHashSet<>();
        double qty;
        Group(String type,String key,String description,String unit,boolean verifiedSpec){
            this.type=type;this.key=key;this.description=description;this.unit=unit;this.verifiedSpec=verifiedSpec;
        }
        void add(double amount,int sourceId){
            qty+=amount;
            if(sourceId>=0&&sourceIds.size()<30)sourceIds.add(sourceId);
        }
    }

    private static final Pattern DN=Pattern.compile("(?:^|[^a-z0-9])(?:dn|cap|capi|diameter|d|ø)\\s*[-_:=]?\\s*(\\d{2,4})(?=$|[^0-9])");
    private static final int MAX_ENTITIES=25000;
    private static final int MAX_GROUPS=180;

    public static Result analyze(MusaAiDrawingIndex drawing,Collection<Rate> verifiedCatalog){
        ArrayList<String> warnings=new ArrayList<>();
        if(drawing==null)return finish(Collections.emptyList(),
            Collections.singletonList("DWG vektör verisi yok; metraj ve fiyat hesaplanmadı."),0);
        double factor=metersPerDrawingUnit(drawing.unitName);
        boolean unitsValid=Double.isFinite(factor)&&factor>0d;
        if(!unitsValid)warnings.add("Çizim birimi/ölçek doğrulanamadı: boru metre metrajları üretilmedi.");
        LinkedHashMap<String,Group> groups=new LinkedHashMap<>();
        HashSet<Integer> uniqueIds=new HashSet<>();
        int examined=0,unknown=0,duplicate=0,unclassified=0;
        for(MusaAiDrawingIndex.Item item:drawing.items()){
            if(item==null)continue;
            if(++examined>MAX_ENTITIES){warnings.add("25.000 CAD nesnesi sınırına ulaşıldı; metraj KISMİ.");break;}
            // A source entity can appear in several sampled indexes. Do not bill twice.
            if(item.sourceId>=0&&!uniqueIds.add(item.sourceId)){duplicate++;continue;}
            final String type=clean(item.type).toUpperCase(Locale.ROOT);
            final String combined=norm(item.layer+" "+item.text);
            String network=network(combined);
            if(isPipeLine(type)&&item.hasLength()){
                if(network.isEmpty()){unclassified++;continue;}
                if(!unitsValid)continue;
                final int diameter=diameter(combined);
                final String material=material(combined);
                final boolean specKnown=diameter>0&&!material.isEmpty();
                final String specification=network+" | "+
                    (material.isEmpty()?"malzeme?":material)+" | "+
                    (diameter>0?"DN"+diameter:"çap?");
                final String key="BORU|"+specification;
                Group group=groups.get(key);
                if(group==null){
                    if(groups.size()>=MAX_GROUPS){warnings.add("180 farklı metraj satırı sınırında duruldu.");break;}
                    group=new Group("BORU",key,specification,"m",specKnown);
                    groups.put(key,group);
                }
                group.add(item.length*factor*Math.max(1,item.quantity),item.sourceId);
            }else if(isBlock(type)){
                String fixture=fixture(combined);
                if(fixture.isEmpty()){unclassified++;continue;}
                String key="CIHAZ|"+fixture;
                Group group=groups.get(key);
                if(group==null){
                    if(groups.size()>=MAX_GROUPS){warnings.add("180 farklı metraj satırı sınırında duruldu.");break;}
                    group=new Group("CIHAZ",key,fixture,"Ad",false);
                    groups.put(key,group);
                }
                group.add(Math.max(1,item.quantity),item.sourceId);
            }else if((type.equals("LINE")||type.equals("LWPOLYLINE")||type.equals("POLYLINE"))&&
                    !item.hasLength()&&!network.isEmpty())unknown++;
        }
        if(duplicate>0)warnings.add(duplicate+" yinelenen kaynak kimliği tekrar metraja alınmadı.");
        if(unknown>0)warnings.add(unknown+" tesisat çizgisinin uzunluk kanıtı eksik; hesaplanmadı.");
        if(unclassified>0)warnings.add(unclassified+" geometrik/blok öğesi tesisat kalemi olarak güvenilir sınıflandırılamadı.");
        warnings.add("Çizgi orta eksen borusunu temsil etmeyebilir; paralel görünüş çizgileri, şemalar ve plan/kesit tekrarları manuel kontrol edilmelidir.");
        warnings.add("Düşey boru uzunlukları, branşmanlar, fittings, vanalar, askılar, yalıtım ve testler ayrıca poz tarifi/kapsamına göre kontrol edilmelidir.");
        warnings.add("Metraj çizimin aktif görünür düzeninden türetilir; diğer layout/harici referans ve eksik paftalar otomatik dahil değildir.");
        Map<String,Rate> rates=new HashMap<>();
        if(verifiedCatalog!=null)for(Rate rate:verifiedCatalog){
            if(rate!=null&&rate.usable())rates.put(norm(rate.itemKey),rate);
        }
        ArrayList<Row> rows=new ArrayList<>();
        for(Group group:groups.values()){
            Rate rate=rates.get(norm(group.key));
            rows.add(new Row(group.type,group.key,group.description,group.unit,group.qty,
                new ArrayList<>(group.sourceIds),group.verifiedSpec,rate));
        }
        rows.sort((a,b)->a.type.equals(b.type)?a.description.compareTo(b.description):
            a.type.compareTo(b.type));
        return finish(rows,warnings,examined);
    }

    private static Result finish(List<Row> rows,List<String> warnings,int sourceCount){
        int priced=0;double subtotal=0d;
        StringBuilder s=new StringBuilder("\n\n========== MALZEME METRAJ / ÇŞİDB POZ KEŞİF ÖN RAPORU ==========");
        s.append("\nCAD nesne örneklemi: ").append(sourceCount);
        s.append("\nÇizimden çıkarılan kalem: ").append(rows.size());
        for(Row r:rows)if(r.priced){priced++;subtotal+=r.amount;}
        s.append("\nBirim fiyatı doğrulanmış kalem: ").append(priced).append("/").append(rows.size());
        for(int i=0;i<Math.min(40,rows.size());i++){
            Row r=rows.get(i);
            s.append("\n• ").append(r.description).append(" = ").append(fmt(r.quantity))
                .append(" ").append(r.unit)
                .append(" [poz eşleştirme anahtarı: ").append(r.itemKey).append("]");
            if(!r.sourceIds.isEmpty())s.append(" [DWG kaynak ").append(r.sourceIds.get(0)).append("]");
            if(!r.verifiedSpecification)s.append(" [teknik özellik/poz eşleştirmesi teyitsiz]");
            if(r.priced)s.append(" • Poz ").append(r.matchedPoz).append(" (").append(r.pricePeriod)
                .append(") × ").append(fmt(r.unitPrice)).append(" TL = ").append(fmt(r.amount))
                .append(" TL [resmî kaynak: ").append(r.priceSource).append("]");
            else s.append(" • ÇŞİDB poz ve fiyatı doğrulanmadı");
        }
        if(rows.size()>40)s.append("\n• Diğer kalemler çıktı sınırı nedeniyle burada gösterilmedi.");
        if(priced>0)s.append("\nDoğrulanmış fiyatlı satırların ara toplamı: ").append(fmt(subtotal)).append(" TL");
        if(rows.isEmpty())s.append("\nGüvenilir boru/cihaz metrajı elde edilemedi.");
        if(priced<rows.size())s.append("\nGENEL TOPLAM HESAPLANMADI: fiyatı/kapsamı kesinleşmemiş iş kalemleri bulunmaktadır.");
        else if(!rows.isEmpty())s.append("\nFiyatlandırılmış çizim kalemleri toplamı: ").append(fmt(subtotal))
            .append(" TL. Bu, sözleşmesel yaklaşık maliyet onayı değildir.");
        s.append("\nNotlar:");
        for(String warning:warnings)s.append("\n• ").append(warning);
        return new Result(rows,warnings,sourceCount,s.toString());
    }

    private static String network(String n){
        if(has(n,"yagmur suyu","yagmursuyu","rainwater"))return "Yağmur suyu";
        if(has(n,"pis su","pissu","atik su","atiksu","kanalizasyon","wastewater"))return "Pis su";
        if(has(n,"sicak su","sıcaksu","hot water"))return "Sıcak su";
        if(has(n,"sirkulasyon","sirkülasyon","circulation"))return "Sirkülasyon";
        if(has(n,"temiz su","temizsu","soguk su","soguksu","kullanma suyu","domestic water"))return "Temiz su";
        return "";
    }
    private static String material(String n){
        if(has(n,"pprc","ppr","polipropilen"))return "PPRC";
        if(has(n,"pvc","upvc"))return "PVC";
        if(has(n,"hdpe","pe100","polietilen"))return "PE";
        if(has(n,"celik","galvaniz"))return "Çelik";
        if(has(n,"bakir","copper"))return "Bakır";
        return "";
    }
    private static String fixture(String n){
        if(has(n,"lavabo"))return "Lavabo";
        if(has(n,"klozet","water closet"))return "Klozet";
        if(has(n,"pisuvar"))return "Pisuvar";
        if(has(n,"suzgec","floor drain"))return "Yer süzgeci";
        if(has(n,"evye"))return "Evye";
        if(has(n,"batarya"))return "Batarya";
        if(has(n,"hidrofor"))return "Hidrofor";
        if(has(n,"boyler"))return "Boyler";
        if(has(n,"vana"))return "Vana";
        return "";
    }
    private static boolean has(String n,String... words){
        for(String word:words)if(n.contains(norm(word)))return true;return false;
    }
    private static boolean isPipeLine(String type){
        return type.equals("LINE")||type.equals("POLYLINE")||
            type.equals("LWPOLYLINE")||type.equals("ARC");
    }
    private static boolean isBlock(String type){
        return type.equals("BLOCK")||type.equals("INSERT")||type.equals("MINSERT");
    }
    private static int diameter(String n){
        Matcher match=DN.matcher(n);
        if(!match.find())return 0;
        try{int value=Integer.parseInt(match.group(1));return value>=10&&value<=2000?value:0;}
        catch(Exception e){return 0;}
    }
    private static double metersPerDrawingUnit(String unit){
        switch(clean(unit).toLowerCase(Locale.ROOT)){
            case "m":return 1d;case "mm":return 0.001d;case "cm":return 0.01d;
            case "km":return 1000d;case "ft":return 0.3048d;case "in":return 0.0254d;
            default:return Double.NaN;
        }
    }
    private static String clean(String value){return value==null?"":value.trim();}
    private static String norm(String value){return MusaAiDrawingIndex.normalize(value);}
    private static String fmt(double n){
        DecimalFormat df=new DecimalFormat("#,##0.##",
            DecimalFormatSymbols.getInstance(new Locale("tr","TR")));
        return df.format(n);
    }
    private MusaAiCsbEstimate(){}
}
