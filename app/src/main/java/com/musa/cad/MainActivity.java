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
import com.google.android.material.bottomsheet.BottomSheetDialog;
import java.io.*;
import java.util.*;
import java.util.concurrent.*;

public class MainActivity extends AppCompatActivity {
    private static final int OPEN=20,SAVE_DXF=21,PICK_AUDIO=30,PICK_IMAGE=31,PICK_VIDEO=32;
    private static final int MAX_OPEN_PROJECTS=4;
    private static final int MENU_OPEN=1,MENU_LAYERS=2,MENU_FIT=3,MENU_SHARE=4,MENU_INFO=5,MENU_ABOUT=6,MENU_SAVE_DXF=7,MENU_PRINT=8,MENU_LAYOUTS=9,MENU_NEW_PROJECT=10;
    private final ExecutorService loader=Executors.newSingleThreadExecutor();
    private LoadTask activeLoad;

    private static final class LoadTask {Future<?> future;AlertDialog dialog;TextView progress;}
    private static final class ToolAction {
        final String label;final int icon;final Runnable action;
        ToolAction(String label,int icon,Runnable action){this.label=label;this.icon=icon;this.action=action;}
    }
    private static final class MediaAttachment {
        final String kind,name,mime,uri;
        MediaAttachment(String kind,String name,String mime,Uri uri){this.kind=kind;this.name=name;this.mime=mime;this.uri=uri.toString();}
    }
    private static final class Loaded {
        Uri sourceUri;File file,workingDxf;Bitmap bitmap;DxfParser.Result parsed;NativeScene nativeScene;String name;boolean dxf,handedOff;
        ProjectSession project;
        void dispose(){if(bitmap!=null&&!bitmap.isRecycled())bitmap.recycle();if(workingDxf!=null&&workingDxf!=file)workingDxf.delete();if(file!=null)file.delete();}
    }

    private static final class ProjectSession {
        Uri sourceUri;File file,workingDxf;Bitmap bitmap;DxfParser.Result parsed;NativeScene nativeScene;String name;boolean dxf;
        CadView.SessionState viewState;CadView.ViewBookmark viewBookmark;long savedFingerprint;boolean baselineSet,dirty,preparingEditor;String prepareError;long lastAccessMs;LoadTask prepareTask;
        final Set<String> previousVisibleLayers=new HashSet<>();
        final ArrayDeque<String> measurementHistory=new ArrayDeque<>();
        final ArrayList<MediaAttachment> mediaAttachments=new ArrayList<>();
        String defaultLayer="0";
        void dispose(){
            LoadTask pending=prepareTask;prepareTask=null;if(pending!=null&&pending.future!=null)pending.future.cancel(true);
            Bitmap owned=parsed!=null?parsed.bitmap:bitmap;
            if(owned!=null&&!owned.isRecycled())owned.recycle();
            if(workingDxf!=null&&workingDxf!=file)workingDxf.delete();
            if(file!=null)file.delete();
            file=null;workingDxf=null;bitmap=null;parsed=null;viewState=null;
        }
    }

    private CheckBox snapToggle;
    private DxfParser.Result activeDxf;
    private CadView cad;
    private TextView fileName,result,editStatusText,tabFileName;
    private LinearLayout projectTabsBox;
    private EditText commandInput;
    private File currentFile,editingBaseDxf;
    private String currentDisplayName="cizim.dwg";
    private View[] modeButtons;
    private int[] categoryButtons;
    private View welcomePanel,shareButton,shareToolButton,toolPanelHost;
    private TextView toolPanelTitle;
    private GridLayout toolPanelGrid;
    private HorizontalScrollView categoryScroll;
    private final ArrayList<ProjectSession> projects=new ArrayList<>();
    private ProjectSession currentProject,pendingCloseAfterSave;
    private CadEdit crossProjectClipboard;
    private String crossProjectClipboardSource="";
    private String lastCommandRaw="";
    private int pendingHomeCategory;

