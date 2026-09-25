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
    private volatile boolean closing;
    private boolean saving;
    private Dxf3dMesh mesh;
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
                runOnUiThread(()->{if(closing)return;mesh=loaded;original=Arrays.copyOf(mesh.xyz,mesh.xyz.length);meshView=new Mesh3dView(this,mesh);root.removeAllViews();root.addView(meshView,new FrameLayout.LayoutParams(-1,-1));
                    TextView hint=new TextView(this);hint.setText("← Geri   •   Tek parmak: döndür   •   İki parmak: yakınlaştır   •   İki köşe: 3B ölçüm\n"+(mesh.triangles.length/3)+" üçgen  •  "+(mesh.xyz.length/3)+" köşe"+(mesh.unsupportedSolids>0?"  •  Katı modeller görüntülenemedi":""));
                    hint.setTextColor(Color.WHITE);hint.setTextSize(11);hint.setPadding(16,16,16,16);hint.setBackgroundColor(0xcc07131d);FrameLayout.LayoutParams lp=new FrameLayout.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT,ViewGroup.LayoutParams.WRAP_CONTENT,Gravity.TOP);root.addView(hint,lp);
                    addViewControls(root);
                    addEditControls(root);
                });
            }catch(Exception e){runOnUiThread(()->{if(!closing)status.setText("3B görünüm açılamadı: "+e.getMessage());});}
        },"MusaCAD-3D-load").start();
    }
    private void applyRequestedMode(){
        if(meshView==null)return;
        String mode=getIntent().getStringExtra(EXTRA_MODE);
        if(mode==null)mode=MODE_ISO;
        switch(mode){
            case MODE_FRONT: meshView.setFrontView(); break;
            case MODE_TOP: meshView.setTopView(); break;
            case MODE_RIGHT: meshView.setRightView(); break;
            case MODE_WIREFRAME: meshView.setWireframe(true); meshView.setIsometricView(); break;
            case MODE_SURFACE: meshView.setWireframe(false); meshView.setIsometricView(); break;
            case MODE_ORBIT: meshView.setIsometricView(); break;
            case MODE_MEASURE: meshView.setIsometricView(); break;
            case MODE_EDIT: meshView.setIsometricView(); break;
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

    private void addEditControls(FrameLayout root){
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
        if(request!=SAVE_DXF||result!=RESULT_OK||data==null||data.getData()==null||saving||mesh==null)return;
        Uri uri=data.getData();String path=getIntent().getStringExtra(EXTRA_DXF);saving=true;
        new Thread(()->{String message;
            try(OutputStream output=getContentResolver().openOutputStream(uri,"wt")){
                if(output==null)throw new java.io.IOException("Dosya açılamadı");
                Dxf3dEditor.write(new File(path),output,mesh,original);message="3B DXF kopyası kaydedildi";
            }catch(Exception e){message="3B kayıt başarısız: "+e.getMessage();}
            final String resultMessage=message;runOnUiThread(()->{saving=false;if(!closing)android.widget.Toast.makeText(this,resultMessage,android.widget.Toast.LENGTH_LONG).show();});
        },"MusaCAD-3D-save").start();
    }
    @Override protected void onDestroy(){closing=true;super.onDestroy();}
}
