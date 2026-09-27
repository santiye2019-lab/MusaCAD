package com.musa.cad;

import android.app.*;
import android.content.*;
import android.graphics.*;
import android.graphics.pdf.PdfRenderer;
import android.net.Uri;
import android.os.*;
import android.provider.OpenableColumns;
import android.view.*;
import android.widget.*;
import androidx.appcompat.app.AppCompatActivity;
import androidx.core.graphics.Insets;
import androidx.core.view.ViewCompat;
import androidx.core.view.WindowCompat;
import androidx.core.view.WindowInsetsCompat;
import java.io.*;
import java.util.concurrent.*;

/** In-app PDF and modern Office document reader for MusaCAD. */
public final class DocumentViewerActivity extends AppCompatActivity {
    public static final String EXTRA_NAME="com.musa.cad.DOCUMENT_NAME";

    private final ExecutorService worker=Executors.newSingleThreadExecutor();
    private Uri uri;private String displayName,mime;
    private LinearLayout root;private FrameLayout contentHost;private TextView status,pageLabel;
    private LinearLayout pdfNav;private Button prev,next;private ImageView pageImage;
    private ParcelFileDescriptor pdfFd;private PdfRenderer pdfRenderer;private File pdfTemp;private Bitmap pageBitmap;
    private int pageIndex,renderToken;

    @Override protected void onCreate(Bundle state){
        super.onCreate(state);WindowCompat.setDecorFitsSystemWindows(getWindow(),false);
        uri=getIntent().getData();if(uri==null){finish();return;}
        displayName=getIntent().getStringExtra(EXTRA_NAME);if(displayName==null||displayName.trim().isEmpty())displayName=nameOf(uri);
        mime=getContentResolver().getType(uri);if(mime==null)mime=CadDocumentSupport.bestMime(displayName,null);
        buildUi();
        CadDocumentSupport.Kind kind=CadDocumentSupport.kind(displayName,mime);
        if(kind==CadDocumentSupport.Kind.PDF)openPdf();
        else if(CadDocumentSupport.readableInApp(displayName,mime))openTextDocument();
        else showLegacyOrUnsupported();
    }

    private void buildUi(){
        root=new LinearLayout(this);root.setOrientation(LinearLayout.VERTICAL);root.setBackgroundColor(Color.rgb(7,16,21));
        setContentView(root);
        ViewCompat.setOnApplyWindowInsetsListener(root,(v,insets)->{Insets bars=insets.getInsets(WindowInsetsCompat.Type.systemBars());v.setPadding(0,bars.top,0,bars.bottom);return insets;});

        LinearLayout header=new LinearLayout(this);header.setOrientation(LinearLayout.HORIZONTAL);header.setGravity(Gravity.CENTER_VERTICAL);header.setPadding(dp(8),dp(6),dp(8),dp(6));header.setBackgroundColor(Color.rgb(10,34,47));
        Button back=new Button(this);back.setText("‹");back.setTextSize(27);back.setTextColor(Color.WHITE);back.setAllCaps(false);back.setBackgroundColor(Color.TRANSPARENT);back.setOnClickListener(v->finish());header.addView(back,new LinearLayout.LayoutParams(dp(48),dp(48)));
        LinearLayout titles=new LinearLayout(this);titles.setOrientation(LinearLayout.VERTICAL);
        TextView title=new TextView(this);title.setText(displayName);title.setTextColor(Color.WHITE);title.setTextSize(15);title.setSingleLine(true);title.setEllipsize(android.text.TextUtils.TruncateAt.MIDDLE);
        TextView type=new TextView(this);type.setText(CadDocumentSupport.displayType(displayName,mime)+" • MusaCAD belge görüntüleyici");type.setTextColor(Color.rgb(151,190,205));type.setTextSize(10);
        titles.addView(title);titles.addView(type);header.addView(titles,new LinearLayout.LayoutParams(0,ViewGroup.LayoutParams.WRAP_CONTENT,1f));
        Button external=new Button(this);external.setText("Dışarıda aç");external.setAllCaps(false);external.setTextSize(10);external.setTextColor(Color.WHITE);external.setOnClickListener(v->openExternal());header.addView(external,new LinearLayout.LayoutParams(dp(92),dp(42)));
        root.addView(header,new LinearLayout.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT,ViewGroup.LayoutParams.WRAP_CONTENT));

