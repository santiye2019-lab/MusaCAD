package com.musa.cad;

import android.app.Activity;
import android.app.AlertDialog;
import android.content.Intent;
import android.graphics.Color;
import android.os.Bundle;
import android.net.Uri;
import android.view.Gravity;
import android.view.View;
import android.view.ViewGroup;
import android.widget.Button;
import android.widget.EditText;
import android.widget.FrameLayout;
import android.widget.LinearLayout;
import android.widget.TextView;
import java.io.File;
import java.io.OutputStream;
import java.util.Arrays;

/** Reads the converted DWG/DXF mesh off the UI thread and opens a real 3D orbit view. */
public final class Mesh3dActivity extends Activity {
    public static final String EXTRA_DXF="com.musa.cad.DXF_3D_PATH";
    public static final String EXTRA_MODE="com.musa.cad.DXF_3D_MODE";
    public static final String MODE_ISO="iso",MODE_ORBIT="orbit",MODE_FRONT="front",MODE_TOP="top",
        MODE_RIGHT="right",MODE_WIREFRAME="wireframe",MODE_SURFACE="surface",
        MODE_MEASURE="measure",MODE_EDIT="edit";
    private static final int SAVE_DXF=3103,SAVE_GENERATED=3104;
    private volatile boolean closing;
    private boolean saving;
    private Dxf3dMesh mesh,importedMesh;
    private CadSolidKernel.Mesh generatedSolid;
    private float[] original;
    private Mesh3dView meshView;
    @Override protected void onCreate(Bundle state){
        super.onCreate(state);
        FrameLayout root=new FrameLayout(this);root.setBackgroundColor(Color.rgb(7,19,29));setContentView(root);
        TextView status=new TextView(this);status.setText("3B yüzeyler hazırlanıyor…");status.setTextColor(Color.WHITE);status.setTextSize(16);status.setGravity(Gravity.CENTER);root.addView(status,new FrameLayout.LayoutParams(-1,-1));
        String path=getIntent().getStringExtra(EXTRA_DXF);
        if(path==null){status.setText("3B çalışma dosyası bulunamadı");return;}
        new Thread(()->{
            try{
                Dxf3dMesh loaded=Dxf3dMesh.read(new File(path));
                runOnUiThread(()->{if(closing)return;importedMesh=loaded;original=Arrays.copyOf(loaded.xyz,loaded.xyz.length);
                    installMesh(root,loaded);
                });
            }catch(Exception e){runOnUiThread(()->{if(!closing)status.setText("3B görünüm açılamadı: "+e.getMessage());});}
        },"MusaCAD-3D-load").start();
    }
    private void installMesh(FrameLayout root,Dxf3dMesh loaded){
        mesh=loaded;meshView=new Mesh3dView(this,mesh);applyRequestedMode();
        root.removeAllViews();
        root.addView(meshView,new FrameLayout.LayoutParams(-1,-1));
        TextView hint=new TextView(this);
        hint.setText("← Geri  •  "+modeHint()+"  •  İki parmak: yakınlaştır\n"+
            (generatedSolid==null?modelHint():"MusaCAD 3B ağ modelleme: "+generatedSolid.faceCount()+
                " üçgen • kayıtta 3DFACE DXF üretilecek, orijinal DWG değiştirilmez"));
        hint.setTextColor(Color.WHITE);hint.setTextSize(11);
        hint.setPadding(16,16,16,16);hint.setBackgroundColor(0xcc07131d);
        root.addView(hint,new FrameLayout.LayoutParams(-1,-2,Gravity.TOP));
        addViewControls(root);
        if(generatedSolid!=null)addGeneratedControls(root);
        else if(MODE_EDIT.equals(requestedMode()))addEditControls(root);
    }

