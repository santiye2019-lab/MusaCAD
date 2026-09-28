import com.musa.cad.*;
import java.util.*;

public final class MusaAiProjectPackageTest {
    private static MusaAiDrawingIndex idx(String unit,String layer,String text,double x,double y){
        return new MusaAiDrawingIndex("Model",3,0,
            Arrays.asList(layer),Arrays.asList(layer),
            Arrays.asList(
                new MusaAiDrawingIndex.Item(1,"LINE",layer,text,10,Double.NaN,false,false,layer+"-1",x,y),
                new MusaAiDrawingIndex.Item(2,"TEXT",layer,text,Double.NaN,Double.NaN,false,false,layer+"-2",x+.02,y+.02)
            ),unit);
    }
    private static void has(String s,String n){if(!s.contains(n))throw new AssertionError("missing "+n+" in\n"+s);}
    private static void eq(Object a,Object b,String label){if(!Objects.equals(a,b))throw new AssertionError(label+" expected "+b+" got "+a);}
    public static void main(String[]args){
        MusaAiProjectPackage.Drawing arch=new MusaAiProjectPackage.Drawing(
            "01_MIMARI_ZEMIN.dwg",idx("m","MIM_KAPI","MAHAL KAPI RAMPASI",4,4),null);
        MusaAiProjectPackage.Drawing structural=new MusaAiProjectPackage.Drawing(
            "02_STATIK_KALIP.dwg",idx("mm","S_KIRIS","KİRİŞ REZERVASYON",1000,1000),null);
        MusaAiBoq.Model mechBoq=MusaAiBoq.of("Mekanik keşif",Arrays.asList(
            new MusaAiBoq.Row("M-01","MEK BORU","m",25,"Mekanik keşif")
        ));
        MusaAiProjectPackage.Drawing mechanical=new MusaAiProjectPackage.Drawing(
            "03_MEKANIK.dwg",idx("m","MEK_BORU","MEKANİK BORU DN100",1.10,1.10),mechBoq);
        MusaAiProjectPackage.Drawing electrical=new MusaAiProjectPackage.Drawing(
            "04_ELEKTRIK.dwg",idx("birimsiz","E_PANO","ELEKTRİK PANO KABLO",1.12,1.11),null);

        eq(arch.discipline,MusaAiDiscipline.ARCHITECTURAL,"arch");
        eq(structural.discipline,MusaAiDiscipline.STRUCTURAL,"structural");
        eq(mechanical.discipline,MusaAiDiscipline.MECHANICAL,"mechanical");
        eq(electrical.discipline,MusaAiDiscipline.ELECTRICAL,"electrical");

        if(!MusaAiProjectPackage.asksPackageReview("Tam proje denetim raporu oluştur"))throw new AssertionError("package command");
        if(!MusaAiProjectPackage.asksPackageReview("Açık tüm projeleri proje paketi olarak kontrol et"))throw new AssertionError("package alias");

        MusaAiProjectPackage.Result result=MusaAiProjectPackage.generate(
            Arrays.asList(arch,structural,mechanical,electrical),
            "Tam proje denetim raporu oluştur");
        if(!result.matched)throw new AssertionError("not matched");
        eq(result.drawingCount,4,"drawing count");
        if(result.detectedDisciplineCount<4)throw new AssertionError("discipline count");
        has(result.text,"PROJE PAKETİ");
        has(result.text,"01_MIMARI_ZEMIN.dwg");
        has(result.text,"02_STATIK_KALIP.dwg");
        has(result.text,"03_MEKANIK.dwg");
        has(result.text,"04_ELEKTRIK.dwg");
        has(result.text,"Disiplinler arası yakınlık / koordinasyon adayları");
        has(result.text,"kesin çakışma değildir");
        has(result.text,"Geometrik çapraz kontrol için birim doğrulaması gerekli");
        has(result.text,"PROJE–KEŞİF UYUMLULUĞU");
        has(result.text,"Mekanik");
        has(result.text,"Peyzaj: yüklenmedi / algılanmadı");

        boolean proximity=false,unknownUnit=false;
        for(MusaAiProjectPackage.Finding f:result.findings){
            if(f.title.contains("yakınlık"))proximity=true;
            if(f.title.contains("birim doğrulaması"))unknownUnit=true;
        }
        if(!proximity)throw new AssertionError("expected proximity");
        if(!unknownUnit)throw new AssertionError("expected unknown-unit finding");
        System.out.println("MusaAiProjectPackageTest OK");
    }
}
