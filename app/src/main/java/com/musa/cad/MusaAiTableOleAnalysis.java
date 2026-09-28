package com.musa.cad;

import java.util.*;

/** Offline analysis of drawing tables, legends and embedded OLE/Office content. */
public final class MusaAiTableOleAnalysis {
    public static final class Answer {
        public final boolean matched;
        public final String text;
        public final List<Integer> sourceIds;
        private Answer(boolean matched,String text,Collection<Integer>sourceIds){
            this.matched=matched;this.text=text==null?"":text;
            this.sourceIds=Collections.unmodifiableList(new ArrayList<>(sourceIds));
        }
        public static Answer none(){return new Answer(false,"",Collections.emptyList());}
    }

    private static final String[] TABLE_HEADERS={
        "poz","poz no","malzeme","aciklama","adet","birim","mahal","tip","model",
        "cap","uzunluk","miktar","kod","sira no","numara","sembol","marka"
    };

    public static Answer answer(MusaAiDrawingIndex index,String raw){
        if(index==null)return Answer.none();
        String q=MusaAiDrawingIndex.normalize(raw);
        if(q.isEmpty())return Answer.none();

        if(asksOle(q))return analyzeOle(index,q);
        if(asksLegend(q))return analyzeLegend(index);
        if(asksTable(q))return analyzeTable(index);
        return Answer.none();
    }

    private static Answer analyzeOle(MusaAiDrawingIndex index,String q){
        List<MusaAiDrawingIndex.OleItem>oles=index.oleItems();
        String term=extractOleSearchTerm(q);
        boolean search=!term.isEmpty()&&(q.contains("var mi")||q.contains("geciyor")||q.contains("bul")||q.contains("ara"));

        if(search){
            int hits=0,structuredHits=0;LinkedHashSet<String>types=new LinkedHashSet<>();
            for(MusaAiDrawingIndex.OleItem item:oles){
                if(MusaAiDrawingIndex.normalize(item.text).contains(term)){
                    hits++;if(item.structured)structuredHits++;types.add(item.type);
                }
            }
            String shown=term.replace(' ',' ');
            String text="Gömülü OLE/Office içerik araması\n• Aranan: “"+shown+"”\n• Eşleşen nesne: "+hits+
                (types.isEmpty()?"":"\n• Türler: "+String.join(", ",types))+
                (hits>0?"\n• Yapısal Office içeriğinde eşleşen: "+structuredHits:"")+
                "\nNot: Yapısal olmayan eski OLE nesnelerinde yalnız çıkarılabilen metin ipuçları aranır.";
            return new Answer(true,text,Collections.emptyList());
        }

        int excel=0,word=0,powerpoint=0,pdf=0,other=0,structured=0,sheets=0,cells=0,withText=0;
        LinkedHashSet<String>types=new LinkedHashSet<>(),samples=new LinkedHashSet<>();
        for(MusaAiDrawingIndex.OleItem item:oles){
            types.add(item.type);
            if("EXCEL".equals(item.type))excel++;else if("WORD".equals(item.type))word++;
            else if("POWERPOINT".equals(item.type))powerpoint++;else if("PDF".equals(item.type))pdf++;else other++;
            if(item.structured){structured++;sheets+=item.sheetCount;cells+=item.cellCount;}
            if(!item.text.isEmpty()){withText++;collectLines(samples,item.text,12);}
        }

        StringBuilder b=new StringBuilder();
        b.append("OLE / gömülü belge özeti • ").append(index.layout.isEmpty()?"aktif layout":index.layout);
        b.append("\n• OLE nesnesi: ").append(index.oleObjectCount);
        b.append("\n• Önizlemeli: ").append(index.olePreviewCount).append(" • önizlemesiz: ").append(index.oleMissingPreviewCount);
        if(!types.isEmpty())b.append("\n• Türler: ").append(String.join(", ",types));
        if(excel>0)b.append("\n• Excel: ").append(excel);
        if(word>0)b.append(" • Word: ").append(word);
        if(powerpoint>0)b.append(" • PowerPoint: ").append(powerpoint);
        if(pdf>0)b.append(" • PDF: ").append(pdf);
        if(other>0)b.append(" • Diğer OLE: ").append(other);
        b.append("\n• Okunabilir metin içeriği: ").append(withText);
        if(structured>0)b.append("\n• Yapısal Office paketi: ").append(structured)
            .append(" • sayfa: ").append(sheets).append(" • hücre: ").append(cells);
        if(!samples.isEmpty())b.append("\nİçerikten örnekler:\n• ").append(String.join("\n• ",samples));
        if(index.oleObjectCount==0)b.append("\nBu layoutta görünür OLE2FRAME nesnesi bulunmadı.");
        else if(structured==0)b.append("\nNot: Modern Office hücre yapısı bulunamadıysa eski OLE içeriği yalnız metin ipuçları düzeyinde okunur.");
        return new Answer(true,b.toString(),Collections.emptyList());
    }

