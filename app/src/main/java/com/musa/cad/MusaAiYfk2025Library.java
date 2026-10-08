package com.musa.cad;

import android.content.Context;
import android.content.ContentValues;
import android.database.Cursor;
import android.database.sqlite.SQLiteDatabase;
import android.net.Uri;
import com.tom_roush.pdfbox.android.PDFBoxResourceLoader;
import com.tom_roush.pdfbox.pdmodel.PDDocument;
import com.tom_roush.pdfbox.text.PDFTextStripper;
import java.io.*;
import java.util.*;

/**
 * User-provided 2025 YFK "İnşaat ve Tesisat Birim Fiyatları" catalog.
 * The user's original PDF and derived private catalog stay inside Android
 * app-private files. They are NEVER part of GitHub, APK assets or cloud requests.
 *
 * PDF-derived descriptions are page-local excerpts. A unit left blank means
 * "not reliably parsed", not "piece". Prices are dated 01/2025 historical
 * reference values; never automatically approved as current prices.
 */
public final class MusaAiYfk2025Library {
    private static final String PDF="yfk_2025_user_private.pdf";
    private static final String DB="yfk_2025_user_private.db";
    private static final long MAX_BYTES=45L*1024*1024;
    private static final int MAX_PAGES=900,MAX_CHARS_PAGE=90000,MAX_RESULTS=8;
    public interface Progress { void update(String message); }
    private static File target(Context c,String name){return new File(c.getFilesDir(),name);}
    private static void notify(Progress p,String msg){if(p!=null)p.update(msg);}
    private static void delete(File f){if(f.exists())f.delete();}
    public static final class Status {
        public final boolean installed;
        public final int pages,items;
        public final long pdfBytes;
        public Status(boolean installed,int pages,int items,long bytes){
            this.installed=installed;this.pages=pages;this.items=items;this.pdfBytes=bytes;
        }
    }
    public static Status status(Context c){
        if(c==null)return new Status(false,0,0,0);
        File pdf=target(c,PDF),dbFile=target(c,DB);
        if(!pdf.isFile()||pdf.length()<20000||!dbFile.isFile())return new Status(false,0,0,0);
        try(SQLiteDatabase db=SQLiteDatabase.openDatabase(dbFile.getAbsolutePath(),
                null,SQLiteDatabase.OPEN_READONLY);
            Cursor meta=db.rawQuery("SELECT value FROM meta WHERE key='complete' LIMIT 1",null)){
            if(!meta.moveToFirst()||!"1".equals(meta.getString(0)))
                return new Status(false,0,0,0);
            try(Cursor c1=db.rawQuery("SELECT count(*) FROM pages",null);
                Cursor c2=db.rawQuery("SELECT count(*) FROM items",null)){
                int pages=c1.moveToFirst()?c1.getInt(0):0;
                int items=c2.moveToFirst()?c2.getInt(0):0;
                return new Status(pages>=600&&items>=500,pages,items,pdf.length());
            }
        }catch(Exception ignored){return new Status(false,0,0,0);}
    }
    public static String statusText(Context c){
        Status s=status(c);
        if(!s.installed)return "2025 ÇŞİDB kitabı çevrim dışı kurulu değil. "+
            "“2025 kitabını yükle” diyerek PDF'yi bu telefonda seçin.";
        return "2025 ÇŞİDB KİTABI ÇEVRİM DIŞI HAZIR"+
            "\nPDF sayfa sayısı: "+s.pages+
            "\nPoz/rayiç kayıt sayısı: "+s.items+
            "\nArşiv: telefonun özel depolama alanı"+
            "\nFiyat dönemi: 2025-01 (GÜNCEL 2026 FİYATI DEĞİL)"+
            "\nKomut örneği: '2025 poz 25.100.1005'.";
    }

