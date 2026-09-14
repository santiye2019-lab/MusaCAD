package com.musa.cad;

import android.app.*;
import android.content.*;
import android.graphics.*;
import android.graphics.pdf.PdfDocument;
import android.net.Uri;
import android.os.Bundle;
import android.provider.OpenableColumns;
import android.view.*;
import android.widget.*;
import androidx.appcompat.app.AppCompatActivity;
import androidx.core.content.FileProvider;
import androidx.core.view.ViewCompat;
import androidx.core.view.WindowCompat;
import androidx.core.view.WindowInsetsCompat;
import androidx.core.graphics.Insets;
import java.io.*;
import java.util.concurrent.*;

public class MainActivity extends AppCompatActivity {
    private static final int OPEN=20;
    private final ExecutorService loader=Executors.newSingleThreadExecutor();
    private LoadTask activeLoad;
    private static final class LoadTask {
        Future<?> future;AlertDialog dialog;TextView progress;
    }
    private static final class Loaded {
        File file;Bitmap bitmap;DxfParser.Result parsed;String name;boolean dxf;
        void dispose(){if(bitmap!=null)bitmap.recycle();if(file!=null)file.delete();}
    }
    private CheckBox snapToggle;
    private DxfParser.Result activeDxf;
    private CadView cad; private TextView fileName,result; private File currentFile;
    protected void onCreate(Bundle b){super.onCreate(b);
        WindowCompat.setDecorFitsSystemWindows(getWindow(),false);
        setContentView(R.layout.activity_main);
        View root=findViewById(R.id.mainRoot);
        ViewCompat.setOnApplyWindowInsetsListener(root,(view,insets)->{
            Insets bars=insets.getInsets(WindowInsetsCompat.Type.systemBars()|WindowInsetsCompat.Type.displayCutout());
            view.setPadding(bars.left,bars.top,bars.right,bars.bottom);
            return insets;
        });
        WindowCompat.getInsetsController(getWindow(),root).setAppearanceLightStatusBars(false);
        WindowCompat.getInsetsController(getWindow(),root).setAppearanceLightNavigationBars(false);
        ViewCompat.requestApplyInsets(root);
        cad=findViewById(R.id.cadView);fileName=findViewById(R.id.fileName);result=findViewById(R.id.resultText);cad.setListener(new CadView.Listener(){public void onMeasurement(String v){result.setText(v);updateModeButtons();}public void onCalibrationRequested(double px){showCalibration();}public void onSelectionReady(){previewSelection();}});
        snapToggle=findViewById(R.id.snapToggle);snapToggle.setOnCheckedChangeListener((button,checked)->cad.setSnapEnabled(checked));
        findViewById(R.id.appTitle).setOnClickListener(v->{
            String license;
            try(InputStream in=getAssets().open("COPYING-LibreDWG.txt")){ByteArrayOutputStream out=new ByteArrayOutputStream();byte[] bytes=new byte[4096];int n;while((n=in.read(bytes))!=-1)out.write(bytes,0,n);license=out.toString("UTF-8");}
            catch(IOException e){license="GPL-3.0-or-later";}
            TextView text=new TextView(this);text.setPadding(24,16,24,16);
            text.setText("MusaCAD — LibreDWG ile çevrimdışı DWG okuma\nKaynak kod: https://github.com/santiye2019-lab/MusaCAD\n\n"+license);
            android.text.util.Linkify.addLinks(text,android.text.util.Linkify.WEB_URLS);text.setMovementMethod(android.text.method.LinkMovementMethod.getInstance());
            ScrollView scroll=new ScrollView(this);scroll.addView(text);
            new AlertDialog.Builder(this).setTitle("Lisans ve kaynak kod").setView(scroll).setPositiveButton("KAPAT",null).show();
        });
        findViewById(R.id.homeButton).setOnClickListener(v->{startActivity(new Intent(this,HomeActivity.class).addFlags(Intent.FLAG_ACTIVITY_CLEAR_TOP));finish();});
        findViewById(R.id.infoButton).setOnClickListener(this::showDrawingMenu);
        findViewById(R.id.fileName).setOnClickListener(v->showDrawingInfo());
        findViewById(R.id.fitButton).setOnClickListener(v->cad.fitDrawing());
        updateModeButtons();
        findViewById(R.id.layersButton).setOnClickListener(v->showLayers());
        findViewById(R.id.openButton).setOnClickListener(v->open());
        findViewById(R.id.panButton).setOnClickListener(v->cad.setMode(CadView.Mode.PAN));
        findViewById(R.id.calibrateButton).setOnClickListener(v->cad.setMode(CadView.Mode.CALIBRATE));
        findViewById(R.id.distanceButton).setOnClickListener(v->cad.setMode(CadView.Mode.DISTANCE));
        findViewById(R.id.areaButton).setOnClickListener(v->cad.setMode(CadView.Mode.AREA));
        findViewById(R.id.undoButton).setOnClickListener(v->cad.undo());
        findViewById(R.id.clearButton).setOnClickListener(v->cad.clearMeasurement());
        findViewById(R.id.shareButton).setOnClickListener(v->showShare());
        if(getIntent().getData()!=null)startLoad(getIntent().getData());
    }
    private void showDrawingMenu(View anchor){
        PopupMenu menu=new PopupMenu(this,anchor);
        String[] names={"Çizim bilgisi","Katmanlar","Ekrana sığdır","Dışa aktar / paylaş","Ölçümü temizle","Yardım"};
        for(int i=0;i<names.length;i++)menu.getMenu().add(0,i,i,names[i]);
        menu.setOnMenuItemClickListener(item->{switch(item.getItemId()){
            case 0:showDrawingInfo();break;case 1:showLayers();break;case 2:cad.fitDrawing();break;
            case 3:showShare();break;case 4:cad.clearMeasurement();break;
            case 5:new AlertDialog.Builder(this).setTitle("Çizim araçları").setMessage("İki parmakla yakınlaştırın. Gezin aracıyla çizimi kaydırın. Mesafe veya Alan seçip noktaları işaretleyin. Yakalama açıkken çizgi uçlarına tutunur. Dosya birimi bilinmiyorsa Ölçek ile bilinen uzunluğu girin. Ekrana sığdır görünümü sıfırlar.").setPositiveButton("Kapat",null).show();break;
        }return true;});menu.show();
    }
    private void updateModeButtons(){
        int[] ids={R.id.panButton,R.id.calibrateButton,R.id.distanceButton,R.id.areaButton};
        CadView.Mode[] modes={CadView.Mode.PAN,CadView.Mode.CALIBRATE,CadView.Mode.DISTANCE,CadView.Mode.AREA};
        for(int i=0;i<ids.length;i++)findViewById(ids[i]).setSelected(cad.getMode()==modes[i]);
    }
    private String drawingSummary(){
        if(activeDxf==null)return "Yalnız önizleme • Ayrıntılar için ⓘ";
        String status=activeDxf.skippedCount>0?activeDxf.skippedCount+" nesne gösterilemedi • ":"Çizim hazır • ";
        return status+(cad.hasDrawingScale()?"Ölçüm hazır":"Ölçek gerekli")+" • ⓘ";
    }
    private void showDrawingInfo(){
        StringBuilder text=new StringBuilder();
        if(currentFile==null){text.append("Aç düğmesinden DWG veya DXF seçin.\n\nİki parmakla yakınlaştırın. Mesafe ve Alan araçlarıyla nokta seçin. Ölçek aracıyla bilinen bir uzunluğu tanımlayın.");}
        else if(activeDxf==null){text.append("Yalnız dosyaya gömülü önizleme okunabildi. Bu görüntüde vektör ayrıntıları ve katmanlar bulunmaz; ölçüler yaklaşık olur.");}
        else{
            text.append(activeDxf.entityCount).append(" görüntülenen nesne\n").append(activeDxf.visibleLayers.size()).append(" / ").append(activeDxf.layerCount).append(" görünür katman\n\n");
            text.append("Yakın planda vektör çizimi kullanılır. Yazı yerleşimleri ve bazı CAD öğeleri özgün programdaki görünümden farklı olabilir.\n\n");
            if(Double.isFinite(activeDxf.metersPerPixel))text.append("Dosya birimi: ").append(DxfUnits.label(activeDxf.unitCode)).append(". Ölçümler metre ve m² olarak gösterilir.");
            else text.append("Dosyanın fiziksel birimi tanımlı değil. Metre cinsinden ölçmek için Ölçek düğmesiyle bilinen uzunluğu girin.");
            if(cad.hasDrawingScale())text.append("\nGörünümde ölçüm ölçeği etkin.");
            if(activeDxf.skippedCount>0){text.append("\n\nGösterilemeyen / atlanan nesneler: ").append(activeDxf.skippedCount);
                for(java.util.Map.Entry<String,Integer> e:activeDxf.skippedTypes.entrySet())text.append("\n• ").append(e.getKey()).append(": ").append(e.getValue());
            }
            if(activeDxf.conversionWarnings!=0)text.append("\n\nDWG dönüştürücüsü uyarı bildirdi. Kritik ölçüleri bilinen bir ölçüyle karşılaştırın.");
        }
        TextView body=new TextView(this);body.setText(text);body.setTextSize(15);int pad=(int)(20*getResources().getDisplayMetrics().density);body.setPadding(pad,pad,pad,pad);body.setTextIsSelectable(true);
        ScrollView scroll=new ScrollView(this);scroll.addView(body);
        new AlertDialog.Builder(this).setTitle("Çizim bilgisi").setView(scroll).setPositiveButton("Kapat",null).show();
    }
    private void open(){Intent i=new Intent(Intent.ACTION_OPEN_DOCUMENT);i.setType("*/*");i.addCategory(Intent.CATEGORY_OPENABLE);i.putExtra(Intent.EXTRA_MIME_TYPES,new String[]{"application/acad","application/x-autocad","application/dwg","image/vnd.dwg","application/dxf","application/octet-stream"});startActivityForResult(i,OPEN);}
    protected void onActivityResult(int r,int c,Intent data){
        super.onActivityResult(r,c,data);
        if(r==OPEN&&c==RESULT_OK&&data!=null&&data.getData()!=null){try{getContentResolver().takePersistableUriPermission(data.getData(),Intent.FLAG_GRANT_READ_URI_PERMISSION);}catch(SecurityException ignored){}startLoad(data.getData());}
    }
    private void cancelLoad(){
        LoadTask task=activeLoad;activeLoad=null;
        if(task!=null){if(task.future!=null)task.future.cancel(true);task.dialog.dismiss();}
    }
    private void startLoad(Uri uri){
        cancelLoad();
        LoadTask task=new LoadTask();activeLoad=task;
        LinearLayout box=new LinearLayout(this);box.setOrientation(LinearLayout.VERTICAL);
        int pad=(int)(20*getResources().getDisplayMetrics().density);box.setPadding(pad,pad,pad,pad);
        box.addView(new ProgressBar(this));
        task.progress=new TextView(this);task.progress.setText("Dosya okunuyor…");box.addView(task.progress);
        task.dialog=new AlertDialog.Builder(this).setTitle("Çizim açılıyor").setView(box)
            .setNegativeButton("İPTAL",(d,w)->cancelLoad()).create();
        task.dialog.setOnCancelListener(d->cancelLoad());task.dialog.setCanceledOnTouchOutside(false);task.dialog.show();
        task.future=loader.submit(()->{
            Loaded loaded=new Loaded();
            try{
                FileTransfer.checkCancelled();
                loaded.name=nameOf(uri);
                loaded.dxf=loaded.name.toLowerCase(java.util.Locale.ROOT).endsWith(".dxf");
                loaded.file=File.createTempFile("MusaCAD_acilan_",loaded.dxf?".dxf":".dwg",getCacheDir());
                try(InputStream in=getContentResolver().openInputStream(uri);OutputStream out=new FileOutputStream(loaded.file)){
                    FileTransfer.copy(in,out,loaded.dxf?DxfStream.MAX_BYTES:32L*1024*1024,bytes->runOnUiThread(()->{
                        if(activeLoad==task)task.progress.setText(String.format(java.util.Locale.getDefault(),"Okunan: %.1f MB",bytes/1048576d));
                    }));
                }
                runOnUiThread(()->{if(activeLoad==task)task.progress.setText("Çizim hazırlanıyor…");});
                FileTransfer.checkCancelled();
                if(loaded.dxf){loaded.parsed=DxfParser.render(loaded.file);}
                else{
                    try{loaded.parsed=NativeDwg.read(loaded.file,getCacheDir());}
                    catch(InterruptedIOException cancelled){throw cancelled;}
                    catch(IOException|UnsatisfiedLinkError conversionError){
                        FileTransfer.checkCancelled();
                        loaded.bitmap=DwgPreview.read(loaded.file);
                        if(loaded.bitmap==null)throw new IOException("DWG geometri veya önizleme açılamadı",conversionError);
                    }
                }
                if(loaded.parsed!=null)loaded.bitmap=loaded.parsed.bitmap;
                FileTransfer.checkCancelled();
                if(loaded.bitmap==null)throw new IOException(loaded.dxf?"Desteklenen DXF geometrisi bulunamadı":"DWG içinde görüntülenebilir önizleme bulunamadı");
                runOnUiThread(()->{
                    if(activeLoad!=task||isFinishing()||isDestroyed()){loaded.dispose();return;}
                    activeLoad=null;task.dialog.dismiss();
                    currentFile=loaded.file;activeDxf=loaded.parsed;findViewById(R.id.layersButton).setEnabled(activeDxf!=null);
                    cad.setDrawing(loaded.bitmap);cad.setVectorDrawing(loaded.parsed);updateModeButtons();
                    if(loaded.parsed!=null&&Double.isFinite(loaded.parsed.metersPerPixel))cad.setDrawingScale(loaded.parsed.metersPerPixel);
                    snapToggle.setEnabled(loaded.parsed!=null&&loaded.parsed.snapPoints.length>0);
                    if(loaded.parsed!=null)cad.setSnapIndex(loaded.parsed.snapIndex);
                    RecentDrawings.remember(this,uri,loaded.name,loaded.bitmap);
                    fileName.setText(loaded.name);
                    result.setText(drawingSummary());
                });
            }catch(Exception | OutOfMemoryError e){
                loaded.dispose();
                runOnUiThread(()->{
                    if(activeLoad!=task||isFinishing()||isDestroyed())return;
                    activeLoad=null;task.dialog.dismiss();error(e instanceof Exception?(Exception)e:new IOException("Bu çizim için yeterli bellek yok"));
                });
            }
        });
    }
    @Override protected void onDestroy(){
        cancelLoad();loader.shutdownNow();super.onDestroy();
    }
    private void showLayers(){
        if(activeDxf==null||activeLoad!=null)return;
        String[] names=activeDxf.layerNames.toArray(new String[0]);
        java.util.Set<String> selected=new java.util.HashSet<>(activeDxf.visibleLayers);
        boolean[] checked=new boolean[names.length];
        for(int i=0;i<names.length;i++)checked[i]=selected.contains(names[i]);
        new AlertDialog.Builder(this).setTitle("Görünecek katmanlar")
            .setMultiChoiceItems(names,checked,(dialog,index,enabled)->{
                if(enabled)selected.add(names[index]);else selected.remove(names[index]);
            }).setPositiveButton("UYGULA",(d,w)->applyLayers(selected))
            .setNeutralButton("TÜMÜNÜ GÖSTER",(d,w)->applyLayers(new java.util.HashSet<>(activeDxf.layerNames)))
            .setNegativeButton("İPTAL",null).show();
    }
    private void applyLayers(java.util.Set<String> selected){
        if(activeDxf==null||activeLoad!=null||activeDxf.visibleLayers.equals(selected))return;
        DxfParser.Result source=activeDxf;
        LoadTask task=new LoadTask();activeLoad=task;
        task.dialog=new AlertDialog.Builder(this).setTitle("Katmanlar hazırlanıyor")
            .setMessage("Görünüm güncelleniyor…").setNegativeButton("İPTAL",(d,w)->cancelLoad()).create();
        task.dialog.setOnCancelListener(d->cancelLoad());task.dialog.setCanceledOnTouchOutside(false);task.dialog.show();
        task.future=loader.submit(()->{
            try{
                DxfParser.Result updated=source.withVisibleLayers(selected);
                runOnUiThread(()->{
                    if(activeLoad!=task||activeDxf!=source||isFinishing()||isDestroyed()){updated.bitmap.recycle();return;}
                    activeLoad=null;task.dialog.dismiss();
                    cad.replaceVisibleDrawing(updated.bitmap,updated.snapIndex);activeDxf=updated;cad.setVectorDrawing(updated);
                    snapToggle.setEnabled(updated.snapPoints.length>0);
                    result.setText(drawingSummary());
                });
            }catch(Exception|OutOfMemoryError e){
                runOnUiThread(()->{
                    if(activeLoad!=task||isFinishing()||isDestroyed())return;
                    activeLoad=null;task.dialog.dismiss();error(e instanceof Exception?(Exception)e:new IOException("Yeterli bellek yok"));
                });
            }
        });
    }
    private String nameOf(Uri u){try(android.database.Cursor c=getContentResolver().query(u,null,null,null,null)){if(c!=null&&c.moveToFirst()){int i=c.getColumnIndex(OpenableColumns.DISPLAY_NAME);if(i>=0)return c.getString(i);}}return "cizim.dwg";}
    private void showShare(){
        new AlertDialog.Builder(this).setTitle("Paylaş").setItems(new String[]{
            "Orijinal dosyayı paylaş","Görünümü PDF olarak paylaş","Görünümü resim olarak paylaş",
            "Alan seçerek paylaş"
        },(d,w)->{
            if(w==0)shareFile(currentFile,"application/octet-stream");
            else if(w==3){if(!cad.beginSelection())Toast.makeText(this,"Önce çizim açın",Toast.LENGTH_SHORT).show();}
            else {cad.cancelSelection();exportView(w==1);}
        }).show();
    }
    private void previewSelection(){
        final Bitmap bitmap;
        try{bitmap=cad.selectionSnapshot();}catch(Exception e){error(e);return;}
        ImageView preview=new ImageView(this);preview.setImageBitmap(bitmap);
        preview.setAdjustViewBounds(true);preview.setMaxHeight((int)(getResources().getDisplayMetrics().heightPixels*.55f));preview.setScaleType(ImageView.ScaleType.FIT_CENTER);
        int pad=(int)(12*getResources().getDisplayMetrics().density);preview.setPadding(pad,pad,pad,pad);
        AlertDialog dialog=new AlertDialog.Builder(this).setTitle("Seçili alan önizlemesi")
            .setView(preview).setPositiveButton("PNG PAYLAŞ",(d,w)->exportBitmap(bitmap,false,"alan"))
            .setNeutralButton("PDF PAYLAŞ",(d,w)->exportBitmap(bitmap,true,"alan"))
            .setNegativeButton("YENİDEN SEÇ",(d,w)->cad.beginSelection()).create();
        dialog.setOnCancelListener(d->cad.cancelSelection());
        dialog.setOnDismissListener(d->{preview.setImageDrawable(null);bitmap.recycle();});
        dialog.show();
    }
    private void showCalibration(){
        LinearLayout box=new LinearLayout(this);box.setOrientation(LinearLayout.VERTICAL);int p=(int)(16*getResources().getDisplayMetrics().density);box.setPadding(p,0,p,0);
        EditText value=new EditText(this);value.setHint("Gerçek uzunluk (ör. 2.50)");value.setInputType(android.text.InputType.TYPE_CLASS_NUMBER|android.text.InputType.TYPE_NUMBER_FLAG_DECIMAL);box.addView(value);
        Spinner units=new Spinner(this);units.setAdapter(new ArrayAdapter<String>(this,android.R.layout.simple_spinner_dropdown_item,new String[]{"m","cm","mm"}));box.addView(units);
        AlertDialog dialog=new AlertDialog.Builder(this).setTitle("Ölçeği ayarla").setView(box).setPositiveButton("UYGULA",null).setNegativeButton("İPTAL",(d,w)->cad.clearMeasurement()).create();
        dialog.setOnCancelListener(d->cad.clearMeasurement());
        dialog.setOnShowListener(d->dialog.getButton(AlertDialog.BUTTON_POSITIVE).setOnClickListener(v->{try{
            double n=Double.parseDouble(value.getText().toString().replace(',','.'));cad.setCalibration(n,units.getSelectedItem().toString());dialog.dismiss();
        }catch(Exception e){value.setError("Sıfırdan büyük bir uzunluk girin; iki farklı nokta seçin.");}}));
        dialog.show();
    }
    private File exportDir(){File d=new File(getCacheDir(),"exports");d.mkdirs();return d;}
    private void exportView(boolean pdf){
        Bitmap bitmap=null;
        try{bitmap=cad.snapshot();exportBitmap(bitmap,pdf,"gorunum");}
        catch(Exception e){error(e);}
        finally{if(bitmap!=null)bitmap.recycle();}
    }
    private void exportBitmap(Bitmap bitmap,boolean pdf,String suffix){
        try{
            File file=File.createTempFile("MusaCAD_"+suffix+"_",pdf?".pdf":".png",exportDir());
            if(pdf){
                PdfDocument document=new PdfDocument();
                try{
                    PdfDocument.Page page=document.startPage(new PdfDocument.PageInfo.Builder(
                        bitmap.getWidth(),bitmap.getHeight(),1).create());
                    page.getCanvas().drawBitmap(bitmap,0,0,null);
                    document.finishPage(page);
                    try(OutputStream out=new FileOutputStream(file)){document.writeTo(out);}
                }finally{document.close();}
            }else{
                try(OutputStream out=new FileOutputStream(file)){
                    if(!bitmap.compress(Bitmap.CompressFormat.PNG,100,out))throw new IOException("Resim oluşturulamadı");
                }
            }
            cad.cancelSelection();
            shareFile(file,pdf?"application/pdf":"image/png");
        }catch(Exception e){error(e);}
    }
    private void shareFile(File f,String mime){if(f==null||!f.exists()){Toast.makeText(this,"Önce dosya açın",Toast.LENGTH_SHORT).show();return;}Uri u=FileProvider.getUriForFile(this,getPackageName()+".files",f);Intent s=new Intent(Intent.ACTION_SEND);s.setType(mime);s.putExtra(Intent.EXTRA_STREAM,u);s.setClipData(ClipData.newRawUri("MusaCAD",u));s.addFlags(Intent.FLAG_GRANT_READ_URI_PERMISSION);startActivity(Intent.createChooser(s,"Paylaş"));}
    private void error(Exception e){Toast.makeText(this,"İşlem başarısız: "+e.getMessage(),Toast.LENGTH_LONG).show();}
}
