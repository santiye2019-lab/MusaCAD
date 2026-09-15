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
import androidx.core.graphics.Insets;
import androidx.core.view.ViewCompat;
import androidx.core.view.WindowCompat;
import androidx.core.view.WindowInsetsCompat;
import java.io.*;
import java.util.concurrent.*;

public class MainActivity extends AppCompatActivity {
    private static final int OPEN=20;
    private static final int MENU_OPEN=1,MENU_LAYERS=2,MENU_FIT=3,MENU_SHARE=4,MENU_INFO=5,MENU_ABOUT=6,MENU_LAYOUT=7;
    private final ExecutorService loader=Executors.newSingleThreadExecutor();
    private LoadTask activeLoad;

    private static final class LoadTask {
        Future<?> future;AlertDialog dialog;TextView progress;
    }

    private static final class Loaded {
        File file;Bitmap bitmap;DxfParser.Result parsed;String name;boolean dxf;
        void dispose(){
            if(bitmap!=null&&!bitmap.isRecycled())bitmap.recycle();
            if(file!=null)file.delete();
        }
    }

    private CheckBox snapToggle;
    private DxfParser.Result activeDxf;
    private CadView cad;
    private TextView fileName,result;
    private File currentFile;
    private boolean currentIsDxf;
    private View[] modeButtons;
    private View welcomePanel,shareButton,shareToolButton;

    @Override protected void onCreate(Bundle b){
        super.onCreate(b);
        WindowCompat.setDecorFitsSystemWindows(getWindow(),false);
        setContentView(R.layout.activity_main);

        View root=findViewById(R.id.mainRoot);
        ViewCompat.setOnApplyWindowInsetsListener(root,(v,insets)->{
            Insets bars=insets.getInsets(WindowInsetsCompat.Type.systemBars());
            v.setPadding(0,bars.top,0,bars.bottom+dp(6));
            return insets;
        });

        cad=findViewById(R.id.cadView);
        fileName=findViewById(R.id.fileName);
        result=findViewById(R.id.resultText);
        welcomePanel=findViewById(R.id.welcomePanel);
        shareButton=findViewById(R.id.shareButton);
        shareToolButton=findViewById(R.id.shareToolButton);
        cad.setListener(new CadView.Listener(){
            public void onMeasurement(String v){result.setText(v);}
            public void onCalibrationRequested(double px){showCalibration();}
            public void onSelectionReady(){previewSelection();}
        });

        snapToggle=findViewById(R.id.snapToggle);
        snapToggle.setOnCheckedChangeListener((button,checked)->cad.setSnapEnabled(checked));

        modeButtons=new View[]{
            findViewById(R.id.panButton),findViewById(R.id.calibrateButton),
            findViewById(R.id.distanceButton),findViewById(R.id.areaButton)
        };
        markModeSelected(R.id.panButton);

        int[] interactive={R.id.menuButton,R.id.openButton,R.id.shareButton,R.id.quickOpenButton,R.id.layersButton,R.id.snapToggle,
            R.id.panButton,R.id.calibrateButton,R.id.distanceButton,R.id.areaButton,R.id.zoomInButton,R.id.zoomOutButton,
            R.id.fitButton,R.id.undoButton,R.id.clearButton,R.id.shareToolButton};
        for(int id:interactive)installInteractiveFeedback(findViewById(id));

        findViewById(R.id.menuButton).setOnClickListener(this::showMainMenu);
        findViewById(R.id.appTitle).setOnClickListener(this::showMainMenu);
        findViewById(R.id.layersButton).setOnClickListener(v->showLayers());
        findViewById(R.id.openButton).setOnClickListener(v->open());
        findViewById(R.id.quickOpenButton).setOnClickListener(v->open());
        findViewById(R.id.panButton).setOnClickListener(v->selectMode(R.id.panButton,CadView.Mode.PAN));
        findViewById(R.id.calibrateButton).setOnClickListener(v->selectMode(R.id.calibrateButton,CadView.Mode.CALIBRATE));
        findViewById(R.id.distanceButton).setOnClickListener(v->selectMode(R.id.distanceButton,CadView.Mode.DISTANCE));
        findViewById(R.id.areaButton).setOnClickListener(v->selectMode(R.id.areaButton,CadView.Mode.AREA));
        findViewById(R.id.zoomInButton).setOnClickListener(v->{v.performHapticFeedback(HapticFeedbackConstants.KEYBOARD_TAP);cad.zoomBy(1.35f);});
        findViewById(R.id.zoomOutButton).setOnClickListener(v->{v.performHapticFeedback(HapticFeedbackConstants.KEYBOARD_TAP);cad.zoomBy(1f/1.35f);});
        findViewById(R.id.fitButton).setOnClickListener(v->{v.performHapticFeedback(HapticFeedbackConstants.KEYBOARD_TAP);cad.fitToScreen();});
        findViewById(R.id.undoButton).setOnClickListener(v->{v.performHapticFeedback(HapticFeedbackConstants.KEYBOARD_TAP);cad.undo();});
        findViewById(R.id.clearButton).setOnClickListener(v->{v.performHapticFeedback(HapticFeedbackConstants.KEYBOARD_TAP);cad.clearMeasurement();});
        shareButton.setOnClickListener(v->showShare());
        shareToolButton.setOnClickListener(v->showShare());
        updateShareEnabled(false);
    }

