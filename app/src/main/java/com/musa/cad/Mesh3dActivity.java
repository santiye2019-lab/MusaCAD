package com.musa.cad;

import android.app.Activity;
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
    private static final int SAVE_DXF=3103;
    private static final String UCS_PREFS="musacad_3d_named_ucs";
    private static final String UCS_DATA="profiles";
    private volatile boolean closing;
    private boolean saving;
    private Dxf3dMesh mesh;
    private float[] original;
    private Mesh3dView meshView;
    private CadUcsFrame editingUcs=CadUcsFrame.world();
    private final Cad3dEditHistory editHistory=new Cad3dEditHistory();
    private Button undo3dButton,redo3dButton;
    @Override protected void onCreate(Bundle state){
        super.onCreate(state);
        FrameLayout root=new FrameLayout(this);root.setBackgroundColor(Color.rgb(7,19,29));setContentView(root);
        TextView status=new TextView(this);status.setText("3B yüzeyler hazırlanıyor…");status.setTextColor(Color.WHITE);status.setTextSize(16);status.setGravity(Gravity.CENTER);root.addView(status,new FrameLayout.LayoutParams(-1,-1));
        String path=getIntent().getStringExtra(EXTRA_DXF);
        if(path==null){status.setText("3B çalışma dosyası bulunamadı");return;}
        new Thread(()->{
            try{
                Dxf3dMesh loaded=Dxf3dMesh.read(new File(path));
                runOnUiThread(()->{if(closing)return;mesh=loaded;original=Arrays.copyOf(mesh.xyz,mesh.xyz.length);meshView=new Mesh3dView(this,mesh);
                    editHistory.clear();
                    meshView.setAxisDragListener(this::moveGizmoVertex);
                    meshView.setAxisDragCommitListener(this::recordCompletedAxisDrag);
                    meshView.setUcsFrame(editingUcs);
                    applyRequestedMode();root.removeAllViews();root.addView(meshView,new FrameLayout.LayoutParams(-1,-1));
                    TextView hint=new TextView(this);hint.setText("← Geri   •   "+modeHint()+"   •   İki parmak: yakınlaştır\n"+modelHint());
                    hint.setTextColor(Color.WHITE);hint.setTextSize(11);hint.setPadding(16,16,16,16);hint.setBackgroundColor(0xcc07131d);FrameLayout.LayoutParams lp=new FrameLayout.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT,ViewGroup.LayoutParams.WRAP_CONTENT,Gravity.TOP);root.addView(hint,lp);
                    addViewControls(root);
                    if(MODE_EDIT.equals(requestedMode()))addEditControls(root);
                });
            }catch(Exception e){runOnUiThread(()->{if(!closing)status.setText("3B görünüm açılamadı: "+e.getMessage());});}
        },"MusaCAD-3D-load").start();
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
            case MODE_EDIT:return "Düzenle: köşe seç, X/Y/Z tutamacını sürükle veya adım uygula";
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
        String[] names={"ISO","ÖN","ÜST","SAĞ","TEL","YÜZEY"};
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
                    default: meshView.setIsometricView(); break;
                }
            });
        }
        FrameLayout.LayoutParams lp=new FrameLayout.LayoutParams(-1,-2,Gravity.TOP);
        lp.topMargin=(int)(64*getResources().getDisplayMetrics().density);
        root.addView(bar,lp);
    }

    /** Shared safety gate for touch-gizmo edits. Only direct source vertices are writable. */
    private boolean moveGizmoVertex(int vertex,double x,double y,double z){
        if(saving||mesh==null||mesh.solidProxy||mesh.acisSolids>0||
           vertex<0||vertex>=mesh.sourceLines.length||
           vertex>=mesh.coordinateSlots.length||
           mesh.sourceLines[vertex]<0||mesh.coordinateSlots[vertex]<0)
            return false;
        double[] coords={x,y,z};
        for(double value:coords){
            if(!Double.isFinite(value)||Math.abs(value)>1e9d)return false;
        }
        for(int k=0;k<3;k++)mesh.xyz[vertex*3+k]=(float)coords[k];
        return true;
    }
    private void recordCompletedAxisDrag(int vertex,float[] before,float[] after){
        if(saving||mesh==null||vertex<0||vertex>=mesh.sourceLines.length||
           vertex>=mesh.coordinateSlots.length||mesh.sourceLines[vertex]<0||
           mesh.coordinateSlots[vertex]<0)return;
        if(editHistory.record(mesh.xyz,vertex,before,after))refresh3dUndoButtons();
    }
    private void refresh3dUndoButtons(){
        if(undo3dButton!=null)undo3dButton.setEnabled(editHistory.canUndo()&&!saving);
        if(redo3dButton!=null)redo3dButton.setEnabled(editHistory.canRedo()&&!saving);
    }
    private void perform3dHistory(boolean redo){
        if(saving||mesh==null||meshView==null||meshView.isAxisDragging())return;
        boolean applied=redo?editHistory.redo(mesh.xyz):editHistory.undo(mesh.xyz);
        refresh3dUndoButtons();
        if(applied)meshView.geometryChanged();
        else android.widget.Toast.makeText(this,
            redo?"İleri alınacak geçerli işlem yok":"Geri alınacak geçerli işlem yok",
            android.widget.Toast.LENGTH_SHORT).show();
    }
    private void addEditControls(FrameLayout root){
        if(mesh==null||mesh.solidProxy||mesh.acisSolids>0)return;
        LinearLayout panel=new LinearLayout(this);panel.setOrientation(LinearLayout.VERTICAL);panel.setBackgroundColor(0xee07131d);
        LinearLayout views=new LinearLayout(this);panel.addView(views,new LinearLayout.LayoutParams(-1,-2));
        Button style=new Button(this);style.setText("Ağ çizgileri");views.addView(style,new LinearLayout.LayoutParams(0,-2,1));
        style.setOnClickListener(v->style.setText(meshView.toggleWireframe()?"Yüzey görünümü":"Ağ çizgileri"));
        Button reset=new Button(this);reset.setText("Görünümü sıfırla");views.addView(reset,new LinearLayout.LayoutParams(0,-2,1));reset.setOnClickListener(v->meshView.resetCamera());
        LinearLayout historyRow=new LinearLayout(this);
        panel.addView(historyRow,new LinearLayout.LayoutParams(-1,-2));
        undo3dButton=new Button(this);undo3dButton.setText("↶ GERİ AL");
        historyRow.addView(undo3dButton,new LinearLayout.LayoutParams(0,-2,1));
        undo3dButton.setOnClickListener(v->perform3dHistory(false));
        redo3dButton=new Button(this);redo3dButton.setText("↷ İLERİ AL");
        historyRow.addView(redo3dButton,new LinearLayout.LayoutParams(0,-2,1));
        redo3dButton.setOnClickListener(v->perform3dHistory(true));
        refresh3dUndoButtons();
        Button ucsButton=new Button(this);
        ucsButton.setText("UCS: WCS");
        panel.addView(ucsButton,new LinearLayout.LayoutParams(-1,-2));
        ucsButton.setOnClickListener(v->showUcsSelector(ucsButton));
        EditText step=new EditText(this);step.setSingleLine(true);step.setText("1");step.setHint("UCS ekseninde adım (çizim birimi)");step.setTextColor(Color.WHITE);step.setInputType(8194);panel.addView(step,new LinearLayout.LayoutParams(-1,-2));
        LinearLayout row=new LinearLayout(this);panel.addView(row,new LinearLayout.LayoutParams(-1,-2));
        String[] names={"X−","X+","Y−","Y+","Z−","Z+"};
        for(int n=0;n<names.length;n++){final int axis=n/2,sign=n%2==0?-1:1;Button button=new Button(this);button.setText(names[n]);row.addView(button,new LinearLayout.LayoutParams(0,-2,1));button.setOnClickListener(v->{
            if(saving)return;
            int vertex=meshView.selectedVertex();if(vertex<0){step.setError("Önce modelden bir köşe seçin");return;}
            if(vertex>=mesh.sourceLines.length||mesh.sourceLines[vertex]<0||
               vertex>=mesh.coordinateSlots.length||mesh.coordinateSlots[vertex]<0){
                step.setError("Bu köşe bloktan türetilmiş ve doğrudan düzenlenemez");return;
            }
            try{float amount=Float.parseFloat(step.getText().toString());if(!Float.isFinite(amount)||amount<=0||amount>1e6f)throw new NumberFormatException();
                // Move on the chosen UCS axis but store absolute geometry in WCS.
                double[] delta=editingUcs.move(axis,sign*amount);
                float[] next=new float[3];
                for(int k=0;k<3;k++){
                    double value=mesh.xyz[vertex*3+k]+delta[k];
                    if(!Double.isFinite(value)||Math.abs(value)>1e9d)throw new NumberFormatException();
                    next[k]=(float)value;
                }
                float[] before={mesh.xyz[vertex*3],mesh.xyz[vertex*3+1],mesh.xyz[vertex*3+2]};
                for(int k=0;k<3;k++)mesh.xyz[vertex*3+k]=next[k];
                if(editHistory.record(mesh.xyz,vertex,before,next))refresh3dUndoButtons();
                meshView.geometryChanged();
            }catch(NumberFormatException error){step.setError("Pozitif bir adım girin");}
        });}
        Button save=new Button(this);save.setText("3B DXF kopyasını kaydet");panel.addView(save,new LinearLayout.LayoutParams(-1,-2));save.setOnClickListener(v->{
            if(saving)return;
            if(Arrays.equals(original,mesh.xyz)){step.setError("Önce bir köşeyi düzenleyin");return;}
            Intent intent=new Intent(Intent.ACTION_CREATE_DOCUMENT);intent.addCategory(Intent.CATEGORY_OPENABLE);intent.setType("application/dxf");intent.putExtra(Intent.EXTRA_TITLE,"MusaCAD-3B-duzenleme.dxf");startActivityForResult(intent,SAVE_DXF);
        });
        root.addView(panel,new FrameLayout.LayoutParams(-1,-2,Gravity.BOTTOM));
    }
    private void showUcsSelector(Button control){
        if(saving)return;
        String[] options={"WCS (Dünya)","Ön düzlem (XZ)","Sağ düzlem (YZ)",
            "Özel UCS eksenleri","Kayıtlı UCS seç","Geçerli UCS'yi kaydet","Kayıtlı UCS sil"};
        new android.app.AlertDialog.Builder(this)
            .setTitle("3B koordinat sistemi")
            .setItems(options,(dialog,selected)->{
                if(saving)return;
                switch(selected){
                    case 0: editingUcs=CadUcsFrame.world();meshView.setUcsFrame(editingUcs);control.setText("UCS: WCS");break;
                    case 1: editingUcs=CadUcsFrame.front();meshView.setUcsFrame(editingUcs);control.setText("UCS: ÖN / XZ");break;
                    case 2: editingUcs=CadUcsFrame.right();meshView.setUcsFrame(editingUcs);control.setText("UCS: SAĞ / YZ");break;
                    case 3: showCustomUcsInput(control);break;
                    case 4: showSavedUcsSelector(control);break;
                    case 5: showSaveUcsDialog(control);break;
                    case 6: showDeleteUcsDialog();break;
                }
            })
            .setNegativeButton("İPTAL",null)
            .show();
    }
    private void showCustomUcsInput(Button control){
        if(saving)return;
        EditText input=new EditText(this);
        input.setSingleLine(false);
        input.setText("0,0,0,1,0,0,0,1,0");
        input.setHint("Başlangıç XYZ, X ekseni XYZ, Y ekseni XYZ");
        input.setTextColor(Color.WHITE);
        input.setSelectAllOnFocus(true);
        new android.app.AlertDialog.Builder(this)
            .setTitle("Özel UCS • 9 sayısal değer")
            .setMessage("Önce başlangıç X,Y,Z; sonra X ekseni vektörü; ardından Y ekseni vektörü. Örnek: 0,0,0,1,0,0,0,0,1")
            .setView(input)
            .setNegativeButton("İPTAL",null)
            .setPositiveButton("UYGULA",(dialog,which)->{
                if(saving)return;
                try{
                    CadUcsFrame custom=CadUcsFrame.parse(input.getText().toString());
                    editingUcs=custom;
                    meshView.setUcsFrame(editingUcs);
                    control.setText("UCS: ÖZEL");
                }catch(IllegalArgumentException invalid){
                    android.widget.Toast.makeText(this,"Geçersiz UCS: "+invalid.getMessage(),
                        android.widget.Toast.LENGTH_LONG).show();
                }
            }).show();
    }

    private String storedUcs(){
        return getSharedPreferences(UCS_PREFS,MODE_PRIVATE).getString(UCS_DATA,"");
    }
    private void storeUcs(String contents){
        // apply() stores preferences on the device; no drawings or server data are modified.
        getSharedPreferences(UCS_PREFS,MODE_PRIVATE).edit().putString(UCS_DATA,contents).apply();
    }
    private void showSavedUcsSelector(Button control){
        if(saving)return;
        java.util.Map<String,CadUcsFrame> saved=CadUcsProfiles.read(storedUcs());
        if(saved.isEmpty()){
            android.widget.Toast.makeText(this,"Henüz kayıtlı özel UCS yok",android.widget.Toast.LENGTH_LONG).show();
            return;
        }
        String[] names=saved.keySet().toArray(new String[0]);
        new android.app.AlertDialog.Builder(this)
            .setTitle("Kayıtlı UCS seç")
            .setItems(names,(dialog,which)->{
                if(saving||which<0||which>=names.length)return;
                // Use the immutable frame that was already validated by the preset codec.
                editingUcs=saved.get(names[which]);
                meshView.setUcsFrame(editingUcs);
                control.setText("UCS: "+names[which]);
            })
            .setNegativeButton("İPTAL",null).show();
    }
    private void showSaveUcsDialog(Button control){
        if(saving)return;
        EditText input=new EditText(this);
        input.setSingleLine(true);
        input.setHint("Örneğin: Çatı Eğimi");
        new android.app.AlertDialog.Builder(this)
            .setTitle("Geçerli UCS'yi cihazda kaydet")
            .setMessage("En fazla 12 adlandırılmış UCS kaydedilebilir. Aynı adla kaydetmek eskisini günceller.")
            .setView(input)
            .setNegativeButton("İPTAL",null)
            .setPositiveButton("KAYDET",(dialog,which)->{
                if(saving)return;
                try{
                    String name=CadUcsProfiles.validName(input.getText().toString());
                    storeUcs(CadUcsProfiles.put(storedUcs(),name,editingUcs));
                    control.setText("UCS: "+name);
                    android.widget.Toast.makeText(this,"UCS kaydedildi: "+name,android.widget.Toast.LENGTH_SHORT).show();
                }catch(IllegalArgumentException ex){
                    android.widget.Toast.makeText(this,ex.getMessage(),android.widget.Toast.LENGTH_LONG).show();
                }
            }).show();
    }
    private void showDeleteUcsDialog(){
        if(saving)return;
        java.util.Map<String,CadUcsFrame> saved=CadUcsProfiles.read(storedUcs());
        if(saved.isEmpty()){
            android.widget.Toast.makeText(this,"Silinecek kayıtlı UCS yok",android.widget.Toast.LENGTH_LONG).show();
            return;
        }
        String[] names=saved.keySet().toArray(new String[0]);
        new android.app.AlertDialog.Builder(this)
            .setTitle("Silinecek UCS seç")
            .setItems(names,(dialog,which)->{
                if(saving||which<0||which>=names.length)return;
                String name=names[which];
                new android.app.AlertDialog.Builder(this)
                    .setTitle("UCS kaydını sil")
                    .setMessage(name+" kaydı silinsin mi? Mevcut çizim ve aktif eksenler değişmeyecek.")
                    .setNegativeButton("İPTAL",null)
                    .setPositiveButton("SİL",(confirm,button)->{
                        if(saving)return;
                        storeUcs(CadUcsProfiles.remove(storedUcs(),name));
                        android.widget.Toast.makeText(this,"UCS kaydı silindi",android.widget.Toast.LENGTH_SHORT).show();
                    }).show();
            }).setNegativeButton("İPTAL",null).show();
    }

    @Override protected void onActivityResult(int request,int result,Intent data){
        super.onActivityResult(request,result,data);
        if(request!=SAVE_DXF||result!=RESULT_OK||data==null||data.getData()==null||saving||mesh==null)return;
        Uri uri=data.getData();String path=getIntent().getStringExtra(EXTRA_DXF);saving=true;
        refresh3dUndoButtons();
        new Thread(()->{String message;File staged=null;
            try{
                // Prepare and verify the full DXF before opening Android's potentially truncating URI.
                staged=Dxf3dSafeSave.stage(path==null?null:new File(path),mesh,original,getCacheDir());
                try(OutputStream output=getContentResolver().openOutputStream(uri,"wt")){
                    if(output==null)throw new java.io.IOException("Dosya açılamadı");
                    Dxf3dSafeSave.publish(staged,output);
                }
                message="3B DXF kopyası kaydedildi";
            }catch(Exception e){message="3B kayıt başarısız: "+e.getMessage();}
            finally{if(staged!=null)staged.delete();}
            final String resultMessage=message;runOnUiThread(()->{
                saving=false;refresh3dUndoButtons();
                if(!closing)android.widget.Toast.makeText(this,resultMessage,android.widget.Toast.LENGTH_LONG).show();
            });
        },"MusaCAD-3D-save").start();
    }
    @Override protected void onDestroy(){closing=true;super.onDestroy();}
}