    private static Answer analyzeLegend(MusaAiDrawingIndex index){
        ArrayList<MusaAiDrawingIndex.Item>heading=new ArrayList<>();
        LinkedHashSet<String>layers=new LinkedHashSet<>();
        for(MusaAiDrawingIndex.Item item:index.items()){
            String n=MusaAiDrawingIndex.normalize(item.text);
            if(n.contains("lejant")||n.contains("legend")||
               (n.contains("sembol")&&n.contains("aciklama"))){
                heading.add(item);if(!item.layer.isEmpty())layers.add(item.layer);
            }
        }

        LinkedHashSet<String>samples=new LinkedHashSet<>();LinkedHashSet<Integer>ids=new LinkedHashSet<>();
        for(MusaAiDrawingIndex.Item h:heading)if(h.sourceId>=0)ids.add(h.sourceId);
        if(!layers.isEmpty()){
            for(MusaAiDrawingIndex.Item item:index.items()){
                if(item.text.isEmpty()||!containsLayer(layers,item.layer))continue;
                String n=MusaAiDrawingIndex.normalize(item.text);
                if(n.contains("lejant")||n.contains("legend"))continue;
                addSample(samples,item.text,14);
            }
        }
        if(samples.isEmpty()){
            for(MusaAiDrawingIndex.Item item:index.items()){
                String n=MusaAiDrawingIndex.normalize(item.text);
                if(n.contains("sembol")||n.contains("aciklama")||n.contains("tip")||n.contains("kod")){
                    addSample(samples,item.text,10);if(item.sourceId>=0)ids.add(item.sourceId);
                }
            }
        }

        StringBuilder b=new StringBuilder("Lejant analizi • "+(index.layout.isEmpty()?"aktif layout":index.layout));
        b.append("\n• Açık lejant başlığı: ").append(heading.size());
        if(!layers.isEmpty())b.append("\n• İlişkili katmanlar: ").append(String.join(", ",layers));
        if(!samples.isEmpty())b.append("\nLejant/aynı katmandaki metinlerden örnekler:\n• ").append(String.join("\n• ",samples));
        else b.append("\nBelirgin bir lejant başlığı veya sembol-açıklama metni bulunamadı.");
        b.append("\nNot: Bu analiz metin ve katman ilişkisini kullanır; grafik sembol anlamını tek başına kesinleştirmez.");
        return new Answer(true,b.toString(),ids);
    }

    private static Answer analyzeTable(MusaAiDrawingIndex index){
        int headerHits=0;LinkedHashSet<String>headers=new LinkedHashSet<>();LinkedHashSet<Integer>ids=new LinkedHashSet<>();
        for(MusaAiDrawingIndex.Item item:index.items()){
            if(item.text.isEmpty())continue;
            String n=MusaAiDrawingIndex.normalize(item.text);
            int score=headerScore(n);
            if(score<=0)continue;
            headerHits+=score;addSample(headers,item.text,16);if(item.sourceId>=0)ids.add(item.sourceId);
        }

        int structuredExcel=0,sheets=0,cells=0;LinkedHashSet<String>excelSamples=new LinkedHashSet<>();
        for(MusaAiDrawingIndex.OleItem ole:index.oleItems()){
            if(!"EXCEL".equals(ole.type))continue;
            if(ole.structured){structuredExcel++;sheets+=ole.sheetCount;cells+=ole.cellCount;collectLines(excelSamples,ole.text,12);}
        }

        boolean likely=structuredExcel>0||headerHits>=2;
        StringBuilder b=new StringBuilder("Tablo analizi • "+(index.layout.isEmpty()?"aktif layout":index.layout));
        b.append("\n• Tablo adayı: ").append(likely?"var":"belirgin değil");
        if(headerHits>0)b.append("\n• Başlık ipucu skoru: ").append(headerHits);
        if(!headers.isEmpty())b.append("\n• Olası başlıklar: ").append(String.join(" | ",headers));
        if(structuredExcel>0){
            b.append("\n• Gömülü yapısal Excel: ").append(structuredExcel)
             .append(" • sayfa: ").append(sheets).append(" • hücre: ").append(cells);
            if(!excelSamples.isEmpty())b.append("\nExcel içeriğinden örnekler:\n• ").append(String.join("\n• ",excelSamples));
        }
        if(!likely)b.append("\nÇizim metinlerinde yeterli tablo başlığı kalıbı veya yapısal Excel bulunamadı.");
        b.append("\nNot: CAD içindeki çizgi ızgarası hücre semantiğini garanti etmez; sonuç başlık metinleri ve gömülü Office verisine dayanır.");
        return new Answer(true,b.toString(),ids);
    }

    private static int headerScore(String n){
        int score=0;
        for(String h:TABLE_HEADERS)if(containsPhrase(n,h))score++;
        return score;
    }
    private static boolean containsPhrase(String n,String raw){
        String h=MusaAiDrawingIndex.normalize(raw);
        return (" "+n+" ").contains(" "+h+" ");
    }
    private static boolean asksOle(String q){
        return q.contains("ole")||q.contains("excel")||q.contains("word")||q.contains("powerpoint")||
            q.contains("gomulu belge")||q.contains("gomulu dosya");
    }
    private static boolean asksLegend(String q){return q.contains("lejant")||q.contains("legend");}
    private static boolean asksTable(String q){return q.contains("tablo")||q.contains("cetvel");}

    private static String extractOleSearchTerm(String q){
        String s=q;
        String[]remove={"excel","ole","word","powerpoint","gomulu","belge","dosya","tablo","icerigi","iceriginde",
            "icinde","de","da","var","mi","bul","ara","geciyor","gecen","ne","neler","ozetle","goster","bana"};
        for(String r:remove)s=s.replaceAll("\\b"+r+"\\b"," ");
        return s.trim().replaceAll("\\s+"," ");
    }
    private static boolean containsLayer(Collection<String>layers,String value){
        for(String layer:layers)if(layer.equalsIgnoreCase(value))return true;return false;
    }
    private static void collectLines(Set<String>out,String text,int max){
        if(text==null)return;
        for(String raw:text.replace('\r','\n').split("\\n")){
            String s=raw.trim();if(s.isEmpty())continue;addSample(out,s,max);if(out.size()>=max)return;
        }
    }
    private static void addSample(Set<String>out,String value,int max){
        if(out.size()>=max||value==null)return;String s=value.replaceAll("\\s+"," ").trim();
        if(s.isEmpty())return;if(s.length()>180)s=s.substring(0,180)+"…";out.add(s);
    }
    private MusaAiTableOleAnalysis(){}
}