        status=new TextView(this);status.setTextColor(Color.rgb(189,211,220));status.setTextSize(11);status.setPadding(dp(12),dp(7),dp(12),dp(7));status.setText("Belge hazırlanıyor…");root.addView(status);
        contentHost=new FrameLayout(this);root.addView(contentHost,new LinearLayout.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT,0,1f));

        pdfNav=new LinearLayout(this);pdfNav.setOrientation(LinearLayout.HORIZONTAL);pdfNav.setGravity(Gravity.CENTER);pdfNav.setPadding(dp(8),dp(5),dp(8),dp(7));pdfNav.setVisibility(View.GONE);
        prev=new Button(this);prev.setText("‹ Önceki");prev.setAllCaps(false);prev.setOnClickListener(v->showPdfPage(pageIndex-1));pdfNav.addView(prev,new LinearLayout.LayoutParams(0,dp(44),1f));
        pageLabel=new TextView(this);pageLabel.setGravity(Gravity.CENTER);pageLabel.setTextColor(Color.WHITE);pageLabel.setTextSize(13);pdfNav.addView(pageLabel,new LinearLayout.LayoutParams(dp(100),dp(44)));
        next=new Button(this);next.setText("Sonraki ›");next.setAllCaps(false);next.setOnClickListener(v->showPdfPage(pageIndex+1));pdfNav.addView(next,new LinearLayout.LayoutParams(0,dp(44),1f));
        root.addView(pdfNav);
    }

    private void openPdf(){
        status.setText("PDF açılıyor…");
        worker.submit(()->{
            try{
                try{
                    pdfFd=getContentResolver().openFileDescriptor(uri,"r");
                    if(pdfFd==null)throw new IOException("PDF dosyası açılamadı");
                    pdfRenderer=new PdfRenderer(pdfFd);
                }catch(Exception first){
                    closePdfObjects();pdfTemp=File.createTempFile("MusaCAD_pdf_",".pdf",getCacheDir());
                    try(InputStream in=getContentResolver().openInputStream(uri);OutputStream out=new FileOutputStream(pdfTemp)){
                        if(in==null)throw new IOException("PDF dosyası okunamadı");byte[] buf=new byte[64*1024];int n;long total=0;while((n=in.read(buf))!=-1){total+=n;if(total>512L*1024L*1024L)throw new IOException("PDF 512 MB sınırını aşıyor");out.write(buf,0,n);}
                    }
                    pdfFd=ParcelFileDescriptor.open(pdfTemp,ParcelFileDescriptor.MODE_READ_ONLY);pdfRenderer=new PdfRenderer(pdfFd);
                }
                if(pdfRenderer.getPageCount()<1)throw new IOException("PDF sayfası bulunamadı");
                runOnUiThread(()->setupPdfUi());
            }catch(Exception e){showError("PDF açılamadı: "+safeMessage(e));}
        });
    }

    private void setupPdfUi(){
        if(isFinishing()||isDestroyed()||pdfRenderer==null)return;
        ScrollView scroll=new ScrollView(this);scroll.setFillViewport(true);scroll.setBackgroundColor(Color.rgb(35,39,42));
        pageImage=new ImageView(this);pageImage.setAdjustViewBounds(true);pageImage.setScaleType(ImageView.ScaleType.FIT_CENTER);pageImage.setBackgroundColor(Color.WHITE);int p=dp(8);pageImage.setPadding(p,p,p,p);
        scroll.addView(pageImage,new ScrollView.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT,ViewGroup.LayoutParams.WRAP_CONTENT));contentHost.removeAllViews();contentHost.addView(scroll);
        pdfNav.setVisibility(View.VISIBLE);status.setText("PDF • "+pdfRenderer.getPageCount()+" sayfa • sayfa seçimi alttan yapılır");showPdfPage(0);
    }

    private void showPdfPage(int requested){
        PdfRenderer renderer=pdfRenderer;if(renderer==null)return;int count=renderer.getPageCount();int index=Math.max(0,Math.min(count-1,requested));pageIndex=index;int token=++renderToken;
        prev.setEnabled(index>0);next.setEnabled(index+1<count);pageLabel.setText((index+1)+" / "+count);status.setText("PDF • Sayfa "+(index+1)+" hazırlanıyor…");
        worker.submit(()->{
            Bitmap bitmap=null;PdfRenderer.Page page=null;
            try{
                page=renderer.openPage(index);int targetW=Math.min(2400,Math.max(900,getResources().getDisplayMetrics().widthPixels*2));float ratio=page.getHeight()/(float)Math.max(1,page.getWidth());int targetH=Math.max(1,Math.round(targetW*ratio));
                bitmap=Bitmap.createBitmap(targetW,targetH,Bitmap.Config.ARGB_8888);bitmap.eraseColor(Color.WHITE);
                Rect rect=new Rect(0,0,targetW,targetH);page.render(bitmap,rect,null,PdfRenderer.Page.RENDER_MODE_FOR_DISPLAY);final Bitmap ready=bitmap;bitmap=null;
                runOnUiThread(()->{
                    if(token!=renderToken||isFinishing()||isDestroyed()){ready.recycle();return;}
                    Bitmap old=pageBitmap;pageBitmap=ready;pageImage.setImageBitmap(ready);if(old!=null&&!old.isRecycled())old.recycle();
                    status.setText("PDF • Sayfa "+(pageIndex+1)+" / "+renderer.getPageCount());
                });
            }catch(Exception e){if(bitmap!=null&&!bitmap.isRecycled())bitmap.recycle();showError("PDF sayfası gösterilemedi: "+safeMessage(e));}
            finally{if(page!=null)page.close();}
        });
    }

    private void openTextDocument(){
        status.setText(CadDocumentSupport.displayType(displayName,mime)+" • içerik okunuyor…");
        worker.submit(()->{
            try(InputStream in=getContentResolver().openInputStream(uri)){
                if(in==null)throw new IOException("Belge açılamadı");String text=OfficeTextExtractor.extract(in,displayName,mime);
                runOnUiThread(()->showText(text));
            }catch(Exception e){showError("Belge okunamadı: "+safeMessage(e));}
        });
    }

    private void showText(String value){
        if(isFinishing()||isDestroyed())return;ScrollView scroll=new ScrollView(this);scroll.setFillViewport(true);scroll.setBackgroundColor(Color.WHITE);
        TextView text=new TextView(this);text.setText(value==null||value.isEmpty()?"[Belgede gösterilebilir metin bulunamadı.]":value);text.setTextColor(Color.rgb(20,25,28));text.setTextSize(14);text.setTextIsSelectable(true);text.setPadding(dp(18),dp(16),dp(18),dp(28));
        if(CadDocumentSupport.kind(displayName,mime)==CadDocumentSupport.Kind.XLSX||CadDocumentSupport.kind(displayName,mime)==CadDocumentSupport.Kind.CSV)text.setTypeface(Typeface.MONOSPACE);
        scroll.addView(text,new ScrollView.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT,ViewGroup.LayoutParams.WRAP_CONTENT));contentHost.removeAllViews();contentHost.addView(scroll);
        status.setText(CadDocumentSupport.displayType(displayName,mime)+" • çevrimdışı okunabilir metin görünümü");
    }

    private void showLegacyOrUnsupported(){
        TextView info=new TextView(this);info.setGravity(Gravity.CENTER);info.setTextColor(Color.WHITE);info.setTextSize(15);info.setPadding(dp(30),dp(30),dp(30),dp(30));
        if(CadDocumentSupport.isLegacyOffice(displayName,mime))info.setText("Bu dosya eski ikili Office biçiminde ("+CadDocumentSupport.displayType(displayName,mime)+").\n\nMusaCAD, DOCX / XLSX / PPTX dosyalarını uygulama içinde çevrimdışı okur. Bu eski dosya için “Dışarıda aç” kullanılabilir.");
        else info.setText("Bu belge biçimi MusaCAD belge görüntüleyicisinde desteklenmiyor.");
        contentHost.removeAllViews();contentHost.addView(info);status.setText("Belge biçimi • sınırlı destek");
    }

    private void openExternal(){
        try{
            Intent view=new Intent(Intent.ACTION_VIEW);view.setDataAndType(uri,CadDocumentSupport.bestMime(displayName,mime));view.addFlags(Intent.FLAG_GRANT_READ_URI_PERMISSION);startActivity(Intent.createChooser(view,"Belgeyi aç"));
        }catch(Exception e){Toast.makeText(this,"Bu belgeyi açabilecek başka uygulama bulunamadı",Toast.LENGTH_LONG).show();}
    }

    private String nameOf(Uri value){
        try(android.database.Cursor c=getContentResolver().query(value,new String[]{OpenableColumns.DISPLAY_NAME},null,null,null)){if(c!=null&&c.moveToFirst()){String n=c.getString(0);if(n!=null&&!n.trim().isEmpty())return n;}}catch(Exception ignored){}
        String path=value.getLastPathSegment();return path==null||path.trim().isEmpty()?"Belge":path;
    }

    private void showError(String message){runOnUiThread(()->{status.setText(message);TextView error=new TextView(this);error.setText(message);error.setGravity(Gravity.CENTER);error.setTextColor(Color.rgb(255,190,190));error.setTextSize(14);error.setPadding(dp(24),dp(24),dp(24),dp(24));contentHost.removeAllViews();contentHost.addView(error);});}
    private static String safeMessage(Throwable e){return e==null||e.getMessage()==null||e.getMessage().trim().isEmpty()?"Bilinmeyen hata":e.getMessage();}
    private int dp(int value){return Math.round(value*getResources().getDisplayMetrics().density);}

    private void closePdfObjects(){
        PdfRenderer r=pdfRenderer;pdfRenderer=null;if(r!=null)try{r.close();}catch(Exception ignored){}
        ParcelFileDescriptor fd=pdfFd;pdfFd=null;if(fd!=null)try{fd.close();}catch(Exception ignored){}
    }

    @Override protected void onDestroy(){
        renderToken++;worker.shutdownNow();if(pageImage!=null)pageImage.setImageDrawable(null);if(pageBitmap!=null&&!pageBitmap.isRecycled())pageBitmap.recycle();pageBitmap=null;closePdfObjects();if(pdfTemp!=null)pdfTemp.delete();super.onDestroy();
    }
}