    @Override protected void onCreate(Bundle b){
        super.onCreate(b);WindowCompat.setDecorFitsSystemWindows(getWindow(),false);setContentView(R.layout.activity_main);
        getOnBackPressedDispatcher().addCallback(this,new androidx.activity.OnBackPressedCallback(true){
            @Override public void handleOnBackPressed(){handleBackNavigation();}
        });
        View root=findViewById(R.id.mainRoot);
        ViewCompat.setOnApplyWindowInsetsListener(root,(v,insets)->{Insets bars=insets.getInsets(WindowInsetsCompat.Type.systemBars());v.setPadding(0,bars.top,0,bars.bottom+dp(6));return insets;});

        cad=findViewById(R.id.cadView);fileName=findViewById(R.id.fileName);result=findViewById(R.id.resultText);welcomePanel=findViewById(R.id.welcomePanel);
        editStatusText=findViewById(R.id.editStatusText);tabFileName=findViewById(R.id.tabFileName);projectTabsBox=findViewById(R.id.projectTabsBox);commandInput=findViewById(R.id.commandInput);
        shareButton=findViewById(R.id.shareButton);shareToolButton=findViewById(R.id.shareToolButton);
        toolPanelHost=findViewById(R.id.toolPanelHost);toolPanelTitle=findViewById(R.id.toolPanelTitle);toolPanelGrid=findViewById(R.id.toolPanelGrid);categoryScroll=findViewById(R.id.categoryScroll);
        findViewById(R.id.toolPanelClose).setOnClickListener(v->hideToolPanel());
        cad.setListener(new CadView.Listener(){
            public void onMeasurement(String v){result.setText(v);recordMeasurement(v);}
            public void onCalibrationRequested(double px){showCalibration();}
            public void onSelectionReady(){previewSelection();}
            public void onTextRequested(float x,float y){showTextEditor(x,y);}
        });

        snapToggle=findViewById(R.id.snapToggle);snapToggle.setOnCheckedChangeListener((button,checked)->cad.setSnapEnabled(checked));
        modeButtons=new View[]{findViewById(R.id.panButton),findViewById(R.id.selectEntityButton),findViewById(R.id.calibrateButton),findViewById(R.id.distanceButton),findViewById(R.id.areaButton),findViewById(R.id.lineButton),findViewById(R.id.polylineButton),findViewById(R.id.rectangleButton),findViewById(R.id.circleButton),findViewById(R.id.pointButton),findViewById(R.id.textButton)};
        markModeSelected(R.id.panButton);
        categoryButtons=new int[]{R.id.groupAnnotateToolsButton,R.id.groupLineToolsButton,R.id.groupEditToolsButton,R.id.groupLayerToolsButton,R.id.groupMeasureToolsButton,R.id.groupDimensionToolsButton,R.id.groupColorToolsButton,R.id.groupMoreToolsButton,R.id.groupLayoutToolsButton,R.id.groupViewToolsButton};

        int[] interactive={R.id.menuButton,R.id.openButton,R.id.shareButton,R.id.headerMoreButton,R.id.quickOpenButton,R.id.newProjectButton,R.id.layersButton,R.id.propertiesButton,R.id.colorButton,R.id.lineTypeButton,R.id.pointButton,R.id.bottomLayersButton,R.id.rightLayersButton,R.id.snapToggle,R.id.panButton,R.id.selectEntityButton,R.id.moveEntityButton,R.id.rotateEntityButton,R.id.copyEntityButton,R.id.deleteEntityButton,R.id.calibrateButton,R.id.distanceButton,R.id.bottomMeasureButton,R.id.hatchButton,R.id.moreToolsButton,R.id.areaButton,R.id.lineButton,R.id.polylineButton,R.id.rectangleButton,R.id.circleButton,R.id.textButton,R.id.finishEditButton,R.id.saveDxfButton,R.id.zoomInButton,R.id.zoomOutButton,R.id.rightZoomInButton,R.id.rightZoomOutButton,R.id.fitButton,R.id.rightFitButton,R.id.undoButton,R.id.clearButton,R.id.shareToolButton,R.id.commandSendButton,R.id.toolPanelClose,R.id.closeFileButton,R.id.nativeModeChip,R.id.sceneModeChip,R.id.groupLayerToolsButton,R.id.groupDimensionToolsButton,R.id.groupColorToolsButton,R.id.groupLayoutToolsButton,R.id.groupLineToolsButton,R.id.groupShapeToolsButton,R.id.groupEditToolsButton,R.id.groupMeasureToolsButton,R.id.groupViewToolsButton,R.id.groupAnnotateToolsButton,R.id.groupMoreToolsButton};
        for(int id:interactive)installInteractiveFeedback(findViewById(id));

        findViewById(R.id.menuButton).setOnClickListener(v->{hideToolPanel();showMainMenu(v);});findViewById(R.id.headerMoreButton).setOnClickListener(v->{hideToolPanel();showMainMenu(v);});findViewById(R.id.appTitle).setOnClickListener(v->{hideToolPanel();showMainMenu(v);});
        findViewById(R.id.closeFileButton).setOnClickListener(v->{v.performHapticFeedback(HapticFeedbackConstants.KEYBOARD_TAP);if(currentProject==null)Toast.makeText(this,"Açık proje yok",Toast.LENGTH_SHORT).show();else requestCloseProject(currentProject);});
        findViewById(R.id.nativeModeChip).setOnClickListener(v->{v.performHapticFeedback(HapticFeedbackConstants.KEYBOARD_TAP);if(currentProject==null)result.setText("DWG Native • Önce çizim açın");else if(currentProject.nativeScene!=null)result.setText("DWG Native • hızlı sahne etkin");else result.setText("Vektör görünüm • tam çizim modeli");});
        findViewById(R.id.sceneModeChip).setOnClickListener(v->{v.performHapticFeedback(HapticFeedbackConstants.KEYBOARD_TAP);showViewToolsSheet();});
        findViewById(R.id.layersButton).setOnClickListener(v->showLayers());
        findViewById(R.id.propertiesButton).setOnClickListener(v->showSelectedProperties());
        findViewById(R.id.colorButton).setOnClickListener(v->showSelectedColor());
        findViewById(R.id.lineTypeButton).setOnClickListener(v->showSelectedLineType());
        findViewById(R.id.pointButton).setOnClickListener(v->selectEditMode(R.id.pointButton,CadView.Mode.DRAW_POINT));
        findViewById(R.id.bottomLayersButton).setOnClickListener(v->showLayers());
        findViewById(R.id.rightLayersButton).setOnClickListener(v->showLayers());
        findViewById(R.id.bottomMeasureButton).setOnClickListener(v->showMeasureTools());
        findViewById(R.id.hatchButton).setOnClickListener(v->runHatchCommand());
        findViewById(R.id.moreToolsButton).setOnClickListener(v->showMoreTools());
        findViewById(R.id.groupLineToolsButton).setOnClickListener(v->openCategory(R.id.groupLineToolsButton,this::showLineToolsSheet));
        findViewById(R.id.groupShapeToolsButton).setOnClickListener(v->showShapeToolsSheet());
        findViewById(R.id.groupEditToolsButton).setOnClickListener(v->openCategory(R.id.groupEditToolsButton,this::showEditToolsSheet));
        findViewById(R.id.groupLayerToolsButton).setOnClickListener(v->openCategory(R.id.groupLayerToolsButton,this::showLayerToolsPanel));
        findViewById(R.id.groupMeasureToolsButton).setOnClickListener(v->openCategory(R.id.groupMeasureToolsButton,this::showMeasureToolsSheet));
        findViewById(R.id.groupDimensionToolsButton).setOnClickListener(v->openCategory(R.id.groupDimensionToolsButton,this::showDimensionToolsPanel));
        findViewById(R.id.groupColorToolsButton).setOnClickListener(v->openCategory(R.id.groupColorToolsButton,this::showColorToolsPanel));
        findViewById(R.id.groupMoreToolsButton).setOnClickListener(v->openCategory(R.id.groupMoreToolsButton,this::showOtherToolsSheet));
        findViewById(R.id.groupLayoutToolsButton).setOnClickListener(v->openCategory(R.id.groupLayoutToolsButton,this::showLayoutToolsPanel));
        findViewById(R.id.groupViewToolsButton).setOnClickListener(v->openCategory(R.id.groupViewToolsButton,this::showViewToolsSheet));
        findViewById(R.id.groupAnnotateToolsButton).setOnClickListener(v->openCategory(R.id.groupAnnotateToolsButton,this::showAnnotationToolsSheet));
        findViewById(R.id.openButton).setOnClickListener(v->open());findViewById(R.id.quickOpenButton).setOnClickListener(v->open());findViewById(R.id.newProjectButton).setOnClickListener(v->showNewProjectSheet());
        tabFileName.setOnLongClickListener(v->{if(currentProject!=null)requestCloseProject(currentProject);return true;});
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
        int[] homeInteractive={R.id.homeOpenButton,R.id.homeNewButton,R.id.homeRecentButton,R.id.homeImportButton,R.id.homeRecentCardButton,R.id.homeLicenseButton};
        for(int id:homeInteractive)installInteractiveFeedback(findViewById(id));
        findViewById(R.id.homeOpenButton).setOnClickListener(v->open());
        findViewById(R.id.homeNewButton).setOnClickListener(v->createBlankDrawing());
        findViewById(R.id.homeRecentButton).setOnClickListener(v->open());
        findViewById(R.id.homeImportButton).setOnClickListener(v->open(true));
        findViewById(R.id.homeRecentCardButton).setOnClickListener(v->open());
        findViewById(R.id.homeLicenseButton).setOnClickListener(v->showLicense());
        findViewById(R.id.homeAnnotateCategory).setOnClickListener(v->openHomeCategory(R.id.groupAnnotateToolsButton));
        findViewById(R.id.homeDrawCategory).setOnClickListener(v->openHomeCategory(R.id.groupLineToolsButton));
        findViewById(R.id.homeEditCategory).setOnClickListener(v->openHomeCategory(R.id.groupEditToolsButton));
        findViewById(R.id.homeLayerCategory).setOnClickListener(v->openHomeCategory(R.id.groupLayerToolsButton));
        findViewById(R.id.homeMeasureCategory).setOnClickListener(v->openHomeCategory(R.id.groupMeasureToolsButton));
        findViewById(R.id.homeDimensionCategory).setOnClickListener(v->openHomeCategory(R.id.groupDimensionToolsButton));
        findViewById(R.id.homeColorCategory).setOnClickListener(v->openHomeCategory(R.id.groupColorToolsButton));
        findViewById(R.id.homeOtherCategory).setOnClickListener(v->openHomeCategory(R.id.groupMoreToolsButton));
        findViewById(R.id.homeLayoutCategory).setOnClickListener(v->openHomeCategory(R.id.groupLayoutToolsButton));
        findViewById(R.id.homeViewCategory).setOnClickListener(v->openHomeCategory(R.id.groupViewToolsButton));
        updateShareEnabled(false);updateEditorEnabled(false);showHomeUi();handleIncomingIntent(getIntent());
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
            case HATCH:
                runHatchCommand();
                break;
            case STRETCH:
                if(!ensureTransformSelection("STRETCH"))break;
                if(cad.armStretchSelected())result.setText("STRETCH • Taşınacak köşe/vertex noktasına dokunun");
                else result.setText("STRETCH • LINE, açık/kapalı POLYLINE veya RECTANGLE seçin");
                break;
            case BLOCK:
                runBlockCommand();
                break;
            case INSERT:
                runInsertCommand();
                break;
            case DIMSTYLE:
                runDimStyleCommand();
                break;
            case DIMLINEAR:
                if(!canEdit()){Toast.makeText(this,"Bu çizim düzenleme için vektörel olarak açılamadı",Toast.LENGTH_SHORT).show();break;}
                cad.setMode(CadView.Mode.DRAW_DIM_LINEAR);markModeSelected(0);result.setText("DIMLINEAR • İki ölçü noktası ve ölçü çizgisi konumu seçin");
                break;
            case DIMALIGNED:
                if(!canEdit()){Toast.makeText(this,"Bu çizim düzenleme için vektörel olarak açılamadı",Toast.LENGTH_SHORT).show();break;}
                cad.setMode(CadView.Mode.DRAW_DIM_ALIGNED);markModeSelected(0);result.setText("DIMALIGNED • İki ölçü noktası ve ölçü çizgisi konumu seçin");
                break;
            case LAYER:
                showLayers();
                break;
            case PROPERTIES:
                showSelectedProperties();
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

    private boolean ensureSelectedForQuickTool(String tool){
        if(!canEdit()){Toast.makeText(this,"Bu çizim düzenleme için vektörel olarak açılamadı",Toast.LENGTH_SHORT).show();return false;}
        if(cad.hasSelectedEntity())return true;
        selectEditMode(R.id.selectEntityButton,CadView.Mode.SELECT_ENTITY);
        result.setText(tool+" • Önce nesne seçin, sonra "+tool+" düğmesine tekrar basın");
        return false;
    }

    private void showSelectedProperties(){
        if(!ensureSelectedForQuickTool("Özellik"))return;
        LinearLayout box=new LinearLayout(this);box.setOrientation(LinearLayout.VERTICAL);int p=dp(16);box.setPadding(p,p/2,p,p/2);
        TextView info=new TextView(this);info.setText(cad.selectedEntityInfo());info.setTextIsSelectable(true);box.addView(info);

        TextView layerLabel=new TextView(this);layerLabel.setText("Katman");layerLabel.setPadding(0,p/2,0,0);box.addView(layerLabel);
        Spinner layer=new Spinner(this);String[] layers=activeDxf.layerNames.toArray(new String[0]);layer.setAdapter(new ArrayAdapter<>(this,android.R.layout.simple_spinner_dropdown_item,layers));
        String currentLayer=cad.selectedLayer();for(int i=0;i<layers.length;i++)if(layers[i].equals(currentLayer)){layer.setSelection(i);break;}box.addView(layer);

        EditText scale=new EditText(this);scale.setSingleLine(true);scale.setHint("Çizgi tipi ölçeği");scale.setText(String.format(Locale.US,"%.3f",cad.selectedLineTypeScale()));scale.setInputType(android.text.InputType.TYPE_CLASS_NUMBER|android.text.InputType.TYPE_NUMBER_FLAG_DECIMAL);box.addView(scale);
        EditText weight=new EditText(this);weight.setSingleLine(true);weight.setHint("Çizgi kalınlığı (DXF 1/100 mm; örn. 25)");weight.setText(Integer.toString(cad.selectedLineWeight()));weight.setInputType(android.text.InputType.TYPE_CLASS_NUMBER|android.text.InputType.TYPE_NUMBER_FLAG_SIGNED);box.addView(weight);

        AlertDialog dialog=new AlertDialog.Builder(this).setTitle("Özellik • Seçili nesne").setView(box).setPositiveButton("UYGULA",null).setNegativeButton("İPTAL",null).create();
        dialog.setOnShowListener(d->dialog.getButton(AlertDialog.BUTTON_POSITIVE).setOnClickListener(v->{
            try{
                double ls=Double.parseDouble(scale.getText().toString().trim().replace(',','.'));
                int lw=Integer.parseInt(weight.getText().toString().trim());
                String selectedLayer=(String)layer.getSelectedItem();
                if(!Double.isFinite(ls)||ls<=0d){scale.setError("Sıfırdan büyük bir ölçek girin");return;}
                if(!cad.updateSelectedStyle(selectedLayer,null,null,ls,lw)){result.setText("Özellik • Değişiklik uygulanamadı");dialog.dismiss();return;}
                result.setText("Özellik • Katman / çizgi ölçeği / kalınlık güncellendi");dialog.dismiss();
            }catch(Exception e){scale.setError("Geçerli ölçek ve kalınlık değerleri girin");}
        }));
        dialog.show();
    }

    private void showSelectedColor(){
        if(!ensureSelectedForQuickTool("Renk"))return;
        final String[] labels={"Kırmızı","Sarı","Yeşil","Camgöbeği","Mavi","Mor","Beyaz","Özel RGB…"};
        final int[] colors={Color.RED,Color.YELLOW,Color.GREEN,Color.CYAN,Color.BLUE,Color.MAGENTA,Color.WHITE};
        new AlertDialog.Builder(this).setTitle(String.format(Locale.US,"Renk • Mevcut #%06X",cad.selectedColor()&0xFFFFFF))
            .setItems(labels,(d,which)->{
                if(which<colors.length){
                    if(cad.updateSelectedStyle(null,colors[which],null,null,null))result.setText(String.format(Locale.US,"Renk • #%06X uygulandı",colors[which]&0xFFFFFF));
                    return;
                }
                EditText input=new EditText(this);input.setSingleLine(true);input.setHint("#RRGGBB");input.setText(String.format(Locale.US,"#%06X",cad.selectedColor()&0xFFFFFF));input.setSelectAllOnFocus(true);
                AlertDialog custom=new AlertDialog.Builder(this).setTitle("Özel RGB renk").setView(input).setPositiveButton("UYGULA",null).setNegativeButton("İPTAL",null).create();
                custom.setOnShowListener(x->custom.getButton(AlertDialog.BUTTON_POSITIVE).setOnClickListener(v->{
                    try{
                        String raw=input.getText().toString().trim();if(!raw.startsWith("#"))raw="#"+raw;
                        int color=Color.parseColor(raw);if(!cad.updateSelectedStyle(null,color,null,null,null)){custom.dismiss();result.setText("Renk • Değişiklik uygulanamadı");return;}
                        custom.dismiss();result.setText(String.format(Locale.US,"Renk • #%06X uygulandı",color&0xFFFFFF));
                    }catch(Exception e){input.setError("#RRGGBB biçiminde renk girin");}
                }));custom.show();
            }).setNegativeButton("İPTAL",null).show();
    }

    private void showSelectedLineType(){
        if(!ensureSelectedForQuickTool("Çizgi Tipi"))return;
        ArrayList<String> names=new ArrayList<>(activeDxf.lineTypeNames());if(names.isEmpty())names.add("CONTINUOUS");String current=cad.selectedLineType();boolean hasCurrent=false;for(String n:names)if(n.equalsIgnoreCase(current)){hasCurrent=true;break;}if(!hasCurrent&&current!=null&&!current.trim().isEmpty())names.add(0,current);
        LinearLayout box=new LinearLayout(this);box.setOrientation(LinearLayout.VERTICAL);int p=dp(16);box.setPadding(p,p/2,p,p/2);
        Spinner spinner=new Spinner(this);spinner.setAdapter(new ArrayAdapter<>(this,android.R.layout.simple_spinner_dropdown_item,names));
        for(int i=0;i<names.size();i++)if(names.get(i).equalsIgnoreCase(current)){spinner.setSelection(i);break;}box.addView(spinner);
        EditText scale=new EditText(this);scale.setSingleLine(true);scale.setHint("Çizgi tipi ölçeği");scale.setText(String.format(Locale.US,"%.3f",cad.selectedLineTypeScale()));scale.setInputType(android.text.InputType.TYPE_CLASS_NUMBER|android.text.InputType.TYPE_NUMBER_FLAG_DECIMAL);box.addView(scale);
        AlertDialog dialog=new AlertDialog.Builder(this).setTitle("Çizgi Tipi").setView(box).setPositiveButton("UYGULA",null).setNegativeButton("İPTAL",null).create();
        dialog.setOnShowListener(d->dialog.getButton(AlertDialog.BUTTON_POSITIVE).setOnClickListener(v->{
            try{
                double ls=Double.parseDouble(scale.getText().toString().trim().replace(',','.'));if(!Double.isFinite(ls)||ls<=0d){scale.setError("Sıfırdan büyük bir ölçek girin");return;}
                String name=(String)spinner.getSelectedItem();if(!cad.updateSelectedStyle(null,null,name,ls,null)){dialog.dismiss();result.setText("Çizgi Tipi • Değişiklik uygulanamadı");return;}
                dialog.dismiss();result.setText("Çizgi Tipi • "+name+" uygulandı");
            }catch(Exception e){scale.setError("Geçerli ölçek değeri girin");}
        }));dialog.show();
    }

    private ToolAction tool(String label,int icon,Runnable action){return new ToolAction(label,icon,action);}

    private void hideToolPanel(){
        if(toolPanelHost!=null)toolPanelHost.setVisibility(View.GONE);
    }

    private void showToolPanel(String title,ToolAction...tools){
        if(toolPanelHost==null||toolPanelGrid==null||toolPanelTitle==null)return;
        toolPanelTitle.setText(title);
        toolPanelGrid.removeAllViews();
        for(ToolAction item:tools){
            Button b=new Button(this);
            b.setText(item.label);
            b.setTextColor(0xFFF1F7FA);
            b.setTextSize(8.2f);
            b.setAllCaps(false);
            b.setTypeface(android.graphics.Typeface.create("sans-serif-medium",android.graphics.Typeface.NORMAL));
            b.setGravity(Gravity.CENTER);
            b.setCompoundDrawablesWithIntrinsicBounds(0,item.icon,0,0);
            b.setCompoundDrawablePadding(dp(3));
            b.setBackgroundResource(R.drawable.tool_popup_tile_bg);
            b.setPadding(dp(2),dp(5),dp(2),dp(4));
            b.setMinWidth(0);b.setMinHeight(0);b.setSingleLine(false);b.setMaxLines(2);
            boolean enabled=item.action!=null;b.setEnabled(enabled);b.setAlpha(enabled?1f:.38f);
            GridLayout.LayoutParams lp=new GridLayout.LayoutParams();
            lp.width=0;lp.height=dp(66);lp.columnSpec=GridLayout.spec(GridLayout.UNDEFINED,1f);
            lp.setMargins(dp(2),dp(2),dp(2),dp(2));
            toolPanelGrid.addView(b,lp);
            if(enabled)b.setOnClickListener(v->{hideToolPanel();item.action.run();});
            installInteractiveFeedback(b);
        }
        toolPanelHost.setVisibility(View.VISIBLE);
        toolPanelHost.bringToFront();
    }

    private void showToolSheet(String title,ToolAction...tools){
        BottomSheetDialog sheet=new BottomSheetDialog(this);
        LinearLayout root=new LinearLayout(this);root.setOrientation(LinearLayout.VERTICAL);int pad=dp(12);root.setPadding(pad,pad,pad,dp(18));root.setBackgroundColor(0xFF071A27);
        TextView heading=new TextView(this);heading.setText(title);heading.setTextColor(Color.WHITE);heading.setTextSize(16);heading.setTypeface(android.graphics.Typeface.DEFAULT_BOLD);heading.setPadding(dp(4),dp(2),dp(4),dp(10));root.addView(heading,new LinearLayout.LayoutParams(LinearLayout.LayoutParams.MATCH_PARENT,LinearLayout.LayoutParams.WRAP_CONTENT));
        ScrollView scroll=new ScrollView(this);GridLayout grid=new GridLayout(this);grid.setColumnCount(4);grid.setAlignmentMode(GridLayout.ALIGN_BOUNDS);grid.setUseDefaultMargins(false);
        for(ToolAction item:tools){
            Button b=new Button(this);b.setText(item.label);b.setTextColor(0xFFF1F7FA);b.setTextSize(11);b.setAllCaps(false);b.setGravity(Gravity.CENTER);b.setCompoundDrawablesWithIntrinsicBounds(0,item.icon,0,0);b.setCompoundDrawablePadding(dp(5));b.setBackgroundResource(R.drawable.tool_tile_blue);b.setPadding(dp(3),dp(7),dp(3),dp(6));b.setMinWidth(0);b.setMinHeight(0);b.setSingleLine(false);b.setMaxLines(2);
            GridLayout.LayoutParams lp=new GridLayout.LayoutParams();lp.width=0;lp.height=dp(78);lp.columnSpec=GridLayout.spec(GridLayout.UNDEFINED,1f);lp.setMargins(dp(2),dp(2),dp(2),dp(2));grid.addView(b,lp);
            b.setOnClickListener(v->{sheet.dismiss();if(item.action!=null)item.action.run();});
            installInteractiveFeedback(b);
        }
        scroll.addView(grid,new ScrollView.LayoutParams(ScrollView.LayoutParams.MATCH_PARENT,ScrollView.LayoutParams.WRAP_CONTENT));root.addView(scroll,new LinearLayout.LayoutParams(LinearLayout.LayoutParams.MATCH_PARENT,0,1f));
        sheet.setContentView(root);sheet.setOnShowListener(d->{View bottom=sheet.findViewById(com.google.android.material.R.id.design_bottom_sheet);if(bottom!=null){bottom.getLayoutParams().height=Math.min(dp(430),getResources().getDisplayMetrics().heightPixels*2/3);bottom.requestLayout();}});
        sheet.show();
    }

    private void showLineToolsSheet(){
        showToolPanel("Çiz",
            tool("Polyline",R.drawable.ic_polyline,()->selectEditMode(R.id.polylineButton,CadView.Mode.DRAW_POLYLINE)),
            tool("Eskiz",R.drawable.ic_line,()->{if(canEdit()){cad.setMode(CadView.Mode.FREEHAND);markModeSelected(0);result.setText("Eskiz • Parmağınız veya kaleminizle serbest çizin");}}),
            tool("Daire",R.drawable.ic_circle,()->selectEditMode(R.id.circleButton,CadView.Mode.DRAW_CIRCLE)),
            tool("Yay",R.drawable.ic_line,()->{if(canEdit()){cad.setMode(CadView.Mode.DRAW_ARC);markModeSelected(0);result.setText("Yay • 3 nokta seçin");}}),
            tool("Dikdörtgen",R.drawable.ic_rectangle,()->selectEditMode(R.id.rectangleButton,CadView.Mode.DRAW_RECTANGLE)),
            tool("Elips",R.drawable.ic_circle,()->{if(canEdit()){cad.setMode(CadView.Mode.DRAW_ELLIPSE);markModeSelected(0);result.setText("Elips • Merkez ve eksenleri seçin");}}),
            tool("Akıllı Kalem",R.drawable.ic_line,()->{if(canEdit()){cad.setMode(CadView.Mode.FREEHAND);markModeSelected(0);result.setText("Akıllı Kalem • Basınca duyarlı serbest çizim etkin");}}),
            tool("Multileader",R.drawable.ic_text,()->{if(canEdit()){cad.setMode(CadView.Mode.DRAW_MULTILEADER);markModeSelected(0);result.setText("Multileader • Ok ucunu ve metin bağlantı noktasını seçin");}}),
            tool("Revcloud",R.drawable.ic_polyline,()->{if(canEdit()){cad.setMode(CadView.Mode.DRAW_REVCLOUD);markModeSelected(0);result.setText("Revcloud • Bulut alanının iki karşı köşesini seçin");}}),
            tool("Divide",R.drawable.ic_point,this::runDivideCommand),
            tool("Hatch",R.drawable.ic_hatch,this::runHatchCommand)
        );
    }

    private void showShapeToolsSheet(){
        showToolSheet("Geometrik Şekiller",
            tool("Dikdörtgen",R.drawable.ic_rectangle,()->selectEditMode(R.id.rectangleButton,CadView.Mode.DRAW_RECTANGLE)),
            tool("Daire",R.drawable.ic_circle,()->selectEditMode(R.id.circleButton,CadView.Mode.DRAW_CIRCLE)),
            tool("Elips",R.drawable.ic_circle,()->{if(canEdit()){cad.setMode(CadView.Mode.DRAW_ELLIPSE);markModeSelected(0);result.setText("Elips • Merkez ve eksenleri seçin");}}),
            tool("Nokta",R.drawable.ic_point,()->selectEditMode(R.id.pointButton,CadView.Mode.DRAW_POINT))
        );
    }

    private void showEditToolsSheet(){
        showToolPanel("Düzenle",
            tool("Taşı",R.drawable.ic_move,()->{if(!cad.armMoveSelected())noSourceSelection();}),
            tool("Kopyala",R.drawable.ic_copy,()->{if(!cad.copySelectedEntity())noSourceSelection();}),
            tool("Döndür",R.drawable.ic_rotate,()->{if(!cad.rotateSelectedEntity())noSourceSelection();}),
            tool("Sil",R.drawable.ic_delete,()->{if(!cad.deleteSelectedEntity())noSourceSelection();}),
            tool("Düzelt / Trim",R.drawable.ic_line,()->{if(ensureTransformSelection("KES / TRIM")){if(cad.armTrimSelected())result.setText("Kesme • Kesme sınırı olacak ikinci çizgiye dokunun");else result.setText("Kesme • Hedef nesne çizgi olmalı");}}),
            tool("Uzat",R.drawable.ic_line,()->{if(ensureTransformSelection("UZAT / EXTEND")){if(cad.armExtendSelected())result.setText("Uzat • Sınır çizgisine dokunun");else result.setText("Uzat • Hedef nesne çizgi olmalı");}}),
            tool("Offset",R.drawable.ic_line,this::runOffsetCommand),
            tool("Ölçekle",R.drawable.ic_scale,this::runScaleCommand),
            tool("Aynala",R.drawable.ic_rotate,this::runMirrorCommand),
            tool("Fileto",R.drawable.ic_circle,this::runFilletCommand),
            tool("Oluk / Pah",R.drawable.ic_line,this::runChamferCommand),
            tool("Kır",R.drawable.ic_line,()->{if(ensureTransformSelection("KIR / BREAK")){if(cad.armBreakSelected())result.setText("Kır • Bölme noktasına dokunun");else result.setText("Kır • Hedef nesne çizgi olmalı");}}),
            tool("Stretch",R.drawable.ic_move,()->{if(ensureTransformSelection("STRETCH")){if(cad.armStretchSelected())result.setText("Stretch • Köşe/vertex seçin");}}),
            tool("Array",R.drawable.ic_copy,this::runArrayCommand),
            tool("Patlat",R.drawable.ic_more,()->{if(ensureTransformSelection("EXPLODE")){if(cad.explodeSelectedEntity())result.setText("Patlat • Nesne parçalara ayrıldı");else result.setText("Patlat • Bu nesne desteklenmiyor");}}),
            tool("Birleştir",R.drawable.ic_polyline,()->{if(ensureTransformSelection("JOIN")){if(cad.armJoinSelected())result.setText("Birleştir • İkinci nesneye dokunun");else result.setText("Birleştir • Uygun nesne seçin");}})
        );
    }

    private void showMeasureToolsSheet(){
        showToolPanel("Ölçüm",
            tool("Mesafe",R.drawable.ic_distance,()->selectMode(R.id.distanceButton,CadView.Mode.DISTANCE)),
            tool("Alan",R.drawable.ic_area,()->selectMode(R.id.areaButton,CadView.Mode.AREA)),
            tool("Varlık",R.drawable.ic_select,()->selectEditMode(R.id.selectEntityButton,CadView.Mode.SELECT_ENTITY)),
            tool("ID Noktası",R.drawable.ic_point,()->{cad.setMode(CadView.Mode.ID_POINT);markModeSelected(0);result.setText("ID Noktası • Koordinat için bir noktaya dokunun");}),
            tool("Kalibrasyon",R.drawable.ic_scale,()->selectMode(R.id.calibrateButton,CadView.Mode.CALIBRATE)),
            tool("Açı",R.drawable.ic_distance,()->{cad.setMode(CadView.Mode.ANGLE);markModeSelected(0);result.setText("Açı • Köşe ortada olacak şekilde 3 nokta seçin");}),
            tool("Yay uzunluğu",R.drawable.ic_distance,()->{cad.setMode(CadView.Mode.ARC_LENGTH);markModeSelected(0);result.setText("Yay uzunluğu • Bir yay veya daireye dokunun");}),
            tool("Cephe",R.drawable.ic_distance,this::showFacadeMeasureSetup),
            tool("Sonuç",R.drawable.ic_properties,this::showMeasurementResults),
            tool("Sonuç sayısı",R.drawable.ic_properties,this::showMeasurementCount),
            tool("Hassas",R.drawable.ic_scale,this::showMeasurementPrecision)
        );
    }

    private void showFacadeMeasureSetup(){
        if(activeDxf==null){result.setText("Cephe • Önce çizim açın");return;}
        EditText height=new EditText(this);height.setSingleLine(true);height.setHint("Cephe yüksekliği ("+activeDxf.drawingUnitName()+")");height.setInputType(android.text.InputType.TYPE_CLASS_NUMBER|android.text.InputType.TYPE_NUMBER_FLAG_DECIMAL);height.setText("1");
        AlertDialog dialog=new AlertDialog.Builder(this)
            .setTitle("Cephe ölçümü")
            .setMessage("Kiriş, kolon veya duvar yan yüzeyi için yüksekliği girin. Ardından çizimde taban/iz boyunca noktaları seçin; MusaCAD toplam uzunluk × yüksekliği hesaplar.")
            .setView(height).setPositiveButton("BAŞLAT",null).setNegativeButton("İPTAL",null).create();
        dialog.setOnShowListener(d->dialog.getButton(AlertDialog.BUTTON_POSITIVE).setOnClickListener(v->{
            try{
                double h=Double.parseDouble(height.getText().toString().trim().replace(',','.'));
                if(!Double.isFinite(h)||h<=0d){height.setError("Sıfırdan büyük yükseklik girin");return;}
                cad.setFacadeHeight(h);markModeSelected(0);dialog.dismiss();
                result.setText("Cephe • Taban/iz boyunca en az iki nokta seçin • Yükseklik "+String.format(Locale.getDefault(),"%.3f",h)+" "+activeDxf.drawingUnitName());
            }catch(Exception e){height.setError("Geçerli bir yükseklik girin");}
        }));dialog.show();
    }

    private boolean isCompletedMeasurement(String value){
        if(value==null)return false;
        String v=value.trim();
        return v.startsWith("Mesafe:")||v.startsWith("Alan:")||v.startsWith("Cephe:")||v.startsWith("Açı:")||v.startsWith("Yay uzunluğu:")||
               v.startsWith("ID Noktası • X=")||v.startsWith("Radius ölçüsü:")||v.startsWith("Çap ölçüsü:")||
               v.startsWith("Açısal ölçü:");
    }

    private void recordMeasurement(String value){
        if(currentProject==null||!isCompletedMeasurement(value))return;
        ArrayDeque<String> history=currentProject.measurementHistory;
        if(!history.isEmpty()&&value.equals(history.peekLast()))return;
        history.addLast(value);
        while(history.size()>50)history.removeFirst();
    }

    private void showMeasurementResults(){
        if(currentProject==null){result.setText("Sonuç • Önce çizim açın");return;}
        ArrayDeque<String> history=currentProject.measurementHistory;
        if(history.isEmpty()){result.setText("Sonuç • Henüz tamamlanmış ölçüm yok");return;}
        StringBuilder text=new StringBuilder();int index=1;
        for(String value:history)text.append(index++).append(". ").append(value).append("\n");
        TextView out=new TextView(this);out.setText(text.toString().trim());out.setTextIsSelectable(true);out.setTextSize(12f);int p=dp(16);out.setPadding(p,p/2,p,p);
        ScrollView scroll=new ScrollView(this);scroll.addView(out);
        new AlertDialog.Builder(this).setTitle("Ölçüm sonuçları • "+history.size()).setView(scroll)
            .setNeutralButton("TEMİZLE",(d,w)->{history.clear();result.setText("Ölçüm sonuçları temizlendi");})
            .setPositiveButton("TAMAM",null).show();
    }

    private void showMeasurementCount(){
        if(currentProject==null){result.setText("Sonuç sayısı • Önce çizim açın");return;}
        int count=currentProject.measurementHistory.size();
        result.setText("Sonuç sayısı • "+count);
        new AlertDialog.Builder(this).setTitle("Sonuç sayısı").setMessage("Bu projede kayıtlı tamamlanmış ölçüm: "+count).setPositiveButton("TAMAM",null).show();
    }

    private void saveCurrentView(){
        if(currentProject==null){result.setText("Yeni görünüm • Önce çizim açın");return;}
        currentProject.viewBookmark=cad.captureViewBookmark();
        result.setText(currentProject.viewBookmark==null?"Yeni görünüm • Görünüm kaydedilemedi":"Yeni görünüm • Mevcut görünüm kaydedildi");
    }

    private void showViewBookmark(){
        if(currentProject==null){result.setText("Yer imi • Önce çizim açın");return;}
        String[] items=currentProject.viewBookmark==null?new String[]{"Mevcut görünümü kaydet"}:new String[]{"Mevcut görünümü kaydet","Kayıtlı görünüme dön","Yer imini temizle"};
        new AlertDialog.Builder(this).setTitle("Yer imi").setItems(items,(d,which)->{
            if(which==0)saveCurrentView();
            else if(which==1){
                if(cad.restoreViewBookmark(currentProject.viewBookmark))result.setText("Yer imi • Kayıtlı görünüme dönüldü");
                else result.setText("Yer imi • Görünüm geri yüklenemedi");
            }else if(which==2){currentProject.viewBookmark=null;result.setText("Yer imi • Temizlendi");}
        }).setNegativeButton("İPTAL",null).show();
    }

    private void showMeasurementPrecision(){
        if(activeDxf==null){result.setText("Hassasiyet • Önce çizim açın");return;}
        String[] items={"0 ondalık","1 ondalık","2 ondalık","3 ondalık","4 ondalık","5 ondalık","6 ondalık"};
        new AlertDialog.Builder(this)
            .setTitle("Ölçüm hassasiyeti")
            .setSingleChoiceItems(items,cad.dimensionPrecision(),null)
            .setPositiveButton("UYGULA",(dialog,which)->{
                AlertDialog a=(AlertDialog)dialog;
                int selected=a.getListView().getCheckedItemPosition();
                if(selected<0)selected=cad.dimensionPrecision();
                if(cad.setDimensionStyle(cad.dimensionTextHeightDrawing(),cad.dimensionArrowSizeDrawing(),selected))
                    result.setText("Ölçüm hassasiyeti • "+selected+" ondalık basamak");
            })
            .setNegativeButton("İPTAL",null)
            .show();
    }

    private void showDimensionToolsPanel(){
        showToolPanel("Boyut",
            tool("Doğrusal",R.drawable.ic_distance,()->{if(canEdit()){cad.setMode(CadView.Mode.DRAW_DIM_LINEAR);markModeSelected(0);result.setText("Doğrusal Ölçü • İki nokta ve ölçü çizgisi konumu seçin");}}),
            tool("Hizalı",R.drawable.ic_distance,()->{if(canEdit()){cad.setMode(CadView.Mode.DRAW_DIM_ALIGNED);markModeSelected(0);result.setText("Hizalı Ölçü • İki nokta ve ölçü çizgisi konumu seçin");}}),
            tool("Ölçü Stili",R.drawable.ic_properties,this::runDimStyleCommand),
            tool("Açısal",R.drawable.ic_distance,()->{if(canEdit()){cad.setMode(CadView.Mode.DRAW_DIM_ANGULAR);markModeSelected(0);result.setText("Açısal ölçü • İlk kol, köşe ve ikinci kol için 3 nokta seçin");}}),
            tool("Radius",R.drawable.ic_circle,()->{if(canEdit()){cad.setMode(CadView.Mode.DRAW_DIM_RADIUS);markModeSelected(0);result.setText("Radius ölçüsü • Bir daireye dokunun");}}),
            tool("Çap",R.drawable.ic_circle,()->{if(canEdit()){cad.setMode(CadView.Mode.DRAW_DIM_DIAMETER);markModeSelected(0);result.setText("Çap ölçüsü • Bir daireye dokunun");}}),
            tool("Yay boyu",R.drawable.ic_line,()->{cad.setMode(CadView.Mode.ARC_LENGTH);markModeSelected(0);result.setText("Yay boyu • Bir yay veya daireye dokunun");}),
            tool("Three-point",R.drawable.ic_distance,()->{if(canEdit()){cad.setMode(CadView.Mode.DRAW_DIM_ANGULAR);markModeSelected(0);result.setText("Three-point • İlk kol, köşe ve ikinci kol için 3 nokta seçin");}})
        );
    }

    private void showViewToolsSheet(){
        showToolPanel("Görsel stil",
            tool("Kaydır",R.drawable.ic_pan,()->selectMode(R.id.panButton,CadView.Mode.PAN)),
            tool("Sığdır",R.drawable.ic_fit,()->cad.fitToScreen()),
            tool("Yakınlaştır",R.drawable.ic_zoom_in,()->cad.zoomBy(1.35f)),
            tool("Uzaklaştır",R.drawable.ic_zoom_out,()->cad.zoomBy(1f/1.35f)),
            tool("2D",R.drawable.ic_fit,()->{cad.regenerate();result.setText("2D görünüm etkin");}),
            tool("3D",R.drawable.ic_fit,null),
            tool("Katmanlar",R.drawable.ic_layers,this::showLayers),
            tool("Model/Layout",R.drawable.ic_layers,this::showLayouts)
        );
    }

    private void showAnnotationToolsSheet(){
        showToolPanel("Ek açıklama",
            tool("Taslak kroki",R.drawable.ic_line,()->{if(canEdit()){cad.setMode(CadView.Mode.FREEHAND);markModeSelected(0);result.setText("Taslak kroki • Serbest çizim etkin");}}),
            tool("Ok",R.drawable.ic_line,()->{if(canEdit()){cad.setMode(CadView.Mode.DRAW_ARROW);markModeSelected(0);result.setText("Ok • Önce ok ucunu, sonra kuyruk noktasını seçin");}}),
            tool("Metin",R.drawable.ic_text,()->selectEditMode(R.id.textButton,CadView.Mode.DRAW_TEXT)),
            tool("Revcloud",R.drawable.ic_polyline,()->{if(canEdit()){cad.setMode(CadView.Mode.DRAW_REVCLOUD);markModeSelected(0);result.setText("Revcloud • Bulut alanının iki karşı köşesini seçin");}}),
            tool("Ses",R.drawable.ic_more,()->pickMedia(PICK_AUDIO,"audio/*")),
            tool("Görüntü",R.drawable.ic_open_file,()->pickMedia(PICK_IMAGE,"image/*")),
            tool("Video",R.drawable.ic_more,()->pickMedia(PICK_VIDEO,"video/*")),
            tool("Kılavuz",R.drawable.ic_text,()->{if(canEdit()){cad.setMode(CadView.Mode.DRAW_XLINE);markModeSelected(0);result.setText("Kılavuz • Sonsuz yardımcı doğru için iki nokta seçin");}}),
            tool("Çizgi",R.drawable.ic_line,()->selectEditMode(R.id.lineButton,CadView.Mode.DRAW_LINE)),
            tool("Dikdörtgen",R.drawable.ic_rectangle,()->selectEditMode(R.id.rectangleButton,CadView.Mode.DRAW_RECTANGLE)),
            tool("Elips",R.drawable.ic_circle,()->{if(canEdit()){cad.setMode(CadView.Mode.DRAW_ELLIPSE);markModeSelected(0);result.setText("Elips • Merkez ve eksenleri seçin");}}),
            tool("Numbering",R.drawable.ic_text,this::showNumberingSetup)
        );
    }

    private void showNumberingSetup(){
        if(!canEdit()){result.setText("Numbering • Düzenlenebilir bir çizim açın");return;}
        EditText input=new EditText(this);input.setSingleLine(true);input.setHint("Başlangıç numarası");input.setText("1");input.setInputType(android.text.InputType.TYPE_CLASS_NUMBER);
        AlertDialog dialog=new AlertDialog.Builder(this).setTitle("Numbering").setMessage("Başlangıç numarasını belirleyin; sonra çizimde dokunduğunuz her noktaya sıradaki numara yerleşir.").setView(input).setPositiveButton("BAŞLAT",null).setNegativeButton("İPTAL",null).create();
        dialog.setOnShowListener(d->dialog.getButton(AlertDialog.BUTTON_POSITIVE).setOnClickListener(v->{
            try{
                int start=Integer.parseInt(input.getText().toString().trim());
                if(start<1||start>999999){input.setError("1 ile 999999 arasında değer girin");return;}
                cad.setNumberingStart(start);markModeSelected(0);dialog.dismiss();result.setText("Numbering • Sıradaki: "+start+" • Yerleştirmek için dokunun");
            }catch(Exception e){input.setError("Geçerli başlangıç numarası girin");}
        }));dialog.show();
    }

    private void showOtherToolsSheet(){
        showToolPanel("Alet",
            tool("Geri Al",R.drawable.ic_undo,()->cad.undo()),
            tool("Yinele",R.drawable.ic_rotate,()->{if(!cad.redo())result.setText("Yinele • İşlem yok");}),
            tool("Temizle",R.drawable.ic_clear,()->cad.clearMeasurement()),
            tool("Blok ekle",R.drawable.ic_open_file,this::runInsertCommand),
            tool("Çizgi Tipi",R.drawable.ic_line,this::showSelectedLineType),
            tool("Özellik",R.drawable.ic_properties,this::showSelectedProperties),
            tool("Bulmak",R.drawable.ic_select,()->showEntitySearch(false)),
            tool("Artımlı Kopya",R.drawable.ic_copy,this::runArrayCommand),
            tool("Sayaç bloğu",R.drawable.ic_properties,this::showBlockCount),
            tool("Graphic lookup",R.drawable.ic_select,this::showGraphicLookup),
            tool("Açıklama ara",R.drawable.ic_text,()->showEntitySearch(true)),
            tool("Yer imi",R.drawable.ic_more,this::showViewBookmark),
            tool("Copy across",R.drawable.ic_copy,this::copyAcrossProjects),
            tool("Paste across",R.drawable.ic_copy,this::pasteAcrossProjects),
            tool("Medya Ekleri",R.drawable.ic_open_file,this::showMediaAttachments),
            tool("Yardım",R.drawable.ic_more,this::showCommandHelp)
        );
    }

    private void pickMedia(int request,String mime){
        if(!canEdit()||currentProject==null){result.setText("Medya • Düzenlenebilir bir çizim açın");return;}
        Intent intent=new Intent(Intent.ACTION_OPEN_DOCUMENT);intent.addCategory(Intent.CATEGORY_OPENABLE);intent.setType(mime);
        startActivityForResult(intent,request);
    }

    private void handleMediaPicked(int request,Uri uri){
        if(uri==null||currentProject==null)return;
        String kind=request==PICK_AUDIO?"Ses":request==PICK_IMAGE?"Görüntü":"Video";
        String mime=request==PICK_AUDIO?"audio/*":request==PICK_IMAGE?"image/*":"video/*";
        try{getContentResolver().takePersistableUriPermission(uri,Intent.FLAG_GRANT_READ_URI_PERMISSION);}catch(Exception ignored){}
        String name=nameOf(uri);if(name==null||name.trim().isEmpty())name=kind+" eki";
        PointF center=cad.visibleCenterContent();
        cad.addTextEdit(center.x,center.y,kind+" • "+name);
        currentProject.mediaAttachments.add(new MediaAttachment(kind,name,mime,uri));
        result.setText(kind+" • "+name+" eklendi • görünüm merkezine bağlantı notu yerleştirildi");
    }

    private void showMediaAttachments(){
        if(currentProject==null||currentProject.mediaAttachments.isEmpty()){result.setText("Medya Ekleri • Bu projede ek yok");return;}
        String[] labels=new String[currentProject.mediaAttachments.size()];
        for(int i=0;i<labels.length;i++){MediaAttachment m=currentProject.mediaAttachments.get(i);labels[i]=m.kind+" • "+m.name;}
        new AlertDialog.Builder(this).setTitle("Medya Ekleri").setItems(labels,(d,which)->{
            MediaAttachment m=currentProject.mediaAttachments.get(which);
            try{
                Intent view=new Intent(Intent.ACTION_VIEW,Uri.parse(m.uri));view.setType(m.mime);view.addFlags(Intent.FLAG_GRANT_READ_URI_PERMISSION);startActivity(view);
            }catch(Exception e){Toast.makeText(this,"Bu medya için açılabilir uygulama bulunamadı",Toast.LENGTH_LONG).show();}
        }).setNegativeButton("KAPAT",null).show();
    }

    private void copyAcrossProjects(){
        if(!ensureSelectedForQuickTool("Copy across"))return;
        if(activeDxf==null){result.setText("Copy across • Tam vektör model gerekli");return;}
        CadEdit selected=cad.selectedEntityCopy();
        CadEdit drawing=activeDxf.drawingEditFromContent(selected);
        if(drawing==null){result.setText("Copy across • Seçili nesne kopyalanamadı");return;}
        crossProjectClipboard=drawing;crossProjectClipboardSource=currentDisplayName;
        result.setText("Copy across • Nesne panoya alındı • "+currentDisplayName);
    }

    private void pasteAcrossProjects(){
        if(!canEdit()||activeDxf==null){result.setText("Paste across • Düzenlenebilir bir hedef çizim açın");return;}
        if(crossProjectClipboard==null){result.setText("Paste across • Önce Copy across ile bir nesne kopyalayın");return;}
        CadEdit content=activeDxf.contentEditFromDrawing(crossProjectClipboard);
        if(content==null||!cad.addImportedEdit(content)){result.setText("Paste across • Nesne yapıştırılamadı");return;}
        result.setText("Paste across • "+(crossProjectClipboardSource.isEmpty()?"Panodaki nesne":crossProjectClipboardSource+" kaynağındaki nesne")+" eklendi");
    }

    private void showGraphicLookup(){
        if(!canEdit()){result.setText("Graphic lookup • Tam vektör model gerekli");return;}
        if(!cad.hasSelectedEntity()){
            selectEditMode(R.id.selectEntityButton,CadView.Mode.SELECT_ENTITY);
            result.setText("Graphic lookup • Çizimde incelemek istediğiniz nesneyi seçin, sonra araca tekrar basın");
            return;
        }
        String info=cad.selectedEntityInfo();
        TextView out=new TextView(this);out.setText(info);out.setTextIsSelectable(true);out.setTextSize(12f);int p=dp(16);out.setPadding(p,p/2,p,p);
        ScrollView scroll=new ScrollView(this);scroll.addView(out);
        new AlertDialog.Builder(this).setTitle("Graphic lookup").setView(scroll).setPositiveButton("TAMAM",null).show();
        result.setText("Graphic lookup • Seçili nesne bilgileri açıldı");
    }

    private void showBlockCount(){
        if(activeDxf==null){result.setText("Sayaç bloğu • Önce çizim açın");return;}
        int count=0;
        LinkedHashMap<String,Integer> byLayer=new LinkedHashMap<>();
        for(DxfParser.SourceEntity source:activeDxf.editableSources()){
            CadEdit edit=source.prototype();
            if(!"INSERT".equalsIgnoreCase(source.type)&&edit.type!=CadEdit.Type.INSERT)continue;
            count++;String layer=source.layer==null?"0":source.layer;byLayer.put(layer,byLayer.getOrDefault(layer,0)+1);
        }
        StringBuilder detail=new StringBuilder("Toplam blok yerleşimi: ").append(count);
        int shown=0;
        for(Map.Entry<String,Integer> entry:byLayer.entrySet()){
            if(shown++>=12){detail.append("\n…");break;}
            detail.append("\n").append(entry.getKey()).append(": ").append(entry.getValue());
        }
        new AlertDialog.Builder(this).setTitle("Sayaç bloğu").setMessage(detail.toString()).setPositiveButton("TAMAM",null).show();
        result.setText("Sayaç bloğu • "+count+" blok");
    }

    private void showEntitySearch(boolean textOnly){
        if(activeDxf==null){result.setText("Bul • Önce çizim açın");return;}
        EditText input=new EditText(this);input.setSingleLine(true);input.setHint(textOnly?"Açıklama / metin":"Katman, nesne tipi veya metin");
        AlertDialog dialog=new AlertDialog.Builder(this).setTitle(textOnly?"Açıklama ara":"Bulmak").setView(input).setPositiveButton("ARA",null).setNegativeButton("İPTAL",null).create();
        dialog.setOnShowListener(d->dialog.getButton(AlertDialog.BUTTON_POSITIVE).setOnClickListener(v->{
            String query=input.getText().toString().trim().toLowerCase(Locale.ROOT);
            if(query.isEmpty()){input.setError("Aranacak ifadeyi girin");return;}
            int count=0;StringBuilder lines=new StringBuilder();
            for(DxfParser.SourceEntity source:activeDxf.editableSources()){
                CadEdit edit=source.prototype();
                boolean isText="TEXT".equalsIgnoreCase(source.type)||"MTEXT".equalsIgnoreCase(source.type)||edit.type==CadEdit.Type.TEXT;
                if(textOnly&&!isText)continue;
                String label=(source.type==null?"":source.type)+" • "+(source.layer==null?"0":source.layer);
                String text=edit.text==null?"":edit.text;
                String hay=(label+" "+text).toLowerCase(Locale.ROOT);
                if(!hay.contains(query))continue;
                count++;
                if(count<=30){lines.append(label);if(!text.isEmpty())lines.append(" • ").append(text.replace('\n',' '));lines.append("\n");}
            }
            dialog.dismiss();
            String message=count==0?"Eşleşme bulunamadı":("Eşleşme: "+count+"\n\n"+lines+(count>30?"… İlk 30 sonuç gösteriliyor.":""));
            TextView out=new TextView(this);out.setText(message);out.setTextIsSelectable(true);int p=dp(16);out.setPadding(p,p/2,p,p);
            ScrollView scroll=new ScrollView(this);scroll.addView(out);
            new AlertDialog.Builder(this).setTitle(textOnly?"Açıklama sonuçları":"Bul sonuçları").setView(scroll).setPositiveButton("TAMAM",null).show();
            result.setText((textOnly?"Açıklama ara":"Bul")+" • "+count+" eşleşme");
        }));dialog.show();
    }

    private void showLayerToolsPanel(){
        showToolPanel("Katman",
            tool("Yeni katman",R.drawable.ic_layers,this::showCreateLayer),
            tool("Katman Listesi",R.drawable.ic_layers,this::showLayers),
            tool("Katmanı Kapat",R.drawable.ic_layers,this::hideSelectedLayer),
            tool("Diğer katmanlar",R.drawable.ic_layers,this::isolateSelectedLayer),
            tool("Önceki katman",R.drawable.ic_layers,this::restorePreviousLayers),
            tool("Tüm Katmanlar",R.drawable.ic_layers,this::showAllLayers),
            tool("Katmanı varsayılan",R.drawable.ic_layers,this::showSetDefaultLayer),
            tool("Özellik",R.drawable.ic_properties,this::showSelectedProperties)
        );
    }

    private void showColorToolsPanel(){
        showToolPanel("Renk",
            tool("Renk ayarı",R.drawable.ic_color,this::showSelectedColor),
            tool("Özellik",R.drawable.ic_properties,this::showSelectedProperties),
            tool("ByLayer",R.drawable.ic_layers,()->applySelectedColorMode(SourceReplacement.COLOR_BYLAYER,"ByLayer")),
            tool("ByBlock",R.drawable.ic_rectangle,()->applySelectedColorMode(SourceReplacement.COLOR_BYBLOCK,"ByBlock")),
            tool("ACI 1–255",R.drawable.ic_color,this::showAciColorPicker)
        );
    }

    private void applySelectedColorMode(int mode,String label){
        if(!ensureSelectedForQuickTool(label))return;
        if(cad.updateSelectedColorMode(mode))result.setText(label+" • Kaydetmede DXF renk modu uygulanacak");
        else result.setText(label+" • Değişiklik uygulanamadı");
    }

    private void showAciColorPicker(){
        if(!ensureSelectedForQuickTool("ACI Renk"))return;
        LinearLayout root=new LinearLayout(this);root.setOrientation(LinearLayout.VERTICAL);int pad=dp(14);root.setPadding(pad,pad/2,pad,pad/2);
        TextView info=new TextView(this);info.setText("AutoCAD Color Index • 1–255");info.setTextSize(12f);info.setPadding(0,0,0,dp(8));root.addView(info);
        final int[] selected={7};
        final TextView chosen=new TextView(this);chosen.setText("Seçili ACI: 7");chosen.setTextSize(11f);chosen.setPadding(0,dp(6),0,dp(4));
        GridLayout palette=new GridLayout(this);palette.setColumnCount(7);palette.setAlignmentMode(GridLayout.ALIGN_BOUNDS);palette.setUseDefaultMargins(false);
        int[] samples={1,2,3,4,5,6,7,10,20,30,40,50,60,70,80,90,100,110,120,130,140,150,160,170,180,190,200,250};
        for(int aci:samples){
            TextView swatch=new TextView(this);swatch.setText(Integer.toString(aci));swatch.setTextSize(7f);swatch.setGravity(Gravity.CENTER);
            int argb=DxfColor.aciArgb(aci);double luminance=.2126*Color.red(argb)+.7152*Color.green(argb)+.0722*Color.blue(argb);
            swatch.setTextColor(luminance>150?Color.BLACK:Color.WHITE);
            android.graphics.drawable.GradientDrawable bg=new android.graphics.drawable.GradientDrawable();bg.setShape(android.graphics.drawable.GradientDrawable.OVAL);bg.setColor(argb);bg.setStroke(dp(1),0xFF6A8795);swatch.setBackground(bg);
            GridLayout.LayoutParams lp=new GridLayout.LayoutParams();lp.width=dp(38);lp.height=dp(38);lp.setMargins(dp(3),dp(3),dp(3),dp(3));palette.addView(swatch,lp);
            swatch.setOnClickListener(v->{selected[0]=aci;chosen.setText("Seçili ACI: "+aci);});
            installInteractiveFeedback(swatch);
        }
        root.addView(palette);root.addView(chosen);
        EditText input=new EditText(this);input.setSingleLine(true);input.setHint("1–255");input.setText("7");input.setInputType(android.text.InputType.TYPE_CLASS_NUMBER);root.addView(input);
        AlertDialog dialog=new AlertDialog.Builder(this).setTitle("Renk ayarı").setView(root).setPositiveButton("TAMAM",null).setNegativeButton("İPTAL",null).create();
        dialog.setOnShowListener(d->dialog.getButton(AlertDialog.BUTTON_POSITIVE).setOnClickListener(v->{
            try{
                String raw=input.getText().toString().trim();
                int aci=raw.isEmpty()?selected[0]:Integer.parseInt(raw);
                if(aci<1||aci>255){input.setError("1 ile 255 arasında bir değer girin");return;}
                int color=DxfColor.aciArgb(aci);
                if(!cad.updateSelectedStyle(null,color,null,null,null)){dialog.dismiss();result.setText("ACI renk • Değişiklik uygulanamadı");return;}
                dialog.dismiss();result.setText("ACI renk • "+aci+" uygulandı");
            }catch(Exception e){input.setError("1 ile 255 arasında bir değer girin");}
        }));dialog.show();
    }

    private void showLayoutToolsPanel(){
        showToolPanel("Düzen",
            tool("Model / Layout",R.drawable.ic_layers,this::showLayouts),
            tool("Katmanlar",R.drawable.ic_layers,this::showLayers),
            tool("Sığdır",R.drawable.ic_fit,()->cad.fitToScreen()),
            tool("Yeni görünüm",R.drawable.ic_rectangle,this::saveCurrentView),
            tool("Viewport",R.drawable.ic_rectangle,this::showViewportBrowser)
        );
    }

    private void showViewportBrowser(){
        if(activeDxf==null){result.setText("Viewport • Önce çizim açın");return;}
        List<DxfViewport.View> viewports=activeDxf.viewports();
        if(viewports.isEmpty()){result.setText("Viewport • Aktif düzende viewport bulunamadı");return;}
        String[] labels=new String[viewports.size()];
        for(int i=0;i<viewports.size();i++){
            DxfViewport.View vp=viewports.get(i);
            labels[i]="Viewport "+vp.id+" • "+String.format(Locale.getDefault(),"%.3f",vp.scale())+"x • "+String.format(Locale.getDefault(),"%.1f × %.1f",vp.paperWidth,vp.paperHeight);
        }
        new AlertDialog.Builder(this).setTitle("Viewport • "+activeDxf.activeLayout).setItems(labels,(d,which)->{
            DxfViewport.View vp=viewports.get(which);RectF bounds=activeDxf.viewportContentBounds(vp);
            if(cad.fitContentRect(bounds))result.setText("Viewport "+vp.id+" • Görünüme odaklandı");
            else result.setText("Viewport • Görünüm sınırı kullanılamadı");
        }).setNegativeButton("İPTAL",null).show();
    }

    private void showCreateLayer(){
        if(activeDxf==null||editingBaseDxf==null||!editingBaseDxf.exists()){result.setText("Yeni katman • Önce düzenlenebilir bir çizim açın");return;}
        EditText input=new EditText(this);input.setSingleLine(true);input.setHint("Katman adı");input.setText("MUSA_LAYER");input.setSelectAllOnFocus(true);
        AlertDialog dialog=new AlertDialog.Builder(this).setTitle("Yeni katman").setView(input).setPositiveButton("OLUŞTUR",null).setNegativeButton("İPTAL",null).create();
        dialog.setOnShowListener(d->dialog.getButton(AlertDialog.BUTTON_POSITIVE).setOnClickListener(v->{
            String name=input.getText().toString().trim();
            if(name.isEmpty()){input.setError("Katman adı girin");return;}
            if(name.length()>120||name.matches(".*[<>/\\\":;?*|=,].*")){input.setError("Geçerli bir DXF katman adı girin");return;}
            for(String existing:activeDxf.layerNames)if(existing.equalsIgnoreCase(name)){input.setError("Bu katman zaten var");return;}
            dialog.dismiss();createLayerAsync(name);
        }));dialog.show();
    }

    private void createLayerAsync(String name){
        if(activeLoad!=null||editingBaseDxf==null)return;
        final File base=editingBaseDxf;final ProjectSession project=currentProject;final DxfParser.Result before=activeDxf;
        LoadTask task=new LoadTask();activeLoad=task;task.dialog=new AlertDialog.Builder(this).setTitle("Yeni katman").setMessage(name+" oluşturuluyor…").setCancelable(false).create();task.dialog.show();
        task.future=loader.submit(()->{
            try{
                DxfLayerEditor.addLayer(base,name);
                DxfParser.Result updated=DxfParser.render(base);
                runOnUiThread(()->{
                    if(activeLoad!=task||currentProject!=project||isFinishing()||isDestroyed())return;
                    activeLoad=null;task.dialog.dismiss();activeDxf=updated;project.parsed=updated;project.nativeScene=null;
                    cad.replaceVisibleDrawing(updated);snapToggle.setEnabled(updated.snapPoints.length>0);updateLayerButtons(true);refreshProjectTabs();
                    result.setText("Yeni katman • "+name+" oluşturuldu");
                    if(before.bitmap!=null&&before.bitmap!=updated.bitmap&&!before.bitmap.isRecycled())before.bitmap.recycle();
                });
            }catch(Exception e){runOnUiThread(()->{if(activeLoad!=task||isFinishing()||isDestroyed())return;activeLoad=null;task.dialog.dismiss();error(e);});}
        });
    }

    private void showSetDefaultLayer(){
        if(activeDxf==null||currentProject==null){result.setText("Katmanı varsayılan • Önce çizim açın");return;}
        String[] layers=activeDxf.layerNames.toArray(new String[0]);int checked=0;
        for(int i=0;i<layers.length;i++)if(layers[i].equalsIgnoreCase(currentProject.defaultLayer)){checked=i;break;}
        new AlertDialog.Builder(this).setTitle("Katmanı varsayılan").setSingleChoiceItems(layers,checked,null)
            .setPositiveButton("UYGULA",(d,w)->{
                AlertDialog a=(AlertDialog)d;int pos=a.getListView().getCheckedItemPosition();if(pos<0)pos=0;
                currentProject.defaultLayer=layers[pos];result.setText("Varsayılan katman • "+currentProject.defaultLayer);
            }).setNegativeButton("İPTAL",null).show();
    }

    private void hideSelectedLayer(){
        if(!ensureSelectedForQuickTool("Katmanı Kapat"))return;
        String layer=cad.selectedLayer();if(layer==null||layer.trim().isEmpty()){result.setText("Katmanı Kapat • Katman bulunamadı");return;}
        HashSet<String> visible=new HashSet<>(activeDxf.visibleLayers);visible.remove(layer);
        if(visible.isEmpty()){result.setText("Katmanı Kapat • Son görünür katman kapatılamaz");return;}
        applyLayers(visible);result.setText("Katman kapatılıyor • "+layer);
    }

    private void isolateSelectedLayer(){
        if(!ensureSelectedForQuickTool("Diğer katmanlar"))return;
        String layer=cad.selectedLayer();if(layer==null||layer.trim().isEmpty()){result.setText("Katman yalıt • Katman bulunamadı");return;}
        HashSet<String> visible=new HashSet<>();visible.add(layer);applyLayers(visible);result.setText("Katman yalıtılıyor • "+layer);
    }

    private void restorePreviousLayers(){
        if(activeDxf==null||currentProject==null||currentProject.previousVisibleLayers.isEmpty()){result.setText("Önceki katman • Kayıtlı görünüm yok");return;}
        applyLayers(new HashSet<>(currentProject.previousVisibleLayers));
    }

    private void showAllLayers(){
        if(activeDxf==null){Toast.makeText(this,"Katmanlar için önce bir çizim açın",Toast.LENGTH_SHORT).show();return;}
        applyLayers(new HashSet<>(activeDxf.layerNames));
    }

    private void showNewProjectSheet(){
        showToolSheet("Yeni Proje",
            tool("Yeni Boş Çizim",R.drawable.ic_open_file,this::createBlankDrawing),
            tool("DWG/DXF Aç",R.drawable.ic_open_file,this::open)
        );
    }

    private void createBlankDrawing(){
        if(projects.size()>=MAX_OPEN_PROJECTS){Toast.makeText(this,"Aynı anda en fazla "+MAX_OPEN_PROJECTS+" proje açık tutulur.",Toast.LENGTH_LONG).show();return;}
        try{
            int number=projects.size()+1;String name="Yeni Çizim "+number+".dxf";
            File base=new File(getCacheDir(),"musacad_blank_"+System.currentTimeMillis()+".dxf");
            String dxf="0\nSECTION\n2\nHEADER\n9\n$ACADVER\n1\nAC1015\n0\nENDSEC\n0\nSECTION\n2\nTABLES\n0\nTABLE\n2\nLAYER\n70\n1\n0\nLAYER\n2\n0\n70\n0\n62\n7\n6\nCONTINUOUS\n0\nENDTAB\n0\nENDSEC\n0\nSECTION\n2\nBLOCKS\n0\nENDSEC\n0\nSECTION\n2\nENTITIES\n0\nENDSEC\n0\nEOF\n";
            try(OutputStream out=new FileOutputStream(base)){out.write(dxf.getBytes(java.nio.charset.StandardCharsets.US_ASCII));}
            ProjectSession project=new ProjectSession();project.file=base;project.workingDxf=base;project.parsed=DxfParser.blankDrawing();project.name=name;project.dxf=true;project.lastAccessMs=System.currentTimeMillis();
            projects.add(project);activateProject(project);project.savedFingerprint=cad.editFingerprint();project.baselineSet=true;project.dirty=false;refreshProjectTabs();
            result.setText("Yeni boş çizim hazır • Çizgi, Daire, Dikdörtgen veya diğer araçları seçin");
        }catch(Exception e){error(e);}
    }

    private void showMoreTools(){
        if(activeDxf==null){Toast.makeText(this,"Önce bir çizim açın",Toast.LENGTH_SHORT).show();return;}
        String[] items={"Çoklu Çizgi (PL)","Yay (ARC)","Elips (ELLIPSE)","XLINE","TRIM","EXTEND","FILLET","CHAMFER","OFFSET","STRETCH","ARRAY","BLOCK","INSERT","Geri Al","REDO","Ölçümü / seçimi temizle"};
        new AlertDialog.Builder(this).setTitle("Diğer CAD araçları").setItems(items,(d,which)->{
            switch(which){
                case 0:selectEditMode(R.id.polylineButton,CadView.Mode.DRAW_POLYLINE);break;
                case 1:if(canEdit()){cad.setMode(CadView.Mode.DRAW_ARC);markModeSelected(0);result.setText("ARC • Başlangıç, yay üzeri ve bitiş olmak üzere 3 nokta seçin");}break;
                case 2:if(canEdit()){cad.setMode(CadView.Mode.DRAW_ELLIPSE);markModeSelected(0);result.setText("ELLIPSE • Merkez, ana eksen ucu ve kısa eksen yönünü seçin");}break;
                case 3:if(canEdit()){cad.setMode(CadView.Mode.DRAW_XLINE);markModeSelected(0);result.setText("XLINE • Doğrultu için iki nokta seçin");}break;
                case 4:if(ensureTransformSelection("TRIM")){if(cad.armTrimSelected())result.setText("TRIM • Kesme sınırı olacak ikinci çizgiye dokunun");else result.setText("TRIM • Hedef nesne LINE olmalı");}break;
                case 5:if(ensureTransformSelection("EXTEND")){if(cad.armExtendSelected())result.setText("EXTEND • Uzatma sınırı olacak ikinci çizgiye dokunun");else result.setText("EXTEND • Hedef nesne LINE olmalı");}break;
                case 6:runFilletCommand();break;
                case 7:runChamferCommand();break;
                case 8:runOffsetCommand();break;
                case 9:if(ensureTransformSelection("STRETCH")){if(cad.armStretchSelected())result.setText("STRETCH • Taşınacak köşe/vertex noktasına dokunun");else result.setText("STRETCH • LINE, POLYLINE veya RECTANGLE seçin");}break;
                case 10:runArrayCommand();break;
                case 11:runBlockCommand();break;
                case 12:runInsertCommand();break;
                case 13:cad.undo();break;
                case 14:if(!cad.redo())result.setText("REDO • Yeniden uygulanacak işlem yok");break;
                case 15:cad.clearMeasurement();break;
            }
        }).setNegativeButton("İPTAL",null).show();
    }

    private void showMeasureTools(){
        if(activeDxf==null){Toast.makeText(this,"Önce bir çizim açın",Toast.LENGTH_SHORT).show();return;}
        String[] items={"Mesafe","Alan","Ölçek kalibrasyonu","DIMLINEAR","DIMALIGNED","DIMSTYLE"};
        new AlertDialog.Builder(this).setTitle("Ölç / Ölçülendir").setItems(items,(d,which)->{
            if(which==0)selectMode(R.id.distanceButton,CadView.Mode.DISTANCE);
            else if(which==1)selectMode(R.id.areaButton,CadView.Mode.AREA);
            else if(which==2)selectMode(R.id.calibrateButton,CadView.Mode.CALIBRATE);
            else if(which==3){if(canEdit()){cad.setMode(CadView.Mode.DRAW_DIM_LINEAR);markModeSelected(0);result.setText("DIMLINEAR • İki ölçü noktası ve ölçü çizgisi konumu seçin");}else Toast.makeText(this,"Bu çizim düzenleme için vektörel olarak açılamadı",Toast.LENGTH_SHORT).show();}
            else if(which==4){if(canEdit()){cad.setMode(CadView.Mode.DRAW_DIM_ALIGNED);markModeSelected(0);result.setText("DIMALIGNED • İki ölçü noktası ve ölçü çizgisi konumu seçin");}else Toast.makeText(this,"Bu çizim düzenleme için vektörel olarak açılamadı",Toast.LENGTH_SHORT).show();}
            else runDimStyleCommand();
        }).setNegativeButton("İPTAL",null).show();
    }

    private void runDimStyleCommand(){
        if(activeDxf==null){result.setText("DIMSTYLE • Önce vektörel bir çizim açın");return;}
        LinearLayout box=new LinearLayout(this);box.setOrientation(LinearLayout.VERTICAL);int p=dp(16);box.setPadding(p,p/2,p,p/2);
        EditText textHeight=new EditText(this);textHeight.setHint("Yazı yüksekliği (çizim birimi)");textHeight.setText(String.format(Locale.US,"%.3f",cad.dimensionTextHeightDrawing()));textHeight.setInputType(android.text.InputType.TYPE_CLASS_NUMBER|android.text.InputType.TYPE_NUMBER_FLAG_DECIMAL);box.addView(textHeight);
        EditText arrowSize=new EditText(this);arrowSize.setHint("Ok/tik boyu (çizim birimi)");arrowSize.setText(String.format(Locale.US,"%.3f",cad.dimensionArrowSizeDrawing()));arrowSize.setInputType(android.text.InputType.TYPE_CLASS_NUMBER|android.text.InputType.TYPE_NUMBER_FLAG_DECIMAL);box.addView(arrowSize);
        EditText precision=new EditText(this);precision.setHint("Ondalık hassasiyet (0-6)");precision.setText(Integer.toString(cad.dimensionPrecision()));precision.setInputType(android.text.InputType.TYPE_CLASS_NUMBER);box.addView(precision);
        AlertDialog dialog=new AlertDialog.Builder(this)
            .setTitle("DIMSTYLE • Ölçülendirme stili")
            .setMessage("Değerler çizimin kendi birimindedir. Yeni oluşturulacak ölçülerde kullanılır.")
            .setView(box)
            .setPositiveButton("UYGULA",null)
            .setNegativeButton("İPTAL",null)
            .create();
        dialog.setOnShowListener(d->dialog.getButton(AlertDialog.BUTTON_POSITIVE).setOnClickListener(v->{
            try{
                double th=Double.parseDouble(textHeight.getText().toString().trim().replace(',','.'));
                double ar=Double.parseDouble(arrowSize.getText().toString().trim().replace(',','.'));
                int pr=Integer.parseInt(precision.getText().toString().trim());
                if(!cad.setDimensionStyle(th,ar,pr)){textHeight.setError("Pozitif değerler ve 0-6 hassasiyet girin");return;}
                dialog.dismiss();result.setText(String.format(Locale.getDefault(),"DIMSTYLE • Yazı %.3f • Ok %.3f • Hassasiyet %d",th,ar,pr));
            }catch(Exception e){textHeight.setError("Geçerli ölçülendirme değerleri girin");}
        }));
        dialog.show();
    }

    private void runBlockCommand(){
        if(!ensureTransformSelection("BLOCK"))return;
        EditText input=new EditText(this);
        input.setSingleLine(true);
        input.setHint("Blok adı");
        input.setText("MUSA_BLOCK_1");
        input.setSelectAllOnFocus(true);
        AlertDialog dialog=new AlertDialog.Builder(this)
            .setTitle("BLOCK • Blok oluştur")
            .setMessage("Seçili nesne merkez noktası baz alınarak isimli blok tanımına dönüştürülecek. Kaynak nesne çizimde kalır.")
            .setView(input)
            .setPositiveButton("OLUŞTUR",null)
            .setNegativeButton("İPTAL",null)
            .create();
        dialog.setOnShowListener(d->dialog.getButton(AlertDialog.BUTTON_POSITIVE).setOnClickListener(v->{
            String name=CadBlock.normalizeName(input.getText().toString());
            if(name.isEmpty()){input.setError("Geçerli bir blok adı girin");return;}
            if(!cad.defineBlockFromSelected(name)){input.setError("Seçili nesneden blok oluşturulamadı");return;}
            dialog.dismiss();result.setText("BLOCK • "+name+" oluşturuldu");
        }));
        dialog.show();
    }

    private void runInsertCommand(){
        List<String> names=cad.userBlockNames();
        if(names.isEmpty()){result.setText("INSERT • Önce B / BLOCK ile bir blok oluşturun");return;}
        String[] items=names.toArray(new String[0]);
        new AlertDialog.Builder(this)
            .setTitle("INSERT • Blok seç")
            .setItems(items,(d,which)->showInsertOptions(items[which]))
            .setNegativeButton("İPTAL",null)
            .show();
    }

    private void showInsertOptions(String name){
        LinearLayout box=new LinearLayout(this);box.setOrientation(LinearLayout.VERTICAL);int p=dp(16);box.setPadding(p,p/2,p,p/2);
        EditText scale=new EditText(this);scale.setHint("Ölçek");scale.setText("1");scale.setInputType(android.text.InputType.TYPE_CLASS_NUMBER|android.text.InputType.TYPE_NUMBER_FLAG_DECIMAL);box.addView(scale);
        EditText rotation=new EditText(this);rotation.setHint("Döndürme açısı (°)");rotation.setText("0");rotation.setInputType(android.text.InputType.TYPE_CLASS_NUMBER|android.text.InputType.TYPE_NUMBER_FLAG_DECIMAL|android.text.InputType.TYPE_NUMBER_FLAG_SIGNED);box.addView(rotation);
        AlertDialog dialog=new AlertDialog.Builder(this)
            .setTitle("INSERT • "+name)
            .setMessage("Ölçek ve açıyı belirleyin; ardından çizimde yerleştirme noktasına dokunun.")
            .setView(box)
            .setPositiveButton("YERLEŞTİR",null)
            .setNegativeButton("İPTAL",null)
            .create();
        dialog.setOnShowListener(d->dialog.getButton(AlertDialog.BUTTON_POSITIVE).setOnClickListener(v->{
            try{
                float sc=Float.parseFloat(scale.getText().toString().trim().replace(',','.'));
                float ro=Float.parseFloat(rotation.getText().toString().trim().replace(',','.'));
                if(!Float.isFinite(sc)||sc<=0f){scale.setError("Sıfırdan büyük bir ölçek girin");return;}
                if(!Float.isFinite(ro)){rotation.setError("Geçerli bir açı girin");return;}
                if(!cad.armInsertBlock(name,sc,ro)){dialog.dismiss();result.setText("INSERT • Blok yerleştirme başlatılamadı");return;}
                dialog.dismiss();result.setText("INSERT • "+name+" • Yerleştirme noktasına dokunun");
            }catch(Exception e){scale.setError("Geçerli ölçek ve açı değerleri girin");}
        }));
        dialog.show();
    }

    private void runHatchCommand(){
        if(!ensureTransformSelection("HATCH"))return;
        new AlertDialog.Builder(this)
            .setTitle("HATCH • Tarama")
            .setItems(new String[]{"SOLID • Dolu tarama","ANSI31 • 45° çizgili tarama"},(d,which)->{
                if(which==0){
                    if(cad.hatchSelected("SOLID",0f,1f))result.setText("HATCH • SOLID tarama oluşturuldu");
                    else result.setText("HATCH • Kapalı POLYLINE, RECTANGLE, CIRCLE veya ELLIPSE seçin");
                    return;
                }
                LinearLayout box=new LinearLayout(this);box.setOrientation(LinearLayout.VERTICAL);int p=dp(16);box.setPadding(p,p/2,p,p/2);
                EditText angle=new EditText(this);angle.setHint("Ek açı (derece)");angle.setText("0");angle.setInputType(android.text.InputType.TYPE_CLASS_NUMBER|android.text.InputType.TYPE_NUMBER_FLAG_DECIMAL|android.text.InputType.TYPE_NUMBER_FLAG_SIGNED);box.addView(angle);
                EditText spacing=new EditText(this);spacing.setHint("Çizgi aralığı / ölçek");spacing.setText("10");spacing.setInputType(android.text.InputType.TYPE_CLASS_NUMBER|android.text.InputType.TYPE_NUMBER_FLAG_DECIMAL);box.addView(spacing);
                AlertDialog dialog=new AlertDialog.Builder(this)
                    .setTitle("ANSI31 tarama ayarı")
                    .setView(box)
                    .setPositiveButton("OLUŞTUR",null)
                    .setNegativeButton("İPTAL",null)
                    .create();
                dialog.setOnShowListener(x->dialog.getButton(AlertDialog.BUTTON_POSITIVE).setOnClickListener(v->{
                    try{
                        float a=Float.parseFloat(angle.getText().toString().trim().replace(',','.'));
                        float sc=Float.parseFloat(spacing.getText().toString().trim().replace(',','.'));
                        if(!Float.isFinite(sc)||sc<=0f){spacing.setError("Sıfırdan büyük bir aralık girin");return;}
                        if(cad.hatchSelected("ANSI31",a,sc)){dialog.dismiss();result.setText("HATCH • ANSI31 tarama oluşturuldu");}
                        else {dialog.dismiss();result.setText("HATCH • Kapalı POLYLINE, RECTANGLE, CIRCLE veya ELLIPSE seçin");}
                    }catch(Exception e){spacing.setError("Geçerli açı ve aralık değerleri girin");}
                }));
                dialog.show();
            })
            .setNegativeButton("İPTAL",null)
            .show();
    }

    private void runDivideCommand(){
        if(!ensureTransformSelection("DIVIDE"))return;
        EditText input=new EditText(this);input.setSingleLine(true);input.setHint("Parça sayısı (2–200)");input.setText("2");input.setInputType(android.text.InputType.TYPE_CLASS_NUMBER);
        AlertDialog dialog=new AlertDialog.Builder(this).setTitle("DIVIDE • Eşit böl").setMessage("Seçili çizgi veya daire üzerine eşit aralıklı nokta nesneleri yerleştirir.").setView(input).setPositiveButton("UYGULA",null).setNegativeButton("İPTAL",null).create();
        dialog.setOnShowListener(d->dialog.getButton(AlertDialog.BUTTON_POSITIVE).setOnClickListener(v->{
            try{
                int segments=Integer.parseInt(input.getText().toString().trim());
                if(segments<2||segments>200){input.setError("2 ile 200 arasında değer girin");return;}
                int added=cad.divideSelectedEntity(segments);
                if(added<=0){input.setError("DIVIDE yalnız çizgi veya daire üzerinde uygulanabilir");return;}
                dialog.dismiss();result.setText("DIVIDE • "+added+" nokta eklendi");
            }catch(Exception e){input.setError("Geçerli bir parça sayısı girin");}
        }));dialog.show();
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
            "H / HATCH • SOLID veya ANSI31 tarama oluştur\n"+
            "S / STRETCH • Seçili vertex/köşeyi yeni konuma taşı\n"+
            "B / BLOCK • Seçili nesneden isimli blok oluştur\n"+
            "I / INSERT • Oluşturulan bloğu yerleştir\n"+
            "D / DIMSTYLE • Ölçülendirme stilini ayarla\n"+
            "DLI / DIMLINEAR • Yatay/dikey doğrusal ölçü\n"+
            "DAL / DIMALIGNED • Hizalı ölçü\n"+
            "LA / LAYER • Katman\n"+
            "PR / PROPERTIES / PROP • Özellik/Bilgi\n"+
            "DI / DIST / DISTANCE • Mesafe\n"+
            "AA / AREA • Alan\n"+
            "Z E / ZE / ZOOM EXTENTS • Ekrana sığdır\n"+
            "Z / ZOOM • Zoom komutu\n"+
            "U / UNDO • Geri al\n"+
            "REDO • Geri alınan işlemi yeniden uygula\n"+
            "QS / QSAVE / SAVE • Kaydet";
        new AlertDialog.Builder(this)
            .setTitle("MusaCAD komutları")
            .setMessage(text)
            .setPositiveButton("TAMAM",null)
            .show();
    }

    private void noSourceSelection(){Toast.makeText(this,"Önce Seç ile düzenlenebilir bir kaynak nesne seçin",Toast.LENGTH_SHORT).show();}

    private void handleBackNavigation(){
        if(activeLoad!=null){cancelLoad();Toast.makeText(this,"Devam eden işlem iptal edildi",Toast.LENGTH_SHORT).show();return;}
        if(currentProject!=null){
            ProjectSession project=currentProject;
            if(isProjectDirty(project)){
                new AlertDialog.Builder(this).setTitle("Kaydedilmemiş değişiklikler")
                    .setMessage("Bu projeden çıkmadan önce değişiklikleri kaydetmek ister misiniz?")
                    .setPositiveButton("KAYDET VE ÇIK",(d,w)->{pendingCloseAfterSave=project;requestEditedDxfSave();})
                    .setNeutralButton("KAYDETMEDEN ÇIK",(d,w)->closeProjectNow(project))
                    .setNegativeButton("İPTAL",null).show();
            }else closeProjectNow(project);
            return;
        }
        finish();
    }

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
        menu.add(0,MENU_NEW_PROJECT,0,"Yeni Proje Aç");menu.add(0,MENU_OPEN,1,"Dosya aç");menu.add(0,MENU_LAYERS,2,"Katmanlar").setEnabled(activeDxf!=null);menu.add(0,MENU_LAYOUTS,3,"Model / Layout").setEnabled(activeDxf!=null&&activeDxf.layoutNames.size()>1);menu.add(0,MENU_FIT,4,"Ekrana sığdır").setEnabled(currentFile!=null);menu.add(0,MENU_SAVE_DXF,5,"Kaydet / DXF dışa aktar").setEnabled(canEdit());menu.add(0,MENU_PRINT,6,"Yazdır").setEnabled(currentFile!=null);menu.add(0,MENU_SHARE,7,"Paylaş").setEnabled(currentFile!=null);menu.add(0,MENU_INFO,8,"Çizim bilgileri").setEnabled(activeDxf!=null);menu.add(0,MENU_ABOUT,9,"Geliştirici / Hakkında");
        popup.setOnMenuItemClickListener(item->{switch(item.getItemId()){case MENU_NEW_PROJECT:showNewProjectSheet();return true;case MENU_OPEN:open();return true;case MENU_LAYERS:showLayers();return true;case MENU_LAYOUTS:showLayouts();return true;case MENU_FIT:cad.fitToScreen();return true;case MENU_SAVE_DXF:requestEditedDxfSave();return true;case MENU_PRINT:printDrawing();return true;case MENU_SHARE:showShare();return true;case MENU_INFO:showDrawingInfo();return true;case MENU_ABOUT:startActivity(new Intent(this,AboutActivity.class));return true;default:return false;}});popup.show();
    }