    private int dp(int value){return Math.round(value*getResources().getDisplayMetrics().density);}

    private void installInteractiveFeedback(View view){
        if(view==null)return;
        view.setHapticFeedbackEnabled(true);
        view.setOnTouchListener((v,e)->{
            int action=e.getActionMasked();
            if(action==MotionEvent.ACTION_DOWN){
                v.animate().scaleX(.94f).scaleY(.94f).setDuration(70).start();
            }else if(action==MotionEvent.ACTION_UP||action==MotionEvent.ACTION_CANCEL){
                v.animate().scaleX(1f).scaleY(1f).setDuration(100).start();
            }
            return false;
        });
    }

    private void showMainMenu(View anchor){
        anchor.performHapticFeedback(HapticFeedbackConstants.KEYBOARD_TAP);
        PopupMenu popup=new PopupMenu(this,anchor);
        Menu menu=popup.getMenu();
        menu.add(0,MENU_OPEN,0,"Dosya aç");
        menu.add(0,MENU_LAYERS,1,"Katmanlar").setEnabled(activeDxf!=null);
        menu.add(0,MENU_LAYOUT,2,"Model / Pafta").setEnabled(activeDxf!=null&&activeDxf.layoutNames.size()>1);
        menu.add(0,MENU_FIT,3,"Ekrana sığdır").setEnabled(currentFile!=null);
        menu.add(0,MENU_SHARE,4,"Paylaş").setEnabled(currentFile!=null);
        menu.add(0,MENU_INFO,5,"Çizim bilgileri").setEnabled(activeDxf!=null);
        menu.add(0,MENU_ABOUT,6,"MusaCAD hakkında");
        popup.setOnMenuItemClickListener(item->{
            switch(item.getItemId()){
                case MENU_OPEN:open();return true;
                case MENU_LAYERS:showLayers();return true;
                case MENU_LAYOUT:showLayouts();return true;
                case MENU_FIT:cad.fitToScreen();return true;
                case MENU_SHARE:showShare();return true;
                case MENU_INFO:showDrawingInfo();return true;
                case MENU_ABOUT:showLicense();return true;
                default:return false;
            }
        });
        popup.show();
    }

    private void showDrawingInfo(){
        if(activeDxf==null)return;
        String text="Dosya başarıyla açıldı.\n\n"+
            "Nesne: "+activeDxf.entityCount+"\n"+
            "Katman: "+activeDxf.layerCount+"\n"+
            "Görünür katman: "+activeDxf.visibleLayers.size()+"\n"+
            "Ölçü birimi: "+(activeDxf.automaticUnits?DxfUnits.name(activeDxf.insUnits)+" (otomatik)":"kalibrasyon gerekli")+"\n"+
            "Görüntüleme: vektörel / net yakınlaştırma";
        new AlertDialog.Builder(this).setTitle("Çizim bilgileri").setMessage(text).setPositiveButton("TAMAM",null).show();
    }

    private void updateShareEnabled(boolean enabled){
        shareButton.setEnabled(enabled);shareButton.setAlpha(enabled?1f:.45f);
        shareToolButton.setEnabled(enabled);shareToolButton.setAlpha(enabled?1f:.55f);
    }

    private void selectMode(int id,CadView.Mode mode){
        View button=findViewById(id);
        if(button!=null)button.performHapticFeedback(HapticFeedbackConstants.KEYBOARD_TAP);
        cad.setMode(mode);markModeSelected(id);
    }
    private void markModeSelected(int id){
        if(modeButtons==null)return;
        for(View button:modeButtons)button.setSelected(button.getId()==id);
    }

