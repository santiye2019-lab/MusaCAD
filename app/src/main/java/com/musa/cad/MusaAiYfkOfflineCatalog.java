package com.musa.cad;

import android.content.Context;
import android.database.Cursor;
import android.database.sqlite.SQLiteDatabase;
import android.content.ContentValues;

import com.tom_roush.pdfbox.android.PDFBoxResourceLoader;
import com.tom_roush.pdfbox.pdmodel.PDDocument;
import com.tom_roush.pdfbox.text.PDFTextStripper;

import java.io.*;
import java.net.HttpURLConnection;
import java.net.URL;
import java.security.MessageDigest;
import java.util.Locale;

/**
 * App-private user-initiated copy of the original 2026 YFK PDF and full-text
 * page index. It is NEVER shipped as a repo/app asset or redistributed.
 *
 * Price figures in PDF snippets are not automatically accepted as unit rates:
 * unit, installed-work scope, published period and edition must still be
 * verified before BOQ pricing. The source book is January 2026 only, not a
 * live monthly price feed.
 */
public final class MusaAiYfkOfflineCatalog {
    private static final String PDF_NAME="yfk_2026_offline_private.pdf";
    private static final String DB_NAME="yfk_2026_offline_private.db";
    private static final long MAX_DOWNLOAD=64L*1024L*1024L;
    private static final int MAX_PAGES=1600;
    private static final int MAX_CHARS_PER_PAGE=90000;
    private static final int MAX_RESULTS=8;
    private static final int CONNECT_TIMEOUT_MS=15000;
    private static final int READ_TIMEOUT_MS=45000;

    public interface Progress{
        void update(String message);
    }
    public static final class Status {
        public final boolean installed;
        public final int pages;
        public final String source,period;
        public final long bytes;
        private Status(boolean installed,int pages,String source,String period,long bytes){
            this.installed=installed;this.pages=pages;this.source=source;this.period=period;
            this.bytes=bytes;
        }
    }
    public static final class Lookup {
        public final String answer;
        public final int hits;
        public Lookup(String answer,int hits){this.answer=answer;this.hits=hits;}
    }

    private static File pdf(Context ctx){return new File(ctx.getFilesDir(),PDF_NAME);}
    private static File db(Context ctx){return new File(ctx.getFilesDir(),DB_NAME);}
    private static File stagedPdf(Context ctx){return new File(ctx.getFilesDir(),PDF_NAME+".part");}
    private static File stagedDb(Context ctx){return new File(ctx.getFilesDir(),DB_NAME+".part");}

    public static Status status(Context context){
        if(context==null)return new Status(false,0,"","",0);
        File file=pdf(context),index=db(context);
        if(!file.isFile()||file.length()<20000||!index.isFile())
            return new Status(false,0,"","",0);
        try(SQLiteDatabase sqlite=SQLiteDatabase.openDatabase(index.getAbsolutePath(),
                    null,SQLiteDatabase.OPEN_READONLY);
            Cursor cursor=sqlite.rawQuery("SELECT v FROM meta WHERE k='complete' LIMIT 1",null)){
            if(!cursor.moveToFirst()||!"1".equals(cursor.getString(0)))
                return new Status(false,0,"","",0);
            try(Cursor rows=sqlite.rawQuery("SELECT count(*) FROM pages",null)){
                int count=rows.moveToFirst()?rows.getInt(0):0;
                if(count<1)return new Status(false,0,"","",0);
                return new Status(true,count,MusaAiYfkCatalogQuery.SOURCE_URL,
                    MusaAiYfkCatalogQuery.PRICE_PERIOD,file.length());
            }
        }catch(Exception e){return new Status(false,0,"","",0);}
    }