    private void showModelCommands(FrameLayout root){
        final String[] commands={"BOX","EXTRUDE","REVOLVE","LOFT","UNION","INTERSECTION","DIFFERENCE","UCS"};
        final String[] suggestions={
            "0,0,0,10,6,4",
            "0,0;8,0;8,5;0,5|4",
            "1,0;2,0;2,5;1,5|48|360",
            "0,0;4,0;4,4;0,4|0|1,1;3,1;3,3;1,3|6",
            "0,0,0,8,6,5|4,3,2,11,10,7",
            "0,0,0,8,6,5|4,3,2,11,10,7",
            "0,0,0,8,6,5|4,3,2,11,10,7",
            "10,20,30|0,1,0|-1,0,0"
        };
        new AlertDialog.Builder(this).setTitle("3B Modelleme • üçgen ağ / DXF")
            .setItems(commands,(dialog,index)->{
                EditText input=new EditText(this);
                input.setText(suggestions[index]);input.setSelectAllOnFocus(true);
                input.setTextColor(Color.WHITE);
                input.setPadding(20,12,20,12);input.setSingleLine(false);
                new AlertDialog.Builder(this).setTitle(commands[index]+" • parametreler")
                    .setMessage("EXTRUDE: x,y;...|yükseklik  •  REVOLVE: r,z;...|dilim|açı\n"+
                        "BOOLEAN yalnız eksene paralel iki kapalı kutu için; ACIS katı değildir.")
                    .setView(input).setNegativeButton("İPTAL",null)
                    .setPositiveButton("MODELİ OLUŞTUR",(d,button)->{
                        try{
                            CadSolidKernel.Mesh produced="UCS".equals(commands[index])
                                ?CadSolidCommand.applyUcs(generatedSolid,input.getText().toString())
                                :CadSolidCommand.execute(commands[index],input.getText().toString());
                            Dxf3dMesh preview=Dxf3dMesh.fromGenerated(produced);
                            generatedSolid=produced;installMesh(root,preview);
                        }catch(Exception error){
                            android.widget.Toast.makeText(this,"3B modelleme: "+error.getMessage(),
                                android.widget.Toast.LENGTH_LONG).show();
                        }
                    }).show();
            }).setNegativeButton("KAPAT",null).show();
    }

    private void addGeneratedControls(FrameLayout root){
        LinearLayout actions=new LinearLayout(this);
        actions.setOrientation(LinearLayout.HORIZONTAL);actions.setBackgroundColor(0xee07131d);
        Button originalButton=new Button(this);originalButton.setText("ORİJİNAL");
        actions.addView(originalButton,new LinearLayout.LayoutParams(0,-2,1f));
        originalButton.setOnClickListener(v->{
            if(importedMesh!=null){generatedSolid=null;installMesh(root,importedMesh);}
        });
        Button saveButton=new Button(this);saveButton.setText("3B DXF KAYDET");
        actions.addView(saveButton,new LinearLayout.LayoutParams(0,-2,1f));
        saveButton.setOnClickListener(v->{
            if(saving||generatedSolid==null)return;
            Intent intent=new Intent(Intent.ACTION_CREATE_DOCUMENT);
            intent.addCategory(Intent.CATEGORY_OPENABLE);intent.setType("application/dxf");
            intent.putExtra(Intent.EXTRA_TITLE,"MusaCAD-3B-model-3DFACE.dxf");
            startActivityForResult(intent,SAVE_GENERATED);
        });
        root.addView(actions,new FrameLayout.LayoutParams(-1,-2,Gravity.BOTTOM));
    }

    private String requestedMode(){
        String mode=getIntent().getStringExtra(EXTRA_MODE);
        return mode==null?MODE_ISO:mode;
    }

    private String modelHint(){
        StringBuilder text=new StringBuilder();
        text.append(mesh.triangles.length/3).append(" üçgen  •  ")
            .append(mesh.edges.length/2).append(" kenar  •  ")
            .append(mesh.xyz.length/3).append(" köşe");
        if(mesh.flatPlan){
            text.append("\n2B plan algılandı: Z aralığı ")
                .append(String.format(java.util.Locale.getDefault(),"%.3f",mesh.minZ))
                .append(" … ")
                .append(String.format(java.util.Locale.getDefault(),"%.3f",mesh.maxZ))
                .append(". Üst görünüş otomatik açıldı; ISO ile eğik görünüşe geçebilirsiniz.");
        }
        if(mesh.acisSolids>0&&!mesh.solidProxy)text.append("\nACIS B-Rep görünümü salt-okunurdur; 3B ölçüm kullanılabilir, tessellated ACIS köşeleri DXF koordinatı gibi kaydedilmez.");
        if(mesh.solidProxy){
            text.append("\n")
                .append(mesh.unsupportedSolids)
                .append(" adet 3DSOLID/BODY/REGION için kullanılabilir ACIS B-Rep verisi bulunamadı; yalnız bu durumda sınır kutusu tel-kafes fallback gösteriliyor.");
        }else if(mesh.unsupportedSolids>0){
            text.append("\n")
                .append(mesh.unsupportedSolids)
                .append(" adet ACIS katının gerçek B-Rep kenarları gösterildi; tam tessellate edilemeyen eğrisel/spline yüzeyler tel-kafes bırakıldı.");
        }
        return text.toString();
    }

