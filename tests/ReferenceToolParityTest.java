import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Path;
import java.util.*;
import java.util.regex.*;

public final class ReferenceToolParityTest {
    private static void require(boolean value,String message){
        if(!value)throw new AssertionError(message);
    }
    public static void main(String[] args)throws Exception{
        String source=Files.readString(Path.of("app/src/main/java/com/musa/cad/MainActivity.java"),StandardCharsets.UTF_8);
        String[] categories={"Ek açıklama","Çiz","Düzenle","Katman","Ölçüm","Boyut","Renk","Alet","Düzen","Görsel stil"};
        for(String label:categories)require(source.contains("showToolPanel(\""+label+"\""),"Missing category panel: "+label);

        String[][] required={
            {"Taslak kroki"},{"Ok"},{"Metin"},{"Revcloud"},{"Ses"},{"Görüntü"},{"Video"},{"Kılavuz"},{"Çizgi"},{"Dikdörtgen"},{"Elips"},{"Numbering"},
            {"Polyline"},{"Eskiz"},{"Daire"},{"Yay"},{"Akıllı Kalem"},{"Multileader"},{"Divide"},{"Hatch"},
            {"Düzelt / Trim","Düzeltmek"},{"Uzat","Uzatmak"},{"Offset","Dengelemek"},{"Fileto"},{"Oluk / Pah","Oluk"},
            {"Yeni katman"},{"Katman Listesi"},{"Katmanı Kapat"},{"Diğer katmanlar"},{"Önceki katman"},{"Tüm Katmanlar"},{"Katmanı varsayılan"},
            {"Mesafe"},{"Varlık"},{"Alan"},{"Cephe"},{"ID Noktası"},{"Yay uzunluğu"},{"Açı"},{"Kalibrasyon","Ölçek"},{"Sonuç"},{"Sonuç sayısı"},{"Hassas"},
            {"Hizalı"},{"Doğrusal"},{"Açısal"},{"Radius"},{"Çap"},{"Yay boyu"},{"Three-point"},
            {"ByLayer"},{"ByBlock"},{"ACI 1–255"},
            {"Bulmak"},{"Artımlı Kopya"},{"Sayaç bloğu"},{"Graphic lookup"},{"Blok ekle"},{"Açıklama ara"},{"Yer imi"},{"Copy across"},{"Paste across"}
        };
        for(String[] aliases:required){
            boolean found=false;for(String label:aliases)if(source.contains("tool(\""+label+"\"")){found=true;break;}
            require(found,"Missing reference tool aliases: "+Arrays.toString(aliases));
        }

        Pattern nullTool=Pattern.compile("tool\\(\\\"([^\\\"]+)\\\",R\\.drawable\\.[^,]+,null\\)");
        Matcher matcher=nullTool.matcher(source);
        ArrayList<String> disabled=new ArrayList<>();
        while(matcher.find())disabled.add(matcher.group(1));
        require(disabled.isEmpty(),"Unexpected disabled reference tools: "+disabled);
        require(source.contains("tool(\"3D\",R.drawable.ic_fit,this::open3d)"),"3D view action missing");
        System.out.println("ReferenceToolParityTest OK: "+required.length+" reference tool groups wired; 3D view enabled.");
    }
}
