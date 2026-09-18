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
import java.util.*;
import java.util.concurrent.*;

public class MainActivity extends AppCompatActivity {
    private static final int OPEN=20,SAVE_DXF=21;
    private static final int MENU_OPEN=1,MENU_LAYERS=2,MENU_FIT=3,MENU_SHARE=4,MENU_INFO=5,MENU_ABOUT=6,MENU_SAVE_DXF=7,MENU_PRINT=8,MENU_LAYOUTS=9;
    private final ExecutorService loader=Executors.newSingleThreadExecutor();
    private LoadTask activeLoad;

    private static final class LoadTask {Future<?> future;AlertDialog dialog;TextView progress;}
    private static final class Loaded {
        File file,workingDxf;Bitmap bitmap;DxfParser.Result parsed;String name;boolean dxf;
        void dispose(){if(bitmap!=null&&!bitmap.isRecycled())bitmap.recycle();if(workingDxf!=null&&workingDxf!=file)workingDxf.delete();if(file!=null)file.delete();}
    }

    private CheckBox snapToggle;
    private DxfParser.Result activeDxf;
    private CadView cad;
    private TextView fileName,result,editStatusText,tabFileName;
    private EditText commandInput;
    private File currentFile,editingBaseDxf;
    private String currentDisplayName="cizim.dwg";
    private View[] modeButtons;
    private View welcomePanel,shareButton,shareToolButton;
    private String lastCommandRaw="";

    @Override protected void onCreate(Bundle b){
        super.onCreate(b);WindowCompat.setDecorFitsSystemWindows(getWindow(),false);setContentView(R.layout.activity_main);
        View root=findViewById(R.id.mainRoot);
        ViewCompat.setOnApplyWindowInsetsListener(root,(v,insets)->{Insets bars=insets.getInsets(WindowInsetsCompat.Type.systemBars());v.setPadding(0,bars.top,0,bars.bottom+dp(6));return insets;});

        cad=findViewById(R.id.cadView);fileName=findViewById(R.id.fileName);result=findViewById(R.id.resultText);welcomePanel=findViewById(R.id.welcomePanel);
        editStatusText=findViewById(R.id.editStatusText);tabFileName=findViewById(R.id.tabFileName);commandInput=findViewById(R.id.commandInput);
        shareButton=findViewById(R.id.shareButton);shareToolButton=findViewById(R.id.shareToolButton);
        cad.setListener(new CadView.Listener(){
            public void onMeasurement(String v){result.setText(v);}
            public void onCalibrationRequested(double px){showCalibration();}
            public void onSelectionReady(){previewSelection();}
            public void onTextRequested(float x,float y){showTextEditor(x,y);}
        });

        snapToggle=findViewById(R.id.snapToggle);snapToggle.setOnCheckedChangeListener((button,checked)->cad.setSnapEnabled(checked));
        modeButtons=new View[]{findViewById(R.id.panButton),findViewById(R.id.selectEntityButton),findViewById(R.id.calibrateButton),findViewById(R.id.distanceButton),findViewById(R.id.areaButton),findViewById(R.id.lineButton),findViewById(R.id.polylineButton),findViewById(R.id.rectangleButton),findViewById(R.id.circleButton),findViewById(R.id.textButton)};
        markModeSelected(R.id.panButton);

        int[] interactive={R.id.menuButton,R.id.openButton,R.id.shareButton,R.id.quickOpenButton,R.id.layersButton,R.id.bottomLayersButton,R.id.rightLayersButton,R.id.snapToggle,R.id.panButton,R.id.selectEntityButton,R.id.moveEntityButton,R.id.rotateEntityButton,R.id.copyEntityButton,R.id.deleteEntityButton,R.id.calibrateButton,R.id.distanceButton,R.id.bottomMeasureButton,R.id.areaButton,R.id.lineButton,R.id.polylineButton,R.id.rectangleButton,R.id.circleButton,R.id.textButton,R.id.finishEditButton,R.id.saveDxfButton,R.id.zoomInButton,R.id.zoomOutButton,R.id.rightZoomInButton,R.id.rightZoomOutButton,R.id.fitButton,R.id.rightFitButton,R.id.undoButton,R.id.clearButton,R.id.shareToolButton,R.id.commandSendButton};
        for(int id:interactive)installInteractiveFeedback(findViewById(id));

        findViewById(R.id.menuButton).setOnClickListener(this::showMainMenu);findViewById(R.id.appTitle).setOnClickListener(this::showMainMenu);
        findViewById(R.id.layersButton).setOnClickListener(v->showLayers());
        findViewById(R.id.bottomLayersButton).setOnClickListener(v->showLayers());
        findViewById(R.id.rightLayersButton).setOnClickListener(v->showLayers());
        findViewById(R.id.bottomMeasureButton).setOnClickListener(v->selectMode(R.id.distanceButton,CadView.Mode.DISTANCE));
        findViewById(R.id.openButton).setOnClickListener(v->open());findViewById(R.id.quickOpenButton).setOnClickListener(v->open());
        findViewById(R.id.panButton).setOnClickListener(v->selectMode(R.id.panButton,CadView.Mode.PAN));
        findViewById(R.id.selectEntityButton).setOnClickListener(v->selectEditMode(R.id.selectEntityButton,CadView.Mode.SELECT_ENTITY));
        findViewById(R.id.moveEntityButton).setOnClickListener(v->{v.performHapticFeedback(HapticFeedbackConstants.KEYBOARD_TAP);if(!cad.armMoveSelected())noSourceSelection();});
        findViewById(R.id.rotateEntityButton).setOnClickListener(v->{v.performHapticFeedback(HapticFeedbackConstants.KEYBOARD_TAP);if(!cad.rotateSelectedEntity())noSourceSelection();});
        findViewById(R.id.copyEntityButton).setOnClickListener(v->{v.performHapticFeedback(HapticFeedbackConstants.KEYBOARD_TAP);if(!cad.copySelectedEntity())noSourceSelection();});
        findViewById(R.id.deleteEntityButton).setOnClickListener(v->{v.performHapticFeedback(HapticFeedbackConstants.KEYBOARD_TAP);if(!cad.deleteSelectedEntity())noSourceSelection();});
        findViewById(R.id.calibrateButton).setOnClickListener(v->selectMode(R.id.calibrateButton,CadView.Mode.CALIBRATE));
        findViewById(R.id.distanceButton).setOnClickListener(v->selectMode(R.id.distanceButton,CadView.Mode.DISTANCE));
        findViewById(R.id.areaButton).setOnClickListener(v->selectMode(R.id.areaButton,CadView.Mode.AREA));
        findViewById(R.id.lineButton).setOnClickListener(v->selectEditMode(R.id.lineButton,CadView.Mode.DRAW_LINE));
        findViewById(R.id.polylineButton).setOnClickListener(v->selectEditMode(R.id.polylineButton,CadView.Mode.DRAW_POLYLINE));
        findViewById(R.id.rectangleButton).setOnClickListener(v->selectEditMode(R.id.rectangleButton,CadView.Mode.DRAW_RECTANGLE));
        findViewById(R.id.circleButton).setOnClickListener(v->selectEditMode(R.id.circleButton,CadView.Mode.DRAW_CIRCLE));
        findViewById(R.id.textButton).setOnClickListener(v->selectEditMode(R.id.textButton,CadView.Mode.DRAW_TEXT));
        findViewById(R.id.finishEditButton).setOnClickListener(v->{v.performHapticFeedback(HapticFeedbackConstants.KEYBOARD_TAP);if(!cad.finishEdit())Toast.makeText(this,"Bitirmek için Çoklu çizgi modunda en az 2 nokta seçin",Toast.LENGTH_SHORT).show();});
        findViewById(R.id.saveDxfButton).setOnClickListener(v->{v.performHapticFeedback(HapticFeedbackConstants.KEYBOARD_TAP);requestEditedDxfSave();});
        findViewById(R.id.zoomInButton).setOnClickListener(v->{v.performHapticFeedback(HapticFeedbackConstants.KEYBOARD_TAP);cad.zoomBy(1.35f);});
        findViewById(R.id.zoomOutButton).setOnClickListener(v->{v.performHapticFeedback(HapticFeedbackConstants.KEYBOARD_TAP);cad.zoomBy(1f/1.35f);});
        findViewById(R.id.rightZoomInButton).setOnClickListener(v->{v.performHapticFeedback(HapticFeedbackConstants.KEYBOARD_TAP);cad.zoomBy(1.35f);});
        findViewById(R.id.rightZoomOutButton).setOnClickListener(v->{v.performHapticFeedback(HapticFeedbackConstants.KEYBOARD_TAP);cad.zoomBy(1f/1.35f);});
        findViewById(R.id.fitButton).setOnClickListener(v->{v.performHapticFeedback(HapticFeedbackConstants.KEYBOARD_TAP);cad.fitToScreen();});
        findViewById(R.id.rightFitButton).setOnClickListener(v->{v.performHapticFeedback(HapticFeedbackConstants.KEYBOARD_TAP);cad.fitToScreen();});
        findViewById(R.id.undoButton).setOnClickListener(v->{v.performHapticFeedback(HapticFeedbackConstants.KEYBOARD_TAP);cad.undo();});
        findViewById(R.id.clearButton).setOnClickListener(v->{v.performHapticFeedback(HapticFeedbackConstants.KEYBOARD_TAP);cad.clearMeasurement();});
        shareButton.setOnClickListener(v->showShare());shareToolButton.setOnClickListener(v->showShare());
        findViewById(R.id.commandSendButton).setOnClickListener(v->executeCommand());
        commandInput.setOnEditorActionListener((v,action,event)->{
            if(action==android.view.inputmethod.EditorInfo.IME_ACTION_DONE||action==android.view.inputmethod.EditorInfo.IME_ACTION_GO||
               (event!=null&&event.getKeyCode()==KeyEvent.KEYCODE_ENTER&&event.getAction()==KeyEvent.ACTION_DOWN)){
                executeCommand();return true;
            }
            return false;
        });
        commandInput.setOnKeyListener((v,keyCode,event)->{
            if(keyCode==KeyEvent.KEYCODE_ENTER&&event.getAction()==KeyEvent.ACTION_DOWN){executeCommand();return true;}
            return false;
        });
        updateShareEnabled(false);updateEditorEnabled(false);handleIncomingIntent(getIntent());
    }

