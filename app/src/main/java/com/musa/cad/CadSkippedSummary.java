package com.musa.cad;

import java.util.List;
import java.util.Map;

/** Human-readable and bounded diagnostic, not proof of complete AutoCAD compatibility. */
public final class CadSkippedSummary {
    private CadSkippedSummary() {}

    public static String describe(int total,List<DxfBlocks.SkippedRecord> samples,
                                  Map<String,Integer> byType){
        if(total<=0)return "Kayıt atlama uyarısı yok (tam AutoCAD uyumluluğu kanıtlanmış değildir).";
        StringBuilder out=new StringBuilder(768);
        out.append("Çizilemeyen / atlanan CAD kayıtları: ").append(total);
        out.append("\nBu sayaç, DXF ayrıştırma ve çizim dönüştürme aşamalarına aittir;");
        out.append(" orijinal DWG'ye göre kayıp geometrinin tam sayımı değildir.");
        if(byType!=null&&!byType.isEmpty()){
            out.append("\nTürlere göre: ");
            int i=0;
            for(Map.Entry<String,Integer> e:byType.entrySet()){
                if(i>=12){out.append("…");break;}
                if(i++>0)out.append(", ");
                out.append(safe(e.getKey(),48)).append(": ").append(e.getValue());
            }
        }
        if(samples!=null&&!samples.isEmpty()){
            out.append("\nİlk ").append(Math.min(12,samples.size())).append(" örnek:");
            int shown=0;
            for(DxfBlocks.SkippedRecord s:samples){
                if(s==null)continue;
                if(shown++>=12)break;
                out.append("\n").append(shown).append(". ")
                   .append(safe(s.type,48)).append("  #").append(safe(s.handle.isEmpty()?"?":s.handle,72))
                   .append("  katman=").append(safe(s.layer,72))
                   .append("  • ").append(safe(s.reason,100));
            }
            if(total>shown)out.append("\n… ").append(total-shown).append(" ek atlama (ayrıntı listesi sınırlı)");
        }
        return out.toString();
    }

    private static String safe(String s,int max){
        if(s==null||s.isEmpty())return "?";
        StringBuilder b=new StringBuilder(Math.min(max,s.length()));
        int count=0;
        for(int i=0;i<s.length()&&count<max;){
            int c=s.codePointAt(i);i+=Character.charCount(c);count++;
            if(Character.isISOControl(c)||Character.getType(c)==Character.FORMAT)b.append('?');
            else b.appendCodePoint(c);
        }
        if(s.codePointCount(0,s.length())>max)b.append('…');
        return b.toString();
    }
}