    private void showDrawingInfo(){
        if(activeDxf==null)return;String shx=activeDxf.fontFallbacks.isEmpty()?"yok":android.text.TextUtils.join(", ",activeDxf.fontFallbacks);String text="Dosya başarıyla açıldı.\n\n"+"Layout: "+activeDxf.activeLayout+" ("+activeDxf.layoutNames.size()+")\n"+"Nesne: "+activeDxf.entityCount+"\n"+"Katman: "+activeDxf.layerCount+"\n"+"Görünür katman: "+activeDxf.visibleLayers.size()+"\n"+"Seçilebilir kaynak nesne: "+activeDxf.editableSourceCount()+"\n"+"Düzenleme toplamı: "+cad.editCount()+"\n"+"Kaynak nesne değişikliği: "+cad.sourceModifiedCount()+"\n"+"SHX metin fallback: "+shx+"\n"+"Complex SHX shape fallback: "+(activeDxf.externalShapeFallback?"var":"yok")+"\n"+"Düzenleme: "+(canEdit()?"açık":"yalnız görüntüleme")+"\n"+"Görüntüleme: vektörel / net yakınlaştırma";
        new AlertDialog.Builder(this).setTitle("Çizim bilgileri").setMessage(text).setPositiveButton("TAMAM",null).show();
    }

