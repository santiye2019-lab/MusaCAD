import java.nio.charset.StandardCharsets;
import java.nio.file.*;

public final class MusaAiDisciplineControlContractTest {
    private static String read(String p)throws Exception{return Files.readString(Path.of(p),StandardCharsets.UTF_8);}
    private static void require(String s,String n,String area){if(!s.contains(n))throw new AssertionError(area+" missing "+n);}
    public static void main(String[]args)throws Exception{
        String main=read("app/src/main/java/com/musa/cad/MainActivity.java");
        String panel=read("app/src/main/java/com/musa/cad/MusaAiPanel.java");
        String engine=read("app/src/main/java/com/musa/cad/MusaAiDisciplineControl.java");
        String boq=read("app/src/main/java/com/musa/cad/MusaAiBoq.java");

        require(main,"MusaAiDisciplineControl.analyze","discipline control bridge");
        require(main,"MusaAiBoq.disciplineCompatibility","discipline BOQ bridge");
        require(main,"currentProject.boqModel","per-project BOQ integration");
        require(panel,"{\"Disiplin\",\"AI_DISIPLIN_KONTROL\"}","AI quick action");

        require(engine,"ARCHITECTURE","architecture discipline");
        require(engine,"STRUCTURAL","structural discipline");
        require(engine,"MECHANICAL","mechanical discipline");
        require(engine,"ELECTRICAL","electrical discipline");
        require(engine,"FIRE","fire discipline");
        require(engine,"INFRASTRUCTURE","infrastructure discipline");
        require(engine,"LANDSCAPE","landscape discipline");
        require(engine,"ELEVATOR","elevator discipline");
        require(engine,"Mimari ↔ Statik","coordination checks");
        require(engine,"Statik hesap","engineering disclaimer");

        require(boq,"filterByDiscipline","discipline BOQ filter");
        require(boq,"DİSİPLİN BAZLI PROJE – KEŞİF UYUM ÖZETİ","discipline BOQ output");
        System.out.println("MusaAiDisciplineControlContractTest OK");
    }
}