    private void hideWelcomePanel(){
        if(welcomePanel==null||welcomePanel.getVisibility()!=View.VISIBLE)return;
        welcomePanel.animate().alpha(0f).setDuration(180).withEndAction(()->{
            welcomePanel.setVisibility(View.GONE);welcomePanel.setAlpha(1f);
        }).start();
    }

    private void showLicense(){
        String license;
        try(InputStream in=getAssets().open("COPYING-LibreDWG.txt")){
            ByteArrayOutputStream out=new ByteArrayOutputStream();byte[] bytes=new byte[4096];int n;
            while((n=in.read(bytes))!=-1)out.write(bytes,0,n);license=out.toString("UTF-8");
        }catch(IOException e){license="GPL-3.0-or-later";}
        TextView text=new TextView(this);text.setPadding(24,16,24,16);
        text.setText("MusaCAD — LibreDWG ile çevrimdışı DWG okuma\nKaynak kod: https://github.com/santiye2019-lab/MusaCAD\n\n"+license);
        android.text.util.Linkify.addLinks(text,android.text.util.Linkify.WEB_URLS);
        text.setMovementMethod(android.text.method.LinkMovementMethod.getInstance());
        ScrollView scroll=new ScrollView(this);scroll.addView(text);
        new AlertDialog.Builder(this).setTitle("Lisans ve kaynak kod").setView(scroll).setPositiveButton("KAPAT",null).show();
    }

    private void open(){
        Intent i=new Intent(Intent.ACTION_OPEN_DOCUMENT);
        i.addCategory(Intent.CATEGORY_OPENABLE);
        i.setType("*/*");
        i.addFlags(Intent.FLAG_GRANT_READ_URI_PERMISSION|Intent.FLAG_GRANT_PERSISTABLE_URI_PERMISSION);
        startActivityForResult(i,OPEN);
    }

    @Override protected void onActivityResult(int r,int c,Intent data){
        super.onActivityResult(r,c,data);
        if(r==OPEN&&c==RESULT_OK&&data!=null&&data.getData()!=null)startLoad(data.getData());
    }

    private void cancelLoad(){
        LoadTask task=activeLoad;activeLoad=null;
        if(task!=null){if(task.future!=null)task.future.cancel(true);task.dialog.dismiss();}
    }

    private void startLoad(Uri uri){
        cancelLoad();
        LoadTask task=new LoadTask();activeLoad=task;
        LinearLayout box=new LinearLayout(this);box.setOrientation(LinearLayout.VERTICAL);
        int pad=dp(20);box.setPadding(pad,pad,pad,pad);
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
                    FileTransfer.copy(in,out,32L*1024*1024,bytes->runOnUiThread(()->{
                        if(activeLoad==task)task.progress.setText(String.format(java.util.Locale.getDefault(),"Okunan: %.1f MB",bytes/1048576d));
                    }));
                }
                runOnUiThread(()->{if(activeLoad==task)task.progress.setText("Çizim hazırlanıyor…");});
                FileTransfer.checkCancelled();