    private String modeHint(){
        switch(requestedMode()){
            case MODE_MEASURE:return "Ölçüm: iki köşe seç";
            case MODE_EDIT:return "Düzenle: bir köşe seç, sonra X/Y/Z adımı uygula";
            case MODE_FRONT:return "Ön görünüş • tek parmakla döndür";
            case MODE_TOP:return "Üst görünüş • tek parmakla döndür";
            case MODE_RIGHT:return "Sağ görünüş • tek parmakla döndür";
            case MODE_WIREFRAME:return "Tel kafes • tek parmakla döndür";
            case MODE_SURFACE:return "Yüzey • tek parmakla döndür";
            default:return "Tek parmak: döndür";
        }
    }

    private void applyRequestedMode(){
        if(meshView==null)return;
        String mode=requestedMode();
        meshView.setInteractionMode(MODE_MEASURE.equals(mode)?Mesh3dView.InteractionMode.MEASURE:MODE_EDIT.equals(mode)&&!mesh.solidProxy&&mesh.acisSolids==0?Mesh3dView.InteractionMode.EDIT:Mesh3dView.InteractionMode.ORBIT);
        if(mesh.solidProxy){
            meshView.setWireframe(true);
        }
        if(mesh.flatPlan&&(MODE_ISO.equals(mode)||MODE_ORBIT.equals(mode))){
            meshView.setWireframe(true);
            meshView.setTopView();
            return;
        }
        switch(mode){
            case MODE_FRONT: meshView.setFrontView(); break;
            case MODE_TOP: meshView.setTopView(); break;
            case MODE_RIGHT: meshView.setRightView(); break;
            case MODE_WIREFRAME: meshView.setWireframe(true); meshView.setIsometricView(); break;
            case MODE_SURFACE: meshView.setWireframe(false); meshView.setIsometricView(); break;
            default: meshView.setIsometricView(); break;
        }
    }

    private void addViewControls(FrameLayout root){
        LinearLayout bar=new LinearLayout(this);
        bar.setOrientation(LinearLayout.HORIZONTAL);
        bar.setBackgroundColor(0xee07131d);
        String[] names={"ISO","ÖN","ÜST","SAĞ","TEL","YÜZEY","MODEL"};
        for(String name:names){
            Button b=new Button(this);
            b.setText(name);
            b.setTextSize(10f);
            b.setMinWidth(0);b.setMinHeight(0);
            bar.addView(b,new LinearLayout.LayoutParams(0,-2,1f));
            b.setOnClickListener(v->{
                if(meshView==null)return;
                switch(name){
                    case "ÖN": meshView.setFrontView(); break;
                    case "ÜST": meshView.setTopView(); break;
                    case "SAĞ": meshView.setRightView(); break;
                    case "TEL": meshView.setWireframe(true); break;
                    case "YÜZEY": meshView.setWireframe(false); break;
                    case "MODEL": showModelCommands(root); break;
                    default: meshView.setIsometricView(); break;
                }
            });
        }
        FrameLayout.LayoutParams lp=new FrameLayout.LayoutParams(-1,-2,Gravity.TOP);
        lp.topMargin=(int)(64*getResources().getDisplayMetrics().density);
        root.addView(bar,lp);
    }