    private void updateShareEnabled(boolean enabled){shareButton.setEnabled(enabled);shareButton.setAlpha(enabled?1f:.45f);shareToolButton.setEnabled(enabled);shareToolButton.setAlpha(enabled?1f:.55f);}
    private void updateLayerButtons(boolean enabled){
        int[] ids={R.id.layersButton,R.id.bottomLayersButton,R.id.rightLayersButton};
        for(int id:ids){View v=findViewById(id);if(v!=null){v.setEnabled(enabled);v.setAlpha(1f);}}
    }
    private void updateEditorEnabled(boolean enabled){
        int[] ids={R.id.propertiesButton,R.id.colorButton,R.id.lineTypeButton,R.id.pointButton,R.id.hatchButton,R.id.selectEntityButton,R.id.moveEntityButton,R.id.rotateEntityButton,R.id.copyEntityButton,R.id.deleteEntityButton,R.id.lineButton,R.id.polylineButton,R.id.rectangleButton,R.id.circleButton,R.id.textButton,R.id.finishEditButton,R.id.saveDxfButton};
        for(int id:ids){View v=findViewById(id);v.setEnabled(enabled);v.setAlpha(enabled?1f:.45f);}
        if(editStatusText!=null){
            if(currentFile==null){editStatusText.setText("Dosya yok");editStatusText.setTextColor(0xFF8FB7C5);}
            else if(enabled){editStatusText.setText("● Düzenlenebilir");editStatusText.setTextColor(0xFF63E6BE);}
            else {editStatusText.setText("Salt görüntüleme");editStatusText.setTextColor(0xFFFFC766);}
        }
    }
    private void selectCategory(int id){
        if(categoryButtons==null)return;
        View selected=null;
        for(int categoryId:categoryButtons){
            View button=findViewById(categoryId);
            if(button!=null){button.setSelected(categoryId==id);if(categoryId==id)selected=button;}
        }
        if(selected!=null&&categoryScroll!=null){
            final View target=selected;
            categoryScroll.post(()->{
                int center=target.getLeft()+target.getWidth()/2-categoryScroll.getWidth()/2;
                categoryScroll.smoothScrollTo(Math.max(0,center),0);
            });
        }
    }
    private void openCategory(int id,Runnable action){
        selectCategory(id);
        View button=findViewById(id);if(button!=null)button.performHapticFeedback(HapticFeedbackConstants.KEYBOARD_TAP);
        if(action!=null)action.run();
    }

