package com.musa.cad;

import android.app.Activity;
import android.content.Intent;
import android.graphics.Color;
import android.os.Bundle;
import android.os.Build;
import android.window.OnBackInvokedCallback;
import android.window.OnBackInvokedDispatcher;
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
import java.io.IOException;
import java.util.HashSet;
import java.util.Set;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;
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
    // A process-wide FIFO queue ensures the previous Activity's onStop write
    // completes before a replacement Activity reads the journal on rotation.
    private static final ExecutorService RECOVERY_IO=Executors.newSingleThreadExecutor(r->{
        Thread worker=new Thread(r,"MusaCAD-3D-recovery");
        worker.setDaemon(true);
        return worker;
    });
    private volatile boolean closing;
    private boolean saving;
    private Dxf3dMesh mesh;
    private Cad3dSurfaceMetrics.Result openingSurfaceMetrics;
    private Cad3dMeshTopology.Result openingTopology;
    private Cad3dSignedVolume.Result openingSignedVolume;
    private float[] original;
    private CadSaveSourceIntegrity openedSourceIntegrity;
    private Mesh3dView meshView;
    private CadUcsFrame editingUcs=CadUcsFrame.world();
    private final Cad3dEditHistory editHistory=new Cad3dEditHistory();
    private Cad3dSaveCheckpoint saveCheckpoint;
    private File recoveryFile;
    private String recoveryFingerprint;
    private boolean explicitlyDiscarding;
    private boolean pendingRecoveryDecision;
    private final Set<Integer> touchedVertices=new HashSet<>();
    private OnBackInvokedCallback systemBackCallback;
    private Button undo3dButton,redo3dButton;
    @Override protected void onCreate(Bundle state){
        super.onCreate(state);
        // Android 13+ predictive back also follows the unsaved-edit guard.
        if(Build.VERSION.SDK_INT>=33){
            systemBackCallback=this::request3dExit;
            getOnBackInvokedDispatcher().registerOnBackInvokedCallback(
                OnBackInvokedDispatcher.PRIORITY_DEFAULT,systemBackCallback);
        }
        FrameLayout root=new FrameLayout(this);root.setBackgroundColor(Color.rgb(7,19,29));setContentView(root);
        TextView status=new TextView(this);status.setText("3B yüzeyler hazırlanıyor…");status.setTextColor(Color.WHITE);status.setTextSize(16);status.setGravity(Gravity.CENTER);root.addView(status,new FrameLayout.LayoutParams(-1,-1));
        String path=getIntent().getStringExtra(EXTRA_DXF);
        if(path==null){status.setText("3B çalışma dosyası bulunamadı");return;}
        RECOVERY_IO.execute(()->{
            try{
                File sourceFile=new File(path);
                // Capture original bytes BEFORE parsing and verify after parsing.
                // The open session is never allowed to silently mix two source revisions.
                CadSaveSourceIntegrity openedIntegrity=CadSaveSourceIntegrity.capture(sourceFile);
                Dxf3dMesh loaded=Dxf3dMesh.read(sourceFile);
                openedIntegrity.requireUnchanged(sourceFile);
                // Calculate measurements on the loading worker, not on the
                // Android drawing/gesture thread. Keep the 3D viewport usable
                // even if a malformed mesh cannot be quantified.
                Cad3dSurfaceMetrics.Result measured=null;
                try{measured=Cad3dSurfaceMetrics.measure(loaded.xyz,loaded.triangles);}
                catch(IllegalArgumentException ignored){}
                final Cad3dSurfaceMetrics.Result initialMetrics=measured;
                // Bounded audit: never allocate full 3-edge-per-face maps for
                // multi-million-face CAD drawings on an Android phone.
                Cad3dMeshTopology.Result topology=null;
                try{topology=Cad3dMeshTopology.inspect(loaded.xyz,loaded.triangles);}
                catch(IllegalArgumentException ignored){}
                final Cad3dMeshTopology.Result initialTopology=topology;
                Cad3dSignedVolume.Result volume=null;
                if(topology!=null&&topology.closedOrientedByIndex){
                    try{volume=Cad3dSignedVolume.measure(
                            loaded.xyz,loaded.triangles,topology);}
                    catch(IllegalArgumentException ignored){}
                }
                final Cad3dSignedVolume.Result initialVolume=volume;
                float[] openedOriginal=Arrays.copyOf(loaded.xyz,loaded.xyz.length);
                File recoveryPath=Cad3dRecoveryJournal.pathFor(
                    new File(getFilesDir(),"musacad-3d-recovery"),sourceFile);
                String sourceFingerprint=Cad3dRecoveryJournal.fingerprint(
                    sourceFile,openedOriginal,loaded.sourceLines,loaded.coordinateSlots);
                Cad3dRecoveryJournal.Snapshot candidate=null;
                String recoveryWarning=null;
                if(recoveryPath.isFile()){
                    try{
                        candidate=Cad3dRecoveryJournal.load(recoveryPath,sourceFingerprint,
                            openedOriginal,loaded.sourceLines,loaded.coordinateSlots);
                    }catch(IOException incompatible){
                        recoveryWarning="Eski 3B kurtarma kaydı bu projeyle eşleşmiyor; uygulanmadı";
                    }
                }
                final Cad3dRecoveryJournal.Snapshot recovered=candidate;
                final String warning=recoveryWarning;
                runOnUiThread(()->{if(closing)return;mesh=loaded;original=openedOriginal;
                    openedSourceIntegrity=openedIntegrity;
                    openingSurfaceMetrics=initialMetrics;
                    openingTopology=initialTopology;
                    openingSignedVolume=initialVolume;
                    recoveryFile=recoveryPath;recoveryFingerprint=sourceFingerprint;
                    saveCheckpoint=new Cad3dSaveCheckpoint(original);
                    meshView=new Mesh3dView(this,mesh);
                    editHistory.clear();
                    meshView.setAxisDragListener(this::moveGizmoVertex);
                    meshView.setAxisDragCommitListener(this::recordCompletedAxisDrag);
                    meshView.setUcsFrame(editingUcs);
                    applyRequestedMode();root.removeAllViews();root.addView(meshView,new FrameLayout.LayoutParams(-1,-1));
                    TextView hint=new TextView(this);hint.setText("← Geri   •   "+modeHint()+"   •   İki parmak: yakınlaştır\n"+modelHint());
                    hint.setTextColor(Color.WHITE);hint.setTextSize(11);hint.setPadding(16,16,16,16);hint.setBackgroundColor(0xcc07131d);FrameLayout.LayoutParams lp=new FrameLayout.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT,ViewGroup.LayoutParams.WRAP_CONTENT,Gravity.TOP);root.addView(hint,lp);
                    addViewControls(root);
                    if(MODE_EDIT.equals(requestedMode()))addEditControls(root);
                    if(recovered!=null)offer3dRecovery(recovered);
                    else if(warning!=null)
                        android.widget.Toast.makeText(this,warning,
                            android.widget.Toast.LENGTH_LONG).show();
                });
            }catch(Exception e){runOnUiThread(()->{if(!closing)status.setText("3B görünüm açılamadı: "+e.getMessage());});}
        });
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
        if(mesh.approximateCurves>0){
            text.append("\n").append(mesh.approximateCurves)
                .append(" daire/yay yaklaşık çizgi parçalarıyla gösterildi. ")
                .append("Bu köşeler salt okunurdur; görünen yay uzunluğu kesin CAD ölçüsü veya metraj değildir.");
        }
        if(mesh.skippedBlockReferences>0){
            text.append("\nUYARI: ")
                .append(mesh.skippedBlockReferences)
                .append(" blok başvurusu açılamadı (eksik XREF/blok, geçersiz dönüşüm veya çevrim). ")
                .append("3B görünüm ve ölçümler eksiktir; tüm proje için tam metraj/uygunluk kanıtı değildir.");
        }
        if(openingSurfaceMetrics!=null&&openingSurfaceMetrics.triangleCount>0){
            text.append("\nAçılışta okunan üçgen yüzey alanı: ")
                .append(String.format(java.util.Locale.getDefault(),"%.3f",openingSurfaceMetrics.area))
                .append(" çizim birimi² (tam B-Rep yüzeyi veya kesin metraj değildir)");
            if(openingSurfaceMetrics.degenerateCount>0)
                text.append(" • ").append(openingSurfaceMetrics.degenerateCount)
                    .append(" dejenere üçgen");
            if(mesh.solidProxy||mesh.unsupportedSolids>0||mesh.skippedBlockReferences>0)
                text.append(" • Eksik blok/ACIS geometrisi nedeniyle kısmi");
        }
        if(openingTopology!=null&&openingTopology.triangles>0){
            if(!openingTopology.checked)
                text.append("\n3B ağ topolojisi: Büyük modelde kontrol atlandı (50.000 üçgen üstü)");
            else if(openingTopology.closedOrientedByIndex)
                text.append("\n3B ağ topolojisi: Kapalı ve yönlü üçgen ağı. ")
                    .append("Bu sonuç geçerli ACIS/B-Rep katı veya kesin hacim garantisi değildir.");
            else text.append("\n3B ağ topolojisi: ")
                .append(openingTopology.boundaryEdges).append(" açık kenar, ")
                .append(openingTopology.orientationConflicts).append(" ters yönlü kenar, ")
                .append(openingTopology.nonManifoldEdges).append(" çoklu bağlantı, ")
                .append(openingTopology.degenerateTriangles).append(" dejenere üçgen, ")
                .append(openingTopology.duplicateFaces).append(" yinelenen yüz. ")
                .append("Kapalı katı olarak kabul edilmez.");
        }
        if(openingSignedVolume!=null&&openingSignedVolume.available){
            text.append("\nAçılışta okunan yönlendirilmiş üçgen ağın cebirsel hacmi: ")
                .append(String.format(java.util.Locale.getDefault(),
                        "%.3f",openingSignedVolume.signedVolume))
                .append(" çizim birimi³ (yalnız ağ hesabı; kesin metraj, ")
                .append("öz-kesişimsiz B-Rep veya geçerli ACIS katısı garantisi değildir)");
            if(mesh.acisSolids>0||mesh.unsupportedSolids>0||mesh.solidProxy||mesh.skippedBlockReferences>0)
                text.append(" • Eksik blok/ACIS geometrisi nedeniyle kısmi");
        }
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
        if(editHistory.record(mesh.xyz,vertex,before,after)){
            touchedVertices.add(vertex);
            refresh3dUndoButtons();
        }
    }
    private void offer3dRecovery(Cad3dRecoveryJournal.Snapshot recovered){
        pendingRecoveryDecision=true;
        new android.app.AlertDialog.Builder(this)
            .setTitle("3B düzenleme kurtarma kaydı")
            .setMessage(recovered.changedVertices()+" köşe için cihazda tamamlanmamış bir çalışma bulundu. Yalnız aynı çizim verisine ait kayıt kullanılabilir. Değişiklikler geri yüklensin mi?")
            .setCancelable(false)
            .setPositiveButton("KURTAR",(dialog,which)->{
                pendingRecoveryDecision=false;
                float[] saved=recovered.savedGeometry(original);
                recovered.restoreCurrent(mesh.xyz);
                saveCheckpoint.markSaved(saved);
                touchedVertices.clear();
                for(int vertex:recovered.indices())touchedVertices.add(vertex);
                // Source-compatible coordinates were recovered, not the old undo stack.
                editHistory.clear();
                refresh3dUndoButtons();
                meshView.geometryChanged();
            })
            .setNegativeButton("KAYDI SİL",(dialog,which)->{
                pendingRecoveryDecision=false;
                delete3dRecovery();
            })
            .show();
    }
    private void delete3dRecovery(){
        final File target=recoveryFile;
        if(target==null)return;
        // File removal is serialized after any pending checkpoint write.
        RECOVERY_IO.execute(()->{
            if(target.exists()&&!target.delete())
                android.util.Log.w("MusaCAD-3D","Could not remove recovery journal");
        });
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
            // A second-finger button press must never interleave a preview drag and a step.
            meshView.cancelActiveAxisDrag();
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
                if(editHistory.record(mesh.xyz,vertex,before,next)){
                    touchedVertices.add(vertex);
                    refresh3dUndoButtons();
                }
                meshView.geometryChanged();
            }catch(NumberFormatException error){step.setError("Pozitif bir adım girin");}
        });}
        Button save=new Button(this);save.setText("3B DXF kopyasını kaydet");panel.addView(save,new LinearLayout.LayoutParams(-1,-2));
        save.setOnClickListener(v->begin3dSavePicker());
        root.addView(panel,new FrameLayout.LayoutParams(-1,-2,Gravity.BOTTOM));
    }
    private void begin3dSavePicker(){
        if(saving||mesh==null||meshView==null)return;
        // Uncommitted gizmo previews must not be exported as real edits.
        meshView.cancelActiveAxisDrag();
        if(Arrays.equals(original,mesh.xyz)){
            android.widget.Toast.makeText(this,"Önce bir köşeyi düzenleyin",
                android.widget.Toast.LENGTH_SHORT).show();
            return;
        }
        Intent intent=new Intent(Intent.ACTION_CREATE_DOCUMENT);
        intent.addCategory(Intent.CATEGORY_OPENABLE);
        intent.setType("application/dxf");
        intent.putExtra(Intent.EXTRA_TITLE,"MusaCAD-3B-duzenleme.dxf");
        startActivityForResult(intent,SAVE_DXF);
    }

    @Override public void onBackPressed(){
        request3dExit();
    }

    private void request3dExit(){
        if(saving){
            android.widget.Toast.makeText(this,"3B DXF kaydı sürüyor; bitmesini bekleyin",
                android.widget.Toast.LENGTH_SHORT).show();
            return;
        }
        if(meshView!=null)meshView.cancelActiveAxisDrag();
        if(mesh!=null&&saveCheckpoint!=null&&saveCheckpoint.hasUnsavedChanges(mesh.xyz)){
            new android.app.AlertDialog.Builder(this)
                .setTitle("Kaydedilmemiş 3B düzenlemeler")
                .setMessage("Son başarılı DXF kopyası kaydından sonra değişiklikler var. Kaydetmeden çıkarsanız bu değişiklikler kaybolacak.")
                .setPositiveButton("DXF KOPYASI KAYDET",(dialog,which)->begin3dSavePicker())
                .setNegativeButton("KAYDETMEDEN ÇIK",(dialog,which)->{
                    explicitlyDiscarding=true;
                    delete3dRecovery();
                    finish();
                })
                .setNeutralButton("İPTAL",null)
                .show();
            return;
        }
        explicitlyDiscarding=true;
        delete3dRecovery();
        finish();
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
        if(meshView!=null)meshView.cancelActiveAxisDrag();
        // UI edits are blocked until the staged write and destination publish finish.
        final float[] exportSnapshot=Arrays.copyOf(mesh.xyz,mesh.xyz.length);
        Uri uri=data.getData();String path=getIntent().getStringExtra(EXTRA_DXF);saving=true;
        refresh3dUndoButtons();
        new Thread(()->{String message;Dxf3dSafeSave.VerifiedStage staged=null;boolean exported=false;
            try{
                // Prepare and verify the full DXF before opening Android's potentially truncating URI.
                staged=Dxf3dSafeSave.stage(path==null?null:new File(path),mesh,original,getCacheDir(),openedSourceIntegrity);
                // VerifiedStage checks its SHA-256 before SAF may truncate a URI.
                Dxf3dSafeSave.publish(staged,
                    ()->getContentResolver().openOutputStream(uri,"wt"));
                exported=true;
                message="3B DXF kopyası kaydedildi";
            }catch(Exception e){message="3B kayıt başarısız: "+e.getMessage();}
            finally{if(staged!=null)staged.delete();}
            final String resultMessage=message;
            final boolean finishedExport=exported;
            runOnUiThread(()->{
                if(finishedExport&&saveCheckpoint!=null){
                    saveCheckpoint.markSaved(exportSnapshot);
                    if(!saveCheckpoint.hasUnsavedChanges(mesh.xyz))delete3dRecovery();
                }
                saving=false;refresh3dUndoButtons();
                if(!closing)android.widget.Toast.makeText(this,resultMessage,android.widget.Toast.LENGTH_LONG).show();
            });
        },"MusaCAD-3D-save").start();
    }
    @Override protected void onStop(){
        super.onStop();
        if(mesh==null||saveCheckpoint==null||recoveryFile==null||
            recoveryFingerprint==null||pendingRecoveryDecision)return;
        if(meshView!=null)meshView.cancelActiveAxisDrag();
        if(explicitlyDiscarding||!saveCheckpoint.hasUnsavedChanges(mesh.xyz)){
            delete3dRecovery();
            return;
        }
        try{
            // Capture only edited vertices while the Activity still owns UI state.
            // Serialization, fsync and rename run on the shared ordered I/O queue.
            final Cad3dRecoveryJournal.Snapshot snapshot=Cad3dRecoveryJournal.capture(
                original,mesh.xyz,mesh.sourceLines,mesh.coordinateSlots,
                saveCheckpoint,touchedVertices);
            final File destination=recoveryFile;
            final String identity=recoveryFingerprint;
            final int vertexCount=mesh.xyz.length/3;
            RECOVERY_IO.execute(()->{
                try{
                    Cad3dRecoveryJournal.persist(destination,identity,vertexCount,snapshot);
                }catch(IOException failure){
                    android.util.Log.e("MusaCAD-3D","Recovery snapshot was not written",failure);
                }
            });
        }catch(IOException failure){
            android.util.Log.e("MusaCAD-3D","Recovery snapshot capture failed",failure);
        }
    }
    @Override protected void onDestroy(){
        closing=true;
        if(Build.VERSION.SDK_INT>=33&&systemBackCallback!=null){
            getOnBackInvokedDispatcher().unregisterOnBackInvokedCallback(systemBackCallback);
            systemBackCallback=null;
        }
        super.onDestroy();
    }
}