    private void addEditControls(FrameLayout root){
        if(mesh==null||mesh.solidProxy||mesh.acisSolids>0)return;
        LinearLayout panel=new LinearLayout(this);panel.setOrientation(LinearLayout.VERTICAL);panel.setBackgroundColor(0xee07131d);
        LinearLayout views=new LinearLayout(this);panel.addView(views,new LinearLayout.LayoutParams(-1,-2));
        Button style=new Button(this);style.setText("Ağ çizgileri");views.addView(style,new LinearLayout.LayoutParams(0,-2,1));
        style.setOnClickListener(v->style.setText(meshView.toggleWireframe()?"Yüzey görünümü":"Ağ çizgileri"));
        Button reset=new Button(this);reset.setText("Görünümü sıfırla");views.addView(reset,new LinearLayout.LayoutParams(0,-2,1));reset.setOnClickListener(v->meshView.resetCamera());
        EditText step=new EditText(this);step.setSingleLine(true);step.setText("1");step.setHint("Adım (çizim birimi)");step.setTextColor(Color.WHITE);step.setInputType(8194);panel.addView(step,new LinearLayout.LayoutParams(-1,-2));
        LinearLayout row=new LinearLayout(this);panel.addView(row,new LinearLayout.LayoutParams(-1,-2));
        String[] names={"X−","X+","Y−","Y+","Z−","Z+"};
        for(int n=0;n<names.length;n++){final int axis=n/2,sign=n%2==0?-1:1;Button button=new Button(this);button.setText(names[n]);row.addView(button,new LinearLayout.LayoutParams(0,-2,1));button.setOnClickListener(v->{
            if(saving)return;
            int vertex=meshView.selectedVertex();if(vertex<0){step.setError("Önce modelden bir köşe seçin");return;}
            try{float amount=Float.parseFloat(step.getText().toString());if(!Float.isFinite(amount)||amount<=0||amount>1e6f)throw new NumberFormatException();
                float next=mesh.xyz[vertex*3+axis]+sign*amount;if(!Float.isFinite(next)||Math.abs(next)>1e9f)throw new NumberFormatException();
                mesh.xyz[vertex*3+axis]=next;meshView.geometryChanged();
            }catch(NumberFormatException error){step.setError("Pozitif bir adım girin");}
        });}
        Button save=new Button(this);save.setText("3B DXF kopyasını kaydet");panel.addView(save,new LinearLayout.LayoutParams(-1,-2));save.setOnClickListener(v->{
            if(saving)return;
            if(Arrays.equals(original,mesh.xyz)){step.setError("Önce bir köşeyi düzenleyin");return;}
            Intent intent=new Intent(Intent.ACTION_CREATE_DOCUMENT);intent.addCategory(Intent.CATEGORY_OPENABLE);intent.setType("application/dxf");intent.putExtra(Intent.EXTRA_TITLE,"MusaCAD-3B-duzenleme.dxf");startActivityForResult(intent,SAVE_DXF);
        });
        root.addView(panel,new FrameLayout.LayoutParams(-1,-2,Gravity.BOTTOM));
    }
    @Override protected void onActivityResult(int request,int result,Intent data){
        super.onActivityResult(request,result,data);
        if(result!=RESULT_OK||data==null||data.getData()==null||saving||mesh==null)return;
        if(request==SAVE_GENERATED){
            final CadSolidKernel.Mesh generated=generatedSolid;
            if(generated==null)return;
            Uri uri=data.getData();saving=true;
            new Thread(()->{
                File staged=null;String message;
                try{
                    staged=CadDxfSaveStage.prepare(getCacheDir(),out->CadSolidKernel.writeDxf(generated,out));
                    try(OutputStream output=getContentResolver().openOutputStream(uri,"wt")){
                        if(output==null)throw new java.io.IOException("Dosya açılamadı");
                        CadDxfSaveStage.publish(staged,output);output.flush();
                    }
                    message="3B model 3DFACE DXF olarak kaydedildi (ACIS değildir)";
                }catch(Exception e){message="3B model kaydı başarısız: "+e.getMessage();}
                finally{if(staged!=null)staged.delete();}
                final String resultMessage=message;
                runOnUiThread(()->{saving=false;if(!closing)android.widget.Toast.makeText(this,resultMessage,android.widget.Toast.LENGTH_LONG).show();});
            },"MusaCAD-3D-mesh-save").start();
            return;
        }
        if(request!=SAVE_DXF)return;
        Uri uri=data.getData();String path=getIntent().getStringExtra(EXTRA_DXF);saving=true;
        new Thread(()->{
            String message;File stage=null;
            try{
                stage=CadDxfSaveStage.prepare(getCacheDir(),out->
                    Dxf3dEditor.write(new File(path),out,mesh,original));
                try(OutputStream output=getContentResolver().openOutputStream(uri,"wt")){
                    if(output==null)throw new java.io.IOException("Dosya açılamadı");
                    CadDxfSaveStage.publish(stage,output);output.flush();
                }
                message="3B DXF kopyası doğrulanarak kaydedildi";
            }catch(Exception e){message="3B kayıt başarısız: "+e.getMessage();}
            finally{if(stage!=null)stage.delete();}
            final String resultMessage=message;
            runOnUiThread(()->{
                saving=false;
                if(!closing)android.widget.Toast.makeText(this,resultMessage,
                    android.widget.Toast.LENGTH_LONG).show();
            });
        },"MusaCAD-3D-save").start();
    }
    @Override protected void onDestroy(){closing=true;super.onDestroy();}
}
