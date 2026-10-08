package com.musa.cad;

import android.content.Context;
import android.content.ContentValues;
import android.database.Cursor;
import android.database.sqlite.SQLiteDatabase;
import com.tom_roush.pdfbox.android.PDFBoxResourceLoader;
import com.tom_roush.pdfbox.pdmodel.PDDocument;
import com.tom_roush.pdfbox.text.PDFTextStripper;
import java.io.*;
import java.net.*;
import java.util.*;

/**
 * Device-local, user-initiated index of the 2026 YFK mechanical analyses.
 * Contains technical descriptions and source pages, not an approved price list.
 * Official books are NOT packaged in the APK or copied into the public repo.
 *
 * All methods that read large documents or query the fulltext index must run
 * on a background executor; never on the Android main thread.
 */
public final class MusaAiYfkTechnicalLibrary {
    private static final int CONNECT_TIMEOUT_MS=15000,READ_TIMEOUT_MS=50000;
    private static final long MAX_PDF_BYTES=96L*1024*1024;
    private static final int MAX_PAGES=2500,MAX_PAGE_CHARS=90000;
    private static final int MAX_RESULTS=9;

    public interface Progress { void update(String message); }
    private static File file(Context c,String id,String extension) {
        return new File(c.getFilesDir(),"yfk_2026_"+id+"_private."+extension);
    }
    private static void notify(Progress p,String msg){if(p!=null)p.update(msg);}
    private static void delete(File f){if(f.exists())f.delete();}
    public static int installedCount(Context c) {
        int count=0;
        for(MusaAiYfkTechnicalSources.Volume v:MusaAiYfkTechnicalSources.VOLUMES)
            if(isInstalled(c,v))count++;
        return count;
    }
    private static boolean isInstalled(Context c,MusaAiYfkTechnicalSources.Volume v) {
        if(c==null)return false;
        File pdf=file(c,v.id,"pdf"), db=file(c,v.id,"db");
        if(!pdf.isFile() || pdf.length()<20000 || !db.isFile())return false;
        try(SQLiteDatabase sqlite=SQLiteDatabase.openDatabase(db.getAbsolutePath(),
                null,SQLiteDatabase.OPEN_READONLY);
            Cursor cur=sqlite.rawQuery(
                "SELECT value FROM meta WHERE key='complete' LIMIT 1",null)) {
            return cur.moveToFirst() && "1".equals(cur.getString(0));
        }catch(Exception ignored){return false;}
    }
    public static String status(Context c) {
        StringBuilder b=new StringBuilder("2026 YFK MEKANİK ANALİZ KÜTÜPHANESİ");
        for(MusaAiYfkTechnicalSources.Volume v:MusaAiYfkTechnicalSources.VOLUMES)
            b.append("\n• ").append(v.title).append(": ")
                .append(isInstalled(c,v)?"cihazda hazır":"henüz kurulmadı");
        b.append("\nYıl/sürüm: ").append(MusaAiYfkTechnicalSources.EDITION)
            .append(". İçerik: tarif ve analiz için kaynaklı sayfa araması. ")
            .append("Resmî poz karşılığı ve metraj ayrıca doğrulanır; fiyat otomatik atanmaz.");
        return b.toString();
    }
    public static String downloadAll(Context c,Progress progress)throws IOException {
        if(c==null)throw new IOException("Android uygulama bağlamı yok.");
        final Context app=c.getApplicationContext();
        PDFBoxResourceLoader.init(app);
        for(MusaAiYfkTechnicalSources.Volume volume:MusaAiYfkTechnicalSources.VOLUMES) {
            if(Thread.currentThread().isInterrupted())throw new IOException("İşlem iptal edildi.");
            if(isInstalled(app,volume))continue;
            notify(progress,volume.title+" • resmî kaynak indiriliyor");
            installVolume(app,volume,progress);
        }
        return status(app);
    }
    private static void installVolume(Context app,MusaAiYfkTechnicalSources.Volume v,
                                      Progress progress)throws IOException {
        File pdfPart=file(app,v.id,"pdf.part"),dbPart=file(app,v.id,"db.part");
        delete(pdfPart);delete(dbPart);
        try{
            download(v.url,pdfPart);
            notify(progress,v.title+" • sayfa/poz dizini hazırlanıyor");
            try(PDDocument doc=PDDocument.load(pdfPart)) {
                int pages=doc.getNumberOfPages();
                if(pages<2 || pages>MAX_PAGES)
                    throw new IOException("Beklenmeyen analiz cildi sayfa sayısı: "+pages);
                SQLiteDatabase sqlite=SQLiteDatabase.openOrCreateDatabase(dbPart,null);
                try{
                    sqlite.execSQL("CREATE TABLE pages(page INTEGER PRIMARY KEY,body TEXT NOT NULL,folded TEXT NOT NULL)");
                    sqlite.execSQL("CREATE TABLE positions(code TEXT NOT NULL,page INTEGER NOT NULL,excerpt TEXT NOT NULL,PRIMARY KEY(code,page))");
                    sqlite.execSQL("CREATE INDEX code_index ON positions(code)");
                    sqlite.execSQL("CREATE TABLE meta(key TEXT PRIMARY KEY,value TEXT NOT NULL)");
                    PDFTextStripper stripper=new PDFTextStripper();
                    stripper.setSortByPosition(true);
                    sqlite.beginTransaction();
                    try{
                        int recorded=0;
                        for(int page=1;page<=pages;page++){
                            if(Thread.currentThread().isInterrupted())
                                throw new IOException("Analiz dizini oluşturma iptal edildi.");
                            stripper.setStartPage(page);stripper.setEndPage(page);
                            String body=stripper.getText(doc);
                            if(body==null)body="";
                            if(body.length()>MAX_PAGE_CHARS)body=body.substring(0,MAX_PAGE_CHARS);
                            ContentValues row=new ContentValues();
                            row.put("page",page);row.put("body",body);
                            row.put("folded",MusaAiYfkTechnicalSources.normalize(body));
                            sqlite.insertOrThrow("pages",null,row);
                            for(String code:MusaAiYfkTechnicalSources.codesIn(body)){
                                int at=body.indexOf(code);
                                if(at<0)continue;
                                int start=Math.max(0,at-80),end=Math.min(body.length(),at+340);
                                String snippet=body.substring(start,end).replaceAll("[\\r\\n]+"," ").replaceAll("\\s+"," ").trim();
                                ContentValues pos=new ContentValues();
                                pos.put("code",code);pos.put("page",page);
                                pos.put("excerpt",snippet);
                                sqlite.insertWithOnConflict("positions",null,pos,SQLiteDatabase.CONFLICT_IGNORE);
                                recorded++;
                            }
                            if(page%25==0 || page==pages)
                                notify(progress,v.title+": "+page+"/"+pages+" sayfa tarandı");
                        }
                        putMeta(sqlite,"complete","1");
                        putMeta(sqlite,"edition",MusaAiYfkTechnicalSources.EDITION);
                        putMeta(sqlite,"url",v.url);
                        putMeta(sqlite,"pages",Integer.toString(pages));
                        putMeta(sqlite,"position_occurrences",Integer.toString(recorded));
                        sqlite.setTransactionSuccessful();
                    }finally{sqlite.endTransaction();}
                }finally{sqlite.close();}
            }
            if(!pdfPart.renameTo(file(app,v.id,"pdf")))
                throw new IOException("İndirilen analiz PDF'si saklanamadı.");
            if(!dbPart.renameTo(file(app,v.id,"db")))
                throw new IOException("Analiz indeks dosyası saklanamadı.");
            if(!isInstalled(app,v))throw new IOException("Kurulan mekanik analiz cildi doğrulanamadı.");
        }catch(IOException e){throw e;}
        catch(OutOfMemoryError e){throw new IOException("Analiz cildi için telefonun kullanılabilir belleği yetersiz.",e);}
        catch(Exception e){throw new IOException(v.title+" indekslenemedi: "+e.getMessage(),e);}
        finally{delete(pdfPart);delete(dbPart);}
    }
    private static void putMeta(SQLiteDatabase db,String key,String val) {
        ContentValues row=new ContentValues();row.put("key",key);row.put("value",val);
        db.insertWithOnConflict("meta",null,row,SQLiteDatabase.CONFLICT_REPLACE);
    }
    private static void download(String source,File dest)throws IOException {
        if(!MusaAiYfkTechnicalSources.isOfficial(source))
            throw new IOException("Onaylı resmî YFK kaynağı değil.");
        HttpURLConnection connection=null;
        try {
            connection=(HttpURLConnection)new URL(source).openConnection();
            connection.setInstanceFollowRedirects(false);
            connection.setConnectTimeout(CONNECT_TIMEOUT_MS);
            connection.setReadTimeout(READ_TIMEOUT_MS);
            connection.setRequestProperty("Accept","application/pdf");
            connection.setRequestProperty("User-Agent","MusaCAD/1.2 (device-private technical index)");
            int http=connection.getResponseCode();
            if(http!=200)throw new IOException("Resmî YFK sunucusu HTTP "+http+" yanıtı verdi.");
            int declared=connection.getContentLength();
            if(declared>MAX_PDF_BYTES)throw new IOException("PDF izin verilen cihaz başına boyutu aşıyor.");
            long size=0;
            try(InputStream in=new BufferedInputStream(connection.getInputStream());
                OutputStream out=new BufferedOutputStream(new FileOutputStream(dest))){
                byte[] buffer=new byte[32768];int n;
                while((n=in.read(buffer))!=-1) {
                    if(Thread.currentThread().isInterrupted())throw new IOException("İndirme iptal edildi.");
                    size+=n;
                    if(size>MAX_PDF_BYTES)throw new IOException("PDF 96 MB sınırını aşıyor.");
                    out.write(buffer,0,n);
                }
            }
            if(size<20000)throw new IOException("PDF eksik veya boş indirildi.");
            try(InputStream in=new FileInputStream(dest)){
                byte[] magic=new byte[4];
                if(in.read(magic)!=4 || magic[0]!='%' || magic[1]!='P' ||
                    magic[2]!='D' || magic[3]!='F')
                    throw new IOException("Sunucu PDF yerine farklı bir dosya gönderdi.");
            }
        }finally{if(connection!=null)connection.disconnect();}
    }
    private static String escapeLike(String raw) {
        return raw.replace("\\","\\\\").replace("%","\\%").replace("_","\\_");
    }
    private static String compact(String text,String term,int before,int after) {
        if(text==null)return "";
        int at=MusaAiYfkTechnicalSources.normalize(text).indexOf(
            MusaAiYfkTechnicalSources.normalize(term));
        // The normalized string may not preserve original character offsets;
        // use an original substring search for offsets where possible.
        int exact=text.toLowerCase(new Locale("tr","TR")).indexOf(term.toLowerCase(new Locale("tr","TR")));
        if(exact>=0)at=exact;
        if(at<0)at=0;
        int from=Math.max(0,Math.min(at,text.length())-before);
        int until=Math.min(text.length(),Math.min(at,text.length())+after);
        return (from>0?"…":"")+text.substring(from,until)
            .replaceAll("[\\r\\n]+"," ").replaceAll("\\s+"," ").trim()+
            (until<text.length()?"…":"");
    }
    public static String find(Context c,String raw) {
        String term=MusaAiYfkTechnicalSources.query(raw);
        if(term.isEmpty())return "Analiz kitabında aranacak poz kodu veya malzeme adı belirtilmedi.";
        if(installedCount(c)==0)return "Mekanik analiz ciltleri kurulu değil. “Mekanik analizleri indir” yazın.";
        boolean byCode=term.matches("25\\.\\d{3}\\.\\d{4}(?:/\\d{1,3})?");
        StringBuilder b=new StringBuilder("YFK 2026 MEKANİK TARİF / ANALİZ ARAMASI\nSorgu: ")
            .append(term).append("\n");
        int hits=0;
        for(MusaAiYfkTechnicalSources.Volume v:MusaAiYfkTechnicalSources.VOLUMES) {
            if(!isInstalled(c,v))continue;
            try(SQLiteDatabase db=SQLiteDatabase.openDatabase(file(c,v.id,"db").getAbsolutePath(),
                    null,SQLiteDatabase.OPEN_READONLY)){
                String sql=byCode?
                    "SELECT page,excerpt FROM positions WHERE code=? ORDER BY page LIMIT 3":
                    "SELECT page,body FROM pages WHERE folded LIKE ? ESCAPE '\\' ORDER BY page LIMIT 3";
                String search=byCode?term:"%"+escapeLike(MusaAiYfkTechnicalSources.normalize(term))+"%";
                try(Cursor cur=db.rawQuery(sql,new String[]{search})){
                    while(cur.moveToNext()&&hits<MAX_RESULTS){
                        int page=cur.getInt(0);
                        String excerpt=byCode?cur.getString(1):compact(cur.getString(1),term,75,270);
                        hits++;
                        b.append("\n• ").append(v.title).append(" • sayfa ").append(page)
                            .append("\n").append(excerpt)
                            .append("\nKaynak: ").append(v.url).append("#page=").append(page).append("\n");
                    }
                }
            }catch(Exception e){b.append("\n• ").append(v.title).append(": indeks okunamadı.");}
        }
        if(hits==0)b.append("\nAranan ifadeyle eşleşme bulunamadı. Yazımı veya poz kodunu kontrol edin.");
        b.append("\nNOT: Bu arama resmî analiz ciltlerindeki kaynak sayfalara yönlendirir. ")
            .append("Bir DWG nesnesinin o pozla teknik uygunluğu, imalat kapsamı ve birimi ")
            .append("doğrulanmadan eşleştirme kesinleşmez. Fiyat üretilmez.");
        return b.toString();
    }