    /** Call only on a worker executor after an explicit Android file-picker action. */
    public static Status importPdf(Context context,Uri uri,Progress progress)throws IOException {
        if(context==null||uri==null)throw new IOException("Kullanıcı PDF dosyası seçmedi.");
        Context c=context.getApplicationContext();
        if(status(c).installed)return status(c);
        File stagingPdf=target(c,PDF+".part");
        File stagingDb=target(c,DB+".part");
        delete(stagingPdf);delete(stagingDb);
        try {
            notify(progress,"2025 kitabı yalnız telefonun özel alanına kopyalanıyor…");
            try(InputStream in=c.getContentResolver().openInputStream(uri)){
                if(in==null)throw new IOException("Seçilen PDF okunamıyor.");
                long copied=0;
                try(OutputStream out=new BufferedOutputStream(new FileOutputStream(stagingPdf))){
                    byte[] buffer=new byte[32768];int n;
                    while((n=in.read(buffer))!=-1){
                        if(Thread.currentThread().isInterrupted())throw new IOException("İçe aktarma iptal edildi.");
                        copied+=n;
                        if(copied>MAX_BYTES)throw new IOException("2025 PDF 45 MB sınırını aşıyor.");
                        out.write(buffer,0,n);
                    }
                }
                if(copied<20000)throw new IOException("PDF boş veya eksik.");
            }
            try(InputStream in=new FileInputStream(stagingPdf)){
                byte[] head=new byte[4];
                if(in.read(head)!=4||head[0]!='%'||head[1]!='P'||head[2]!='D'||head[3]!='F')
                    throw new IOException("Seçilen dosya PDF değil.");
            }
            PDFBoxResourceLoader.init(c);
            try(PDDocument doc=PDDocument.load(stagingPdf)){
                int pages=doc.getNumberOfPages();
                if(pages<600||pages>MAX_PAGES)
                    throw new IOException("Bu dosya beklenen 2025 tam kitabı gibi görünmüyor (sayfa: "+pages+").");
                PDFTextStripper stripper=new PDFTextStripper();
                stripper.setSortByPosition(true);
                stripper.setStartPage(1);
                stripper.setEndPage(Math.min(7,pages));
                String begin=MusaAiYfk2025Parser.fold(stripper.getText(doc));
                if(!begin.contains("2025")||!begin.contains("tesisat")||
                   !begin.contains("birim fiyat"))
                    throw new IOException("PDF kapağı 2025 inşaat ve tesisat kitabını doğrulamıyor.");
                SQLiteDatabase db=SQLiteDatabase.openOrCreateDatabase(stagingDb,null);
                try{
                    db.execSQL("CREATE TABLE meta(key TEXT PRIMARY KEY,value TEXT NOT NULL)");
                    db.execSQL("CREATE TABLE pages(page INTEGER PRIMARY KEY,text TEXT NOT NULL,folded TEXT NOT NULL)");
                    db.execSQL("CREATE TABLE items(code TEXT PRIMARY KEY,description TEXT NOT NULL,unit TEXT NOT NULL,price_2025 TEXT NOT NULL,mounting_2025 TEXT NOT NULL,section TEXT NOT NULL,page INTEGER NOT NULL,folded TEXT NOT NULL)");
                    db.execSQL("CREATE INDEX items_search ON items(folded)");
                    int count=0;
                    db.beginTransaction();
                    try {
                        for(int page=1;page<=pages;page++){
                            if(Thread.currentThread().isInterrupted())
                                throw new IOException("2025 poz indeksleme iptal edildi.");
                            stripper.setStartPage(page);stripper.setEndPage(page);
                            String body=stripper.getText(doc);
                            if(body==null)body="";
                            if(body.length()>MAX_CHARS_PAGE)
                                body=body.substring(0,MAX_CHARS_PAGE);
                            ContentValues pageRow=new ContentValues();
                            pageRow.put("page",page);pageRow.put("text",body);
                            pageRow.put("folded",MusaAiYfk2025Parser.fold(body));
                            db.insertOrThrow("pages",null,pageRow);
                            for(MusaAiYfk2025Parser.Item pos:MusaAiYfk2025Parser.parsePage(body,page)){
                                ContentValues item=new ContentValues();
                                item.put("code",pos.code);
                                item.put("description",pos.description);
                                item.put("unit",pos.unit);
                                item.put("price_2025",pos.price2025);
                                item.put("mounting_2025",pos.mounting2025);
                                item.put("section",pos.section);
                                item.put("page",pos.pdfPage);
                                item.put("folded",MusaAiYfk2025Parser.fold(pos.description));
                                if(db.insertWithOnConflict("items",null,item,
                                    SQLiteDatabase.CONFLICT_IGNORE)!=-1)count++;
                            }
                            if(page%25==0||page==pages)
                                notify(progress,"2025 pozları indeksleniyor: "+page+"/"+pages+
                                    " sayfa • "+count+" aday");
                        }
                        if(count<500)
                            throw new IOException("PDF metin katmanı yeterli sayıda poz içermiyor.");
                        putMeta(db,"complete","1");
                        putMeta(db,"edition","2025-01");
                        putMeta(db,"pages",Integer.toString(pages));
                        putMeta(db,"items",Integer.toString(count));
                        db.setTransactionSuccessful();
                    }finally{db.endTransaction();}
                }finally{db.close();}
            }
            // User-specific PDF and SQLite are never staged in git-tracked files.
            if(!stagingPdf.renameTo(target(c,PDF)))
                throw new IOException("2025 PDF özel saklama alanına taşınamadı.");
            if(!stagingDb.renameTo(target(c,DB)))
                throw new IOException("2025 poz dizini özel saklama alanına taşınamadı.");
            Status status=status(c);
            if(!status.installed)throw new IOException("2025 kitap kurulumu doğrulanamadı.");
            notify(progress,"2025 kataloğu hazır: "+status.items+" aday poz/rayiç, "+
                status.pages+" PDF sayfası.");
            return status;
        }catch(IOException e){throw e;}
        catch(OutOfMemoryError e){throw new IOException("PDF indeksi için bellek yetersiz.",e);}
        catch(Exception e){throw new IOException("2025 poz kataloğu indeksleme hatası: "+e.getMessage(),e);}
        finally{delete(stagingPdf);delete(stagingDb);}
    }
    private static void putMeta(SQLiteDatabase db,String key,String value){
        ContentValues row=new ContentValues();
        row.put("key",key);row.put("value",value);
        db.insertWithOnConflict("meta",null,row,SQLiteDatabase.CONFLICT_REPLACE);
    }
    private static String escapeLike(String s){
        return s.replace("\\","\\\\").replace("%","\\%").replace("_","\\_");
    }
    public static String lookup(Context c,String input){
        if(!status(c).installed)return statusText(c);
        String term=MusaAiYfk2025Parser.searchTerm(input);
        if(term.isEmpty())return "2025 kitabında aranacak poz veya malzeme adını yazın.";
        boolean code=!MusaAiYfk2025Parser.findCode(term).isEmpty();
        StringBuilder result=new StringBuilder(
            "ÇŞİDB / YFK 2025 • ÇEVRİM DIŞI KATALOG\nSorgu: "+term+
            "\nFiyatlar yalnız 01.01.2025 tarihli referanstır.");
        try(SQLiteDatabase db=SQLiteDatabase.openDatabase(target(c,DB).getAbsolutePath(),
                null,SQLiteDatabase.OPEN_READONLY)){
            String sql=code?
                "SELECT code,description,unit,price_2025,mounting_2025,page FROM items WHERE code=? LIMIT 1":
                "SELECT code,description,unit,price_2025,mounting_2025,page FROM items WHERE folded LIKE ? ESCAPE '\\' ORDER BY code LIMIT "+MAX_RESULTS;
            String arg=code?term:"%"+escapeLike(MusaAiYfk2025Parser.fold(term))+"%";
            int hits=0;
            try(Cursor cursor=db.rawQuery(sql,new String[]{arg})){
                while(cursor.moveToNext()){
                    hits++;
                    result.append("\n\n• Poz ").append(cursor.getString(0));
                    String description=cursor.getString(1);
                    if(description!=null&&!description.isEmpty())
                        result.append("\nTarif/kayıt özeti: ").append(description);
                    String unit=cursor.getString(2);
                    result.append("\nBirim: ").append(unit==null||unit.isEmpty()?
                        "PDF'den kesin çıkarılamadı":unit);
                    String price=cursor.getString(3),mounting=cursor.getString(4);
                    if(price!=null&&!price.isEmpty())
                        result.append("\n2025 fiyat sütunu: ").append(price).append(" TL");
                    if(mounting!=null&&!mounting.isEmpty())
                        result.append("\n2025 montaj bedeli sütunu: ").append(mounting).append(" TL");
                    result.append("\nPDF sayfa: ").append(cursor.getInt(5));
                }
            }
            if(hits==0){
                try(Cursor pages=db.rawQuery(
                    "SELECT page,text FROM pages WHERE folded LIKE ? ESCAPE '\\' ORDER BY page LIMIT 3",
                    new String[]{"%"+escapeLike(MusaAiYfk2025Parser.fold(term))+"%"})){
                    while(pages.moveToNext()){
                        hits++;
                        String pageText=pages.getString(1);
                        String norm=MusaAiYfk2025Parser.fold(pageText);
                        int at=norm.indexOf(MusaAiYfk2025Parser.fold(term));
                        // Avoid pretending normalized offsets are exact in Unicode text.
                        int start=Math.max(0,Math.min(pageText.length(),at)-70);
                        int end=Math.min(pageText.length(),start+330);
                        result.append("\n\nPDF sayfa ").append(pages.getInt(0))
                            .append(" • ham metin eşleşmesi: ")
                            .append(pageText.substring(start,end).replaceAll("\\s+"," "));
                    }
                }
            }
            if(hits==0)result.append("\nBu ifadeyle eşleşme bulunamadı.");
        }catch(Exception e){
            return "2025 PDF arşivi okunamadı. Kataloğun durumunu kontrol edin.";
        }
        result.append("\n\nÖnemli: Bazı pozların açıklaması önceki sayfada olabilir; "+
            "metraj birimi ve tarifin tam kapsamı özgün PDF'den ayrıca denetlenmelidir. "+
            "2025 fiyatları güncel keşfe otomatik uygulanmaz.");
        return result.toString();
    }
    public static String compareNew2026(Context c) {
        if(!status(c).installed)return "Önce 2025 kitabını telefona yükleyin.";
        if(!MusaAiYfkOfflineCatalog.status(c).installed)
            return "2026 kitap arşivi yüklü değil. 'ÇŞB kitabını indir' komutuyla "+
                "resmî 2026 kitabını cihazda kurun.";
        File db2026=target(c,"yfk_2026_offline_private.db");
        HashSet<String> existing=new HashSet<>();
        try(SQLiteDatabase database=SQLiteDatabase.openDatabase(
                target(c,DB).getAbsolutePath(),null,SQLiteDatabase.OPEN_READONLY);
            Cursor cursor=database.rawQuery("SELECT code FROM items",null)){
            while(cursor.moveToNext())existing.add(cursor.getString(0));
        }catch(Exception e){return "2025 poz listesi okunamadı.";}
        LinkedHashMap<String,Integer> additions=new LinkedHashMap<>();
        int scanned=0;
        try(SQLiteDatabase db=SQLiteDatabase.openDatabase(db2026.getAbsolutePath(),
                null,SQLiteDatabase.OPEN_READONLY);
            Cursor cursor=db.rawQuery("SELECT page,body FROM pages ORDER BY page",null)){
            while(cursor.moveToNext()){
                if(Thread.currentThread().isInterrupted())
                    return "Yeni poz karşılaştırması kullanıcı tarafından durduruldu.";
                int page=cursor.getInt(0);scanned++;
                for(String code:MusaAiYfk2025Parser.pageCodes(cursor.getString(1))){
                    if(!existing.contains(code)&&!additions.containsKey(code))
                        additions.put(code,page);
                }
            }
        }catch(Exception e){return "2026 kitap arşivinden karşılaştırma yapılamadı.";}
        StringBuilder result=new StringBuilder("2025 → 2026 POZ FARKI (ÖN KONTROL)\n")
            .append("2025 mevcut kod: ").append(existing.size())
            .append("\n2026 incelenen sayfa: ").append(scanned)
            .append("\n2025'te bulunmayan 2026 kod ADAYLARI: ").append(additions.size());
        int shown=0;
        for(Map.Entry<String,Integer> row:additions.entrySet()){
            if(shown++>=40){result.append("\n… İlk 40 aday gösterildi.");break;}
            result.append("\n• ").append(row.getKey()).append(" • 2026 PDF sayfa ")
                .append(row.getValue());
        }
        result.append("\n\nBu yalnız metin/kod farkı taramasıdır. "+
            "Pozun gerçekten yeni yayımlandığını, tarifi veya fiyatının "+
            "geçerli olduğunu ispatlamaz. Her aday resmî 2026 kitapta incelenmelidir. "+
            "2025 özel kütüphanesi değiştirilmedi.");
        return result.toString();
    }
    /**
     * Optional material dictionary bridge. Returns review candidates only;
     * never mutates a project, selects an official position or applies a rate.
     */
    public static String suggestForTakeoff(Context c,MusaAiCsbEstimate.Result measured) {
        if(!status(c).installed)return "2025 kullanıcı kataloğu henüz bu cihazda kurulu değil.";
        if(measured==null||measured.rows.isEmpty())
            return "2025 poz adayları için doğrulanmış çizim metraj kalemi bulunamadı.";
        StringBuilder out=new StringBuilder(
            "2025 ÇŞİDB • METRAJ/MALZEME SÖZLÜĞÜ ADAYLARI (ÇEVRİM DIŞI)");
        int shown=0;
        try(SQLiteDatabase db=SQLiteDatabase.openDatabase(target(c,DB).getAbsolutePath(),
                null,SQLiteDatabase.OPEN_READONLY)){
            for(MusaAiCsbEstimate.Row row:measured.rows){
                if(shown++>=8){out.append("\nDiğer çizim kalemleri bu ön izlemede gösterilmedi.");break;}
                String description=row.description;
                String term=materialToken(MusaAiYfk2025Parser.fold(description));
                out.append("\n\n• Çizim: ").append(description).append(" / ")
                    .append(row.quantity).append(" ").append(row.unit);
                if(term.isEmpty()){
                    out.append("\nTeknik malzeme türü yeterince belirgin değil; poz atanmadı.");
                    continue;
                }
                String sql="SELECT code,description,unit,price_2025,page FROM items "+
                    "WHERE folded LIKE ? ESCAPE '\\' AND code LIKE '25.%' "+
                    "ORDER BY code LIMIT 3";
                int hits=0;
                try(Cursor cur=db.rawQuery(sql,
                    new String[]{"%"+escapeLike(term)+"%"})){
                    while(cur.moveToNext()){
                        hits++;
                        out.append("\n  • ADAY ").append(cur.getString(0))
                            .append(" • sayfa ").append(cur.getInt(4));
                        String candidate=cur.getString(1);
                        if(candidate!=null&&!candidate.isEmpty())
                            out.append(" • ").append(candidate.substring(0,Math.min(140,candidate.length())));
                        String unit=cur.getString(2);
                        if(unit!=null&&!unit.isEmpty())out.append(" [birim ").append(unit).append("]");
                        String old=cur.getString(3);
                        if(old!=null&&!old.isEmpty())out.append(" [2025: ").append(old).append(" TL]");
                    }
                }
                if(hits==0)out.append("\n  Bu malzeme adına ait 2025 sayfa adayı bulunamadı.");
            }
        }catch(Exception e){return "2025 poz sözlüğü okunamadı.";}
        out.append("\n\nDİKKAT: Bunlar yalnız anahtar kelime üzerinden kaynak adaylarıdır. ")
            .append("DN/PN, boru malzemesi, imalat tanımı, ölçü birimi, kapsam, ")
            .append("kat/kesit metrajı ve mükerrerlik kontrolü yapılmadan resmi poz sayılmaz. ")
            .append("2025 tarihli fiyat güncel proje toplamına aktarılmaz.");
        return out.toString();
    }
    private static String materialToken(String desc) {
        for(String token:new String[]{"hidrofor","boyler","klozet","lavabo",
            "pisuvar","batarya","evye","suzgec","pprc","polipropilen",
            "pvc","polietilen","celik","bakir","yangin pompasi","hava kanali",
            "boru","vana","radiator","fan coil","klima"})
            if(desc.contains(token))return token;
        return "";
    }

    private MusaAiYfk2025Library(){}
}