    private void executeCommand(){
        if(commandInput==null)return;
        String raw=commandInput.getText().toString().trim();
        commandInput.setText("");
        android.view.inputmethod.InputMethodManager imm=(android.view.inputmethod.InputMethodManager)getSystemService(INPUT_METHOD_SERVICE);
        if(imm!=null)imm.hideSoftInputFromWindow(commandInput.getWindowToken(),0);

        if(raw.isEmpty()){
            if(cad.confirmCurrentCommand()){
                result.setText("Komut tamamlandı • Enter ile onaylandı");
                return;
            }
            if(lastCommandRaw.isEmpty()){
                result.setText("Komut bekleniyor • ? yazarak listeyi görün");
                return;
            }
            raw=lastCommandRaw;
        }else lastCommandRaw=raw;

        CadCommand.Action action=CadCommand.parse(raw);
        switch(action){
            case LINE:
                selectEditMode(R.id.lineButton,CadView.Mode.DRAW_LINE);
                result.setText("LINE • İlk noktayı seçin");
                break;
            case POLYLINE:
                selectEditMode(R.id.polylineButton,CadView.Mode.DRAW_POLYLINE);
                result.setText("PLINE • Noktaları seçin • Enter/Bitir ile tamamlayın");
                break;
            case CIRCLE:
                selectEditMode(R.id.circleButton,CadView.Mode.DRAW_CIRCLE);
                result.setText("CIRCLE • Merkez ve yarıçap noktası seçin");
                break;
            case ARC:
                if(!canEdit()){Toast.makeText(this,"Bu çizim düzenleme için vektörel olarak açılamadı",Toast.LENGTH_SHORT).show();break;}
                cad.setMode(CadView.Mode.DRAW_ARC);
                markModeSelected(0);
                result.setText("ARC • Başlangıç, yay üzeri ve bitiş olmak üzere 3 nokta seçin");
                break;
            case ELLIPSE:
                if(!canEdit()){Toast.makeText(this,"Bu çizim düzenleme için vektörel olarak açılamadı",Toast.LENGTH_SHORT).show();break;}
                cad.setMode(CadView.Mode.DRAW_ELLIPSE);
                markModeSelected(0);
                result.setText("ELLIPSE • Merkez, ana eksen ucu ve kısa eksen yönünü seçin");
                break;
            case POINT:
                if(!canEdit()){Toast.makeText(this,"Bu çizim düzenleme için vektörel olarak açılamadı",Toast.LENGTH_SHORT).show();break;}
                cad.setMode(CadView.Mode.DRAW_POINT);
                markModeSelected(0);
                result.setText("POINT • Noktanın yerini seçin");
                break;
            case XLINE:
                if(!canEdit()){Toast.makeText(this,"Bu çizim düzenleme için vektörel olarak açılamadı",Toast.LENGTH_SHORT).show();break;}
                cad.setMode(CadView.Mode.DRAW_XLINE);
                markModeSelected(0);
                result.setText("XLINE • Doğrultu için iki nokta seçin");
                break;
            case RECTANGLE:
                selectEditMode(R.id.rectangleButton,CadView.Mode.DRAW_RECTANGLE);
                result.setText("RECTANG • İki köşe seçin");
                break;
            case TEXT:
                selectEditMode(R.id.textButton,CadView.Mode.DRAW_TEXT);
                result.setText("TEXT/MTEXT • Yazı konumuna dokunun");
                break;
            case SELECT:
                selectEditMode(R.id.selectEntityButton,CadView.Mode.SELECT_ENTITY);
                result.setText("SELECT • Nesne seçin");
                break;
            case PAN:
                selectMode(R.id.panButton,CadView.Mode.PAN);
                result.setText("PAN • Çizimi sürükleyin");
                break;
            case MOVE:
                if(!cad.armMoveSelected()){
                    selectEditMode(R.id.selectEntityButton,CadView.Mode.SELECT_ENTITY);
                    result.setText("MOVE • Önce nesne seçin, sonra M yazın");
                }
                break;
            case COPY:
                if(!cad.copySelectedEntity()){
                    selectEditMode(R.id.selectEntityButton,CadView.Mode.SELECT_ENTITY);
                    result.setText("COPY • Önce nesne seçin, sonra CO yazın");
                }
                break;
            case ROTATE:
                if(!cad.rotateSelectedEntity()){
                    selectEditMode(R.id.selectEntityButton,CadView.Mode.SELECT_ENTITY);
                    result.setText("ROTATE • Önce nesne seçin, sonra RO yazın");
                }
                break;
            case ERASE:
                if(!cad.deleteSelectedEntity()){
                    selectEditMode(R.id.selectEntityButton,CadView.Mode.SELECT_ENTITY);
                    result.setText("ERASE • Önce nesne seçin, sonra E yazın");
                }
                break;
            case SCALE:
                runScaleCommand();
                break;
            case MIRROR:
                runMirrorCommand();
                break;
            case OFFSET:
                runOffsetCommand();
                break;
            case ARRAY:
                runArrayCommand();
                break;
            case EXPLODE:
                if(!ensureTransformSelection("EXPLODE"))break;
                if(cad.explodeSelectedEntity())result.setText("EXPLODE • Çoklu çizgi/dikdörtgen parçalara ayrıldı");
                else result.setText("EXPLODE • Bu nesne tipi için patlatma desteklenmiyor");
                break;
            case OSNAP:
                if(activeDxf==null){result.setText("OSNAP • Önce çizim açın");break;}
                snapToggle.setChecked(!snapToggle.isChecked());
                result.setText("OSNAP • Nesne yakalama "+(snapToggle.isChecked()?"AÇIK":"KAPALI"));
                break;
            case REGEN:
                cad.regenerate();
                result.setText("REGEN • Görünüm yeniden oluşturuldu");
                break;
            case TRIM:
                if(!ensureTransformSelection("TRIM"))break;
                if(cad.armTrimSelected())result.setText("TRIM • Kesme sınırı olacak ikinci çizgiye dokunun");
                else result.setText("TRIM • Hedef nesne LINE olmalı");
                break;
            case EXTEND:
                if(!ensureTransformSelection("EXTEND"))break;
                if(cad.armExtendSelected())result.setText("EXTEND • Uzatma sınırı olacak ikinci çizgiye dokunun");
                else result.setText("EXTEND • Hedef nesne LINE olmalı");
                break;
            case FILLET:
                runFilletCommand();
                break;
            case CHAMFER:
                runChamferCommand();
                break;
            case BREAK:
                if(!ensureTransformSelection("BREAK"))break;
                if(cad.armBreakSelected())result.setText("BREAK • Çizgiyi böleceğiniz noktaya dokunun");
                else result.setText("BREAK • Hedef nesne LINE olmalı");
                break;
            case PEDIT:
                if(!ensureTransformSelection("PEDIT"))break;
                if(cad.toggleSelectedPolylineClosed())result.setText("PEDIT • Polyline açık/kapalı durumu değiştirildi");
                else result.setText("PEDIT • Seçili nesne POLYLINE olmalı");
                break;
            case LIST:
                if(!ensureTransformSelection("LIST"))break;
                String entityInfo=cad.selectedEntityInfo();
                if(entityInfo==null)result.setText("LIST • Seçili nesne bilgisi alınamadı");
                else new AlertDialog.Builder(this).setTitle("LIST • Nesne bilgisi").setMessage(entityInfo).setPositiveButton("TAMAM",null).show();
                break;
            case MATCHPROP:
                if(!ensureTransformSelection("MATCHPROP"))break;
                if(cad.armMatchProperties())result.setText("MATCHPROP • Özelliklerin aktarılacağı hedef nesneye dokunun");
                else result.setText("MATCHPROP • Kaynak nesne seçilemedi");
                break;
            case JOIN:
                if(!ensureTransformSelection("JOIN"))break;
                if(cad.armJoinSelected())result.setText("JOIN • Birleştirilecek ikinci LINE/POLYLINE nesnesine dokunun");
                else result.setText("JOIN • Seçili nesne açık LINE veya POLYLINE olmalı");
                break;
            case LAYER:
                showLayers();
                break;
            case PROPERTIES:
                if(activeDxf!=null)showDrawingInfo();
                else Toast.makeText(this,"Özellikler için önce bir çizim açın",Toast.LENGTH_SHORT).show();
                break;
            case DISTANCE:
                selectMode(R.id.distanceButton,CadView.Mode.DISTANCE);
                result.setText("DIST • İki nokta seçin");
                break;
            case AREA:
                selectMode(R.id.areaButton,CadView.Mode.AREA);
                result.setText("AREA • Sınır noktalarını seçin");
                break;
            case ZOOM:
                result.setText("ZOOM • Extents için Z E veya ZE kullanın");
                break;
            case ZOOM_EXTENTS:
                cad.fitToScreen();
                result.setText("ZOOM EXTENTS • Çizim ekrana sığdırıldı");
                break;
            case UNDO:
                cad.undo();
                break;
            case REDO:
                if(!cad.redo())result.setText("REDO • Yeniden uygulanacak işlem yok");
                break;
            case SAVE:
                requestEditedDxfSave();
                break;
            case HELP:
                showCommandHelp();
                break;
            case UNSUPPORTED:
                result.setText(CadCommand.canonical(raw)+" • Komut tanındı; MusaCAD motor desteği henüz yok");
                break;
            default:
                result.setText("Bilinmeyen komut: "+raw+" • ? yazarak komutları görün");
                break;
        }
    }