    /** Conservative drawing-material candidate lookup; not an automatic poz assignment. */
    public static String suggestForTakeoff(Context c,MusaAiCsbEstimate.Result measured) {
        if(measured==null || measured.rows.isEmpty())
            return "Projeden doğrulanabilir malzeme metrajı çıkarılamadı.";
        if(installedCount(c)==0)
            return "Önce “Mekanik analizleri indir” komutuyla resmî analizleri cihaza kurun.";
        StringBuilder b=new StringBuilder("YFK 2026 • PROJE MALZEMESİ / POZ ÖN EŞLEŞTİRME");
        int seen=0;
        for(MusaAiCsbEstimate.Row row:measured.rows) {
            if(seen++>=10){b.append("\nDiğer kalemler bu ön izlemede gösterilmedi.");break;}
            String desc=MusaAiYfkTechnicalSources.normalize(row.description);
            String key=termFromDescription(desc);
            b.append("\n\n• ").append(row.description).append(" (")
                .append(row.quantity).append(" ").append(row.unit).append(")");
            if(key.isEmpty()){b.append("\n  Resmî poz adayı için yeterli teknik tanım yok.");continue;}
            String found=findCandidate(c,key);
            b.append("\n  Analiz arama terimi: ").append(key).append("\n  ").append(found);
        }
        b.append("\n\nBu liste sayfa/poz ARAMA ADAYIDIR: boru çapı, sınıfı, standardı, ")
            .append("malzeme, montaj kapsamı ve aynı imalatın mükerrer sayılmadığı ")
            .append("teyit edilmeden keşfe kesin poz veya fiyat atanmaz.");
        return b.toString();
    }
    private static String termFromDescription(String d) {
        for(String s:new String[]{"hidrofor","boyler","klozet","lavabo","pisuvar",
            "batarya","evye","suzgec","pprc","polipropilen","pvc","galvaniz",
            "bakir","yangin pompasi","hava kanali","boru","vana"})
            if(d.contains(s))return s;
        return "";
    }
    private static String findCandidate(Context c,String term) {
        for(MusaAiYfkTechnicalSources.Volume v:MusaAiYfkTechnicalSources.VOLUMES) {
            if(!isInstalled(c,v))continue;
            try(SQLiteDatabase db=SQLiteDatabase.openDatabase(file(c,v.id,"db").getAbsolutePath(),
                        null,SQLiteDatabase.OPEN_READONLY);
                Cursor cur=db.rawQuery(
                    "SELECT page,body FROM pages WHERE folded LIKE ? ESCAPE '\\' ORDER BY page LIMIT 1",
                    new String[]{"%"+escapeLike(term)+"%"})){
                if(!cur.moveToFirst())continue;
                int page=cur.getInt(0);
                String context=compact(cur.getString(1),term,75,160);
                return v.title+" • s."+page+" (ADAY)\n  "+context+
                    "\n  "+v.url+"#page="+page;
            }catch(Exception ignored){}
        }
        return "Bu terimle cihazdaki analizlerde sayfa adayı bulunamadı.";
    }
    private MusaAiYfkTechnicalLibrary(){}
}