    private void selectMode(int id,CadView.Mode mode){View button=findViewById(id);if(button!=null)button.performHapticFeedback(HapticFeedbackConstants.KEYBOARD_TAP);cad.setMode(mode);markModeSelected(id);}
    private void selectEditMode(int id,CadView.Mode mode){if(!canEdit()){Toast.makeText(this,"Bu çizim düzenleme için vektörel olarak açılamadı",Toast.LENGTH_SHORT).show();return;}selectMode(id,mode);}
    private void markModeSelected(int id){if(modeButtons==null)return;for(View button:modeButtons)button.setSelected(button.getId()==id);}
    private void setEditorChromeVisible(boolean visible){
        int state=visible?View.VISIBLE:View.GONE;
        int[] ids={R.id.fileModeBar,R.id.quickToolsBar,R.id.commandBar,R.id.categoryScroll,R.id.rightToolRail,R.id.resultText};
        for(int id:ids){View v=findViewById(id);if(v!=null)v.setVisibility(state);}
        if(!visible)hideToolPanel();
    }
    private void showHomeUi(){
        setEditorChromeVisible(false);
        if(welcomePanel!=null){welcomePanel.setVisibility(View.VISIBLE);welcomePanel.setAlpha(1f);}
    }
    private void hideWelcomePanel(){
        setEditorChromeVisible(true);
        if(welcomePanel==null||welcomePanel.getVisibility()!=View.VISIBLE)return;
        welcomePanel.animate().alpha(0f).setDuration(180).withEndAction(()->{welcomePanel.setVisibility(View.GONE);welcomePanel.setAlpha(1f);}).start();
    }