    private boolean ensureTransformSelection(String command){
        if(cad.hasSelectedEntity())return true;
        selectEditMode(R.id.selectEntityButton,CadView.Mode.SELECT_ENTITY);
        result.setText(command+" • Önce nesne seçin, sonra komutu tekrar yazın");
        return false;
    }

    private void runScaleCommand(){
        if(!ensureTransformSelection("SCALE"))return;
        EditText input=new EditText(this);
        input.setSingleLine(true);
        input.setHint("Ölçek katsayısı (örn. 2 veya 0.5)");
        input.setText("2");
        input.setSelectAllOnFocus(true);
        input.setInputType(android.text.InputType.TYPE_CLASS_NUMBER|android.text.InputType.TYPE_NUMBER_FLAG_DECIMAL);
        AlertDialog dialog=new AlertDialog.Builder(this)
            .setTitle("SCALE • Ölçekle")
            .setMessage("Seçili nesne kendi merkezine göre ölçeklenecek.")
            .setView(input)
            .setPositiveButton("UYGULA",null)
            .setNegativeButton("İPTAL",null)
            .create();
        dialog.setOnShowListener(d->dialog.getButton(AlertDialog.BUTTON_POSITIVE).setOnClickListener(v->{
            try{
                float factor=Float.parseFloat(input.getText().toString().trim().replace(',','.'));
                if(!Float.isFinite(factor)||factor<=0f){input.setError("Sıfırdan büyük bir değer girin");return;}
                if(!cad.scaleSelectedEntity(factor)){dialog.dismiss();result.setText("SCALE • Seçili nesne ölçeklenemedi");return;}
                dialog.dismiss();result.setText(String.format(Locale.getDefault(),"SCALE • Ölçek %.3f uygulandı",factor));
            }catch(Exception e){input.setError("Geçerli bir ölçek katsayısı girin");}
        }));
        dialog.show();
    }

    private void runMirrorCommand(){
        if(!ensureTransformSelection("MIRROR"))return;
        new AlertDialog.Builder(this)
            .setTitle("MIRROR • Aynala")
            .setItems(new String[]{"Dikey eksene göre","Yatay eksene göre"},(d,which)->{
                boolean vertical=which==0;
                if(cad.mirrorSelectedEntity(vertical))result.setText("MIRROR • "+(vertical?"Dikey":"Yatay")+" eksene göre aynalandı");
                else result.setText("MIRROR • Seçili nesne aynalanamadı");
            })
            .setNegativeButton("İPTAL",null)
            .show();
    }

    private void runOffsetCommand(){
        if(!ensureTransformSelection("OFFSET"))return;
        EditText input=new EditText(this);
        input.setSingleLine(true);
        input.setHint("Ofset mesafesi (+ / -)");
        input.setText("10");
        input.setSelectAllOnFocus(true);
        input.setInputType(android.text.InputType.TYPE_CLASS_NUMBER|android.text.InputType.TYPE_NUMBER_FLAG_DECIMAL|android.text.InputType.TYPE_NUMBER_FLAG_SIGNED);
        AlertDialog dialog=new AlertDialog.Builder(this)
            .setTitle("OFFSET • Paralel kopya")
            .setMessage("Çizgi, daire ve dikdörtgen desteklenir. Negatif değer karşı yön / iç tarafa ofset uygular.")
            .setView(input)
            .setPositiveButton("UYGULA",null)
            .setNegativeButton("İPTAL",null)
            .create();
        dialog.setOnShowListener(d->dialog.getButton(AlertDialog.BUTTON_POSITIVE).setOnClickListener(v->{
            try{
                float distance=Float.parseFloat(input.getText().toString().trim().replace(',','.'));
                if(!Float.isFinite(distance)||Math.abs(distance)<1e-6f){input.setError("Sıfırdan farklı bir mesafe girin");return;}
                if(!cad.offsetSelectedEntity(distance)){
                    dialog.dismiss();
                    result.setText("OFFSET • Bu nesne tipinde henüz desteklenmiyor veya mesafe geçersiz");
                    return;
                }
                dialog.dismiss();result.setText(String.format(Locale.getDefault(),"OFFSET • %.3f birim paralel kopya oluşturuldu",distance));
            }catch(Exception e){input.setError("Geçerli bir mesafe girin");}
        }));
        dialog.show();
    }