    /**
     * The user explicitly consents before this method is called.
     * Invoke ONLY on a background thread. Progress is optional and receives
     * bounded messages; never show the PDF text itself as status.
     */
    public static Status downloadAndIndex(Context context,Progress progress)throws IOException{
        if(context==null)throw new IOException("Android uygulama bağlamı bulunamadı.");
        final Context app=context.getApplicationContext();
        final String official=MusaAiYfkCatalogQuery.SOURCE_URL;
        if(!MusaAiYfkCatalogQuery.isOfficialUrl(official))
            throw new IOException("Resmî bağlantı doğrulanamadı.");
        if(status(app).installed)return status(app);
        File temp=stagedPdf(app),index=stagedDb(app);
        delete(temp);delete(index);
        try{
            notify(progress,"2026 ÇŞİDB resmî kitabı indiriliyor…");
            download(official,temp,progress);
            notify(progress,"Kitap indirildi; PDF metin katmanı denetleniyor…");
            PDFBoxResourceLoader.init(app);
            try(PDDocument doc=PDDocument.load(temp)){
                int pages=doc.getNumberOfPages();
                if(pages<300||pages>MAX_PAGES)
                    throw new IOException("Beklenen tam 2026 birim fiyat kitabı yerine "+pages+" sayfa geldi.");
                PDFTextStripper stripper=new PDFTextStripper();
                stripper.setSortByPosition(true);
                stripper.setStartPage(1);
                stripper.setEndPage(Math.min(6,pages));
                String first=stripper.getText(doc);
                String firstFold=MusaAiYfkCatalogQuery.fold(first);
                if(!firstFold.contains("2026")||
                    !(firstFold.contains("yuksek fen")||firstFold.contains("iklim degisikligi")))
                    throw new IOException("PDF kapağı beklenen 2026 YFK kitabıyla uyuşmuyor.");
                SQLiteDatabase sqlite=SQLiteDatabase.openOrCreateDatabase(index,null);
                try{
                    sqlite.execSQL("CREATE TABLE pages (page INTEGER PRIMARY KEY,body TEXT NOT NULL,folded TEXT NOT NULL)");
                    sqlite.execSQL("CREATE TABLE meta (k TEXT PRIMARY KEY,v TEXT NOT NULL)");
                    sqlite.beginTransaction();
                    try{
                        ContentValues row=new ContentValues();
                        for(int page=1;page<=pages;page++){
                            if(Thread.currentThread().isInterrupted())
                                throw new IOException("Kitap indeksleme işlemi iptal edildi.");
                            stripper.setStartPage(page);
                            stripper.setEndPage(page);
                            String body=stripper.getText(doc);
                            if(body==null)body="";
                            if(body.length()>MAX_CHARS_PER_PAGE)
                                body=body.substring(0,MAX_CHARS_PER_PAGE);
                            row.clear();
                            row.put("page",page);
                            row.put("body",body);
                            row.put("folded",MusaAiYfkCatalogQuery.fold(body));
                            sqlite.insertOrThrow("pages",null,row);
                            if(page%20==0||page==pages)
                                notify(progress,"Poz arşivi hazırlanıyor: "+page+"/"+pages+" sayfa");
                        }
                        putMeta(sqlite,"complete","1");
                        putMeta(sqlite,"source",official);
                        putMeta(sqlite,"period",MusaAiYfkCatalogQuery.PRICE_PERIOD);
                        putMeta(sqlite,"pages",Integer.toString(pages));
                        sqlite.setTransactionSuccessful();
                    }finally{
                        sqlite.endTransaction();
                    }
                }finally{sqlite.close();}
            }
            // Keep previous, valid files until staging succeeded. No PDF text
            // or pages are committed into a public source tree.
            if(!temp.renameTo(pdf(app)))
                throw new IOException("PDF özel saklama alanına taşınamadı.");
            if(!index.renameTo(db(app)))
                throw new IOException("İndeks dosyası özel saklama alanına taşınamadı.");
            Status finalStatus=status(app);
            if(!finalStatus.installed)throw new IOException("İndirilen kitap doğrulanamadı.");
            notify(progress,"ÇŞİDB 2026 kitabı hazır: "+finalStatus.pages+" sayfa. Artık çevrim dışı arayabilirsiniz.");
            return finalStatus;
        }catch(IOException e){
            throw e;
        }catch(Exception e){
            throw new IOException("2026 kitap indirimi/indeksi tamamlanamadı: "+
                (e.getMessage()==null?"PDF okunamadı":e.getMessage()),e);
        }finally{
            delete(temp);
            delete(index);
        }
    }

