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
import java.io.*;

public class MainActivity extends AppCompatActivity {
    private static final int OPEN=20;
    private CadView cad; private TextView fileName,result; private File currentFile;
    protected void onCreate(Bundle b){super.onCreate(b);setContentView(R.layout.activity_main);
        cad=findViewById(R.id.cadView);fileName=findViewById(R.id.fileName);result=findViewById(R.id.resultText);cad.setListener(new CadView.Listener(){public void onMeasurement(String v){result.setText(v);}public void onCalibrationRequested(double px){showCalibration();}public void onSelectionReady(){previewSelection();}});
        findViewById(R.id.openButton).setOnClickListener(v->open());
        findViewById(R.id.panButton).setOnClickListener(v->cad.setMode(CadView.Mode.PAN));
        findViewById(R.id.calibrateButton).setOnClickListener(v->cad.setMode(CadView.Mode.CALIBRATE));
        findViewById(R.id.distanceButton).setOnClickListener(v->cad.setMode(CadView.Mode.DISTANCE));
        findViewById(R.id.areaButton).setOnClickListener(v->cad.setMode(CadView.Mode.AREA));
        findViewById(R.id.undoButton).setOnClickListener(v->cad.undo());
        findViewById(R.id.clearButton).setOnClickListener(v->cad.clearMeasurement());
        findViewById(R.id.shareButton).setOnClickListener(v->showShare());
    }
    private void open(){Intent i=new Intent(Intent.ACTION_OPEN_DOCUMENT);i.setType("*/*");i.addCategory(Intent.CATEGORY_OPENABLE);i.putExtra(Intent.EXTRA_MIME_TYPES,new String[]{"application/acad","application/x-autocad","application/dwg","image/vnd.dwg","application/dxf","application/octet-stream"});startActivityForResult(i,OPEN);}
    protected void onActivityResult(int r,int c,Intent data){super.onActivityResult(r,c,data);if(r!=OPEN||c!=RESULT_OK||data==null)return;Uri u=data.getData();try{String n=nameOf(u);currentFile=new File(getCacheDir(),"opened_"+new File(n).getName());try(InputStream in=getContentResolver().openInputStream(u);OutputStream out=new FileOutputStream(currentFile)){byte[] buf=new byte[65536];int k;while((k=in.read(buf))>0)out.write(buf,0,k);}boolean dxf=n.toLowerCase(java.util.Locale.ROOT).endsWith(".dxf");DxfParser.Result parsed=dxf?DxfParser.render(currentFile):null;Bitmap p=dxf?(parsed==null?null:parsed.bitmap):DwgPreview.read(currentFile);if(p!=null){cad.setDrawing(p);fileName.setText(n+(dxf?" — DXF geometri":" — DWG önizleme"));result.setText(dxf?(parsed.entityCount+" nesne, "+parsed.layerCount+" katman; "+parsed.skippedCount+" desteklenmeyen nesne. Yaklaşık görünüm."):"DWG önizlemesi açıldı. Ölçek belirleyerek ölçebilirsiniz.");}else{cad.setDrawing(null);fileName.setText(n);result.setText(dxf?"Desteklenen DXF geometrisi bulunamadı.":"Dosyada görüntülenebilir DWG önizlemesi bulunamadı.");}}catch(Exception e){cad.setDrawing(null);currentFile=null;fileName.setText("Dosya açılamadı");Toast.makeText(this,"Dosya açılamadı: "+e.getMessage(),Toast.LENGTH_LONG).show();}}
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
                try(PdfDocument document=new PdfDocument()){
                    PdfDocument.Page page=document.startPage(new PdfDocument.PageInfo.Builder(
                        bitmap.getWidth(),bitmap.getHeight(),1).create());
                    page.getCanvas().drawBitmap(bitmap,0,0,null);
                    document.finishPage(page);
                    try(OutputStream out=new FileOutputStream(file)){document.writeTo(out);}
                }
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