    private void runFilletCommand(){
        if(!ensureTransformSelection("FILLET"))return;
        EditText input=new EditText(this);
        input.setSingleLine(true);
        input.setHint("Yarıçap");
        input.setText("10");
        input.setSelectAllOnFocus(true);
        input.setInputType(android.text.InputType.TYPE_CLASS_NUMBER|android.text.InputType.TYPE_NUMBER_FLAG_DECIMAL);
        AlertDialog dialog=new AlertDialog.Builder(this)
            .setTitle("FILLET • Köşe yuvarlat")
            .setMessage("Seçili LINE ile dokunacağınız ikinci LINE arasında teğet yay oluşturulur.")
            .setView(input)
            .setPositiveButton("DEVAM",null)
            .setNegativeButton("İPTAL",null)
            .create();
        dialog.setOnShowListener(d->dialog.getButton(AlertDialog.BUTTON_POSITIVE).setOnClickListener(v->{
            try{
                float radius=Float.parseFloat(input.getText().toString().trim().replace(',','.'));
                if(!Float.isFinite(radius)||radius<=0f){input.setError("Sıfırdan büyük bir yarıçap girin");return;}
                if(!cad.armFilletSelected(radius)){dialog.dismiss();result.setText("FILLET • Hedef nesne LINE olmalı");return;}
                dialog.dismiss();result.setText(String.format(Locale.getDefault(),"FILLET • R=%.3f • İkinci çizgiye dokunun",radius));
            }catch(Exception e){input.setError("Geçerli bir yarıçap girin");}
        }));
        dialog.show();
    }

    private void runChamferCommand(){
        if(!ensureTransformSelection("CHAMFER"))return;
        EditText input=new EditText(this);
        input.setSingleLine(true);
        input.setHint("Pah mesafesi");
        input.setText("10");
        input.setSelectAllOnFocus(true);
        input.setInputType(android.text.InputType.TYPE_CLASS_NUMBER|android.text.InputType.TYPE_NUMBER_FLAG_DECIMAL);
        AlertDialog dialog=new AlertDialog.Builder(this)
            .setTitle("CHAMFER • Pah kır")
            .setMessage("Her iki çizgide aynı mesafe kullanılarak düz pah oluşturulur.")
            .setView(input)
            .setPositiveButton("DEVAM",null)
            .setNegativeButton("İPTAL",null)
            .create();
        dialog.setOnShowListener(d->dialog.getButton(AlertDialog.BUTTON_POSITIVE).setOnClickListener(v->{
            try{
                float distance=Float.parseFloat(input.getText().toString().trim().replace(',','.'));
                if(!Float.isFinite(distance)||distance<=0f){input.setError("Sıfırdan büyük bir mesafe girin");return;}
                if(!cad.armChamferSelected(distance)){dialog.dismiss();result.setText("CHAMFER • Hedef nesne LINE olmalı");return;}
                dialog.dismiss();result.setText(String.format(Locale.getDefault(),"CHAMFER • D=%.3f • İkinci çizgiye dokunun",distance));
            }catch(Exception e){input.setError("Geçerli bir pah mesafesi girin");}
        }));
        dialog.show();
    }

    private void runArrayCommand(){
        if(!ensureTransformSelection("ARRAY"))return;
        LinearLayout box=new LinearLayout(this);box.setOrientation(LinearLayout.VERTICAL);int p=dp(16);box.setPadding(p,p/2,p,p/2);
        EditText rows=new EditText(this);rows.setHint("Satır sayısı");rows.setText("2");rows.setInputType(android.text.InputType.TYPE_CLASS_NUMBER);box.addView(rows);
        EditText cols=new EditText(this);cols.setHint("Sütun sayısı");cols.setText("2");cols.setInputType(android.text.InputType.TYPE_CLASS_NUMBER);box.addView(cols);
        EditText dx=new EditText(this);dx.setHint("Sütun aralığı");dx.setText("100");dx.setInputType(android.text.InputType.TYPE_CLASS_NUMBER|android.text.InputType.TYPE_NUMBER_FLAG_DECIMAL|android.text.InputType.TYPE_NUMBER_FLAG_SIGNED);box.addView(dx);
        EditText dy=new EditText(this);dy.setHint("Satır aralığı");dy.setText("100");dy.setInputType(android.text.InputType.TYPE_CLASS_NUMBER|android.text.InputType.TYPE_NUMBER_FLAG_DECIMAL|android.text.InputType.TYPE_NUMBER_FLAG_SIGNED);box.addView(dy);
        AlertDialog dialog=new AlertDialog.Builder(this)
            .setTitle("ARRAY • Dikdörtgen dizi")
            .setView(box)
            .setPositiveButton("OLUŞTUR",null)
            .setNegativeButton("İPTAL",null)
            .create();
        dialog.setOnShowListener(d->dialog.getButton(AlertDialog.BUTTON_POSITIVE).setOnClickListener(v->{
            try{
                int r=Integer.parseInt(rows.getText().toString().trim()),c=Integer.parseInt(cols.getText().toString().trim());
                float sx=Float.parseFloat(dx.getText().toString().trim().replace(',','.'));
                float sy=Float.parseFloat(dy.getText().toString().trim().replace(',','.'));
                if(r<1||c<1||r>50||c>50){rows.setError("1-50 arasında satır/sütun kullanın");return;}
                int count=cad.arraySelectedEntity(r,c,sx,sy);
                if(count<=0){dialog.dismiss();result.setText("ARRAY • Dizi oluşturulamadı");return;}
                dialog.dismiss();result.setText("ARRAY • "+count+" kopya oluşturuldu");
            }catch(Exception e){rows.setError("Geçerli satır, sütun ve aralık değerleri girin");}
        }));
        dialog.show();
    }

    private void showCommandHelp(){
        String text=
            "Çalışan komutlar\n\n"+
            "L / LINE • Çizgi\n"+
            "PL / PLINE / POLYLINE • Çoklu çizgi\n"+
            "C / CIRCLE • Daire\n"+
            "A / ARC • 3 noktadan yay\n"+
            "EL / ELLIPSE • Merkez ve iki eksenle elips\n"+
            "PO / POINT • Nokta yerleştir\n"+
            "XL / XLINE • Sonsuz yardımcı doğru\n"+
            "REC / RECTANG / RECTANGLE • Dikdörtgen\n"+
            "DT / T / TEXT / MTEXT • Yazı\n"+
            "SEL / SELECT • Seç\n"+
            "P / PAN • Gezin\n"+
            "M / MOVE • Taşı\n"+
            "CO / CP / COPY • Kopyala\n"+
            "RO / ROTATE • Döndür\n"+
            "E / ERASE / DELETE • Sil\n"+
            "SC / SCALE • Seçili nesneyi ölçekle\n"+
            "MI / MIRROR • Seçili nesneyi aynala\n"+
            "O / OFFSET • Çizgi/daire/dikdörtgen ofseti\n"+
            "AR / ARRAY • Dikdörtgen dizi\n"+
            "X / EXPLODE • Çoklu çizgi/dikdörtgeni parçala\n"+
            "OS / OSNAP • Nesne yakalamayı aç/kapat\n"+
            "RE / REGEN • Görünümü yenile\n"+
            "TR / TRIM • Seçili çizgiyi ikinci çizgide kes\n"+
            "EX / EXTEND • Seçili çizgiyi ikinci çizgiye uzat\n"+
            "F / FILLET • İki çizgi arasına teğet yay\n"+
            "CHA / CHAMFER • İki çizgi arasında düz pah\n"+
            "BR / BREAK • Seçili çizgiyi dokunulan noktadan böl\n"+
            "PE / PEDIT • Polyline açık/kapalı durumunu değiştir\n"+
            "LI / LIST • Seçili nesnenin bilgilerini göster\n"+
            "MA / MATCHPROP • Seçili nesnenin özelliklerini hedefe aktar\n"+
            "J / JOIN • İki açık LINE/POLYLINE nesnesini birleştir\n"+
            "LA / LAYER • Katman\n"+
            "PR / PROPERTIES / PROP • Özellik/Bilgi\n"+
            "DI / DIST / DISTANCE • Mesafe\n"+
            "AA / AREA • Alan\n"+
            "Z E / ZE / ZOOM EXTENTS • Ekrana sığdır\n"+
            "Z / ZOOM • Zoom komutu\n"+
            "U / UNDO • Geri al\n"+
            "REDO • Geri alınan işlemi yeniden uygula\n"+
            "QS / QSAVE / SAVE • Kaydet\n\n"+
            "Tanınıyor, motor desteği henüz yok\n"+
            "BLOCK, DIMSTYLE, DIMALIGNED, DIMLINEAR, HATCH, INSERT, STRETCH";
        new AlertDialog.Builder(this)
            .setTitle("MusaCAD komutları")
            .setMessage(text)
            .setPositiveButton("TAMAM",null)
            .show();
    }

