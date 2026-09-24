package com.musa.cad;

import android.app.Activity;
import android.graphics.Color;
import android.os.Bundle;
import android.view.Gravity;
import android.view.ViewGroup;
import android.widget.FrameLayout;
import android.widget.TextView;
import java.io.File;

/** Reads the converted DWG/DXF mesh off the UI thread and opens a real 3D orbit view. */
public final class Mesh3dActivity extends Activity {
    public static final String EXTRA_DXF="com.musa.cad.DXF_3D_PATH";
    private volatile boolean closing;
    @Override protected void onCreate(Bundle state){
        super.onCreate(state);
        FrameLayout root=new FrameLayout(this);root.setBackgroundColor(Color.rgb(7,19,29));setContentView(root);
        TextView status=new TextView(this);status.setText("3B yüzeyler hazırlanıyor…");status.setTextColor(Color.WHITE);status.setTextSize(16);status.setGravity(Gravity.CENTER);root.addView(status,new FrameLayout.LayoutParams(-1,-1));
        String path=getIntent().getStringExtra(EXTRA_DXF);
        if(path==null){status.setText("3B çalışma dosyası bulunamadı");return;}
        new Thread(()->{
            try{
                Dxf3dMesh mesh=Dxf3dMesh.read(new File(path));
                runOnUiThread(()->{if(closing)return;Mesh3dView view=new Mesh3dView(this,mesh);root.removeAllViews();root.addView(view,new FrameLayout.LayoutParams(-1,-1));
                    TextView hint=new TextView(this);hint.setText("← Geri   •   Tek parmak: döndür   •   İki parmak: yakınlaştır   •   İki köşe: 3B ölçüm\n"+(mesh.triangles.length/3)+" üçgen  •  "+(mesh.xyz.length/3)+" köşe"+(mesh.unsupportedSolids>0?"  •  Katı modeller görüntülenemedi":""));
                    hint.setTextColor(Color.WHITE);hint.setTextSize(11);hint.setPadding(16,16,16,16);hint.setBackgroundColor(0xcc07131d);FrameLayout.LayoutParams lp=new FrameLayout.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT,ViewGroup.LayoutParams.WRAP_CONTENT,Gravity.TOP);root.addView(hint,lp);
                });
            }catch(Exception e){runOnUiThread(()->{if(!closing)status.setText("3B görünüm açılamadı: "+e.getMessage());});}
        },"MusaCAD-3D-load").start();
    }
    @Override protected void onDestroy(){closing=true;super.onDestroy();}
}