    private void showLicense(){
        String license;try(InputStream in=getAssets().open("COPYING-LibreDWG.txt")){ByteArrayOutputStream out=new ByteArrayOutputStream();byte[] bytes=new byte[4096];int n;while((n=in.read(bytes))!=-1)out.write(bytes,0,n);license=out.toString("UTF-8");}catch(IOException e){license="GPL-3.0-or-later";}
        TextView text=new TextView(this);text.setPadding(24,16,24,16);text.setText("MusaCAD — LibreDWG ile çevrimdışı DWG okuma\nKaynak kod: https://github.com/santiye2019-lab/MusaCAD\n\n"+license);android.text.util.Linkify.addLinks(text,android.text.util.Linkify.WEB_URLS);text.setMovementMethod(android.text.method.LinkMovementMethod.getInstance());ScrollView scroll=new ScrollView(this);scroll.addView(text);new AlertDialog.Builder(this).setTitle("Lisans ve kaynak kod").setView(scroll).setPositiveButton("KAPAT",null).show();
    }

    private void open(){open(false);}
    private void open(boolean directPicker){
        if(activeLoad!=null){Toast.makeText(this,"Devam eden işlem bitmeden yeni dosya açılamaz",Toast.LENGTH_SHORT).show();return;}
        if(hasPreparingProject()){Toast.makeText(this,"Açık DWG'nin tam düzenleme modeli hazırlanıyor; mevcut sekmeler kullanılabilir",Toast.LENGTH_SHORT).show();return;}
        Intent intent=new Intent(this,RecentFilesActivity.class);
        if(directPicker)intent.putExtra(RecentFilesActivity.EXTRA_PICK_IMMEDIATELY,true);
        startActivityForResult(intent,OPEN);
    }
    @Override protected void onActivityResult(int r,int c,Intent data){
        super.onActivityResult(r,c,data);
        if(r==SAVE_DXF&&c!=RESULT_OK){pendingCloseAfterSave=null;return;}
        if(r==OPEN&&c!=RESULT_OK)pendingHomeCategory=0;
        if(c!=RESULT_OK||data==null||data.getData()==null)return;
        if(r==PICK_AUDIO||r==PICK_IMAGE||r==PICK_VIDEO){handleMediaPicked(r,data.getData());return;}
        if(r==OPEN)startLoad(data.getData());else if(r==SAVE_DXF)saveEditedDxf(data.getData());
    }
    private void openHomeCategory(int groupId){pendingHomeCategory=groupId;open();}
    private void showPendingHomeCategory(){
        int id=pendingHomeCategory;pendingHomeCategory=0;
        if(id==R.id.groupAnnotateToolsButton)openCategory(id,this::showAnnotationToolsSheet);
        else if(id==R.id.groupLineToolsButton)openCategory(id,this::showLineToolsSheet);
        else if(id==R.id.groupEditToolsButton)openCategory(id,this::showEditToolsSheet);
        else if(id==R.id.groupLayerToolsButton)openCategory(id,this::showLayerToolsPanel);
        else if(id==R.id.groupMeasureToolsButton)openCategory(id,this::showMeasureToolsSheet);
        else if(id==R.id.groupDimensionToolsButton)openCategory(id,this::showDimensionToolsPanel);
        else if(id==R.id.groupColorToolsButton)openCategory(id,this::showColorToolsPanel);
        else if(id==R.id.groupMoreToolsButton)openCategory(id,this::showOtherToolsSheet);
        else if(id==R.id.groupLayoutToolsButton)openCategory(id,this::showLayoutToolsPanel);
        else if(id==R.id.groupViewToolsButton)openCategory(id,this::showViewToolsSheet);
    }
    private void cancelLoad(){LoadTask task=activeLoad;activeLoad=null;if(task!=null){if(task.future!=null)task.future.cancel(true);if(task.dialog!=null)task.dialog.dismiss();}}