    private void noSourceSelection(){Toast.makeText(this,"Önce Seç ile düzenlenebilir bir kaynak nesne seçin",Toast.LENGTH_SHORT).show();}

    @Override protected void onNewIntent(Intent intent){super.onNewIntent(intent);setIntent(intent);handleIncomingIntent(intent);}
    private void handleIncomingIntent(Intent intent){
        if(intent==null)return;Uri uri=null;
        if(Intent.ACTION_VIEW.equals(intent.getAction()))uri=intent.getData();
        else if(Intent.ACTION_SEND.equals(intent.getAction())){if(android.os.Build.VERSION.SDK_INT>=33)uri=intent.getParcelableExtra(Intent.EXTRA_STREAM,Uri.class);else {@SuppressWarnings("deprecation") Uri legacy=intent.getParcelableExtra(Intent.EXTRA_STREAM);uri=legacy;}}
        if(uri!=null)startLoad(uri);
    }

    private int dp(int value){return Math.round(value*getResources().getDisplayMetrics().density);}
    private void installInteractiveFeedback(View view){
        if(view==null)return;view.setHapticFeedbackEnabled(true);view.setOnTouchListener((v,e)->{int action=e.getActionMasked();if(action==MotionEvent.ACTION_DOWN)v.animate().scaleX(.94f).scaleY(.94f).setDuration(70).start();else if(action==MotionEvent.ACTION_UP||action==MotionEvent.ACTION_CANCEL)v.animate().scaleX(1f).scaleY(1f).setDuration(100).start();return false;});
    }
    private boolean canEdit(){return activeDxf!=null&&editingBaseDxf!=null&&editingBaseDxf.exists();}

    private void showMainMenu(View anchor){
        anchor.performHapticFeedback(HapticFeedbackConstants.KEYBOARD_TAP);PopupMenu popup=new PopupMenu(this,anchor);Menu menu=popup.getMenu();
        menu.add(0,MENU_OPEN,0,"Dosya aç");menu.add(0,MENU_LAYERS,1,"Katmanlar").setEnabled(activeDxf!=null);menu.add(0,MENU_LAYOUTS,2,"Model / Layout").setEnabled(activeDxf!=null&&activeDxf.layoutNames.size()>1);menu.add(0,MENU_FIT,3,"Ekrana sığdır").setEnabled(currentFile!=null);menu.add(0,MENU_SAVE_DXF,4,"Düzenlenmiş DXF kaydet").setEnabled(canEdit());menu.add(0,MENU_PRINT,5,"Yazdır").setEnabled(currentFile!=null);menu.add(0,MENU_SHARE,6,"Paylaş").setEnabled(currentFile!=null);menu.add(0,MENU_INFO,7,"Çizim bilgileri").setEnabled(activeDxf!=null);menu.add(0,MENU_ABOUT,8,"Geliştirici / Hakkında");
        popup.setOnMenuItemClickListener(item->{switch(item.getItemId()){case MENU_OPEN:open();return true;case MENU_LAYERS:showLayers();return true;case MENU_LAYOUTS:showLayouts();return true;case MENU_FIT:cad.fitToScreen();return true;case MENU_SAVE_DXF:requestEditedDxfSave();return true;case MENU_PRINT:printDrawing();return true;case MENU_SHARE:showShare();return true;case MENU_INFO:showDrawingInfo();return true;case MENU_ABOUT:startActivity(new Intent(this,AboutActivity.class));return true;default:return false;}});popup.show();
    }

    private void showDrawingInfo(){
        if(activeDxf==null)return;String shx=activeDxf.fontFallbacks.isEmpty()?"yok":android.text.TextUtils.join(", ",activeDxf.fontFallbacks);String text="Dosya başarıyla açıldı.\n\n"+"Layout: "+activeDxf.activeLayout+" ("+activeDxf.layoutNames.size()+")\n"+"Nesne: "+activeDxf.entityCount+"\n"+"Katman: "+activeDxf.layerCount+"\n"+"Görünür katman: "+activeDxf.visibleLayers.size()+"\n"+"Seçilebilir kaynak nesne: "+activeDxf.editableSourceCount()+"\n"+"Düzenleme toplamı: "+cad.editCount()+"\n"+"Kaynak nesne değişikliği: "+cad.sourceModifiedCount()+"\n"+"SHX metin fallback: "+shx+"\n"+"Complex SHX shape fallback: "+(activeDxf.externalShapeFallback?"var":"yok")+"\n"+"Düzenleme: "+(canEdit()?"açık":"yalnız görüntüleme")+"\n"+"Görüntüleme: vektörel / net yakınlaştırma";
        new AlertDialog.Builder(this).setTitle("Çizim bilgileri").setMessage(text).setPositiveButton("TAMAM",null).show();
    }

    private void updateShareEnabled(boolean enabled){shareButton.setEnabled(enabled);shareButton.setAlpha(enabled?1f:.45f);shareToolButton.setEnabled(enabled);shareToolButton.setAlpha(enabled?1f:.55f);}
    private void updateLayerButtons(boolean enabled){
        int[] ids={R.id.layersButton,R.id.bottomLayersButton,R.id.rightLayersButton};
        for(int id:ids){View v=findViewById(id);if(v!=null){v.setEnabled(enabled);v.setAlpha(enabled?1f:.45f);}}
    }
    private void updateEditorEnabled(boolean enabled){
        int[] ids={R.id.selectEntityButton,R.id.moveEntityButton,R.id.rotateEntityButton,R.id.copyEntityButton,R.id.deleteEntityButton,R.id.lineButton,R.id.polylineButton,R.id.rectangleButton,R.id.circleButton,R.id.textButton,R.id.finishEditButton,R.id.saveDxfButton};
        for(int id:ids){View v=findViewById(id);v.setEnabled(enabled);v.setAlpha(enabled?1f:.45f);}
        if(editStatusText!=null){
            if(currentFile==null){editStatusText.setText("Dosya yok");editStatusText.setTextColor(0xFF8FB7C5);}
            else if(enabled){editStatusText.setText("● Düzenlenebilir");editStatusText.setTextColor(0xFF63E6BE);}
            else {editStatusText.setText("Salt görüntüleme");editStatusText.setTextColor(0xFFFFC766);}
        }
    }
    private void selectMode(int id,CadView.Mode mode){View button=findViewById(id);if(button!=null)button.performHapticFeedback(HapticFeedbackConstants.KEYBOARD_TAP);cad.setMode(mode);markModeSelected(id);}
    private void selectEditMode(int id,CadView.Mode mode){if(!canEdit()){Toast.makeText(this,"Bu çizim düzenleme için vektörel olarak açılamadı",Toast.LENGTH_SHORT).show();return;}selectMode(id,mode);}
    private void markModeSelected(int id){if(modeButtons==null)return;for(View button:modeButtons)button.setSelected(button.getId()==id);}
    private void hideWelcomePanel(){if(welcomePanel==null||welcomePanel.getVisibility()!=View.VISIBLE)return;welcomePanel.animate().alpha(0f).setDuration(180).withEndAction(()->{welcomePanel.setVisibility(View.GONE);welcomePanel.setAlpha(1f);}).start();}

