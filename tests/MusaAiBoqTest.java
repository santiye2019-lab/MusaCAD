import com.musa.cad.*;
import java.util.*;

public final class MusaAiBoqTest {
    private static void require(boolean value,String message){if(!value)throw new AssertionError(message);}
    private static void has(String value,String needle,String message){if(value==null||!value.contains(needle))throw new AssertionError(message+": "+value);}

    private static MusaAiDrawingIndex index(){
        return new MusaAiDrawingIndex("Model",4,0,
            Arrays.asList("PIS_SU","CIHAZ"),
            Arrays.asList("PIS_SU","CIHAZ"),
            Arrays.asList(
                new MusaAiDrawingIndex.Item(1,"LINE","PIS_SU","",12,Double.NaN),
                new MusaAiDrawingIndex.Item(2,"LWPOLYLINE","PIS_SU","",10,20),
                new MusaAiDrawingIndex.Item(3,"CIRCLE","CIHAZ","",Double.NaN,Double.NaN),
                new MusaAiDrawingIndex.Item(4,"TEXT","CIHAZ","Lavabo",Double.NaN,Double.NaN)
            ),"m");
    }

    public static void main(String[]args){
        String csv="Poz;Açıklama;Birim;Miktar\n01;PIS_SU;m;22,00\n02;PIS_SU;m2;20\n03;CIHAZ;adet;2";
        MusaAiBoq.Model boq=MusaAiBoq.parse("kesif.csv",CadDocumentSupport.Kind.CSV,csv);
        require(boq.rows.size()==3,"CSV BOQ row count");
        has(MusaAiBoq.summary(boq),"Okunan keşif satırı: 3","BOQ summary");

        String xlsx="— Sayfa 1 —\nA1\tPoz\nB1\tAçıklama\nC1\tBirim\nD1\tMiktar\n"+
            "A2\t10.001\nB2\tPIS_SU\nC2\tm\nD2\t22,00";
        MusaAiBoq.Model x=MusaAiBoq.parse("kesif.xlsx",CadDocumentSupport.Kind.XLSX,xlsx);
        require(x.rows.size()==1,"XLSX BOQ row count");
        require(Math.abs(x.rows.get(0).quantity-22d)<1e-9,"Turkish decimal parse");

        MusaAiBoq.Model generated=MusaAiBoq.generate(index(),"proje");
        require(!generated.rows.isEmpty(),"generated BOQ");
        MusaAiBoq.Comparison c=MusaAiBoq.compare(boq,generated);
        require(c.compared>=3,"project/BOQ matching");
        require(c.different==0,"equal quantities should not be different");
        has(c.text,"PROJE – KEŞİF KARŞILAŞTIRMASI","comparison report");

        MusaAiBoq.Model mm=MusaAiBoq.of("mm proje",Arrays.asList(
            new MusaAiBoq.Row("","PIS_SU • uzunluk","mm",22000,"test")
        ));
        MusaAiBoq.Model metre=MusaAiBoq.of("m keşif",Arrays.asList(
            new MusaAiBoq.Row("01","PIS_SU","m",22,"test")
        ));
        MusaAiBoq.Comparison converted=MusaAiBoq.compare(metre,mm);
        require(converted.compared==1&&converted.different==0,"mm to m conversion");

        System.out.println("MusaAiBoqTest OK");
    }
}