                if(loaded.dxf){
                    loaded.parsed=DxfParser.render(loaded.file);
                }else{
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
                    currentFile=loaded.file;currentIsDxf=loaded.dxf;activeDxf=loaded.parsed;
                    hideWelcomePanel();
                    updateShareEnabled(true);
                    findViewById(R.id.layersButton).setEnabled(activeDxf!=null);

                    if(loaded.parsed!=null)cad.setVectorDrawing(loaded.parsed);
                    else cad.setDrawing(loaded.bitmap);
                    markModeSelected(R.id.panButton);

                    snapToggle.setEnabled(loaded.parsed!=null&&loaded.parsed.snapPoints.length>0);
                    if(loaded.parsed!=null)cad.setSnapPoints(loaded.parsed.snapPoints);

                    fileName.setText(loaded.name+(loaded.dxf?"  •  DXF":loaded.parsed!=null?"  •  DWG":"  •  DWG önizleme"));
                    if(loaded.parsed!=null){
                        result.setText("Hazır  •  "+loaded.parsed.activeLayout+"  •  "+loaded.parsed.entityCount+" nesne  •  "+loaded.parsed.layerCount+" katman");
                    }else{
                        result.setText("Hazır  •  DWG önizleme modu");
                    }
                });
            }catch(Exception | OutOfMemoryError e){
                loaded.dispose();
                runOnUiThread(()->{
                    if(activeLoad!=task||isFinishing()||isDestroyed())return;
                    activeLoad=null;task.dialog.dismiss();
                    error(e instanceof Exception?(Exception)e:new IOException("Bu çizim için yeterli bellek yok"));
                });
            }
        });
    }

    @Override protected void onDestroy(){cancelLoad();loader.shutdownNow();super.onDestroy();}

    private void showLayouts(){
        if(activeDxf==null||activeLoad!=null)return;
        String[] names=activeDxf.layoutNames.toArray(new String[0]);
        if(names.length<2){Toast.makeText(this,"Bu çizimde tek görünüm var",Toast.LENGTH_SHORT).show();return;}
        int checked=0;for(int i=0;i<names.length;i++)if(names[i].equals(activeDxf.activeLayout)){checked=i;break;}
        new AlertDialog.Builder(this).setTitle("Model / Pafta seç")
            .setSingleChoiceItems(names,checked,(dialog,index)->{dialog.dismiss();applyLayout(names[index]);})
            .setNegativeButton("İPTAL",null).show();
    }

    private void applyLayout(String layout){
        if(activeDxf==null||currentFile==null||activeLoad!=null||activeDxf.activeLayout.equals(layout))return;
        DxfParser.Result source=activeDxf;File file=currentFile;boolean isDxf=currentIsDxf;
        java.util.Set<String> wantedLayers=new java.util.HashSet<>(source.visibleLayers);
        LoadTask task=new LoadTask();activeLoad=task;
        task.dialog=new AlertDialog.Builder(this).setTitle("Pafta hazırlanıyor")
            .setMessage(layout+" yükleniyor…").setNegativeButton("İPTAL",(d,w)->cancelLoad()).create();
        task.dialog.setOnCancelListener(d->cancelLoad());task.dialog.setCanceledOnTouchOutside(false);task.dialog.show();
        task.future=loader.submit(()->{
            DxfParser.Result updated=null;
            try{
                updated=isDxf?DxfParser.render(file,layout):NativeDwg.read(file,getCacheDir(),layout);
                if(updated==null)throw new IOException("Seçilen paftada desteklenen çizim bulunamadı");
                java.util.Set<String> selected=new java.util.HashSet<>(wantedLayers);selected.retainAll(updated.layerNames);
                if(!selected.equals(updated.visibleLayers))updated=updated.withVisibleLayers(selected);
                DxfParser.Result ready=updated;
                runOnUiThread(()->{
                    if(activeLoad!=task||activeDxf!=source||isFinishing()||isDestroyed()){if(ready.bitmap!=null&&!ready.bitmap.isRecycled())ready.bitmap.recycle();return;}
                    activeLoad=null;task.dialog.dismiss();activeDxf=ready;
                    cad.setVectorDrawing(ready);cad.setSnapPoints(ready.snapPoints);cad.fitToScreen();
                    snapToggle.setEnabled(ready.snapPoints.length>0);
                    result.setText("Hazır  •  "+ready.activeLayout+"  •  "+ready.entityCount+" nesne  •  "+ready.visibleLayers.size()+"/"+ready.layerCount+" katman");
                });
            }catch(Exception|OutOfMemoryError e){
                if(updated!=null&&updated.bitmap!=null&&!updated.bitmap.isRecycled())updated.bitmap.recycle();
                runOnUiThread(()->{
                    if(activeLoad!=task||isFinishing()||isDestroyed())return;
                    activeLoad=null;task.dialog.dismiss();error(e instanceof Exception?(Exception)e:new IOException("Yeterli bellek yok"));
                });
            }
        });
    }

    private void showLayers(){
        if(activeDxf==null||activeLoad!=null){
            if(activeDxf==null)Toast.makeText(this,"Katmanlar için önce bir çizim açın",Toast.LENGTH_SHORT).show();
            return;
        }
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
                    if(activeLoad!=task||activeDxf!=source||isFinishing()||isDestroyed()){
                        if(updated.bitmap!=null&&!updated.bitmap.isRecycled())updated.bitmap.recycle();return;
                    }
                    activeLoad=null;task.dialog.dismiss();
                    cad.replaceVisibleDrawing(updated);activeDxf=updated;
                    snapToggle.setEnabled(updated.snapPoints.length>0);
                    result.setText("Hazır  •  "+updated.activeLayout+"  •  "+updated.entityCount+" nesne  •  "+updated.visibleLayers.size()+"/"+updated.layerCount+" katman");
                });
            }catch(Exception|OutOfMemoryError e){
                runOnUiThread(()->{
                    if(activeLoad!=task||isFinishing()||isDestroyed())return;
                    activeLoad=null;task.dialog.dismiss();
                    error(e instanceof Exception?(Exception)e:new IOException("Yeterli bellek yok"));
                });
            }
        });
    }

    private String nameOf(Uri u){
        try(android.database.Cursor c=getContentResolver().query(u,null,null,null,null)){
            if(c!=null&&c.moveToFirst()){
                int i=c.getColumnIndex(OpenableColumns.DISPLAY_NAME);if(i>=0)return c.getString(i);
            }
        }
        return "cizim.dwg";
    }

    private void showShare(){
        if(currentFile==null||!currentFile.exists()){
            Toast.makeText(this,"Paylaşmak için önce bir DWG veya DXF dosyası açın",Toast.LENGTH_SHORT).show();return;
        }
        new AlertDialog.Builder(this).setTitle("Paylaş").setItems(new String[]{
            "Orijinal dosyayı paylaş","Görünümü PDF olarak paylaş","Görünümü resim olarak paylaş","Alan seçerek paylaş"
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
        int pad=dp(12);preview.setPadding(pad,pad,pad,pad);
        AlertDialog dialog=new AlertDialog.Builder(this).setTitle("Seçili alan önizlemesi")
            .setView(preview).setPositiveButton("PNG PAYLAŞ",(d,w)->exportBitmap(bitmap,false,"alan"))
            .setNeutralButton("PDF PAYLAŞ",(d,w)->exportBitmap(bitmap,true,"alan"))
            .setNegativeButton("YENİDEN SEÇ",(d,w)->cad.beginSelection()).create();
        dialog.setOnCancelListener(d->cad.cancelSelection());
        dialog.setOnDismissListener(d->{preview.setImageDrawable(null);bitmap.recycle();});
        dialog.show();
    }

    private void showCalibration(){
        LinearLayout box=new LinearLayout(this);box.setOrientation(LinearLayout.VERTICAL);
        int p=dp(16);box.setPadding(p,0,p,0);
        EditText value=new EditText(this);value.setHint("Gerçek uzunluk (ör. 2.50)");
        value.setInputType(android.text.InputType.TYPE_CLASS_NUMBER|android.text.InputType.TYPE_NUMBER_FLAG_DECIMAL);box.addView(value);
        Spinner units=new Spinner(this);units.setAdapter(new ArrayAdapter<String>(this,android.R.layout.simple_spinner_dropdown_item,new String[]{"m","cm","mm"}));box.addView(units);
        AlertDialog dialog=new AlertDialog.Builder(this).setTitle("Ölçeği ayarla").setView(box)
            .setPositiveButton("UYGULA",null).setNegativeButton("İPTAL",(d,w)->cad.clearMeasurement()).create();
        dialog.setOnCancelListener(d->cad.clearMeasurement());
        dialog.setOnShowListener(d->dialog.getButton(AlertDialog.BUTTON_POSITIVE).setOnClickListener(v->{
            try{
                double n=Double.parseDouble(value.getText().toString().replace(',','.'));
                cad.setCalibration(n,units.getSelectedItem().toString());markModeSelected(R.id.distanceButton);dialog.dismiss();
            }catch(Exception e){value.setError("Sıfırdan büyük bir uzunluk girin; iki farklı nokta seçin.");}
        }));
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
                    PdfDocument.Page page=document.startPage(new PdfDocument.PageInfo.Builder(bitmap.getWidth(),bitmap.getHeight(),1).create());
                    page.getCanvas().drawBitmap(bitmap,0,0,null);document.finishPage(page);
                    try(OutputStream out=new FileOutputStream(file)){document.writeTo(out);}
                }finally{document.close();}
            }else{
                try(OutputStream out=new FileOutputStream(file)){
                    if(!bitmap.compress(Bitmap.CompressFormat.PNG,100,out))throw new IOException("Resim oluşturulamadı");
                }
            }
            cad.cancelSelection();shareFile(file,pdf?"application/pdf":"image/png");
        }catch(Exception e){error(e);}
    }

    private void shareFile(File f,String mime){
        if(f==null||!f.exists()){Toast.makeText(this,"Önce dosya açın",Toast.LENGTH_SHORT).show();return;}
        Uri u=FileProvider.getUriForFile(this,getPackageName()+".files",f);
        Intent s=new Intent(Intent.ACTION_SEND);s.setType(mime);s.putExtra(Intent.EXTRA_STREAM,u);
        s.setClipData(ClipData.newRawUri("MusaCAD",u));s.addFlags(Intent.FLAG_GRANT_READ_URI_PERMISSION);
        startActivity(Intent.createChooser(s,"Paylaş"));
    }

    private void error(Exception e){Toast.makeText(this,"İşlem başarısız: "+e.getMessage(),Toast.LENGTH_LONG).show();}
}