    private void showLicense(){
        String license;try(InputStream in=getAssets().open("COPYING-LibreDWG.txt")){ByteArrayOutputStream out=new ByteArrayOutputStream();byte[] bytes=new byte[4096];int n;while((n=in.read(bytes))!=-1)out.write(bytes,0,n);license=out.toString("UTF-8");}catch(IOException e){license="GPL-3.0-or-later";}
        TextView text=new TextView(this);text.setPadding(24,16,24,16);text.setText("MusaCAD — LibreDWG ile çevrimdışı DWG okuma\nKaynak kod: https://github.com/santiye2019-lab/MusaCAD\n\n"+license);android.text.util.Linkify.addLinks(text,android.text.util.Linkify.WEB_URLS);text.setMovementMethod(android.text.method.LinkMovementMethod.getInstance());ScrollView scroll=new ScrollView(this);scroll.addView(text);new AlertDialog.Builder(this).setTitle("Lisans ve kaynak kod").setView(scroll).setPositiveButton("KAPAT",null).show();
    }

    private void open(){
        if(activeLoad!=null){Toast.makeText(this,"Devam eden işlem bitmeden yeni dosya açılamaz",Toast.LENGTH_SHORT).show();return;}
        startActivityForResult(new Intent(this,RecentFilesActivity.class),OPEN);
    }
    @Override protected void onActivityResult(int r,int c,Intent data){super.onActivityResult(r,c,data);if(c!=RESULT_OK||data==null||data.getData()==null)return;if(r==OPEN)startLoad(data.getData());else if(r==SAVE_DXF)saveEditedDxf(data.getData());}
    private void cancelLoad(){LoadTask task=activeLoad;activeLoad=null;if(task!=null){if(task.future!=null)task.future.cancel(true);if(task.dialog!=null)task.dialog.dismiss();}}

    private void startLoad(Uri uri){
        cancelLoad();LoadTask task=new LoadTask();activeLoad=task;LinearLayout box=new LinearLayout(this);box.setOrientation(LinearLayout.VERTICAL);int pad=dp(20);box.setPadding(pad,pad,pad,pad);box.addView(new ProgressBar(this));task.progress=new TextView(this);task.progress.setText("Dosya okunuyor…");box.addView(task.progress);
        task.dialog=new AlertDialog.Builder(this).setTitle("Çizim açılıyor").setView(box).setNegativeButton("İPTAL",(d,w)->cancelLoad()).create();task.dialog.setOnCancelListener(d->cancelLoad());task.dialog.setCanceledOnTouchOutside(false);task.dialog.show();
        task.future=loader.submit(()->{
            Loaded loaded=new Loaded();
            try{
                FileTransfer.checkCancelled();loaded.name=nameOf(uri);loaded.dxf=loaded.name.toLowerCase(Locale.ROOT).endsWith(".dxf");loaded.file=File.createTempFile("MusaCAD_acilan_",loaded.dxf?".dxf":".dwg",getCacheDir());
                try(InputStream in=getContentResolver().openInputStream(uri);OutputStream out=new FileOutputStream(loaded.file)){FileTransfer.copy(in,out,32L*1024*1024,bytes->runOnUiThread(()->{if(activeLoad==task)task.progress.setText(String.format(Locale.getDefault(),"Okunan: %.1f MB",bytes/1048576d));}));}
                runOnUiThread(()->{if(activeLoad==task)task.progress.setText("Çizim hazırlanıyor…");});FileTransfer.checkCancelled();
                if(loaded.dxf){loaded.parsed=DxfParser.render(loaded.file);loaded.workingDxf=loaded.file;}
                else try{NativeDwg.Conversion conversion=NativeDwg.readWithWorkingCopy(loaded.file,getCacheDir());loaded.parsed=conversion.result;loaded.workingDxf=conversion.dxf;}catch(InterruptedIOException cancelled){throw cancelled;}catch(IOException|UnsatisfiedLinkError conversionError){FileTransfer.checkCancelled();loaded.bitmap=DwgPreview.read(loaded.file);if(loaded.bitmap==null)throw new IOException("DWG geometri veya önizleme açılamadı",conversionError);}
                if(loaded.parsed!=null)loaded.bitmap=loaded.parsed.bitmap;FileTransfer.checkCancelled();if(loaded.bitmap==null)throw new IOException(loaded.dxf?"Desteklenen DXF geometrisi bulunamadı":"DWG içinde görüntülenebilir önizleme bulunamadı");
                RecentFileStore.record(getApplicationContext(),uri,loaded.name,loaded.bitmap);
                runOnUiThread(()->{
                    if(activeLoad!=task||isFinishing()||isDestroyed()){loaded.dispose();return;}activeLoad=null;task.dialog.dismiss();releaseCurrentFiles();currentFile=loaded.file;editingBaseDxf=loaded.workingDxf;currentDisplayName=loaded.name;activeDxf=loaded.parsed;hideWelcomePanel();updateShareEnabled(true);updateEditorEnabled(canEdit());updateLayerButtons(activeDxf!=null);
                    if(loaded.parsed!=null)cad.setVectorDrawing(loaded.parsed);else cad.setDrawing(loaded.bitmap);markModeSelected(R.id.panButton);snapToggle.setEnabled(loaded.parsed!=null&&loaded.parsed.snapPoints.length>0);snapToggle.setChecked(true);if(loaded.parsed!=null)cad.setSnapPoints(loaded.parsed.snapPoints);
                    String editable=canEdit()?"  •  düzenlenebilir":"";fileName.setText(loaded.name+(loaded.dxf?"  •  DXF":loaded.parsed!=null?"  •  DWG":"  •  DWG önizleme")+editable);if(tabFileName!=null)tabFileName.setText(loaded.name);
                    if(loaded.parsed!=null){String fallback=(loaded.parsed.fontFallbacks.isEmpty()&&!loaded.parsed.externalShapeFallback)?"":"  •  SHX fallback";result.setText("Hazır  •  "+loaded.parsed.activeLayout+"  •  "+loaded.parsed.entityCount+" nesne  •  "+loaded.parsed.layerCount+" katman  •  "+loaded.parsed.editableSourceCount()+" seçilebilir"+(canEdit()?"  •  düzenleme açık":"")+fallback);}else result.setText("Hazır  •  DWG önizleme modu");
                });
            }catch(Exception|OutOfMemoryError e){loaded.dispose();runOnUiThread(()->{if(activeLoad!=task||isFinishing()||isDestroyed())return;activeLoad=null;task.dialog.dismiss();error(e instanceof Exception?(Exception)e:new IOException("Bu çizim için yeterli bellek yok"));});}
        });
    }

    private void releaseCurrentFiles(){if(activeDxf!=null&&activeDxf.bitmap!=null&&!activeDxf.bitmap.isRecycled())activeDxf.bitmap.recycle();if(editingBaseDxf!=null&&editingBaseDxf!=currentFile)editingBaseDxf.delete();if(currentFile!=null)currentFile.delete();activeDxf=null;editingBaseDxf=null;currentFile=null;}
    @Override protected void onDestroy(){cancelLoad();releaseCurrentFiles();loader.shutdownNow();super.onDestroy();}

    private void showLayers(){
        if(activeDxf==null||activeLoad!=null){if(activeDxf==null)Toast.makeText(this,"Katmanlar için önce bir çizim açın",Toast.LENGTH_SHORT).show();return;}
        String[] names=activeDxf.layerNames.toArray(new String[0]);Set<String> selected=new HashSet<>(activeDxf.visibleLayers);boolean[] checked=new boolean[names.length];for(int i=0;i<names.length;i++)checked[i]=selected.contains(names[i]);
        new AlertDialog.Builder(this).setTitle("Görünecek katmanlar").setMultiChoiceItems(names,checked,(dialog,index,enabled)->{if(enabled)selected.add(names[index]);else selected.remove(names[index]);}).setPositiveButton("UYGULA",(d,w)->applyLayers(selected)).setNeutralButton("TÜMÜNÜ GÖSTER",(d,w)->applyLayers(new HashSet<>(activeDxf.layerNames))).setNegativeButton("İPTAL",null).show();
    }