    private void startLoad(Uri uri){
        if(uri==null)return;
        ProjectSession existing=findProject(uri);
        if(existing!=null){activateProject(existing);return;}
        if(hasPreparingProject()){Toast.makeText(this,"Açık DWG'nin tam düzenleme modeli hazırlanıyor; yeni dosya bundan sonra açılabilir",Toast.LENGTH_SHORT).show();return;}
        if(projects.size()>=MAX_OPEN_PROJECTS){
            Toast.makeText(this,"Performans için aynı anda en fazla "+MAX_OPEN_PROJECTS+" proje açık tutulur. Bir sekmeye uzun basıp kapatın.",Toast.LENGTH_LONG).show();return;
        }
        cancelLoad();LoadTask task=new LoadTask();activeLoad=task;LinearLayout box=new LinearLayout(this);box.setOrientation(LinearLayout.VERTICAL);int pad=dp(20);box.setPadding(pad,pad,pad,pad);box.addView(new ProgressBar(this));task.progress=new TextView(this);task.progress.setText("Dosya okunuyor…");box.addView(task.progress);
        task.dialog=new AlertDialog.Builder(this).setTitle("Çizim açılıyor").setView(box).setNegativeButton("İPTAL",(d,w)->cancelLoad()).create();task.dialog.setOnCancelListener(d->cancelLoad());task.dialog.setCanceledOnTouchOutside(false);task.dialog.show();
        task.future=loader.submit(()->{
            Loaded loaded=new Loaded();loaded.sourceUri=uri;
            try{
                FileTransfer.checkCancelled();loaded.name=nameOf(uri);loaded.dxf=loaded.name.toLowerCase(Locale.ROOT).endsWith(".dxf");loaded.file=File.createTempFile("MusaCAD_acilan_",loaded.dxf?".dxf":".dwg",getCacheDir());
                try(InputStream in=getContentResolver().openInputStream(uri);OutputStream out=new FileOutputStream(loaded.file)){FileTransfer.copy(in,out,512L*1024*1024,bytes->runOnUiThread(()->{if(activeLoad==task&&task.dialog!=null&&task.dialog.isShowing())task.progress.setText(String.format(Locale.getDefault(),"Okunan: %.1f MB",bytes/1048576d));}));}
                FileTransfer.checkCancelled();

                if(loaded.dxf){
                    runOnUiThread(()->{if(activeLoad==task)task.progress.setText("Vektör çizim hazırlanıyor…");});
                    loaded.parsed=DxfParser.render(loaded.file);loaded.workingDxf=loaded.file;loaded.bitmap=loaded.parsed==null?null:loaded.parsed.bitmap;
                    if(loaded.bitmap==null)throw new IOException("Desteklenen DXF geometrisi bulunamadı");
                    Bitmap recentPreview=loaded.bitmap;
                    runOnUiThread(()->{
                        if(activeLoad!=task||isFinishing()||isDestroyed()){loaded.dispose();return;}activeLoad=null;task.dialog.dismiss();
                        ProjectSession project=new ProjectSession();project.sourceUri=loaded.sourceUri;project.file=loaded.file;project.workingDxf=loaded.workingDxf;project.bitmap=loaded.bitmap;project.parsed=loaded.parsed;project.name=loaded.name;project.dxf=true;project.lastAccessMs=System.currentTimeMillis();
                        loaded.project=project;loaded.handedOff=true;projects.add(project);activateProject(project);project.savedFingerprint=cad.editFingerprint();project.baselineSet=true;project.dirty=false;refreshProjectTabs();
                    });
                    RecentFileStore.record(getApplicationContext(),uri,loaded.name,recentPreview);
                    return;
                }

                runOnUiThread(()->{if(activeLoad==task)task.progress.setText("Native DWG motoru açılıyor…");});
                try(NativeCadEngine engine=NativeCadEngine.open(loaded.file)){
                    NativeScene fast=null;
                    try{fast=engine.fastScene();}catch(IOException|OutOfMemoryError ignored){}
                    Bitmap embeddedPreview=null;
                    if(fast==null){try{embeddedPreview=DwgPreview.read(loaded.file);}catch(Exception ignored){}}
                    if(fast!=null||embeddedPreview!=null){
                        loaded.nativeScene=fast;loaded.bitmap=embeddedPreview;
                        ProjectSession project=new ProjectSession();project.sourceUri=loaded.sourceUri;project.file=loaded.file;project.nativeScene=fast;project.bitmap=embeddedPreview;project.name=loaded.name;project.dxf=false;project.preparingEditor=true;project.lastAccessMs=System.currentTimeMillis();
                        loaded.project=project;
                        java.util.concurrent.CountDownLatch attached=new java.util.concurrent.CountDownLatch(1);
                        java.util.concurrent.atomic.AtomicBoolean accepted=new java.util.concurrent.atomic.AtomicBoolean(false);
                        runOnUiThread(()->{
                            try{
                                if(activeLoad!=task||isFinishing()||isDestroyed())return;
                                if(task.dialog!=null)task.dialog.dismiss();
                                activeLoad=null;project.prepareTask=task;
                                projects.add(project);activateProject(project);project.savedFingerprint=cad.editFingerprint();project.baselineSet=true;project.dirty=false;refreshProjectTabs();
                                String partial=project.nativeScene!=null&&project.nativeScene.truncated?" • hızlı sahne kısmi":"";
                                result.setText((project.nativeScene!=null?"Native hızlı görünüm":"Hızlı DWG önizleme")+" hazır"+partial+" • Tam vektör ve düzenleme araçları hazırlanıyor…");accepted.set(true);
                            }finally{attached.countDown();}
                        });
                        try{attached.await();}catch(InterruptedException interrupted){Thread.currentThread().interrupt();throw new InterruptedIOException("Dosya açma iptal edildi");}
                        if(!accepted.get())throw new InterruptedIOException("Dosya açma iptal edildi");
                        loaded.handedOff=true;
                        Bitmap quickRecent=loaded.bitmap;boolean recycleQuick=false;
                        if(loaded.nativeScene!=null){try{quickRecent=loaded.nativeScene.thumbnail(360,240);recycleQuick=true;}catch(Exception ignored){}}
                        RecentFileStore.record(getApplicationContext(),uri,loaded.name,quickRecent);
                        if(recycleQuick&&quickRecent!=null&&!quickRecent.isRecycled())quickRecent.recycle();
                    }

                    FileTransfer.checkCancelled();
                    runOnUiThread(()->{if(activeLoad==task&&task.dialog!=null&&task.dialog.isShowing())task.progress.setText("Tam vektör model hazırlanıyor…");});
                    File converted=File.createTempFile("MusaCAD_donusen_",".dxf",getCacheDir());boolean keep=false;
                    try{
                        int status=engine.exportDxf(converted);FileTransfer.checkCancelled();
                        if(converted.length()>512L*1024*1024)throw new IOException("Dönüştürülen çizim 512 MB sınırını aşıyor");
                        DxfParser.Result parsed=DxfParser.render(converted,loaded.nativeScene==null);if(parsed==null)throw new IOException("DWG içinde desteklenen 2B nesne bulunamadı");parsed.conversionWarnings=status;
                        loaded.parsed=parsed;loaded.workingDxf=converted;loaded.bitmap=parsed.bitmap;keep=true;
                    }finally{if(!keep)converted.delete();}
                }

                if(loaded.handedOff&&loaded.project!=null){
                    ProjectSession project=loaded.project;DxfParser.Result parsed=loaded.parsed;File working=loaded.workingDxf;Bitmap recentPreview=parsed.bitmap;
                    RecentFileStore.record(getApplicationContext(),uri,loaded.name,recentPreview);
                    runOnUiThread(()->{
                        if(isFinishing()||isDestroyed()){if(working!=null)working.delete();if(parsed.bitmap!=null&&!parsed.bitmap.isRecycled())parsed.bitmap.recycle();return;}
                        if(!projects.contains(project)){if(working!=null)working.delete();if(parsed.bitmap!=null&&!parsed.bitmap.isRecycled())parsed.bitmap.recycle();if(activeLoad==task)activeLoad=null;return;}
                        Bitmap oldPreview=project.bitmap;
                        project.prepareTask=null;project.workingDxf=working;project.parsed=parsed;project.bitmap=parsed.bitmap;project.preparingEditor=false;project.prepareError=null;
                        if(currentProject==project){
                            editingBaseDxf=working;activeDxf=parsed;cad.upgradeNativeDrawing(parsed);snapToggle.setEnabled(parsed.snapPoints.length>0);snapToggle.setChecked(true);cad.setSnapPoints(parsed.snapPoints);updateEditorEnabled(canEdit());updateLayerButtons(true);renderCurrentProjectStatus();
                            project.savedFingerprint=cad.editFingerprint();project.baselineSet=true;project.dirty=false;
                        }else{
                            if(project.nativeScene!=null)project.nativeScene.alignTo(parsed.drawingToContentMatrix());
                            project.viewState=null;
                        }
                        if(oldPreview!=null&&oldPreview!=parsed.bitmap&&!oldPreview.isRecycled())oldPreview.recycle();
                        if(activeLoad==task)activeLoad=null;refreshProjectTabs();
                    });
                    return;
                }

                if(loaded.bitmap==null)throw new IOException("DWG içinde görüntülenebilir geometri bulunamadı");
                Bitmap recentPreview=loaded.bitmap;
                runOnUiThread(()->{
                    if(activeLoad!=task||isFinishing()||isDestroyed()){loaded.dispose();return;}activeLoad=null;if(task.dialog!=null)task.dialog.dismiss();
                    ProjectSession project=new ProjectSession();project.sourceUri=loaded.sourceUri;project.file=loaded.file;project.workingDxf=loaded.workingDxf;project.bitmap=loaded.bitmap;project.parsed=loaded.parsed;project.name=loaded.name;project.dxf=false;project.lastAccessMs=System.currentTimeMillis();
                    loaded.project=project;loaded.handedOff=true;projects.add(project);activateProject(project);project.savedFingerprint=cad.editFingerprint();project.baselineSet=true;project.dirty=false;refreshProjectTabs();
                });
                RecentFileStore.record(getApplicationContext(),uri,loaded.name,recentPreview);
            }catch(Exception|OutOfMemoryError e){
                if(loaded.handedOff&&loaded.project!=null){
                    ProjectSession project=loaded.project;String message=e.getMessage()==null?"Tam vektör model hazırlanamadı":e.getMessage();
                    if(loaded.workingDxf!=null&&loaded.workingDxf!=project.workingDxf)loaded.workingDxf.delete();
                    runOnUiThread(()->{
                        if(activeLoad==task)activeLoad=null;
                        if(projects.contains(project)){project.prepareTask=null;project.preparingEditor=false;project.prepareError=message;if(currentProject==project){updateEditorEnabled(false);renderCurrentProjectStatus();Toast.makeText(this,"Hızlı görünüm açık; düzenleme modeli hazırlanamadı",Toast.LENGTH_LONG).show();}}
                    });
                }else{
                    loaded.dispose();runOnUiThread(()->{if(activeLoad!=task||isFinishing()||isDestroyed())return;activeLoad=null;if(task.dialog!=null)task.dialog.dismiss();error(e instanceof Exception?(Exception)e:new IOException("Bu çizim için yeterli bellek yok"));});
                }
            }
        });
    }

