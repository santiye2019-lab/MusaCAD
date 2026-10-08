import com.musa.cad.*;
import java.util.*;

public final class MusaAiCsbPriceCsvTest {
    private static final String HEADER="item_key;poz;aciklama;birim;birim_fiyat;donem;kaynak;fiyat_kapsami";
    private static final String YFK="https://yfk.csb.gov.tr/birim-fiyatlar-100468";
    private static void check(boolean b,String text){if(!b)throw new AssertionError(text);}
    private static String row(String code,String cost,String date,String url,String unit,String scope){
        return "BORU|Pis su | PVC | DN100;"+code+";ÖRNEK TEST PVC pis su;"+
            unit+";"+cost+";"+date+";"+url+";"+scope;
    }
    public static void main(String[]args){
        String csv=HEADER+"\n"+row("TEST-ONLY-001","1.234,56","2026-10",YFK,"m","malzeme+montaj");
        MusaAiCsbPriceCsv.Draft draft=MusaAiCsbPriceCsv.parse(csv);
        check(draft.rates.size()==1,"proper curated rate imported");
        check(!draft.rates.get(0).usable(),"unconfirmed file must NOT be priced");
        check(Math.abs(draft.rates.get(0).installedUnitPrice-1234.56)<0.0001,
            "Turkish price decimal/thousands parsed");
        List<MusaAiCsbEstimate.Rate> accepted=draft.approve();
        check(accepted.get(0).usable(),"explicit approval enables one rate");
        MusaAiDrawingIndex drawing=new MusaAiDrawingIndex("Model",1,0,
            Arrays.asList("PIS_SU_PVC_DN100"),Arrays.asList("PIS_SU_PVC_DN100"),
            Arrays.asList(new MusaAiDrawingIndex.Item(1,"LINE","PIS_SU_PVC_DN100",
                "",10d,Double.NaN)),"m");
        check(MusaAiCsbEstimate.analyze(drawing,draft.rates).pricedRows==0,
            "draft prices must not alter the report");
        MusaAiCsbEstimate.Result priced=MusaAiCsbEstimate.analyze(drawing,accepted);
        check(priced.pricedRows==1,
            "approved exact source and installed scope can price known item");
        check(priced.report.contains("poz eşleştirme anahtarı: BORU|Pis su | PVC | DN100"),
            "user must see the exact item mapping key");
        check(priced.report.contains(YFK),"official source URL must appear with priced amount");
        check(MusaAiCsbPriceCsv.parse(HEADER+"\n"+
            row("TEST","123,00","2026-10","https://yfk.csb.gov.tr.evil.test/abc","m","malzeme+montaj"))
            .rates.isEmpty(),"lookalike official domain rejected");
        check(MusaAiCsbPriceCsv.parse(HEADER+"\n"+
            row("TEST","123,00","2026-10","http://yfk.csb.gov.tr/abc","m","malzeme+montaj"))
            .rates.isEmpty(),"non-https rejected");
        check(MusaAiCsbPriceCsv.parse(HEADER+"\n"+
            row("TEST","123,00","2026-10",YFK,"m","yalnız malzeme"))
            .rates.isEmpty(),"material-only rate not applied as installed price");
        check(MusaAiCsbPriceCsv.parse(HEADER+"\n"+
            row("TEST","123,00","2026-10",YFK,"m2","malzeme+montaj"))
            .rates.isEmpty(),"wrong unit rejected");
        check(MusaAiCsbPriceCsv.parse(HEADER+"\n"+
            row("TEST","-123,00","2026-10",YFK,"m","malzeme+montaj"))
            .rates.isEmpty(),"negative price rejected");
        check(MusaAiCsbPriceCsv.parse(HEADER+"\n"+
            row("TEST","123,00","2026-10",YFK,"m","malzeme+montaj")+"\n"+
            row("TEST2","500,00","2026-09",YFK,"m","malzeme+montaj"))
            .rates.size()==1,"mixed price periods cannot silently combine");
        check(MusaAiCsbPriceCsv.parse(HEADER+"\n"+
            row("TEST1","123,00","2026-10",YFK,"m","malzeme+montaj")+"\n"+
            row("TEST2","500,00","2026-10",YFK,"m","malzeme+montaj"))
            .rates.size()==1,"duplicate material+unit priced once");
        check(MusaAiCsbPriceCsv.parse("other;bad\n1;2").rates.isEmpty(),
            "incorrect schema rejected");
        System.out.println("MusaAiCsbPriceCsvTest OK");
    }
}