    public static Lookup find(Context ctx,String input)throws IOException{
        String term=MusaAiYfkCatalogQuery.lookup(input);
        if(term.isEmpty())return new Lookup("Geçerli bir poz numarası veya arama terimi yazın.",0);
        if(!status(ctx).installed)
            return new Lookup("2026 ÇŞİDB kitabı bu cihazda kurulu değil. “ÇŞB kitabını indir” komutunu kullanın.",0);
        String target=MusaAiYfkCatalogQuery.fold(term).replace("\\","\\\\")
            .replace("%","\\%").replace("_","\\_");
        String sql="SELECT page,body FROM pages WHERE folded LIKE ? ESCAPE '\\' ORDER BY page LIMIT "+MAX_RESULTS;
        StringBuilder out=new StringBuilder("ÇŞİDB / Yüksek Fen Kurulu — 2026 OCAK");
        out.append("\nResmî kaynak: ").append(MusaAiYfkCatalogQuery.SOURCE_URL);
        out.append("\nArama: ").append(term);
        int hits=0;
        try(SQLiteDatabase sqlite=SQLiteDatabase.openDatabase(db(ctx).getAbsolutePath(),null,
                SQLiteDatabase.OPEN_READONLY);
            Cursor cursor=sqlite.rawQuery(sql,new String[]{"%"+target+"%"})){
            while(cursor.moveToNext()){
                String sample=MusaAiYfkCatalogQuery.safeSnippet(cursor.getString(1),term);
                if(sample.isEmpty())continue;
                hits++;
                out.append("\n\n• PDF sayfa ").append(cursor.getInt(0)).append(": ").append(sample);
            }
        }catch(Exception e){
            throw new IOException("Çevrim dışı poz araması yapılamadı.",e);
        }
        if(hits==0)out.append("\nBu ifadeyle örtüşen PDF metni bulunamadı.");
        if(hits>=MAX_RESULTS)out.append("\nİlk 8 eşleşen sayfa gösterildi; daha özel bir terim/poz kodu kullanın.");
        out.append("\n\nNOT: Bu çıktı, resmî Ocak 2026 PDF metninin kaynaklı alıntısıdır; "+
            "otomatik onaylı poz fiyatı değildir. Güncel aylık revizyonlar, birim ve imalat tarifi "+
            "ayrıca doğrulanmadan keşfe fiyat uygulanmaz. Kitabın tüm sayfaları cihazda saklanır.");
        return new Lookup(out.toString(),hits);
    }

    private static void putMeta(SQLiteDatabase db,String key,String value){
        ContentValues vals=new ContentValues();
        vals.put("k",key);vals.put("v",value);
        db.insertWithOnConflict("meta",null,vals,SQLiteDatabase.CONFLICT_REPLACE);
    }

    private static void download(String source,File dest,Progress progress)throws IOException{
        HttpURLConnection connection=null;
        long written=0;
        try{
            URL url=new URL(source);
            if(!MusaAiYfkCatalogQuery.isOfficialUrl(url.toString()))
                throw new IOException("İzin verilen resmî indirme kaynağı değil.");
            connection=(HttpURLConnection)url.openConnection();
            connection.setConnectTimeout(CONNECT_TIMEOUT_MS);
            connection.setReadTimeout(READ_TIMEOUT_MS);
            connection.setInstanceFollowRedirects(false);
            connection.setRequestProperty("Accept","application/pdf");
            connection.setRequestProperty("User-Agent","MusaCAD/1.2 (private offline YFK catalog)");
            int status=connection.getResponseCode();
            if(status!=200)throw new IOException("Resmî PDF sunucusundan indirme başarısız: HTTP "+status);
            int len=connection.getContentLength();
            if(len>MAX_DOWNLOAD)throw new IOException("PDF indirme sınırından büyük.");
            try(InputStream input=new BufferedInputStream(connection.getInputStream());
                OutputStream output=new BufferedOutputStream(new FileOutputStream(dest))){
                byte[] buffer=new byte[32768];int n;
                while((n=input.read(buffer))!=-1){
                    if(Thread.currentThread().isInterrupted())
                        throw new IOException("İndirme iptal edildi.");
                    written+=n;
                    if(written>MAX_DOWNLOAD)throw new IOException("PDF 64 MB üst sınırını aştı.");
                    output.write(buffer,0,n);
                    if(written%(3*1024*1024)<32768)
                        notify(progress,"İndirildi: "+(written/(1024*1024))+" MB");
                }
            }
            if(written<20000)throw new IOException("İndirilen PDF beklenenden küçük.");
            try(InputStream input=new FileInputStream(dest)){
                byte[] magic=new byte[4];
                if(input.read(magic)!=4||magic[0]!='%'||magic[1]!='P'||
                    magic[2]!='D'||magic[3]!='F')
                    throw new IOException("İndirilen dosya PDF değil.");
            }
        }finally{
            if(connection!=null)connection.disconnect();
        }
    }
    private static void notify(Progress cb,String message){if(cb!=null)cb.update(message);}
    private static void delete(File f){if(f.exists())f.delete();}
    private MusaAiYfkOfflineCatalog(){}
}
