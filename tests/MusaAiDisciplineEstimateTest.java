import com.musa.cad.*;
import java.util.*;
import java.nio.file.*;
import java.nio.charset.StandardCharsets;

public final class MusaAiDisciplineEstimateTest {
    static void check(boolean v,String reason){if(!v)throw new AssertionError(reason);}
    static void eq(double a,double b,String why){check(Math.abs(a-b)<0.00001,why+": "+a+" != "+b);}
    static MusaAiDrawingIndex.Item it(int id,String type,String layer,String text,
                                      double length,double area,boolean closed,int quantity){
        return new MusaAiDrawingIndex.Item(id,type,layer,text,length,area,true,closed,"",0,0,quantity);
    }
    static MusaAiDrawingIndex draw(String unit){
        List<MusaAiDrawingIndex.Item> items=Arrays.asList(
            it(1,"LINE","MEK-TEMIZ-SU-BORU-PPRC","DN25",1000,Double.NaN,false,1),
            it(1,"LINE","MEK-TEMIZ-SU-BORU-PPRC","DN25",1000,Double.NaN,false,1),
            it(2,"LINE","ELK-ZAYIF-AKIM-KABLO","data kablo",5000,Double.NaN,false,1),
            it(3,"LINE","ELK-KUVVETLI-AKIM-KABLO","elektrik kablo",1500,Double.NaN,false,1),
            it(4,"HATCH","MIM-ZEMIN-SERAMIK","seramik",Double.NaN,2000000,true,1),
            it(5,"INSERT","MEK-KLIMA","VRF",Double.NaN,Double.NaN,false,2),
            it(6,"LINE","MIM-DUVAR","",7000,Double.NaN,false,1),
            it(7,"LINE","STATIK-KOLON","",1000,Double.NaN,false,1),
            it(-1,"LINE","MEK-TEMIZ-SU-BORU-PPRC","",1000,Double.NaN,false,1)
        );
        return new MusaAiDrawingIndex("Model",items.size(),0,
            Arrays.asList("MEK-TEMIZ-SU-BORU-PPRC","ELK-ZAYIF-AKIM-KABLO",
                "ELK-KUVVETLI-AKIM-KABLO","MIM-ZEMIN-SERAMIK","MEK-KLIMA",
                "MIM-DUVAR","STATIK-KOLON"),Collections.emptyList(),items,unit);
    }
    static MusaAiDisciplineEstimate.Row find(MusaAiDisciplineEstimate.Result r,String scope){
        for(MusaAiDisciplineEstimate.Row x:r.rows)if(scope.equals(x.section))return x;
        throw new AssertionError("No quantity row for "+scope);
    }
    public static void main(String[]args)throws Exception{
        MusaAiDisciplinePlan.Plan all=MusaAiDisciplinePlan.resolve("Tüm disiplinlerin keşif analizini yap","all");
        MusaAiDisciplineEstimate.Result result=MusaAiDisciplineEstimate.analyze(draw("mm"),all);
        eq(find(result,"cleanwater").quantity,1d,"Pipe length must use mm->m, no repeated source");
        eq(find(result,"lowcurrent").quantity,5d,"Weak-current measured cable");
        eq(find(result,"highcurrent").quantity,1.5d,"Strong-current measured cable");
        eq(find(result,"architectural").quantity,2d,"Closed floor area in m2");
        eq(find(result,"cooling").quantity,2d,"Tagged VRF block count");
        check(result.rows.size()==5,"Untyped wall and structural drawing lines must not be BOQ quantities");
        check(result.report.contains("Mekanik / Temiz su"),"Mechanical row grouped");
        check(result.report.contains("Elektrik / Zayıf akım"),"Electrical row grouped");
        check(result.report.contains("Genel toplam")==false,"Engine never invents total");
        check(result.report.contains("onaylı imalat ölçüsü değildir"),"Advisory disclaimer");
        check(result.sourceIds.size()==5,"Only source-linked quantities");
        MusaAiDisciplineEstimate.Result unknown=MusaAiDisciplineEstimate.analyze(
            draw(""),all);
        check(unknown.rows.size()==1,"Unknown scale must block CAD lengths and areas");
        check("Ad".equals(unknown.rows.get(0).unit),"Tagged blocks may be count candidates");
        check(unknown.report.contains("uzunluk/alan metrajı üretilmedi"),"Scale warning");

        MusaAiDisciplinePlan.Plan weak=MusaAiDisciplinePlan.resolve("Zayıf akım keşfini analiz et","all");
        MusaAiDisciplineEstimate.Result weakReport=MusaAiDisciplineEstimate.analyze(draw("mm"),weak);
        check(weakReport.rows.size()==1&&weakReport.rows.get(0).section.equals("lowcurrent"),
            "Single natural-language branch must not include other systems");
        check(MusaAiDisciplineEstimate.wantsEstimate("Projeye göre keşif analizi hazırla"),
            "natural free-form estimate intent");
        check(MusaAiDisciplineEstimate.wantsEstimate("Elektrik metrajını çıkar"),
            "electrical quantities");
        check(!MusaAiDisciplineEstimate.wantsEstimate("Keşif oluşturma, projeyi sadece incele"),
            "negated takeoff must not execute");
        check(!MusaAiDisciplineEstimate.wantsEstimate("2025 kitabını yükle"),
            "book import must remain separate");

        String main=Files.readString(Path.of(
            "app/src/main/java/com/musa/cad/MainActivity.java"),StandardCharsets.UTF_8);
        String yfk=Files.readString(Path.of(
            "app/src/main/java/com/musa/cad/MusaAiYfk2025Library.java"),StandardCharsets.UTF_8);
        String current=Files.readString(Path.of(
            "app/src/main/java/com/musa/cad/MusaAiYfkOfflineCatalog.java"),StandardCharsets.UTF_8);
        check(main.contains("runMusaAiDisciplineTakeoff(raw,reply)"),"natural local route");
        check(main.contains("drawing.aiDrawingIndex(Integer.MAX_VALUE)"),"background index");
        check(main.contains("MusaAiDisciplineEstimate.analyze(snapshot,plan)"),
            "Qwen-on-demand discovery routed");
        check(main.contains("MusaAiYfk2025Library.suggestForDisciplineTakeoff(this,takeoff)"),
            "2025 visual catalog mapping");
        check(main.contains("MusaAiYfkOfflineCatalog.suggestForDisciplineTakeoff(this,takeoff)"),
            "2026 visual catalog mapping");
        check(yfk.contains("SELECT code,description,unit,price_2025,mounting_2025,page"),
            "2025 private price-book source linkage");
        check(current.contains("SELECT page,body FROM pages"),"2026 private page linkage");
        System.out.println("MusaAiDisciplineEstimateTest OK");
    }
}