    private void applyLayers(Set<String> selected){
        if(activeDxf==null||activeLoad!=null||activeDxf.visibleLayers.equals(selected))return;DxfParser.Result source=activeDxf;LoadTask task=new LoadTask();activeLoad=task;task.dialog=new AlertDialog.Builder(this).setTitle("Katmanlar hazırlanıyor").setMessage("Görünüm güncelleniyor…").setNegativeButton("İPTAL",(d,w)->cancelLoad()).create();task.dialog.setOnCancelListener(d->cancelLoad());task.dialog.setCanceledOnTouchOutside(false);task.dialog.show();
        task.future=loader.submit(()->{try{DxfParser.Result updated=source.withVisibleLayers(selected);runOnUiThread(()->{if(activeLoad!=task||activeDxf!=source||isFinishing()||isDestroyed()){if(updated.bitmap!=null&&!updated.bitmap.isRecycled())updated.bitmap.recycle();return;}activeLoad=null;task.dialog.dismiss();cad.replaceVisibleDrawing(updated);activeDxf=updated;if(source.bitmap!=null&&!source.bitmap.isRecycled())source.bitmap.recycle();snapToggle.setEnabled(updated.snapPoints.length>0);result.setText("Hazır  •  "+updated.activeLayout+"  •  "+updated.entityCount+" nesne  •  "+updated.visibleLayers.size()+"/"+updated.layerCount+" katman");});}catch(Exception|OutOfMemoryError e){runOnUiThread(()->{if(activeLoad!=task||isFinishing()||isDestroyed())return;activeLoad=null;task.dialog.dismiss();error(e instanceof Exception?(Exception)e:new IOException("Yeterli bellek yok"));});}});
    }

    private void showLayouts(){
        if(activeDxf==null||activeLoad!=null)return;if(activeDxf.layoutNames.size()<=1){Toast.makeText(this,"Bu çizimde tek layout var: "+activeDxf.activeLayout,Toast.LENGTH_SHORT).show();return;}
        String[] names=activeDxf.layoutNames.toArray(new String[0]);int current=0;for(int i=0;i<names.length;i++)if(names[i].equals(activeDxf.activeLayout)){current=i;break;}final int checked=current;
        new AlertDialog.Builder(this).setTitle("Model / Layout seç").setSingleChoiceItems(names,checked,(dialog,which)->{dialog.dismiss();requestLayoutChange(names[which]);}).setNegativeButton("İPTAL",null).show();
    }

    private void requestLayoutChange(String layout){
        if(activeDxf==null||activeLoad!=null||layout==null||layout.equals(activeDxf.activeLayout))return;
        if(cad.hasEdits()){new AlertDialog.Builder(this).setTitle("Layout değiştirilsin mi?").setMessage("Mevcut düzenlemeler bu layout koordinatlarına bağlı. Layout değiştirilirse kaydedilmemiş düzenlemeler temizlenecek.").setPositiveButton("DEVAM",(d,w)->applyLayout(layout)).setNegativeButton("İPTAL",null).show();return;}
        applyLayout(layout);
    }

    private void applyLayout(String layout){
        if(activeDxf==null||activeLoad!=null)return;DxfParser.Result source=activeDxf;LoadTask task=new LoadTask();activeLoad=task;task.dialog=new AlertDialog.Builder(this).setTitle("Layout hazırlanıyor").setMessage(layout+" açılıyor…").setNegativeButton("İPTAL",(d,w)->cancelLoad()).create();task.dialog.setOnCancelListener(d->cancelLoad());task.dialog.setCanceledOnTouchOutside(false);task.dialog.show();
        task.future=loader.submit(()->{try{DxfParser.Result updated=source.withLayout(layout);runOnUiThread(()->{if(activeLoad!=task||activeDxf!=source||isFinishing()||isDestroyed()){if(updated.bitmap!=null&&!updated.bitmap.isRecycled())updated.bitmap.recycle();return;}activeLoad=null;task.dialog.dismiss();cad.setVectorDrawing(updated);activeDxf=updated;if(source.bitmap!=null&&!source.bitmap.isRecycled())source.bitmap.recycle();markModeSelected(R.id.panButton);snapToggle.setEnabled(updated.snapPoints.length>0);snapToggle.setChecked(true);cad.setSnapPoints(updated.snapPoints);updateEditorEnabled(canEdit());result.setText("Hazır  •  "+updated.activeLayout+"  •  "+updated.entityCount+" nesne  •  "+updated.visibleLayers.size()+"/"+updated.layerCount+" katman  •  "+updated.editableSourceCount()+" seçilebilir");});}catch(Exception|OutOfMemoryError e){runOnUiThread(()->{if(activeLoad!=task||isFinishing()||isDestroyed())return;activeLoad=null;task.dialog.dismiss();error(e instanceof Exception?(Exception)e:new IOException("Layout için yeterli bellek yok"));});}});
    }

    private String nameOf(Uri u){try(android.database.Cursor c=getContentResolver().query(u,null,null,null,null)){if(c!=null&&c.moveToFirst()){int i=c.getColumnIndex(OpenableColumns.DISPLAY_NAME);if(i>=0)return c.getString(i);}}String last=u.getLastPathSegment();return last==null||last.trim().isEmpty()?"cizim.dwg":last;}

    private void showTextEditor(float x,float y){
        EditText input=new EditText(this);input.setHint("Çizime eklenecek yazı");input.setSingleLine(false);input.setMaxLines(3);int p=dp(16);input.setPadding(p,p/2,p,p/2);AlertDialog dialog=new AlertDialog.Builder(this).setTitle("Yazı ekle").setView(input).setPositiveButton("EKLE",null).setNegativeButton("İPTAL",null).create();dialog.setOnShowListener(d->dialog.getButton(AlertDialog.BUTTON_POSITIVE).setOnClickListener(v->{String text=input.getText().toString().trim();if(text.isEmpty()){input.setError("Bir yazı girin");return;}cad.addTextEdit(x,y,text);dialog.dismiss();}));dialog.show();
    }

    private void requestEditedDxfSave(){
        if(activeLoad!=null){Toast.makeText(this,"Devam eden işlem bitmeden kaydedilemez",Toast.LENGTH_SHORT).show();return;}if(!canEdit()){Toast.makeText(this,"Bu çizim DXF olarak düzenlenebilir durumda değil",Toast.LENGTH_SHORT).show();return;}
        Intent intent=new Intent(Intent.ACTION_CREATE_DOCUMENT);intent.addCategory(Intent.CATEGORY_OPENABLE);intent.setType("application/octet-stream");String base=currentDisplayName==null?"cizim":currentDisplayName.replaceFirst("(?i)\\.(dwg|dxf)$","");intent.putExtra(Intent.EXTRA_TITLE,base+"_duzenlendi.dxf");startActivityForResult(intent,SAVE_DXF);
    }

    private void saveEditedDxf(Uri uri){
        if(!canEdit()||activeLoad!=null)return;final File base=editingBaseDxf;final DxfParser.Result drawing=activeDxf;final List<CadEdit> additions=cad.getAddedEdits();final List<SourceReplacement> replacements=cad.getSourceReplacements();final List<SourceRange> removals=cad.getSourceRemovals();final int total=additions.size()+removals.size();
        LoadTask task=new LoadTask();activeLoad=task;LinearLayout box=new LinearLayout(this);box.setOrientation(LinearLayout.VERTICAL);int pad=dp(20);box.setPadding(pad,pad,pad,pad);box.addView(new ProgressBar(this));task.progress=new TextView(this);task.progress.setText("DXF hazırlanıyor…");box.addView(task.progress);task.dialog=new AlertDialog.Builder(this).setTitle("Düzenlenmiş DXF kaydediliyor").setView(box).setNegativeButton("İPTAL",(d,w)->cancelLoad()).create();task.dialog.setOnCancelListener(d->cancelLoad());task.dialog.setCanceledOnTouchOutside(false);task.dialog.show();
        task.future=loader.submit(()->{try(OutputStream out=getContentResolver().openOutputStream(uri,"wt")){if(out==null)throw new IOException("Kaydedilecek dosya açılamadı");DxfWriter.write(base,out,drawing,additions,replacements,removals);FileTransfer.checkCancelled();runOnUiThread(()->{if(activeLoad!=task||isFinishing()||isDestroyed())return;activeLoad=null;task.dialog.dismiss();Toast.makeText(this,"DXF kaydedildi • "+total+" düzenleme",Toast.LENGTH_LONG).show();});}catch(Exception e){runOnUiThread(()->{if(activeLoad!=task||isFinishing()||isDestroyed())return;activeLoad=null;task.dialog.dismiss();error(e);});}});
    }