    private ProjectSession findProject(Uri uri){
        if(uri==null)return null;String key=uri.toString();
        for(ProjectSession p:projects)if(p.sourceUri!=null&&key.equals(p.sourceUri.toString()))return p;
        return null;
    }
    private boolean hasPreparingProject(){for(ProjectSession p:projects)if(p.preparingEditor)return true;return false;}

    private void captureCurrentProject(){
        if(currentProject==null)return;
        currentProject.file=currentFile;currentProject.workingDxf=editingBaseDxf;currentProject.parsed=activeDxf;currentProject.name=currentDisplayName;
        currentProject.viewState=cad.captureSessionState();currentProject.dirty=currentProject.baselineSet&&cad.editFingerprint()!=currentProject.savedFingerprint;currentProject.lastAccessMs=System.currentTimeMillis();
        // The vector preview bitmap is not used for zoom rendering; recycle it for inactive tabs to reduce RAM pressure.
        if(currentProject.parsed!=null&&currentProject.parsed.bitmap!=null&&!currentProject.parsed.bitmap.isRecycled())currentProject.parsed.bitmap.recycle();
    }

    private void activateProject(ProjectSession project){
        if(project==null||activeLoad!=null)return;
        if(project==currentProject){refreshProjectTabs();return;}
        captureCurrentProject();
        currentProject=project;currentFile=project.file;editingBaseDxf=project.workingDxf;activeDxf=project.parsed;currentDisplayName=project.name==null?"cizim.dwg":project.name;project.lastAccessMs=System.currentTimeMillis();
        if(project.viewState!=null){
            if(project.parsed!=null)cad.restoreSessionState(project.parsed,project.nativeScene,null,project.viewState);
            else if(project.nativeScene!=null)cad.restoreNativeSessionState(project.nativeScene,project.viewState);
            else cad.restoreSessionState(null,project.bitmap,project.viewState);
        }else if(project.parsed!=null){if(project.nativeScene!=null)cad.restoreSessionState(project.parsed,project.nativeScene,null,null);else cad.setVectorDrawing(project.parsed);}else if(project.nativeScene!=null)cad.setNativeDrawing(project.nativeScene);else cad.setDrawing(project.bitmap);
        hideWelcomePanel();markModeSelected(R.id.panButton);
        snapToggle.setEnabled(project.parsed!=null&&project.parsed.snapPoints.length>0);snapToggle.setChecked(true);if(project.parsed!=null)cad.setSnapPoints(project.parsed.snapPoints);
        updateShareEnabled(true);updateEditorEnabled(canEdit());updateLayerButtons(activeDxf!=null);renderCurrentProjectStatus();refreshProjectTabs();
        if(pendingHomeCategory!=0)cad.postDelayed(this::showPendingHomeCategory,220);
    }

    private void renderCurrentProjectStatus(){
        if(currentProject==null){fileName.setText("Henüz proje açılmadı");result.setText("Hazır");return;}
        String editable=canEdit()?"  •  düzenlenebilir":"";
        String mode=currentProject.dxf?"  •  DXF":activeDxf!=null?"  •  DWG":currentProject.nativeScene!=null?"  •  DWG Native":"  •  DWG önizleme";
        fileName.setText(currentDisplayName+mode+editable);
        if(activeDxf!=null){
            String fallback=(activeDxf.fontFallbacks.isEmpty()&&!activeDxf.externalShapeFallback)?"":"  •  SHX fallback";
            result.setText("Hazır  •  "+activeDxf.activeLayout+"  •  "+activeDxf.entityCount+" nesne  •  "+activeDxf.layerCount+" katman  •  "+activeDxf.editableSourceCount()+" seçilebilir"+(canEdit()?"  •  düzenleme açık":"")+fallback);
        }else if(currentProject.nativeScene!=null){
            if(currentProject.preparingEditor)result.setText("Hazır  •  Native hızlı görünüm  •  "+currentProject.nativeScene.primitiveCount+" geometri"+(currentProject.nativeScene.truncated?"  •  hızlı sahne kısmi":"")+"  •  tam vektör hazırlanıyor");
            else if(currentProject.prepareError!=null)result.setText("Native görünüm  •  düzenleme modeli kullanılamadı");
            else result.setText("Hazır  •  Native DWG görünümü");
        }else if(currentProject.preparingEditor)result.setText("Hazır  •  Hızlı DWG önizleme  •  tam vektör hazırlanıyor");
        else result.setText("Hazır  •  DWG önizleme modu");
    }

    private void refreshProjectTabs(){
        if(projectTabsBox==null||tabFileName==null)return;
        while(projectTabsBox.getChildCount()>1)projectTabsBox.removeViewAt(1);
        tabFileName.setText(currentProject==null?"Dosya açılmadı":currentDisplayName);
        tabFileName.setAlpha(currentProject==null?.65f:1f);
        for(ProjectSession p:projects){
            if(p==currentProject)continue;
            TextView tab=new TextView(this);tab.setText(p.name==null?"Çizim":p.name);tab.setTextColor(0xFFD7EAF4);tab.setTextSize(9f);tab.setGravity(Gravity.CENTER_VERTICAL);tab.setSingleLine(true);tab.setEllipsize(android.text.TextUtils.TruncateAt.MIDDLE);tab.setPadding(dp(10),0,dp(10),0);tab.setAlpha(.74f);tab.setBackgroundResource(R.drawable.feature_chip_bg);
            LinearLayout.LayoutParams lp=new LinearLayout.LayoutParams(dp(132),dp(34));lp.setMarginStart(dp(5));projectTabsBox.addView(tab,lp);
            tab.setOnClickListener(v->activateProject(p));tab.setOnLongClickListener(v->{requestCloseProject(p);return true;});
        }
    }

    private boolean isProjectDirty(ProjectSession p){
        if(p==null)return false;
        if(p==currentProject)return p.baselineSet&&cad.editFingerprint()!=p.savedFingerprint;
        return p.dirty;
    }

    private void requestCloseProject(ProjectSession project){
        if(project==null)return;
        if(project!=currentProject)activateProject(project);
        if(isProjectDirty(project)){
            new AlertDialog.Builder(this).setTitle("Kaydedilmemiş değişiklikler")
                .setMessage("Bu projeyi kapatmadan önce değişiklikleri kaydetmek ister misiniz?")
                .setPositiveButton("KAYDET",(d,w)->{pendingCloseAfterSave=project;requestEditedDxfSave();})
                .setNeutralButton("KAYDETMEDEN KAPAT",(d,w)->closeProjectNow(project))
                .setNegativeButton("İPTAL",null).show();
        }else closeProjectNow(project);
    }

    private void closeProjectNow(ProjectSession project){
        if(project==null)return;
        boolean active=project==currentProject;
        projects.remove(project);
        if(active){
            cad.restoreSessionState(null,null,null);currentProject=null;currentFile=null;editingBaseDxf=null;activeDxf=null;currentDisplayName="cizim.dwg";
        }
        project.dispose();
        if(active&&!projects.isEmpty())activateProject(projects.get(projects.size()-1));
        else if(active){
            updateShareEnabled(false);updateEditorEnabled(false);updateLayerButtons(false);
            showHomeUi();fileName.setText("Henüz proje açılmadı");result.setText("Hazır");
        }
        refreshProjectTabs();
    }

    private void releaseAllProjects(){
        captureCurrentProject();cad.restoreSessionState(null,null,null);currentProject=null;
        for(ProjectSession p:new ArrayList<>(projects))p.dispose();projects.clear();
        activeDxf=null;editingBaseDxf=null;currentFile=null;
    }

    @Override protected void onDestroy(){cancelLoad();releaseAllProjects();loader.shutdownNow();super.onDestroy();}

    private void showLayers(){
        if(activeDxf==null||activeLoad!=null){if(activeDxf==null)Toast.makeText(this,"Katmanlar için önce bir çizim açın",Toast.LENGTH_SHORT).show();return;}
        String[] names=activeDxf.layerNames.toArray(new String[0]);Set<String> selected=new HashSet<>(activeDxf.visibleLayers);boolean[] checked=new boolean[names.length];for(int i=0;i<names.length;i++)checked[i]=selected.contains(names[i]);
        new AlertDialog.Builder(this).setTitle("Görünecek katmanlar").setMultiChoiceItems(names,checked,(dialog,index,enabled)->{if(enabled)selected.add(names[index]);else selected.remove(names[index]);}).setPositiveButton("UYGULA",(d,w)->applyLayers(selected)).setNeutralButton("TÜMÜNÜ GÖSTER",(d,w)->applyLayers(new HashSet<>(activeDxf.layerNames))).setNegativeButton("İPTAL",null).show();
    }

    private void applyLayers(Set<String> selected){
        if(activeDxf==null||activeLoad!=null||activeDxf.visibleLayers.equals(selected))return;
        if(currentProject!=null){currentProject.previousVisibleLayers.clear();currentProject.previousVisibleLayers.addAll(activeDxf.visibleLayers);}DxfParser.Result source=activeDxf;LoadTask task=new LoadTask();activeLoad=task;task.dialog=new AlertDialog.Builder(this).setTitle("Katmanlar hazırlanıyor").setMessage("Görünüm güncelleniyor…").setNegativeButton("İPTAL",(d,w)->cancelLoad()).create();task.dialog.setOnCancelListener(d->cancelLoad());task.dialog.setCanceledOnTouchOutside(false);task.dialog.show();
        task.future=loader.submit(()->{try{DxfParser.Result updated=source.withVisibleLayers(selected);runOnUiThread(()->{if(activeLoad!=task||activeDxf!=source||isFinishing()||isDestroyed()){if(updated.bitmap!=null&&!updated.bitmap.isRecycled())updated.bitmap.recycle();return;}activeLoad=null;task.dialog.dismiss();cad.replaceVisibleDrawing(updated);activeDxf=updated;if(currentProject!=null){currentProject.parsed=updated;currentProject.nativeScene=null;}if(source.bitmap!=null&&!source.bitmap.isRecycled())source.bitmap.recycle();snapToggle.setEnabled(updated.snapPoints.length>0);result.setText("Hazır  •  "+updated.activeLayout+"  •  "+updated.entityCount+" nesne  •  "+updated.visibleLayers.size()+"/"+updated.layerCount+" katman");});}catch(Exception|OutOfMemoryError e){runOnUiThread(()->{if(activeLoad!=task||isFinishing()||isDestroyed())return;activeLoad=null;task.dialog.dismiss();error(e instanceof Exception?(Exception)e:new IOException("Yeterli bellek yok"));});}});
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
        task.future=loader.submit(()->{try{DxfParser.Result updated=source.withLayout(layout);runOnUiThread(()->{if(activeLoad!=task||activeDxf!=source||isFinishing()||isDestroyed()){if(updated.bitmap!=null&&!updated.bitmap.isRecycled())updated.bitmap.recycle();return;}activeLoad=null;task.dialog.dismiss();cad.setVectorDrawing(updated);activeDxf=updated;if(currentProject!=null){currentProject.parsed=updated;currentProject.nativeScene=null;currentProject.viewState=null;}if(source.bitmap!=null&&!source.bitmap.isRecycled())source.bitmap.recycle();markModeSelected(R.id.panButton);snapToggle.setEnabled(updated.snapPoints.length>0);snapToggle.setChecked(true);cad.setSnapPoints(updated.snapPoints);updateEditorEnabled(canEdit());result.setText("Hazır  •  "+updated.activeLayout+"  •  "+updated.entityCount+" nesne  •  "+updated.visibleLayers.size()+"/"+updated.layerCount+" katman  •  "+updated.editableSourceCount()+" seçilebilir");});}catch(Exception|OutOfMemoryError e){runOnUiThread(()->{if(activeLoad!=task||isFinishing()||isDestroyed())return;activeLoad=null;task.dialog.dismiss();error(e instanceof Exception?(Exception)e:new IOException("Layout için yeterli bellek yok"));});}});
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
        if(!canEdit()||activeLoad!=null)return;final File base=editingBaseDxf;final DxfParser.Result drawing=activeDxf;final List<CadEdit> additions=cad.getAddedEdits();final List<SourceReplacement> replacements=cad.getSourceReplacements();final List<SourceRange> removals=cad.getSourceRemovals();final List<CadBlock.Definition> blocks=cad.getUserBlocks();final String defaultLayer=currentProject==null?"0":currentProject.defaultLayer;final int total=additions.size()+removals.size()+blocks.size();
        LoadTask task=new LoadTask();activeLoad=task;LinearLayout box=new LinearLayout(this);box.setOrientation(LinearLayout.VERTICAL);int pad=dp(20);box.setPadding(pad,pad,pad,pad);box.addView(new ProgressBar(this));task.progress=new TextView(this);task.progress.setText("DXF hazırlanıyor…");box.addView(task.progress);task.dialog=new AlertDialog.Builder(this).setTitle("Düzenlenmiş DXF kaydediliyor").setView(box).setNegativeButton("İPTAL",(d,w)->cancelLoad()).create();task.dialog.setOnCancelListener(d->cancelLoad());task.dialog.setCanceledOnTouchOutside(false);task.dialog.show();
        task.future=loader.submit(()->{try(OutputStream out=getContentResolver().openOutputStream(uri,"wt")){if(out==null)throw new IOException("Kaydedilecek dosya açılamadı");DxfWriter.write(base,out,drawing,additions,replacements,removals,blocks,defaultLayer);FileTransfer.checkCancelled();runOnUiThread(()->{if(activeLoad!=task||isFinishing()||isDestroyed())return;activeLoad=null;task.dialog.dismiss();
                    if(currentProject!=null){currentProject.savedFingerprint=cad.editFingerprint();currentProject.baselineSet=true;currentProject.dirty=false;currentProject.viewState=cad.captureSessionState();}
                    Toast.makeText(this,"DXF kaydedildi • "+total+" düzenleme",Toast.LENGTH_LONG).show();
                    ProjectSession close=pendingCloseAfterSave;pendingCloseAfterSave=null;
                    if(close!=null)closeProjectNow(close);});}catch(Exception e){runOnUiThread(()->{if(activeLoad!=task||isFinishing()||isDestroyed())return;activeLoad=null;task.dialog.dismiss();pendingCloseAfterSave=null;error(e);});}});
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
        try{if(activeDxf==null){preview=DwgPreview.read(currentFile);if(preview==null)preview=cad.snapshot();}CadPrint.show(this,activeDxf,cad.getAddedEdits(),cad.getSourceReplacements(),cad.getHiddenSourceIds(),cad.getUserBlocks(),preview,currentDisplayName);}catch(Exception e){if(preview!=null&&!preview.isRecycled())preview.recycle();error(e);}
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