    private void showShare(){
        if(currentFile==null||!currentFile.exists()){Toast.makeText(this,"Paylaşmak için önce bir DWG veya DXF dosyası açın",Toast.LENGTH_SHORT).show();return;}
        new AlertDialog.Builder(this).setTitle("Paylaş").setItems(new String[]{"Orijinal dosyayı paylaş","Görünümü PDF olarak paylaş","Görünümü resim olarak paylaş","Alan seçerek paylaş"},(d,w)->{if(w==0)shareOriginalFile();else if(w==3){if(!cad.beginSelection())Toast.makeText(this,"Önce çizim açın",Toast.LENGTH_SHORT).show();}else {cad.cancelSelection();exportView(w==1);}}).show();
    }

    private void shareOriginalFile(){
        if(currentFile==null||!currentFile.exists()){Toast.makeText(this,"Paylaşmak için önce dosya açın",Toast.LENGTH_SHORT).show();return;}
        File copy=null;
        try{
            String lower=currentDisplayName==null?"":currentDisplayName.toLowerCase(Locale.ROOT);
            String suffix=lower.endsWith(".dxf")?".dxf":".dwg";
            copy=File.createTempFile("MusaCAD_orijinal_",suffix,exportDir());
            try(InputStream in=new FileInputStream(currentFile);OutputStream out=new FileOutputStream(copy)){
                byte[] buffer=new byte[64*1024];int n;while((n=in.read(buffer))!=-1)out.write(buffer,0,n);
            }
            shareFile(copy,"application/octet-stream");
        }catch(Exception e){if(copy!=null)copy.delete();error(e);}
    }

    private void printDrawing(){
        if(currentFile==null||!currentFile.exists()){Toast.makeText(this,"Yazdırmak için önce bir DWG veya DXF dosyası açın",Toast.LENGTH_SHORT).show();return;}Bitmap preview=null;
        try{if(activeDxf==null){preview=DwgPreview.read(currentFile);if(preview==null)throw new IOException("DWG önizlemesi yazdırma için hazırlanamadı");}CadPrint.show(this,activeDxf,cad.getAddedEdits(),cad.getSourceReplacements(),cad.getHiddenSourceIds(),preview,currentDisplayName);}catch(Exception e){if(preview!=null&&!preview.isRecycled())preview.recycle();error(e);}
    }

    private void previewSelection(){
        final Bitmap bitmap;try{bitmap=cad.selectionSnapshot();}catch(Exception e){error(e);return;}ImageView preview=new ImageView(this);preview.setImageBitmap(bitmap);preview.setAdjustViewBounds(true);preview.setMaxHeight((int)(getResources().getDisplayMetrics().heightPixels*.55f));preview.setScaleType(ImageView.ScaleType.FIT_CENTER);int pad=dp(12);preview.setPadding(pad,pad,pad,pad);
        AlertDialog dialog=new AlertDialog.Builder(this).setTitle("Seçili alan önizlemesi").setView(preview).setPositiveButton("PNG PAYLAŞ",(d,w)->exportBitmap(bitmap,false,"alan")).setNeutralButton("PDF PAYLAŞ",(d,w)->exportBitmap(bitmap,true,"alan")).setNegativeButton("YENİDEN SEÇ",(d,w)->cad.beginSelection()).create();dialog.setOnCancelListener(d->cad.cancelSelection());dialog.setOnDismissListener(d->{preview.setImageDrawable(null);bitmap.recycle();});dialog.show();
    }

    private void showCalibration(){
        LinearLayout box=new LinearLayout(this);box.setOrientation(LinearLayout.VERTICAL);int p=dp(16);box.setPadding(p,0,p,0);EditText value=new EditText(this);value.setHint("Gerçek uzunluk (ör. 2.50)");value.setInputType(android.text.InputType.TYPE_CLASS_NUMBER|android.text.InputType.TYPE_NUMBER_FLAG_DECIMAL);box.addView(value);Spinner units=new Spinner(this);units.setAdapter(new ArrayAdapter<String>(this,android.R.layout.simple_spinner_dropdown_item,new String[]{"m","cm","mm"}));box.addView(units);
        AlertDialog dialog=new AlertDialog.Builder(this).setTitle("Ölçeği ayarla").setView(box).setPositiveButton("UYGULA",null).setNegativeButton("İPTAL",(d,w)->cad.clearMeasurement()).create();dialog.setOnCancelListener(d->cad.clearMeasurement());dialog.setOnShowListener(d->dialog.getButton(AlertDialog.BUTTON_POSITIVE).setOnClickListener(v->{try{double n=Double.parseDouble(value.getText().toString().replace(',','.'));cad.setCalibration(n,units.getSelectedItem().toString());markModeSelected(R.id.distanceButton);dialog.dismiss();}catch(Exception e){value.setError("Sıfırdan büyük bir uzunluk girin; iki farklı nokta seçin.");}}));dialog.show();
    }

    private File exportDir(){File d=new File(getCacheDir(),"exports");d.mkdirs();return d;}
    private void exportView(boolean pdf){if(pdf){exportViewPdf();return;}Bitmap bitmap=null;try{bitmap=cad.snapshot();exportBitmap(bitmap,false,"gorunum");}catch(Exception e){error(e);}finally{if(bitmap!=null)bitmap.recycle();}}
    private void exportViewPdf(){
        if(cad.getWidth()<=0||cad.getHeight()<=0){error(new IOException("PDF görünümü hazırlanamadı"));return;}File file=null;PdfDocument document=new PdfDocument();
        try{file=File.createTempFile("MusaCAD_gorunum_",".pdf",exportDir());PdfDocument.PageInfo info=new PdfDocument.PageInfo.Builder(cad.getWidth(),cad.getHeight(),1).create();PdfDocument.Page page=document.startPage(info);cad.draw(page.getCanvas());document.finishPage(page);try(OutputStream out=new FileOutputStream(file)){document.writeTo(out);}shareFile(file,"application/pdf");}catch(Exception e){if(file!=null)file.delete();error(e);}finally{document.close();}
    }
    private void exportBitmap(Bitmap bitmap,boolean pdf,String suffix){
        try{File file=File.createTempFile("MusaCAD_"+suffix+"_",pdf?".pdf":".png",exportDir());if(pdf){PdfDocument document=new PdfDocument();try{PdfDocument.Page page=document.startPage(new PdfDocument.PageInfo.Builder(bitmap.getWidth(),bitmap.getHeight(),1).create());page.getCanvas().drawBitmap(bitmap,0,0,null);document.finishPage(page);try(OutputStream out=new FileOutputStream(file)){document.writeTo(out);}}finally{document.close();}}else try(OutputStream out=new FileOutputStream(file)){if(!bitmap.compress(Bitmap.CompressFormat.PNG,100,out))throw new IOException("Resim oluşturulamadı");}cad.cancelSelection();shareFile(file,pdf?"application/pdf":"image/png");}catch(Exception e){error(e);}
    }
    private void shareFile(File f,String mime){if(f==null||!f.exists()){Toast.makeText(this,"Önce dosya açın",Toast.LENGTH_SHORT).show();return;}Uri u=FileProvider.getUriForFile(this,getPackageName()+".files",f);Intent s=new Intent(Intent.ACTION_SEND);s.setType(mime);s.putExtra(Intent.EXTRA_STREAM,u);s.setClipData(ClipData.newRawUri("MusaCAD",u));s.addFlags(Intent.FLAG_GRANT_READ_URI_PERMISSION);startActivityForResult(Intent.createChooser(s,"Paylaş"),0);}
    private void error(Exception e){Toast.makeText(this,"İşlem başarısız: "+e.getMessage(),Toast.LENGTH_LONG).show();}
}
