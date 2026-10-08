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
    private static final int OPEN=20,SAVE_DXF=21,PICK_AUDIO=30,PICK_IMAGE=31,PICK_VIDEO=32,PICK_FONT=33,PICK_DOCUMENT=34,VIEW_DOCUMENT=35,PICK_BOQ=36,PICK_STRUCT_CALC=37;
    private static final int MAX_OPEN_PROJECTS=4;
    private static final int CLOUD_AI_INDEX_MAX_ITEMS=5000;
    private static final String AI_PRIVACY_PREFS="musacad_ai_privacy",K_CLOUD_CONSENT="cloud_cad_json_v1",K_CLOUD_PACKAGE_CONSENT="cloud_cad_package_v1";
    private static final int MENU_OPEN=1,MENU_LAYERS=2,MENU_FIT=3,MENU_SHARE=4,MENU_INFO=5,MENU_ABOUT=6,MENU_SAVE_DXF=7,MENU_PRINT=8,MENU_LAYOUTS=9,MENU_NEW_PROJECT=10;
    private final ExecutorService loader=Executors.newSingleThreadExecutor();
    private final ExecutorService recoveryExecutor=Executors.newSingleThreadExecutor();
    private final ExecutorService aiExecutor=Executors.newSingleThreadExecutor(r->{
        Thread t=new Thread(r,"MusaCAD-cloud-ai");
        t.setPriority(Thread.NORM_PRIORITY-1);
        return t;
    });
    private final ExecutorService recentExecutor=Executors.newSingleThreadExecutor(r->{
        Thread t=new Thread(r,"MusaCAD-recents");
        t.setPriority(Thread.MIN_PRIORITY);
        return t;
    });
    private final android.os.Handler recoveryHandler=new android.os.Handler(android.os.Looper.getMainLooper());
    private static final long RECOVERY_INTERVAL_MS=15_000L;
    private LoadTask activeLoad;
    private boolean recoveryPromptShown;
    private final Runnable recoveryTicker=new Runnable(){
        @Override public void run(){
            queueRecoveryForCurrent(false);
            recoveryHandler.postDelayed(this,RECOVERY_INTERVAL_MS);
        }
    };

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
        List<CadImageOverlay> imageOverlays=Collections.emptyList();
        ProjectSession project;
        void dispose(){if(bitmap!=null&&!bitmap.isRecycled())bitmap.recycle();if(workingDxf!=null&&workingDxf!=file)workingDxf.delete();if(file!=null)file.delete();}
    }

    private static final class ProjectSession {
        Uri sourceUri;File file,workingDxf;Bitmap bitmap;DxfParser.Result parsed;NativeScene nativeScene;String name;boolean dxf;
        CadView.SessionState viewState;CadView.ViewBookmark viewBookmark;long savedFingerprint;boolean baselineSet,dirty,preparingEditor;String prepareError;long lastAccessMs;LoadTask prepareTask;
        String recoveryId;long recoveryFingerprint=Long.MIN_VALUE;
        final Set<String> previousVisibleLayers=new HashSet<>();
        final ArrayDeque<String> measurementHistory=new ArrayDeque<>();
        final ArrayList<MediaAttachment> mediaAttachments=new ArrayList<>();
        final ArrayList<CadImageOverlay> persistedImages=new ArrayList<>();
        MusaAiBoq.Model boqModel;
        String boqName="";
        MusaAiStructuralCalc.Model structuralCalcModel;
        String structuralCalcName="";
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

    private static final class RevisionCandidate {
        final MusaAiDrawingIndex index;final String name;
        RevisionCandidate(MusaAiDrawingIndex index,String name){this.index=index;this.name=name==null?"Referans":name;}
    }

    private static final class AiCloudProjectSource {
        final String name;final DxfParser.Result parsed;final MusaAiBoq.Model boq;
        AiCloudProjectSource(String name,DxfParser.Result parsed,MusaAiBoq.Model boq){
            this.name=name==null?"cizim.dwg":name;this.parsed=parsed;this.boq=boq;
        }
    }

    private CheckBox snapToggle;
    private DxfParser.Result activeDxf;
    private DxfParser.Result aiIndexedDxf;
    private MusaAiDrawingIndex aiDrawingIndex,aiRevisionBaseline;
    private String aiRevisionBaselineName="";
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
    private AlertDialog projectCloseDialog;
    private ProjectSession projectCloseTarget;
    private CadEdit crossProjectClipboard;
    private String crossProjectClipboardSource="";
    private String lastCommandRaw="",lastAiReport="",lastAiReportTitle="MusaCAD AI Raporu";
    private List<Integer> lastAiReportSourceIds=Collections.emptyList();
    private MusaAiPanel.Reply pendingBoqReply;
    private ProjectSession pendingBoqProject;
    private MusaAiPanel.Reply pendingStructuralCalcReply;
    private ProjectSession pendingStructuralCalcProject;
    private volatile List<MusaAiCloudService.Action> pendingAiActions=Collections.emptyList();
    private CadView.SessionState lastGandalfBatchState;
    private ProjectSession lastGandalfBatchProject;
    private long lastGandalfBatchFingerprint=Long.MIN_VALUE;
    private int pendingHomeCategory;
    private boolean pendingPrintWindowSelection;
    private Uri homeFeaturedUri;

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
            public void onSelectionReady(){if(pendingPrintWindowSelection){pendingPrintWindowSelection=false;printDrawing(true);}else previewSelection();}
            public void onTextRequested(float x,float y){showTextEditor(x,y);}
        });

        snapToggle=findViewById(R.id.snapToggle);snapToggle.setOnCheckedChangeListener((button,checked)->cad.setSnapEnabled(checked));
        modeButtons=new View[]{findViewById(R.id.panButton),findViewById(R.id.selectEntityButton),findViewById(R.id.calibrateButton),findViewById(R.id.distanceButton),findViewById(R.id.areaButton),findViewById(R.id.lineButton),findViewById(R.id.polylineButton),findViewById(R.id.rectangleButton),findViewById(R.id.circleButton),findViewById(R.id.pointButton),findViewById(R.id.textButton)};
        markModeSelected(R.id.panButton);
        categoryButtons=new int[]{R.id.groupAnnotateToolsButton,R.id.groupLineToolsButton,R.id.groupEditToolsButton,R.id.groupLayerToolsButton,R.id.groupMeasureToolsButton,R.id.groupDimensionToolsButton,R.id.groupColorToolsButton,R.id.groupMoreToolsButton,R.id.groupLayoutToolsButton,R.id.groupViewToolsButton};

        int[] interactive={R.id.menuButton,R.id.openButton,R.id.shareButton,R.id.headerMoreButton,R.id.quickOpenButton,R.id.newProjectButton,R.id.layersButton,R.id.propertiesButton,R.id.colorButton,R.id.lineTypeButton,R.id.pointButton,R.id.bottomLayersButton,R.id.rightLayersButton,R.id.snapToggle,R.id.panButton,R.id.selectEntityButton,R.id.moveEntityButton,R.id.rotateEntityButton,R.id.copyEntityButton,R.id.deleteEntityButton,R.id.calibrateButton,R.id.distanceButton,R.id.bottomMeasureButton,R.id.hatchButton,R.id.moreToolsButton,R.id.areaButton,R.id.lineButton,R.id.polylineButton,R.id.rectangleButton,R.id.circleButton,R.id.textButton,R.id.finishEditButton,R.id.saveDxfButton,R.id.zoomInButton,R.id.zoomOutButton,R.id.rightZoomInButton,R.id.rightZoomOutButton,R.id.right3dButton,R.id.aiButton,R.id.fitButton,R.id.rightFitButton,R.id.undoButton,R.id.clearButton,R.id.shareToolButton,R.id.commandSendButton,R.id.toolPanelClose,R.id.closeFileButton,R.id.nativeModeChip,R.id.sceneModeChip,R.id.groupLayerToolsButton,R.id.groupDimensionToolsButton,R.id.groupColorToolsButton,R.id.groupLayoutToolsButton,R.id.groupLineToolsButton,R.id.groupShapeToolsButton,R.id.groupEditToolsButton,R.id.groupMeasureToolsButton,R.id.groupViewToolsButton,R.id.groupAnnotateToolsButton,R.id.groupMoreToolsButton};
        for(int id:interactive)installInteractiveFeedback(findViewById(id));

        findViewById(R.id.menuButton).setOnClickListener(v->{hideToolPanel();showMainMenu(v);});findViewById(R.id.headerMoreButton).setOnClickListener(v->{hideToolPanel();showMainMenu(v);});findViewById(R.id.appTitle).setOnClickListener(v->{hideToolPanel();showMainMenu(v);});
        findViewById(R.id.closeFileButton).setOnClickListener(v->{v.performHapticFeedback(HapticFeedbackConstants.KEYBOARD_TAP);if(currentProject==null)Toast.makeText(this,"Açık proje yok",Toast.LENGTH_SHORT).show();else requestCloseProject(currentProject);});
        findViewById(R.id.nativeModeChip).setOnClickListener(v->{v.performHapticFeedback(HapticFeedbackConstants.KEYBOARD_TAP);if(currentProject==null)result.setText("DWG Native • Önce çizim açın");else if(currentProject.nativeScene!=null)result.setText("DWG Native • hızlı sahne etkin");else result.setText("Vektör görünüm • tam çizim modeli");});
        findViewById(R.id.sceneModeChip).setOnClickListener(v->{v.performHapticFeedback(HapticFeedbackConstants.KEYBOARD_TAP);showViewToolsSheet();});
        findViewById(R.id.right3dButton).setOnClickListener(v->show3dToolsSheet());
        findViewById(R.id.aiButton).setOnClickListener(v->showMusaAi());
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
        findViewById(R.id.homeFeaturedOpen).setOnClickListener(v->openFeaturedRecent());
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
        recoveryHandler.postDelayed(this::offerRecoveryIfIdle,650L);
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
            case DIVIDE:
                runDivideCommand();
                break;
            case REVCLOUD:
                if(!canEdit()){result.setText("REVCLOUD • Önce düzenlenebilir çizim açın");break;}
                cad.setMode(CadView.Mode.DRAW_REVCLOUD);markModeSelected(0);result.setText("REVCLOUD • Bulut alanının iki karşı köşesini seçin");
                break;
            case MULTILEADER:
                if(!canEdit()){result.setText("MLEADER • Önce düzenlenebilir çizim açın");break;}
                cad.setMode(CadView.Mode.DRAW_MULTILEADER);markModeSelected(0);result.setText("MLEADER • Ok ucunu ve metin bağlantı noktasını seçin");
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
            case DIMANGULAR:
                if(!canEdit()){result.setText("DIMANGULAR • Önce düzenlenebilir çizim açın");break;}
                cad.setMode(CadView.Mode.DRAW_DIM_ANGULAR);markModeSelected(0);result.setText("DIMANGULAR • İlk kol, köşe ve ikinci kol için 3 nokta seçin");
                break;
            case DIMRADIUS:
                if(!canEdit()){result.setText("DIMRADIUS • Önce düzenlenebilir çizim açın");break;}
                cad.setMode(CadView.Mode.DRAW_DIM_RADIUS);markModeSelected(0);result.setText("DIMRADIUS • Bir daireye dokunun");
                break;
            case DIMDIAMETER:
                if(!canEdit()){result.setText("DIMDIAMETER • Önce düzenlenebilir çizim açın");break;}
                cad.setMode(CadView.Mode.DRAW_DIM_DIAMETER);markModeSelected(0);result.setText("DIMDIAMETER • Bir daireye dokunun");
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
            case ANGLE:
                cad.setMode(CadView.Mode.ANGLE);markModeSelected(0);result.setText("ANGLE • Köşe ortada olacak şekilde 3 nokta seçin");
                break;
            case ID_POINT:
                cad.setMode(CadView.Mode.ID_POINT);markModeSelected(0);result.setText("ID • Koordinat için bir noktaya dokunun");
                break;
            case ARC_LENGTH:
                cad.setMode(CadView.Mode.ARC_LENGTH);markModeSelected(0);result.setText("ARCLEN • Bir yay veya daireye dokunun");
                break;
            case ZOOM:
                result.setText("ZOOM • Extents için Z E veya ZE kullanın");
                break;
            case ZOOM_EXTENTS:
                cad.fitToScreen();
                result.setText("ZOOM EXTENTS • Çizim ekrana sığdırıldı");
                break;
            case VIEW_3D:
                show3dToolsSheet();
                break;
            case VIEW_2D:
                cad.regenerate();result.setText("2D görünüm etkin");
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
        if(cad.selectedIsImage()){
            new AlertDialog.Builder(this).setTitle("Görüntü Özellikleri").setMessage(cad.selectedEntityInfo())
                .setPositiveButton("TAMAM",null)
                .setNeutralButton("90° DÖNDÜR",(d,w)->{if(cad.rotateSelectedEntity())result.setText("Görüntü • 90° döndürüldü");})
                .show();
            return;
        }
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
        if(cad.selectedIsImage()){result.setText("Görüntü • renk ayarı raster görsele uygulanmaz");return;}
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
        if(cad.selectedIsImage()){result.setText("Görüntü • çizgi tipi raster görsele uygulanmaz");return;}
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

    private void showMusaAi(){
        hideToolPanel();
        MusaAiPanel.show(this,new MusaAiPanel.Host(){
            @Override public String contextLabel(){return musaAiContextLabel();}
            @Override public void onPrompt(String prompt,MusaAiPanel.Reply reply){handleMusaAiPrompt(prompt,reply);}
        });
    }

    private String musaAiContextLabel(){
        if(currentProject==null)return "Bağlam • Henüz proje açık değil";
        if(activeDxf!=null){
            int vectorCount=openVectorProjectCount();
            return "Bağlam • "+currentDisplayName+" • "+activeDxf.entityCount+" nesne • "+activeDxf.layerCount+" katman • "+activeDxf.activeLayout+
                (vectorCount>1?" • "+vectorCount+" açık vektör proje":"");
        }
        if(currentProject.nativeScene!=null){
            return "Bağlam • "+currentDisplayName+" • DWG Native • "+currentProject.nativeScene.primitiveCount+" geometri";
        }
        return "Bağlam • "+currentDisplayName+" • çizim açık";
    }

    private MusaAiDrawingIndex currentAiDrawingIndex(){
        if(activeDxf==null)return null;
        if(aiIndexedDxf!=activeDxf||aiDrawingIndex==null){
            aiDrawingIndex=activeDxf.aiDrawingIndex();
            aiIndexedDxf=activeDxf;
        }
        return aiDrawingIndex;
    }

    private int openVectorProjectCount(){
        int count=0;
        for(ProjectSession p:projects){
            if(p==null)continue;
            DxfParser.Result parsed=(p==currentProject&&activeDxf!=null)?activeDxf:p.parsed;
            if(parsed!=null)count++;
        }
        return count;
    }

    private List<MusaAiProjectPackage.Drawing> currentAiProjectPackageDrawings(){
        ArrayList<MusaAiProjectPackage.Drawing> out=new ArrayList<>();
        for(ProjectSession p:projects){
            if(p==null)continue;
            DxfParser.Result parsed=(p==currentProject&&activeDxf!=null)?activeDxf:p.parsed;
            if(parsed==null)continue;
            String name=p==currentProject?currentDisplayName:p.name;
            out.add(new MusaAiProjectPackage.Drawing(
                name==null?"cizim.dwg":name,
                parsed.aiDrawingIndex(),
                p.boqModel
            ));
        }
        return out;
    }

    private RevisionCandidate findOtherRevisionCandidate(){
        RevisionCandidate found=null;
        for(ProjectSession p:projects){
            if(p==null||p==currentProject||p.parsed==null)continue;
            if(found!=null)return null;
            found=new RevisionCandidate(p.parsed.aiDrawingIndex(),p.name);
        }
        return found;
    }

    private boolean isRevisionBaselineSetCommand(String q){
        return q.contains("bu cizimi referans revizyon yap")||
            q.contains("bu cizimi revizyon referansi yap")||
            q.contains("bu cizimi referans al")||
            q.contains("bu cizimi baz al")||
            q.contains("revizyon referansi kaydet")||
            q.contains("revizyon bazini kaydet");
    }

    private boolean isRevisionBaselineClearCommand(String q){
        return (q.contains("revizyon")||q.contains("referans"))&&
            (q.contains("temizle")||q.contains("sil")||q.contains("sifirla"));
    }

    private void handleMusaAiPrompt(String prompt,MusaAiPanel.Reply reply){
        String raw=prompt==null?"":prompt.trim();
        String q=raw.toLowerCase(new java.util.Locale("tr","TR"));
        if(q.isEmpty()){reply.send("Bir soru veya komut yazın.");return;}
        if(q.contains("ne yapabilir")||q.equals("yardım")||q.equals("help")){
            reply.send("MusaCAD AI yetenekleri:\n• Doğal dille CAD komutları ve çizime soru sorma\n• Metraj, keşif/BOQ yükleme, projeden keşif oluşturma ve karşılaştırma\n• Mimari, statik, mekanik, elektrik, peyzaj, altyapı, asansör ve yangın proje kontrolü\n• Statik proje inceleme raporu, hesap raporu/model çıktısı ↔ DWG karşılaştırması ve Proje Paketi tam denetimi\n• Mekanik tesisat proje kontrolü\n• MEKAI_* yerel mekanik uzman komutları\n• MIMAI / STATIKAI / ELKAI / PEYAI / ALTYAPIAI / ASNAI / YANGAI uzman komutları\n• G ile başlayan uzman komutları Gandalf derin analizine gider\n• GMEKAI_* Gandalf derin mekanik uzman analizi\n• Gandalf Cloud AI ile derin proje analizi\n• Akıllı seçim, tablo/lejant/OLE analizi ve revizyon karşılaştırma\n• Word (.docx) ve PDF teknik rapor çıktısı\n• Sesli komut");
            return;
        }

        if(MusaAiMechanicalExpert.isHelpCommand(raw)){
            reply.send(MusaAiMechanicalExpert.commandHelp());
            return;
        }
        if(MusaAiDisciplineExpert.isHelpCommand(raw)){
            reply.send(MusaAiDisciplineExpert.commandHelp());
            return;
        }

        String aiControl=MusaAiDrawingIndex.normalize(raw);
        if(isGandalfUndoCommand(aiControl)){
            undoLastGandalfBatch(reply);
            return;
        }
        if(isGandalfClearCommand(aiControl)){
            pendingAiActions=Collections.emptyList();
            cad.clearAiHighlights();
            reply.send("Bekleyen Gandalf çizim önerileri temizlendi.");
            return;
        }
        if(isGandalfApplyCommand(aiControl)){
            showPendingGandalfActions(reply,true);
            return;
        }
        if(isGandalfPreviewCommand(aiControl)){
            showPendingGandalfActions(reply,false);
            return;
        }

        if(isStructuralCalcLoadCommand(aiControl)){
            if(currentProject==null){
                reply.send("Statik hesap raporu yüklemek için önce ilgili DWG/DXF projesini açın.");
                return;
            }
            pendingStructuralCalcReply=reply;
            pendingStructuralCalcProject=currentProject;
            pickStructuralCalcDocument();
            reply.send("Statik hesap raporu/model dışa aktarımını seçin. Metin tabanlı PDF, DOCX, XLSX, TXT/CSV ve ETABS/SAP/SAFE metin çıktıları (.e2k/.s2k/.f2k) okunabilir. Taranmış görüntü PDF'leri otomatik hesap verisi sayılmaz.");
            return;
        }
        if(isStructuralCalcClearCommand(aiControl)){
            if(currentProject!=null){currentProject.structuralCalcModel=null;currentProject.structuralCalcName="";}
            reply.send("Bu proje için yüklenen statik hesap raporu bağlantısı temizlendi.");
            return;
        }
        if(isStructuralCalcCompareCommand(aiControl)){
            if(activeDxf==null){
                reply.send("Statik hesap raporu–DWG karşılaştırması için tam vektör DWG/DXF çizimi hazır olmalı.");
                return;
            }
            if(currentProject==null||currentProject.structuralCalcModel==null){
                reply.send("Karşılaştırılacak statik hesap raporu yüklenmedi. Önce “Statik hesap raporu yükle” yazın.");
                return;
            }
            MusaAiStructuralCalc.Comparison comparison=MusaAiStructuralCalc.compare(currentAiDrawingIndex(),currentProject.structuralCalcModel);
            lastAiReport=comparison.text;
            lastAiReportTitle="Statik Hesap–Proje Karşılaştırma Raporu";
            lastAiReportSourceIds=Collections.unmodifiableList(new ArrayList<>(comparison.sourceIds));
            int shown=comparison.sourceIds.isEmpty()?0:cad.setAiHighlightedSources(comparison.sourceIds);
            if(comparison.sourceIds.isEmpty())cad.clearAiHighlights();
            reply.send(comparison.text+(shown>0?"\n\n• Farklı kesitle ilişkilendirilen çizim nesnesi: "+shown:"")+
                "\n\nÇıktı: “Raporu Word olarak çıkar” veya “Raporu PDF olarak çıkar”.");
            return;
        }
        if(isStructuralCalcSummaryCommand(aiControl)){
            reply.send(currentProject==null||currentProject.structuralCalcModel==null
                ?"Bu proje için statik hesap raporu yüklenmedi. “Statik hesap raporu yükle” diyebilirsiniz."
                :MusaAiStructuralCalc.summary(currentProject.structuralCalcModel));
            return;
        }

        if(isBoqLoadCommand(aiControl)){
            if(currentProject==null){
                reply.send("Keşif yüklemek için önce ilgili DWG/DXF projesini açın.");
                return;
            }
            pendingBoqReply=reply;
            pendingBoqProject=currentProject;
            pickBoqDocument();
            reply.send("Keşif dosyasını seçin. Excel (.xlsx) ve CSV doğrudan okunur; Word/TXT tablo yapısındaysa denenir. PDF sayısal karşılaştırmada otomatik güvenilir kabul edilmez.");
            return;
        }
        if(isBoqClearCommand(aiControl)){
            if(currentProject!=null){currentProject.boqModel=null;currentProject.boqName="";}
            reply.send("Bu proje için yüklenen keşif bağlantısı temizlendi.");
            return;
        }
        if(isBoqCompareCommand(aiControl)){
            if(activeDxf==null){
                reply.send("Proje–keşif karşılaştırması için tam vektör DWG/DXF çizimi hazır olmalı.");
                return;
            }
            if(currentProject==null||currentProject.boqModel==null||currentProject.boqModel.isEmpty()){
                reply.send("Karşılaştırılacak keşif yüklenmedi. Önce “Keşif yükle” yazın.");
                return;
            }
            MusaAiBoq.Model generated=MusaAiBoq.generate(currentAiDrawingIndex(),currentDisplayName+" • otomatik proje metrajı");
            MusaAiBoq.Comparison comparison=MusaAiBoq.compare(currentProject.boqModel,generated);
            reply.send(comparison.text);
            return;
        }
        if(isBoqGenerateCommand(aiControl)){
            if(activeDxf==null){
                reply.send("Projeden keşif oluşturmak için tam vektör DWG/DXF çizimi hazır olmalı.");
                return;
            }
            MusaAiBoq.Model generated=MusaAiBoq.generate(currentAiDrawingIndex(),currentDisplayName+" • otomatik proje metrajı");
            reply.send("ÇİZİMDEN OTOMATİK KEŞİF\n"+MusaAiBoq.summary(generated));
            return;
        }
        if(isBoqSummaryCommand(aiControl)){
            reply.send(currentProject==null||currentProject.boqModel==null
                ?"Bu proje için keşif yüklenmedi. “Keşif yükle” diyebilirsiniz."
                :MusaAiBoq.summary(currentProject.boqModel));
            return;
        }

        if(isAiReportWordCommand(aiControl)){
            exportLastAiReport(true,reply);
            return;
        }
        if(isAiReportPdfCommand(aiControl)){
            exportLastAiReport(false,reply);
            return;
        }
        if(MusaAiProjectPackage.asksPackageReview(raw)){
            List<MusaAiProjectPackage.Drawing> packageDrawings=currentAiProjectPackageDrawings();
            if(packageDrawings.isEmpty()){
                reply.send("Proje Paketi denetimi için en az bir tam vektör DWG/DXF çizimi açık olmalı.");
                return;
            }
            MusaAiProjectPackage.Result packageReport=MusaAiProjectPackage.generate(packageDrawings,raw);
            if(packageReport.matched){
                lastAiReport=packageReport.text;
                lastAiReportTitle=packageReport.title;
                lastAiReportSourceIds=Collections.emptyList();
                cad.clearAiHighlights();
                reply.send(packageReport.text+
                    "\n\n• Paket denetiminde açık vektör çizim: "+packageReport.drawingCount+
                    "\n• Algılanan disiplin türü: "+packageReport.detectedDisciplineCount+
                    "\n\nÇıktı: “Raporu Word olarak çıkar” veya “Raporu PDF olarak çıkar”.");
                return;
            }
        }
        if(MusaAiDetailedReport.asksDetailedReport(raw)){
            if(activeDxf==null){
                reply.send("Detaylı disiplin raporu için tam vektör DWG/DXF çizimi hazır olmalı.");
                return;
            }
            MusaAiDetailedReport.Result detailed=MusaAiDetailedReport.generate(
                currentAiDrawingIndex(),currentDisplayName,raw,
                currentProject==null?null:currentProject.boqModel,
                currentProject==null?null:currentProject.structuralCalcModel);
            if(detailed.matched){
                lastAiReport=detailed.text;lastAiReportTitle=detailed.title;
                lastAiReportSourceIds=Collections.unmodifiableList(new ArrayList<>(detailed.sourceIds));
                int shown=detailed.sourceIds.isEmpty()?0:cad.setAiHighlightedSources(detailed.sourceIds);
                if(detailed.sourceIds.isEmpty())cad.clearAiHighlights();
                reply.send(detailed.text+
                    (shown>0?"\n\n• Çizimde vurgulanan bulgu: "+shown:"")+
                    "\n\nÇıktı: “Raporu Word olarak çıkar” veya “Raporu PDF olarak çıkar”.");
                return;
            }
        }
        MusaAiDiscipline focusedDiscipline=MusaAiDiscipline.fromQuery(raw);
        if(activeDxf!=null&&focusedDiscipline==MusaAiDiscipline.STRUCTURAL&&MusaAiStructuralAdvanced.isFocusedQuery(raw)){
            MusaAiStructuralCalc.Model calc=currentProject==null?null:currentProject.structuralCalcModel;
            if(MusaAiStructuralAdvanced.focusedQueryNeedsReport(raw)&&calc==null){
                cad.clearAiHighlights();
                reply.send("Bu odaklı statik kontrol için hesap raporu/model çıktısı gerekli. Önce “Statik hesap raporu yükle” yazın. MusaCAD sonuç değeri uydurmaz.");
                return;
            }
            MusaAiStructuralAdvanced.Result focused=MusaAiStructuralAdvanced.analyzeFocused(currentAiDrawingIndex(),calc,raw);
            if(focused.matched){
                lastAiReport=focused.text;
                lastAiReportTitle="Odaklı Statik Kontrol";
                lastAiReportSourceIds=Collections.unmodifiableList(new ArrayList<>(focused.sourceIds));
                int shown=focused.sourceIds.isEmpty()?0:cad.setAiHighlightedSources(focused.sourceIds);
                if(focused.sourceIds.isEmpty())cad.clearAiHighlights();
                reply.send(focused.text+
                    (shown>0?"\n\n• Çizimde vurgulanan bulgu: "+shown:"")+
                    "\n\nÇıktı: “Raporu Word olarak çıkar” veya “Raporu PDF olarak çıkar”.");
                return;
            }
        }

        // Android voice recognition commonly returns "projeye analiz yap" or
        // "projeyi analiz et". This is a full-project command, not an unknown
        // drawing question. Keep the opt-in cloud policy intact.
        if(MusaAiDisciplineAnalyzer.asksGeneralProjectAnalysis(raw)
            &&!MusaAiCloudPolicy.shouldUseCloud(raw)){
            if(activeDxf==null){
                reply.send("Genel proje analizi için DWG/DXF tam vektör modeli henüz hazır değil. Çizim yüklemesi tamamlandığında yeniden 'Projeyi analiz et' deyin.");
                return;
            }
            runMusaAiGeneralProjectAnalysis(reply);
            return;
        }

        if(MusaAiDisciplineAnalyzer.asksAnalysis(raw)){
            MusaAiDiscipline requested=MusaAiDiscipline.fromQuery(raw);
            boolean full=aiControl.contains("tam proje")||aiControl.contains("tum disiplin")||aiControl.contains("disiplinler arasi");
            if(activeDxf!=null&&(full||requested!=MusaAiDiscipline.MECHANICAL)){
                MusaAiDisciplineAnalyzer.Result multi=MusaAiDisciplineAnalyzer.analyze(currentAiDrawingIndex(),raw);
                if(multi.matched){
                    int shown=multi.sourceIds.isEmpty()?0:cad.setAiHighlightedSources(multi.sourceIds);
                    if(multi.sourceIds.isEmpty())cad.clearAiHighlights();
                    reply.send(multi.text+(shown>0?"\n• Çizimde vurgulanan: "+shown:""));
                    return;
                }
            }
        }

        if(MusaAiCloudPolicy.shouldUseCloud(raw)){
            if(currentProject==null){
                reply.send("Gandalf AI ile çizim analizi için önce bir DWG veya DXF projesi açın.");
                return;
            }
            handleMusaAiCloudPrompt(raw,reply);
            return;
        }

        MusaAiCommandRouter.Match command=MusaAiCommandRouter.route(raw);
        if(command.matched){
            if(currentProject==null){
                reply.send("“"+command.description+"” komutunu çalıştırmak için önce bir DWG veya DXF projesi açın.");
                return;
            }
            commandInput.setText(command.command);
            executeCommand();
            CharSequence cadStatus=result==null?null:result.getText();
            String status=cadStatus==null?"":cadStatus.toString().trim();
            reply.send("Komut çalıştırıldı • "+command.description+(status.isEmpty()?"":"\n"+status));
            return;
        }

        if(currentProject==null){
            reply.send("Bu işlem için önce bir DWG veya DXF projesi açın. AI paneli proje açılmadan da kullanılabilir, ancak çizim analizi için aktif proje gerekir.");
            return;
        }

        String aiq=MusaAiDrawingIndex.normalize(raw);
        if(isRevisionBaselineClearCommand(aiq)){
            aiRevisionBaseline=null;aiRevisionBaselineName="";
            cad.clearAiHighlights();
            reply.send("Revizyon referansı temizlendi.");
            return;
        }

        if(isRevisionBaselineSetCommand(aiq)){
            if(activeDxf==null){
                reply.send("Referans revizyon kaydetmek için tam vektör DWG/DXF çiziminin hazır olması gerekiyor.");
                return;
            }
            aiRevisionBaseline=currentAiDrawingIndex();
            aiRevisionBaselineName=currentDisplayName;
            cad.clearAiHighlights();
            reply.send("Revizyon referansı kaydedildi • "+aiRevisionBaselineName+
                "\nŞimdi diğer DWG/DXF revizyonunu açıp “Revizyonları karşılaştır” diyebilirsiniz.");
            return;
        }

        if(aiq.contains("raporu paylas")||aiq.contains("rapor paylas")||
           aiq.contains("raporu disa aktar")||aiq.contains("rapor disa aktar")){
            if(activeDxf==null){
                reply.send("AI raporu paylaşmak için tam vektör DWG/DXF çiziminin hazır olması gerekiyor.");
                return;
            }
            MusaAiAutoReport.Result report=buildCurrentAiReport();
            if(!report.matched){reply.send("AI proje raporu oluşturulamadı.");return;}
            lastAiReport=report.text;lastAiReportTitle="Otomatik Proje Raporu";
            lastAiReportSourceIds=Collections.unmodifiableList(new ArrayList<>(report.sourceIds));
            int shown=report.sourceIds.isEmpty()?0:cad.setAiHighlightedSources(report.sourceIds);
            if(report.sourceIds.isEmpty())cad.clearAiHighlights();
            shareAiReport(lastAiReport);
            reply.send("AI proje raporu oluşturuldu ve TXT paylaşım ekranı açıldı."+
                (shown>0?"\n• Çizimde vurgulanan bulgu/değişiklik: "+shown:""));
            return;
        }

        if(MusaAiAutoReport.asksReport(raw)){
            if(activeDxf==null){
                reply.send("AI proje raporu için tam vektör DWG/DXF çiziminin hazır olması gerekiyor.");
                return;
            }
            MusaAiAutoReport.Result report=buildCurrentAiReport();
            if(report.matched){
                lastAiReport=report.text;lastAiReportTitle="Otomatik Proje Raporu";
                lastAiReportSourceIds=Collections.unmodifiableList(new ArrayList<>(report.sourceIds));
                int shown=report.sourceIds.isEmpty()?0:cad.setAiHighlightedSources(report.sourceIds);
                if(report.sourceIds.isEmpty())cad.clearAiHighlights();
                String highlight=shown>0?"\n\n• Çizimde vurgulanan bulgu/değişiklik: "+shown+
                    (report.sourceIds.size()>shown?" / "+report.sourceIds.size():""):"";
                reply.send(report.text+highlight+"\n\n“Raporu Word olarak çıkar” veya “Raporu PDF olarak çıkar” diyebilirsiniz.");
                return;
            }
        }

        if(MusaAiRevisionCompare.asksComparison(raw)){
            if(activeDxf==null){
                reply.send("Revizyon karşılaştırması için güncel çizimin tam vektör modeli hazır olmalı.");
                return;
            }
            MusaAiDrawingIndex baseline=aiRevisionBaseline;
            String baselineName=aiRevisionBaselineName;
            boolean automatic=false;
            if(baseline==null){
                RevisionCandidate candidate=findOtherRevisionCandidate();
                if(candidate!=null){baseline=candidate.index;baselineName=candidate.name;automatic=true;}
            }
            if(baseline==null){
                reply.send("Karşılaştırılacak referans belirlenmedi. Referans çizimi açıp “Bu çizimi referans revizyon yap” deyin; ardından diğer revizyonu açıp karşılaştırın. İki vektör proje açık olduğunda diğer tek proje de otomatik referans olarak kullanılabilir.");
                return;
            }
            MusaAiRevisionCompare.Result revision=MusaAiRevisionCompare.compare(
                baseline,currentAiDrawingIndex(),raw,baselineName,currentDisplayName);
            if(revision.matched){
                int shown=revision.sourceIds.isEmpty()?0:cad.setAiHighlightedSources(revision.sourceIds);
                if(revision.sourceIds.isEmpty())cad.clearAiHighlights();
                String highlight=shown>0?"\n• Güncel çizimde eklenen/değişen vurgulandı: "+shown+
                    (revision.sourceIds.size()>shown?" / "+revision.sourceIds.size():""):"";
                String auto=automatic?"\n• Referans otomatik seçildi: "+baselineName:"";
                reply.send(revision.text+highlight+auto);
                return;
            }
        }

        if(q.contains("ai seçimini temizle")||q.contains("ai secimini temizle")||q.contains("vurgulamayı temizle")||q.contains("vurgulamayi temizle")){
            cad.clearAiHighlights();
            reply.send("AI çoklu vurgulaması temizlendi.");
            return;
        }

        if(activeDxf!=null){
            MusaAiDisciplineExpert.Result disciplineExpert=MusaAiDisciplineExpert.analyze(currentAiDrawingIndex(),raw);
            if(disciplineExpert.matched){
                int shown=disciplineExpert.sourceIds.isEmpty()?0:cad.setAiHighlightedSources(disciplineExpert.sourceIds);
                if(disciplineExpert.sourceIds.isEmpty())cad.clearAiHighlights();
                String highlight=shown>0?"\n• Disiplin uzman taramasında vurgulanan: "+shown+
                    (disciplineExpert.sourceIds.size()>shown?" / "+disciplineExpert.sourceIds.size():""):"";
                reply.send(disciplineExpert.text+highlight+
                    "\n\nGandalf derin analizi için aynı komutun başına G ekleyin.");
                return;
            }
        }

        if(activeDxf!=null){
            MusaAiMechanicalExpert.Result expert=MusaAiMechanicalExpert.analyze(currentAiDrawingIndex(),raw);
            if(expert.matched){
                int shown=expert.sourceIds.isEmpty()?0:cad.setAiHighlightedSources(expert.sourceIds);
                if(expert.sourceIds.isEmpty())cad.clearAiHighlights();
                String highlight=shown>0?"\n• MEKAI uzman taramasında vurgulanan: "+shown+
                    (expert.sourceIds.size()>shown?" / "+expert.sourceIds.size():""):"";
                reply.send(expert.text+highlight+"\n\nDerin Gandalf analizi için aynı komutun başına G ekleyin: örn. GMEKAI_FIRE.");
                return;
            }
        }

        if(activeDxf!=null){
            MusaAiMechanicalControl.Result mechanical=MusaAiMechanicalControl.analyze(currentAiDrawingIndex(),raw);
            if(mechanical.matched){
                int shown=mechanical.sourceIds.isEmpty()?0:cad.setAiHighlightedSources(mechanical.sourceIds);
                if(mechanical.sourceIds.isEmpty())cad.clearAiHighlights();
                String highlight=shown>0?"\n• Mekanik kontrol adaylarından çizimde vurgulanan: "+shown+
                    (mechanical.sourceIds.size()>shown?" / "+mechanical.sourceIds.size():""):"";
                reply.send(mechanical.text+highlight);
                return;
            }
        }

        if(activeDxf!=null){
            MusaAiSmartSelection.Result selection=MusaAiSmartSelection.plan(
                currentAiDrawingIndex(),raw,cad.selectedSourceType(),cad.selectedLayer());
            if(selection.matched){
                if(selection.totalMatches==0&&selection.sourceIds.isEmpty()){
                    cad.clearAiHighlights();
                    reply.send(selection.description.startsWith("Önce ")
                        ?selection.description
                        :selection.description+" • Eşleşen nesne bulunamadı.");
                    return;
                }
                int shown=cad.setAiHighlightedSources(selection.sourceIds);
                String note=selection.totalMatches>shown
                    ?"\nToplam eşleşme: "+selection.totalMatches+" • Düzenlenebilir/vurgulanabilir: "+shown
                    :"\nVurgulanan: "+shown;
                if(shown==0&&selection.totalMatches>0)note+="\nEşleşmeler blok içi veya doğrudan düzenlenemeyen öğeler olabilir.";
                reply.send(selection.description+note);
                return;
            }
        }

        if(activeDxf!=null){
            MusaAiProjectControl.Result control=MusaAiProjectControl.analyze(currentAiDrawingIndex(),raw);
            if(control.matched){
                int shown=control.sourceIds.isEmpty()?0:cad.setAiHighlightedSources(control.sourceIds);
                if(control.sourceIds.isEmpty())cad.clearAiHighlights();
                String highlight=shown>0?"\n• Çizimde vurgulanan: "+shown+
                    (control.sourceIds.size()>shown?" / "+control.sourceIds.size():""):"";
                reply.send(control.text+highlight);
                return;
            }
        }

        if(activeDxf!=null){
            MusaAiTableOleAnalysis.Answer tableOle=MusaAiTableOleAnalysis.answer(currentAiDrawingIndex(),raw);
            if(tableOle.matched){
                int shown=tableOle.sourceIds.isEmpty()?0:cad.setAiHighlightedSources(tableOle.sourceIds);
                if(tableOle.sourceIds.isEmpty())cad.clearAiHighlights();
                String highlight=shown>0?"\n• İlgili çizim metni vurgulandı: "+shown+
                    (tableOle.sourceIds.size()>shown?" / "+tableOle.sourceIds.size():""):"";
                reply.send(tableOle.text+highlight);
                return;
            }
        }

        if(activeDxf!=null){
            MusaAiQuantityTakeoff.Answer takeoff=MusaAiQuantityTakeoff.answer(currentAiDrawingIndex(),raw);
            if(takeoff.matched){
                reply.send(takeoff.text);
                return;
            }
        }

        if(activeDxf!=null){
            MusaAiDrawingQuestions.Answer drawingAnswer=MusaAiDrawingQuestions.answer(currentAiDrawingIndex(),raw);
            if(drawingAnswer.matched){
                reply.send(drawingAnswer.text);
                return;
            }
        }

        if(q.contains("çizimde neler")||q.contains("çizim özeti")||q.contains("proje özeti")||q.contains("bu proje")){
            if(activeDxf!=null){
                String extra=activeDxf.oleObjectCount>0?" • OLE "+activeDxf.olePreviewCount+"/"+activeDxf.oleObjectCount:"";
                reply.send(currentDisplayName+"\n• Layout: "+activeDxf.activeLayout+"\n• Nesne: "+activeDxf.entityCount+"\n• Katman: "+activeDxf.layerCount+"\n• Seçilebilir nesne: "+activeDxf.editableSourceCount()+extra);
            }else if(currentProject.nativeScene!=null){
                reply.send(currentDisplayName+" açık. Native hızlı sahnede "+currentProject.nativeScene.primitiveCount+" geometri var. Tam vektör model hazır olduğunda AI daha ayrıntılı analiz yapabilecek.");
            }else reply.send(currentDisplayName+" açık. Çizim modeli hazırlanıyor.");
            return;
        }
        if(q.contains("metraj")){
            reply.send("Metraj isteğini aldım. Metraj/sayım motoru MusaCAD AI'nın sonraki modüllerinden biri olarak bu panelde çalışacak.");
            return;
        }
        if(q.contains("kontrol")||q.contains("hata")){
            reply.send(activeDxf==null
                ?"Proje kontrolü için tam vektör çizim modelinin hazırlanması gerekiyor."
                :"Bu kontrol isteği mevcut otomatik CAD kalite kurallarıyla eşleşmedi. “Projeyi kontrol et” veya “mükerrer nesneleri bul” diye deneyin.");
            return;
        }
        reply.send("Bu soruyu yerel çizim analizinde henüz eşleştiremedim. Şu anda nesne türleri, katmanlar, çizim metinleri ve doğal dil CAD komutları destekleniyor.");
    }

    private void runMusaAiGeneralProjectAnalysis(MusaAiPanel.Reply reply){
        final DxfParser.Result drawing=activeDxf;
        final ProjectSession project=currentProject;
        final String drawingName=currentDisplayName;
        if(drawing==null){
            reply.send("Proje analizi için tam vektör çizim modelinin hazırlanması gerekiyor.");
            return;
        }
        // Building a CAD index or inspecting every entity on the main thread
        // can trigger Android's ANR dialog for large DWG files.
        try{
            aiExecutor.submit(()->{
                try{
                    MusaAiDrawingIndex index=drawing.aiDrawingIndex(CLOUD_AI_INDEX_MAX_ITEMS);
                    MusaAiDisciplineAnalyzer.Result report=MusaAiDisciplineAnalyzer.analyzeAll(index);
                    runOnUiThread(()->{
                        if(currentProject!=project||activeDxf!=drawing){
                            reply.send("Analiz sırasında aktif proje değişti. Yeni proje için tekrar 'Projeyi analiz et' deyin.");
                            return;
                        }
                        lastAiReport=report.text;
                        lastAiReportTitle="Gandalf • Genel Proje Ön Analizi";
                        lastAiReportSourceIds=Collections.unmodifiableList(new ArrayList<>(report.sourceIds));
                        int shown=report.sourceIds.isEmpty()?0:cad.setAiHighlightedSources(report.sourceIds);
                        if(report.sourceIds.isEmpty())cad.clearAiHighlights();
                        reply.send("Gandalf • "+drawingName+"\n"+report.text+
                            (shown>0?"\n\nÇizimde vurgulanan inceleme adayı: "+shown:"")+
                            "\n\nBu sonuç sınırlı CAD verisiyle yapılan yerel ön incelemedir. Derin bulut analizi için 'Bu projeyi derin analiz et' deyin."+
                            "\nRapor çıktısı için 'Raporu PDF olarak çıkar' veya 'Raporu Word olarak çıkar' deyin.");
                    });
                }catch(OutOfMemoryError e){
                    reply.send("Gandalf proje analizi bellek sınırına ulaştı. Çizim açık kalacak; daha küçük bir paftayla yeniden deneyin.");
                }catch(Exception e){
                    reply.send("Gandalf proje analizi başlatılamadı. Çizimi tekrar açıp yeniden deneyin.");
                }
            });
        }catch(RejectedExecutionException e){
            reply.send("Gandalf analiz kuyruğu şu an kullanılamıyor. Uygulamayı yeniden açıp tekrar deneyin.");
        }
    }

    private void handleMusaAiCloudPrompt(String raw,MusaAiPanel.Reply reply){
        if(activeDxf==null){
            reply.send("Gandalf AI için tam vektör DWG/DXF çiziminin hazırlanması gerekiyor.");
            return;
        }
        boolean packageMode=MusaAiCloudPolicy.shouldUseProjectPackage(raw)&&openVectorProjectCount()>1;
        SharedPreferences prefs=getSharedPreferences(AI_PRIVACY_PREFS,MODE_PRIVATE);
        String consentKey=packageMode?K_CLOUD_PACKAGE_CONSENT:K_CLOUD_CONSENT;
        if(prefs.getBoolean(consentKey,false)){
            runMusaAiCloud(raw,reply);
            return;
        }
        String scope=packageMode
            ?"Derin analiz için açık vektör proje paketindeki çizimlerin sınırlı CAD-JSON özetleri güvenli MusaCAD sunucusuna gönderilir."
            :"Derin analiz için aktif çizimin sınırlı CAD-JSON özeti güvenli MusaCAD sunucusuna gönderilir.";
        String edit=packageMode
            ?" Proje Paketi modunda farklı dosyalardaki kimlikler karışmasın diye bulut çizim-değiştirme araçları kapalıdır."
            :" Çizim değişikliği önerileri kullanıcı onayı olmadan uygulanmaz.";
        new AlertDialog.Builder(this)
            .setTitle(packageMode?"Gandalf • Proje Paketi":"Gandalf Cloud AI")
            .setMessage(scope+" Ham DWG/DXF dosyaları gönderilmez. Katman adları, çizim metinleri, nesne türleri ve ölçü bilgileri bulut AI tarafından işlenebilir."+edit+" Devam edilsin mi?")
            .setPositiveButton("DEVAM",(d,w)->{
                prefs.edit().putBoolean(consentKey,true).apply();
                runMusaAiCloud(raw,reply);
            })
            .setNegativeButton("İPTAL",(d,w)->reply.send("Gandalf Cloud AI isteği iptal edildi. Yerel MusaCAD AI çevrimdışı kullanılmaya devam edebilir."))
            .setOnCancelListener(d->reply.send("Gandalf Cloud AI isteği iptal edildi."))
            .show();
    }

    private List<AiCloudProjectSource> currentAiCloudProjectSources(){
        ArrayList<AiCloudProjectSource> out=new ArrayList<>();
        for(ProjectSession p:projects){
            if(p==null)continue;
            DxfParser.Result parsed=(p==currentProject&&activeDxf!=null)?activeDxf:p.parsed;
            if(parsed==null)continue;
            String name=p==currentProject?currentDisplayName:p.name;
            out.add(new AiCloudProjectSource(name,parsed,p.boqModel));
            if(out.size()>=MusaAiCadPackageJson.MAX_DRAWINGS)break;
        }
        return Collections.unmodifiableList(out);
    }

    private void runMusaAiCloud(String raw,MusaAiPanel.Reply reply){
        // Capture only cheap immutable references on the UI thread. The expensive
        // CAD-to-AI projection is built below on MusaCAD-cloud-ai so large drawings
        // can never trigger Android's "uygulama yanıt vermiyor" watchdog.
        final DxfParser.Result activeSnapshot=activeDxf;
        final String displayName=currentDisplayName;
        final boolean packageMode=MusaAiCloudPolicy.shouldUseProjectPackage(raw)&&openVectorProjectCount()>1;
        final List<AiCloudProjectSource> packageSources=packageMode
            ?currentAiCloudProjectSources():Collections.emptyList();
        if(activeSnapshot==null){reply.send("Gandalf AI için çizim indeksi hazırlanamadı.");return;}

        reply.progress("Gandalf • Çizim bağlamı hazırlanıyor…");
        try{
            aiExecutor.submit(()->{
                try{
                    MusaAiDrawingIndex snapshot=activeSnapshot.aiDrawingIndex(CLOUD_AI_INDEX_MAX_ITEMS);
                    List<MusaAiProjectPackage.Drawing> packageDrawings=Collections.emptyList();
                    if(packageMode){
                        ArrayList<MusaAiProjectPackage.Drawing> built=new ArrayList<>();
                        for(AiCloudProjectSource source:packageSources){
                            if(source==null||source.parsed==null)continue;
                            MusaAiDrawingIndex index=source.parsed==activeSnapshot
                                ?snapshot:source.parsed.aiDrawingIndex(CLOUD_AI_INDEX_MAX_ITEMS);
                            built.add(new MusaAiProjectPackage.Drawing(source.name,index,source.boq));
                            if(built.size()>=MusaAiCadPackageJson.MAX_DRAWINGS)break;
                        }
                        packageDrawings=Collections.unmodifiableList(built);
                    }

                    reply.progress("Gandalf • Güvenli AI oturumu açılıyor, analiz yanıtı bekleniyor…");
                    MusaAiCloudService.Result cloud=packageMode
                        ?MusaAiCloudService.analyzePackage(getApplicationContext(),snapshot,displayName,packageDrawings,raw)
                        :MusaAiCloudService.analyze(getApplicationContext(),snapshot,displayName,raw);
                    if(!cloud.ok()){
                        reply.progress("Bulut AI tamamlanamadı. Yerel proje kontrolüne geçiliyor…");
                        MusaAiDisciplineAnalyzer.Result localFallback=MusaAiDisciplineAnalyzer.analyzeAll(snapshot);
                        String reason=cloud.message.isEmpty()?"Gandalf Cloud AI kullanılamadı.":cloud.message;
                        if(localFallback.matched){
                            reply.send(reason+
                                "\n\nGandalf yerel araçlarla devam etti:\n"+
                                localFallback.text+
                                "\n\nNot: Bu yedek analiz güncel web/kaynak taraması kullanmaz.");
                        }else{
                            reply.send(reason+" Yerel Gandalf araçları da bu isteği eşleştiremedi.");
                        }
                        return;
                    }
                    pendingAiActions=cloud.actions;
                    StringBuilder out=new StringBuilder();
                    if(MusaAiSessionService.developerCached())
                        out.append("Gandalf Developer • Yönetici modu aktif\n\n");
                    if(packageMode)
                        out.append("Gandalf Proje Paketi • ").append(packageDrawings.size()).append(" açık vektör çizim\n\n");
                    out.append(cloud.text);
                    if(cloud.webUsed)out.append("\n\n• Bu yanıtta güncel web araması kullanıldı.");
                    if(!cloud.sources.isEmpty()){
                        out.append("\n\nKaynaklar:");
                        int sourceCount=0;
                        for(MusaAiCloudService.Source source:cloud.sources){
                            if(sourceCount++>=8){out.append("\n• … +").append(cloud.sources.size()-8).append(" kaynak");break;}
                            out.append("\n• ");
                            if(!source.title.isEmpty())out.append(source.title).append(" — ");
                            out.append(source.url);
                        }
                    }
                    if(packageMode&&!cloud.actions.isEmpty()){
                        pendingAiActions=Collections.emptyList();
                        out.append("\n\n• Güvenlik: Proje Paketi modunda gelen çizim işlem önerileri uygulanmadı.");
                    }else if(!cloud.actions.isEmpty()){
                        out.append("\n\nÖnerilen çizim işlemleri (henüz uygulanmadı):");
                        int shown=0;
                        for(MusaAiCloudService.Action action:cloud.actions){
                            if(shown++>=8){out.append("\n• … +").append(cloud.actions.size()-8).append(" işlem");break;}
                            out.append("\n• ").append(action.name);
                            if(!action.reason.isEmpty())out.append(" — ").append(action.reason);
                        }
                        if(MusaAiSessionService.developerCached())
                            out.append("\n\nGandalf Developer: “Önerileri önizle” veya “Önerileri uygula” diyebilirsiniz. Hiçbir değişiklik açık onay olmadan çizime işlenmez.");
                        else
                            out.append("\n\nBu çizim işlemleri görüntülenebilir; doğrudan uygulama şu anda Gandalf Developer yetkisine ayrılmıştır.");
                    }
                    reply.send(out.toString());
                    if(!packageMode&&!cloud.actions.isEmpty()&&MusaAiSessionService.developerCached())
                        runOnUiThread(()->showPendingGandalfActions(reply,false));
                }catch(OutOfMemoryError e){
                    reply.send("Gandalf CAD-JSON hazırlığı sırasında bellek sınırına ulaşıldı. Çizim açık kalacak; daha küçük ve sınırlı bir AI özetiyle yeniden deneyin.");
                }catch(Exception e){
                    String message=e.getMessage()==null||e.getMessage().trim().isEmpty()
                        ?"Gandalf AI isteği hazırlanamadı.":e.getMessage().trim();
                    reply.send("Gandalf AI isteği hazırlanamadı: "+message);
                }
            });
        }catch(RejectedExecutionException e){
            reply.send("Gandalf AI işlemi şu anda başlatılamıyor. Uygulamayı yeniden açıp tekrar deneyin.");
        }
    }

    private static boolean isGandalfPreviewCommand(String q){
        return q.equals("onerileri onizle")||q.equals("gandalf onerilerini onizle")||
            q.equals("ai onerilerini onizle")||q.equals("duzeltmeleri onizle");
    }

    private static boolean isGandalfApplyCommand(String q){
        return q.equals("onerileri uygula")||q.equals("gandalf onerilerini uygula")||
            q.equals("ai onerilerini uygula")||q.equals("duzeltmeleri uygula")||
            q.equals("gandalf uygula");
    }

    private static boolean isGandalfClearCommand(String q){
        return q.equals("onerileri temizle")||q.equals("gandalf onerilerini temizle")||
            q.equals("ai onerilerini temizle");
    }

    private static boolean isGandalfUndoCommand(String q){
        return q.equals("gandalf geri al")||q.equals("gandalf degisikliklerini geri al")||
            q.equals("ai degisikliklerini geri al")||q.equals("gandalf duzeltmelerini geri al");
    }

    private static boolean isStructuralCalcLoadCommand(String q){
        return q.equals("statik hesap raporu yukle")||q.equals("hesap raporu yukle")||
            q.equals("statik hesap yukle")||q.equals("statik model yukle")||
            q.equals("hesap modeli yukle");
    }
    private static boolean isStructuralCalcClearCommand(String q){
        return (q.contains("statik")||q.contains("hesap"))&&
            (q.contains("rapor")||q.contains("model"))&&
            (q.contains("temizle")||q.contains("sil")||q.contains("kaldir"));
    }
    private static boolean isStructuralCalcCompareCommand(String q){
        return (q.contains("statik")||q.contains("hesap"))&&
            (q.contains("hesap")||q.contains("rapor")||q.contains("model"))&&
            (q.contains("karsilastir")||q.contains("uyum")||q.contains("fark"))&&
            (q.contains("proje")||q.contains("dwg")||q.contains("cizim")||q.contains("statik"));
    }
    private static boolean isStructuralCalcSummaryCommand(String q){
        return q.equals("statik hesap ozeti")||q.equals("hesap raporu ozeti")||
            q.equals("yuklu statik hesap raporu")||q.equals("statik modeli goster");
    }

    private static boolean isBoqLoadCommand(String q){
        return q.equals("kesif yukle")||q.equals("boq yukle")||q.equals("metraj dosyasi yukle")||
            q.equals("kesif dosyasi yukle")||q.equals("kesfi yukle");
    }
    private static boolean isBoqClearCommand(String q){
        return q.equals("kesfi temizle")||q.equals("kesif temizle")||q.equals("boq temizle")||
            q.equals("yuklu kesfi sil");
    }
    private static boolean isBoqCompareCommand(String q){
        return (q.contains("kesif")||q.contains("boq"))&&
            (q.contains("karsilastir")||q.contains("uyum")||q.contains("fark"));
    }
    private static boolean isBoqGenerateCommand(String q){
        return (q.contains("kesif")||q.contains("metraj tablosu"))&&
            (q.contains("olustur")||q.contains("uret")||q.contains("cikar"))&&
            (q.contains("proje")||q.contains("cizim")||q.contains("otomatik"));
    }
    private static boolean isBoqSummaryCommand(String q){
        return q.equals("kesif ozeti")||q.equals("yuklu kesif")||q.equals("boq ozeti")||
            q.equals("kesfi goster");
    }

    private void showPendingGandalfActions(MusaAiPanel.Reply reply,boolean applyRequested){
        List<MusaAiCloudService.Action> actions=pendingAiActions;
        if(actions==null||actions.isEmpty()){
            reply.send("Bekleyen Gandalf çizim önerisi yok.");
            return;
        }
        MusaAiActionExecutor.Preview preview=MusaAiActionExecutor.preview(actions);
        if(!MusaAiSessionService.developerCached()){
            reply.send("Gandalf öneri önizlemesi:\n"+preview.text+
                "\n\nÇizime uygulama için Gandalf Developer yetkisi gerekir.");
            return;
        }
        StringBuilder message=new StringBuilder();
        message.append("Önerilen işlemler:\n").append(preview.text);
        message.append("\n\nÇizim değişikliği: ").append(preview.mutationCount);
        if(preview.deleteCount>0)message.append("\nSİLME işlemi: ").append(preview.deleteCount);
        if(preview.unsupportedCount>0)message.append("\nDesteklenmeyen öneri: ").append(preview.unsupportedCount);
        message.append("\n\nUYGULA seçilmeden çizimde hiçbir değişiklik yapılmaz. Paket uygulanırken bir işlem başarısız olursa tamamı geri alınır.");

        AlertDialog dialog=new AlertDialog.Builder(this)
            .setTitle("Gandalf Developer • Önizleme")
            .setMessage(message.toString())
            .setNegativeButton("KAPAT",(d,w)->{
                if(!applyRequested)reply.send("Gandalf önerileri önizlendi. Çizimde değişiklik yapılmadı.");
            })
            .setPositiveButton("UYGULA",(d,w)->applyPendingGandalfActions(reply))
            .create();
        dialog.setOnShowListener(d->{
            if(preview.mutationCount==0&&preview.highlightCount==0)
                dialog.getButton(AlertDialog.BUTTON_POSITIVE).setEnabled(false);
        });
        dialog.show();
    }

    private void applyPendingGandalfActions(MusaAiPanel.Reply reply){
        if(!MusaAiSessionService.developerCached()){
            reply.send("Gandalf çizim uygulama yetkisi aktif değil.");
            return;
        }
        List<MusaAiCloudService.Action> actions=pendingAiActions;
        if(actions==null||actions.isEmpty()){
            reply.send("Bekleyen Gandalf çizim önerisi yok.");
            return;
        }
        CadView.SessionState before=cad.captureSessionState();
        ProjectSession project=currentProject;
        MusaAiActionExecutor.ApplyResult applied=MusaAiActionExecutor.apply(cad,actions);
        if(!applied.success){
            reply.send(applied.message);
            return;
        }
        pendingAiActions=Collections.emptyList();
        lastGandalfBatchState=before;
        lastGandalfBatchProject=project;
        lastGandalfBatchFingerprint=cad.editFingerprint();
        markCurrentProjectDirty();
        reply.send(applied.message+"\n\nToplu geri almak için “Gandalf geri al” yazabilirsiniz.");
    }

    private void undoLastGandalfBatch(MusaAiPanel.Reply reply){
        if(lastGandalfBatchState==null||lastGandalfBatchProject==null||currentProject!=lastGandalfBatchProject){
            reply.send("Bu açık proje için geri alınabilecek son Gandalf paketi yok.");
            return;
        }
        if(cad.editFingerprint()!=lastGandalfBatchFingerprint){
            reply.send("Gandalf paketinden sonra çizimde başka değişiklikler yapılmış. Sonraki çalışmaların kaybolmaması için toplu geri alma engellendi; normal Undo ile adım adım geri alabilirsiniz.");
            return;
        }
        cad.restoreCapturedSessionState(lastGandalfBatchState);
        lastGandalfBatchState=null;lastGandalfBatchProject=null;lastGandalfBatchFingerprint=Long.MIN_VALUE;
        markCurrentProjectDirty();
        reply.send("Son Gandalf çizim paketi toplu olarak geri alındı.");
    }

    private void markCurrentProjectDirty(){
        if(currentProject==null)return;
        currentProject.viewState=cad.captureSessionState();
        currentProject.dirty=currentProject.baselineSet&&cad.editFingerprint()!=currentProject.savedFingerprint;
    }

    private MusaAiAutoReport.Result buildCurrentAiReport(){
        if(activeDxf==null)return MusaAiAutoReport.Result.none();
        MusaAiDrawingIndex baseline=aiRevisionBaseline;
        String baselineName=aiRevisionBaselineName;
        if(baseline==null){
            RevisionCandidate candidate=findOtherRevisionCandidate();
            if(candidate!=null){baseline=candidate.index;baselineName=candidate.name;}
        }
        return MusaAiAutoReport.generate(currentAiDrawingIndex(),currentDisplayName,baseline,baselineName);
    }

    private static boolean isAiReportWordCommand(String q){
        return (q.contains("rapor")&&q.contains("word"))||q.contains("docx olarak")||q.contains("word olarak cikar");
    }

    private static boolean isAiReportPdfCommand(String q){
        return (q.contains("rapor")&&q.contains("pdf"))||q.contains("pdf olarak cikar");
    }

    private void exportLastAiReport(boolean word,MusaAiPanel.Reply reply){
        if(android.os.Looper.myLooper()!=android.os.Looper.getMainLooper()){
            runOnUiThread(()->exportLastAiReport(word,reply));
            return;
        }
        if(lastAiReport==null||lastAiReport.trim().isEmpty()){
            reply.send("Önce bir proje/disiplin raporu oluşturun.");
            return;
        }
        final String report=lastAiReport,title=lastAiReportTitle;
        Bitmap captured=null;
        try{
            if(cad!=null&&lastAiReportSourceIds!=null&&!lastAiReportSourceIds.isEmpty())
                captured=cad.aiEvidenceSnapshot(lastAiReportSourceIds);
        }catch(Exception ignored){}
        final Bitmap evidence=captured;
        aiExecutor.submit(()->{
            try{
                File file=word?MusaAiReportExport.docx(getApplicationContext(),title,report,evidence)
                              :MusaAiReportExport.pdf(getApplicationContext(),title,report,evidence);
                String mime=word?"application/vnd.openxmlformats-officedocument.wordprocessingml.document":"application/pdf";
                runOnUiThread(()->shareFile(file,mime));
                reply.send((word?"Word (.docx)":"PDF")+" raporu oluşturuldu • "+title+
                    (evidence!=null?"\n• İşaretli proje bölgesinin kanıt görüntüsü rapora eklendi.":""));
            }catch(Exception e){
                reply.send("Rapor çıktısı oluşturulamadı: "+(e.getMessage()==null?"bilinmeyen hata":e.getMessage()));
            }finally{
                if(evidence!=null&&!evidence.isRecycled())evidence.recycle();
            }
        });
    }

    private void shareAiReport(String report){
        if(report==null||report.trim().isEmpty()){
            Toast.makeText(this,"Önce AI proje raporu oluşturun",Toast.LENGTH_SHORT).show();
            return;
        }
        File file=null;
        try{
            file=File.createTempFile("MusaCAD_AI_Rapor_",".txt",exportDir());
            try(OutputStream out=new FileOutputStream(file)){
                out.write(report.getBytes(java.nio.charset.StandardCharsets.UTF_8));
            }
            shareFile(file,"text/plain");
        }catch(Exception e){
            if(file!=null)file.delete();
            error(e);
        }
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
            tool("Birleştir",R.drawable.ic_polyline,()->{if(ensureTransformSelection("JOIN")){if(cad.armJoinSelected())result.setText("Birleştir • İkinci nesneye dokunun");else result.setText("Birleştir • Uygun nesne seçin");}}),
            tool("Font",R.drawable.ic_text,this::showFontManager)
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

    private void open3d(){ open3d(Mesh3dActivity.MODE_ISO); }

    private void open3d(String mode){
        File model=currentProject==null?null:currentProject.workingDxf;
        if(model==null||!model.isFile()){
            Toast.makeText(this,currentProject!=null&&currentProject.preparingEditor
                ?"3B görünüm için DWG dönüşümü hazırlanıyor"
                :"Önce bir DWG/DXF açın",Toast.LENGTH_LONG).show();
            return;
        }
        Intent intent=new Intent(this,Mesh3dActivity.class);
        intent.putExtra(Mesh3dActivity.EXTRA_DXF,model.getAbsolutePath());
        intent.putExtra(Mesh3dActivity.EXTRA_MODE,mode);
        startActivity(intent);
    }

    private void show3dToolsSheet(){
        showToolPanel("3D Araçları",
            tool("İzometrik",R.drawable.ic_fit,()->open3d(Mesh3dActivity.MODE_ISO)),
            tool("Orbit / Döndür",R.drawable.ic_rotate,()->open3d(Mesh3dActivity.MODE_ORBIT)),
            tool("Ön Görünüş",R.drawable.ic_fit,()->open3d(Mesh3dActivity.MODE_FRONT)),
            tool("Üst Görünüş",R.drawable.ic_fit,()->open3d(Mesh3dActivity.MODE_TOP)),
            tool("Sağ Görünüş",R.drawable.ic_fit,()->open3d(Mesh3dActivity.MODE_RIGHT)),
            tool("Tel Kafes",R.drawable.ic_layers,()->open3d(Mesh3dActivity.MODE_WIREFRAME)),
            tool("Yüzey",R.drawable.ic_layers,()->open3d(Mesh3dActivity.MODE_SURFACE)),
            tool("3B Ölçüm",R.drawable.ic_distance,()->open3d(Mesh3dActivity.MODE_MEASURE)),
            tool("3B Düzenle",R.drawable.ic_move,()->open3d(Mesh3dActivity.MODE_EDIT)),
            tool("Katmanlar",R.drawable.ic_layers,this::showLayers),
            tool("2B Görünüme Dön",R.drawable.ic_fit,()->{
                cad.regenerate();
                result.setText("2B görünüm etkin");
            }),
            tool("3B Bilgi",R.drawable.ic_properties,this::show3dSupportInfo)
        );
        result.setText("3D araçları • İzometrik, görünüş, tel kafes, ölçüm ve düzenleme seçenekleri hazır");
    }

    private void show3dSupportInfo(){
        new AlertDialog.Builder(this)
            .setTitle("MusaCAD 3D desteği")
            .setMessage("3B araçlar açıldı. Orbit, 3B ölçüm, yüzey/ağ görünümü ve köşe düzenleme 3B çalışma ekranında kullanılabilir. Mevcut sürüm gerçek 3DFACE ve polyface yüzeyleri işler. Yalnızca 2B çizgi içeren projeler kendiliğinden 3B modele dönüştürülmez.")
            .setPositiveButton("TAMAM",null)
            .show();
    }

    private void showViewToolsSheet(){
        showToolPanel("Görsel stil",
            tool("Kaydır",R.drawable.ic_pan,()->selectMode(R.id.panButton,CadView.Mode.PAN)),
            tool("Sığdır",R.drawable.ic_fit,()->cad.fitToScreen()),
            tool("Yakınlaştır",R.drawable.ic_zoom_in,()->cad.zoomBy(1.35f)),
            tool("Uzaklaştır",R.drawable.ic_zoom_out,()->cad.zoomBy(1f/1.35f)),
            tool("2D",R.drawable.ic_fit,()->{cad.regenerate();result.setText("2D görünüm etkin");}),
            tool("3D Araçları",R.drawable.ic_fit,this::show3dToolsSheet),
            tool("Katmanlar",R.drawable.ic_layers,this::showLayers),
            tool("Model/Layout",R.drawable.ic_layers,this::showLayouts)
        );
    }

    private void showAnnotationToolsSheet(){
        showToolPanel("Ek açıklama",
            tool("Taslak kroki",R.drawable.ic_line,()->{if(canEdit()){cad.setMode(CadView.Mode.FREEHAND);markModeSelected(0);result.setText("Taslak kroki • Serbest çizim etkin");}}),
            tool("Ok",R.drawable.ic_line,()->{if(canEdit()){cad.setMode(CadView.Mode.DRAW_ARROW);markModeSelected(0);result.setText("Ok • Önce ok ucunu, sonra kuyruk noktasını seçin");}}),
            tool("Metin",R.drawable.ic_text,()->selectEditMode(R.id.textButton,CadView.Mode.DRAW_TEXT)),
            tool("Font Yöneticisi",R.drawable.ic_text,this::showFontManager),
            tool("Revcloud",R.drawable.ic_polyline,()->{if(canEdit()){cad.setMode(CadView.Mode.DRAW_REVCLOUD);markModeSelected(0);result.setText("Revcloud • Bulut alanının iki karşı köşesini seçin");}}),
            tool("Ses",R.drawable.ic_more,()->pickMedia(PICK_AUDIO,"audio/*")),
            tool("Görüntü",R.drawable.ic_open_file,()->pickMedia(PICK_IMAGE,"image/*")),
            tool("Video",R.drawable.ic_more,()->pickMedia(PICK_VIDEO,"video/*")),
            tool("Belge",R.drawable.ic_open_file,this::pickDocument),
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
            tool("Blok ekle",R.drawable.ic_open_file,this::showBlockLibrary),
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

    private void pickStructuralCalcDocument(){
        Intent intent=new Intent(Intent.ACTION_OPEN_DOCUMENT);
        intent.addCategory(Intent.CATEGORY_OPENABLE);
        // Deliberately use */* because ETABS/SAP/SAFE text exports often have
        // vendor/unknown MIME types (.e2k/.s2k/.f2k).
        intent.setType("*/*");
        intent.addFlags(Intent.FLAG_GRANT_READ_URI_PERMISSION|Intent.FLAG_GRANT_PERSISTABLE_URI_PERMISSION);
        startActivityForResult(intent,PICK_STRUCT_CALC);
    }

    private void handleStructuralCalcPicked(Uri uri){
        final MusaAiPanel.Reply reply=pendingStructuralCalcReply;
        final ProjectSession target=pendingStructuralCalcProject;
        pendingStructuralCalcReply=null;pendingStructuralCalcProject=null;
        if(uri==null||target==null)return;
        try{getContentResolver().takePersistableUriPermission(uri,Intent.FLAG_GRANT_READ_URI_PERMISSION);}catch(Exception ignored){}

        final String name=nameOf(uri);
        String detected=getContentResolver().getType(uri);
        final String mime=detected==null?CadDocumentSupport.bestMime(name,null):detected;
        final CadDocumentSupport.Kind kind=CadDocumentSupport.kind(name,mime);
        final boolean engineeringText=MusaAiStructuralCalc.isEngineeringTextExtension(name)||
            (mime!=null&&mime.toLowerCase(Locale.ROOT).startsWith("text/"));

        boolean supported=kind==CadDocumentSupport.Kind.PDF||
            kind==CadDocumentSupport.Kind.DOCX||
            kind==CadDocumentSupport.Kind.XLSX||
            kind==CadDocumentSupport.Kind.CSV||
            kind==CadDocumentSupport.Kind.TEXT||
            engineeringText;
        if(!supported){
            String message="Bu statik hesap raporu/model biçimi otomatik okunamıyor: "+name+
                ". Desteklenenler: metin tabanlı PDF, DOCX, XLSX, TXT/CSV, E2K/S2K/F2K ve metin/XML dışa aktarımları.";
            if(reply!=null)reply.send(message);else Toast.makeText(this,message,Toast.LENGTH_LONG).show();
            return;
        }

        result.setText("Statik hesap • "+name+" okunuyor…");
        aiExecutor.submit(()->{
            try(InputStream in=getContentResolver().openInputStream(uri)){
                if(in==null)throw new IOException("Statik hesap raporu açılamadı");
                String extracted;
                if(kind==CadDocumentSupport.Kind.PDF)extracted=MusaAiPdfBoxTextExtractor.extract(getApplicationContext(),in);
                else if(kind==CadDocumentSupport.Kind.DOCX||kind==CadDocumentSupport.Kind.XLSX||
                        kind==CadDocumentSupport.Kind.CSV||kind==CadDocumentSupport.Kind.TEXT)
                    extracted=OfficeTextExtractor.extract(in,name,mime);
                else extracted=MusaAiStructuralCalc.readText(in);

                MusaAiStructuralCalc.Model model=MusaAiStructuralCalc.parse(name,extracted);
                runOnUiThread(()->{
                    if(isFinishing()||isDestroyed())return;
                    target.structuralCalcModel=model;target.structuralCalcName=name;
                    result.setText("Statik hesap • "+name+" • "+model.taggedSections.size()+" etiketli kesit");
                    String message="Statik hesap raporu/model çıktısı yüklendi.\n"+
                        MusaAiStructuralCalc.summary(model)+
                        "\n\n“Statik hesapla projeyi karşılaştır” diyerek aktif DWG ile çapraz kontrol yapabilirsiniz.";
                    if(reply!=null)reply.send(message);
                    else Toast.makeText(this,"Statik hesap raporu yüklendi",Toast.LENGTH_LONG).show();
                });
            }catch(Exception e){
                final String message="Statik hesap raporu okunamadı: "+(e.getMessage()==null?"bilinmeyen hata":e.getMessage());
                runOnUiThread(()->{
                    result.setText(message);
                    if(reply!=null)reply.send(message);else Toast.makeText(this,message,Toast.LENGTH_LONG).show();
                });
            }
        });
    }

    private void pickBoqDocument(){
        Intent intent=new Intent(Intent.ACTION_OPEN_DOCUMENT);intent.addCategory(Intent.CATEGORY_OPENABLE);intent.setType("*/*");
        intent.addFlags(Intent.FLAG_GRANT_READ_URI_PERMISSION|Intent.FLAG_GRANT_PERSISTABLE_URI_PERMISSION);
        intent.putExtra(Intent.EXTRA_MIME_TYPES,new String[]{
            "application/vnd.openxmlformats-officedocument.spreadsheetml.sheet",
            "text/csv","text/plain",
            "application/vnd.openxmlformats-officedocument.wordprocessingml.document",
            "application/pdf"
        });
        startActivityForResult(intent,PICK_BOQ);
    }

    private void handleBoqPicked(Uri uri){
        final MusaAiPanel.Reply reply=pendingBoqReply;
        final ProjectSession target=pendingBoqProject;
        pendingBoqReply=null;pendingBoqProject=null;
        if(uri==null||target==null)return;
        try{getContentResolver().takePersistableUriPermission(uri,Intent.FLAG_GRANT_READ_URI_PERMISSION);}catch(Exception ignored){}
        final String name=nameOf(uri);
        String detected=getContentResolver().getType(uri);
        final String mime=detected==null?CadDocumentSupport.bestMime(name,null):detected;
        final CadDocumentSupport.Kind kind=CadDocumentSupport.kind(name,mime);
        if(kind==CadDocumentSupport.Kind.PDF){
            String message="PDF keşif seçildi: "+name+"\nPDF sayısal hücre yapısı bu aşamada güvenilir satır/miktar verisi olarak otomatik kabul edilmiyor. Karşılaştırma için tercihen XLSX veya CSV yükleyin; PDF inceleme belgesi olarak açılabilir.";
            result.setText("Keşif • PDF için sayısal inceleme gerekli");
            if(reply!=null)reply.send(message);else Toast.makeText(this,message,Toast.LENGTH_LONG).show();
            return;
        }
        if(!(kind==CadDocumentSupport.Kind.XLSX||kind==CadDocumentSupport.Kind.CSV||
             kind==CadDocumentSupport.Kind.DOCX||kind==CadDocumentSupport.Kind.TEXT)){
            String message="Bu keşif biçimi otomatik okunamıyor: "+CadDocumentSupport.displayType(name,mime)+". XLSX veya CSV kullanın.";
            if(reply!=null)reply.send(message);else Toast.makeText(this,message,Toast.LENGTH_LONG).show();
            return;
        }
        result.setText("Keşif • "+name+" okunuyor…");
        aiExecutor.submit(()->{
            try(InputStream in=getContentResolver().openInputStream(uri)){
                if(in==null)throw new IOException("Keşif dosyası açılamadı");
                String extracted=OfficeTextExtractor.extract(in,name,mime);
                MusaAiBoq.Model model=MusaAiBoq.parse(name,kind,extracted);
                runOnUiThread(()->{
                    if(isFinishing()||isDestroyed())return;
                    target.boqModel=model;target.boqName=name;
                    result.setText("Keşif • "+name+" • "+model.rows.size()+" satır");
                    String message="Keşif yüklendi.\n"+MusaAiBoq.summary(model)+
                        "\n\n“Keşifle karşılaştır” diyerek aktif çizimin otomatik metrajıyla karşılaştırabilirsiniz.";
                    if(reply!=null)reply.send(message);else Toast.makeText(this,"Keşif yüklendi • "+model.rows.size()+" satır",Toast.LENGTH_LONG).show();
                });
            }catch(Exception e){
                final String message="Keşif okunamadı: "+(e.getMessage()==null?"bilinmeyen hata":e.getMessage());
                runOnUiThread(()->{result.setText(message);if(reply!=null)reply.send(message);else Toast.makeText(this,message,Toast.LENGTH_LONG).show();});
            }
        });
    }

    private void pickDocument(){
        Intent intent=new Intent(Intent.ACTION_OPEN_DOCUMENT);intent.addCategory(Intent.CATEGORY_OPENABLE);intent.setType("*/*");
        intent.addFlags(Intent.FLAG_GRANT_READ_URI_PERMISSION|Intent.FLAG_GRANT_PERSISTABLE_URI_PERMISSION);
        intent.putExtra(Intent.EXTRA_MIME_TYPES,new String[]{
            "application/pdf",
            "application/vnd.openxmlformats-officedocument.wordprocessingml.document","application/msword",
            "application/vnd.openxmlformats-officedocument.spreadsheetml.sheet","application/vnd.ms-excel",
            "application/vnd.openxmlformats-officedocument.presentationml.presentation","application/vnd.ms-powerpoint",
            "text/plain","text/csv"
        });
        startActivityForResult(intent,PICK_DOCUMENT);
    }

    private void openDocumentViewer(Uri uri,String name){
        if(uri==null)return;Intent view=new Intent(this,DocumentViewerActivity.class);view.setData(uri);view.putExtra(DocumentViewerActivity.EXTRA_NAME,name);view.addFlags(Intent.FLAG_GRANT_READ_URI_PERMISSION);startActivityForResult(view,VIEW_DOCUMENT);
    }

    private void handleDocumentPicked(Uri uri){
        if(uri==null)return;
        try{getContentResolver().takePersistableUriPermission(uri,Intent.FLAG_GRANT_READ_URI_PERMISSION);}catch(Exception ignored){}
        String name=nameOf(uri);String mime=getContentResolver().getType(uri);if(mime==null)mime=CadDocumentSupport.bestMime(name,null);
        if(currentProject!=null&&canEdit()){
            PointF center=cad.visibleCenterContent();cad.addTextEdit(center.x,center.y,"Belge • "+name);
            currentProject.mediaAttachments.add(new MediaAttachment("Belge",name,mime,uri));
            result.setText("Belge • "+name+" eklendi ve MusaCAD görüntüleyicide açıldı");
        }
        openDocumentViewer(uri,name);
    }

    private void handleDocumentImport(Intent data){
        if(data==null||!data.getBooleanExtra(DocumentViewerActivity.EXTRA_IMPORT_TO_CAD,false))return;
        if(currentProject==null||!canEdit()){Toast.makeText(this,"Belgeyi çizime aktarmak için düzenlenebilir bir çizim açın",Toast.LENGTH_LONG).show();return;}
        String name=data.getStringExtra(DocumentViewerActivity.EXTRA_IMPORT_NAME);if(name==null||name.trim().isEmpty())name="Belge";
        String text=data.getStringExtra(Intent.EXTRA_TEXT);
        if(text!=null&&!text.trim().isEmpty()){
            CadDocumentSupport.Kind kind=CadDocumentSupport.kind(name,CadDocumentSupport.bestMime(name,null));
            if(kind==CadDocumentSupport.Kind.XLSX||kind==CadDocumentSupport.Kind.CSV){
                List<CadSpreadsheetLayout.Sheet> sheets=kind==CadDocumentSupport.Kind.XLSX?CadSpreadsheetLayout.parseExtractedXlsx(text):CadSpreadsheetLayout.parseCsv(text);
                int cells=importSpreadsheetTables(sheets);
                if(cells>0){result.setText("Excel / tablo • "+name+" • "+cells+" hücre düzenlenebilir CAD tablosu olarak aktarıldı");return;}
            }
            PointF center=cad.visibleCenterContent();String[] lines=text.replace("\r\n","\n").replace('\r','\n').split("\n",-1);
            ArrayList<CadEdit> imported=new ArrayList<>();float y=center.y,step=34f;
            for(String line:lines){String value=line.trim();if(value.isEmpty()){y+=step;continue;}imported.add(CadEdit.styledText(center.x,y,value,0f,"STANDARD","sans-serif",false,22f,1f,0f,0));y+=step;if(imported.size()>=500)break;}
            int added=cad.addImportedEdits(imported);result.setText("Belge • "+name+" • "+added+" metin satırı çizime aktarıldı");return;
        }
        String raw=data.getStringExtra(DocumentViewerActivity.EXTRA_IMPORT_URI);if(raw==null||raw.trim().isEmpty())return;
        Uri imageUri=Uri.parse(raw);final String finalName=name;result.setText("Belge sayfası • çizime aktarılıyor…");
        new Thread(()->{
            Bitmap bitmap=null;
            try{
                bitmap=decodeCadImage(imageUri);final Bitmap ready=bitmap;
                runOnUiThread(()->{
                    if(isFinishing()||isDestroyed()){if(!ready.isRecycled())ready.recycle();return;}
                    if(!cad.addImageOverlay(ready,finalName,imageUri.toString())){if(!ready.isRecycled())ready.recycle();result.setText("Belge sayfası • çizime aktarılamadı");return;}
                    result.setText("Belge sayfası • "+finalName+" çizime görüntü olarak aktarıldı");
                });
            }catch(Exception e){if(bitmap!=null&&!bitmap.isRecycled())bitmap.recycle();final String m=e.getMessage()==null?"Belge sayfası okunamadı":e.getMessage();runOnUiThread(()->Toast.makeText(this,m,Toast.LENGTH_LONG).show());}
        },"MusaCAD-document-import").start();
    }

    private int importSpreadsheetTables(List<CadSpreadsheetLayout.Sheet> sheets){
        if(sheets==null||sheets.isEmpty()||activeDxf==null)return 0;
        PointF center=cad.visibleCenterContent();ArrayList<CadEdit> imported=new ArrayList<>();int cellCount=0;float y=center.y;
        for(CadSpreadsheetLayout.Sheet sheet:sheets){
            if(sheet==null||sheet.isEmpty()||sheet.rows<1||sheet.columns<1)continue;
            float[] widths=CadSpreadsheetLayout.columnWidths(sheet,70f,190f,8.5f);float rawWidth=0f;for(float width:widths)rawWidth+=width;
            float maxWidth=Math.max(400f,activeDxf.contentWidth()*.82f),factor=rawWidth>maxWidth?Math.max(.35f,maxWidth/rawWidth):1f;
            float tableWidth=rawWidth*factor,rowHeight=38f*factor,textHeight=Math.max(9f,19f*factor),padding=Math.max(2.5f,5f*factor);
            float left=center.x-tableWidth*.5f,top=y+textHeight*1.6f,bottom=top+sheet.rows*rowHeight;
            imported.add(CadEdit.styledText(left,y,sheet.title,0f,"STANDARD","sans-serif",false,Math.max(12f,22f*factor),1f,0f,0));
            float x=left;imported.add(CadEdit.line(x,top,x,bottom));
            for(float width:widths){x+=width*factor;imported.add(CadEdit.line(x,top,x,bottom));}
            for(int row=0;row<=sheet.rows;row++){float yy=top+row*rowHeight;imported.add(CadEdit.line(left,yy,left+tableWidth,yy));}
            float[] starts=new float[widths.length];x=left;for(int i=0;i<widths.length;i++){starts[i]=x;x+=widths[i]*factor;}
            for(CadSpreadsheetLayout.Cell cell:sheet.cells){
                if(cell.row<1||cell.row>sheet.rows||cell.col<1||cell.col>starts.length)continue;String value=cell.value==null?"":cell.value.trim();if(value.isEmpty())continue;
                int maxChars=Math.max(4,(int)((widths[cell.col-1]*factor-padding*2f)/Math.max(4f,textHeight*.55f)));if(value.length()>maxChars)value=value.substring(0,Math.max(1,maxChars-1))+"…";
                float tx=starts[cell.col-1]+padding,ty=top+(cell.row-1)*rowHeight+rowHeight*.68f;
                imported.add(CadEdit.styledText(tx,ty,value,0f,"STANDARD","sans-serif",false,textHeight,1f,0f,0));cellCount++;
            }
            y=bottom+Math.max(45f,textHeight*3f);
            if(imported.size()>12000)break;
        }
        return cad.addImportedEdits(imported)>0?cellCount:0;
    }

    private void handleMediaPicked(int request,Uri uri){
        if(uri==null||currentProject==null)return;
        String kind=request==PICK_AUDIO?"Ses":request==PICK_IMAGE?"Görüntü":"Video";
        String mime=request==PICK_AUDIO?"audio/*":request==PICK_IMAGE?"image/*":"video/*";
        try{getContentResolver().takePersistableUriPermission(uri,Intent.FLAG_GRANT_READ_URI_PERMISSION);}catch(Exception ignored){}
        String found=nameOf(uri);final String name=found==null||found.trim().isEmpty()?kind+" eki":found;
        if(request==PICK_IMAGE){
            final ProjectSession target=currentProject;result.setText("Görüntü • hazırlanıyor…");
            new Thread(()->{
                Bitmap bitmap=null;
                try{
                    bitmap=decodeCadImage(uri);final Bitmap ready=bitmap;
                    runOnUiThread(()->{
                        if(isFinishing()||isDestroyed()||currentProject!=target){if(!ready.isRecycled())ready.recycle();return;}
                        if(!cad.addImageOverlay(ready,name,uri.toString())){if(!ready.isRecycled())ready.recycle();result.setText("Görüntü • çizime yerleştirilemedi");return;}
                        target.mediaAttachments.add(new MediaAttachment(kind,name,mime,uri));
                        result.setText("Görüntü • "+name+" çizime yerleştirildi • Seç ile taşı / döndür / ölçekle / sil");
                    });
                }catch(Exception e){
                    final String message=e.getMessage()==null?"Görüntü okunamadı":e.getMessage();
                    if(bitmap!=null&&!bitmap.isRecycled())bitmap.recycle();
                    runOnUiThread(()->Toast.makeText(this,"Görüntü eklenemedi: "+message,Toast.LENGTH_LONG).show());
                }
            },"MusaCAD-image-import").start();
            return;
        }
        PointF center=cad.visibleCenterContent();
        cad.addTextEdit(center.x,center.y,kind+" • "+name);
        currentProject.mediaAttachments.add(new MediaAttachment(kind,name,mime,uri));
        result.setText(kind+" • "+name+" eklendi • görünüm merkezine bağlantı notu yerleştirildi");
    }

    private Bitmap decodeCadImage(Uri uri)throws IOException{
        BitmapFactory.Options bounds=new BitmapFactory.Options();bounds.inJustDecodeBounds=true;
        try(InputStream in=getContentResolver().openInputStream(uri)){if(in==null)throw new IOException("Dosya açılamadı");BitmapFactory.decodeStream(in,null,bounds);}
        if(bounds.outWidth<=0||bounds.outHeight<=0)throw new IOException("Geçerli JPG, PNG veya WebP görüntüsü değil");
        int sample=1;long pixels=(long)bounds.outWidth*bounds.outHeight;
        while(bounds.outWidth/sample>4096||bounds.outHeight/sample>4096||pixels/((long)sample*sample)>12_000_000L)sample*=2;
        BitmapFactory.Options options=new BitmapFactory.Options();options.inSampleSize=sample;options.inPreferredConfig=Bitmap.Config.ARGB_8888;
        try(InputStream in=getContentResolver().openInputStream(uri)){if(in==null)throw new IOException("Dosya açılamadı");Bitmap bitmap=BitmapFactory.decodeStream(in,null,options);if(bitmap==null)throw new IOException("Görüntü çözümlenemedi");return bitmap;}
    }

    private void showMediaAttachments(){
        if(currentProject==null||currentProject.mediaAttachments.isEmpty()){result.setText("Medya Ekleri • Bu projede ek yok");return;}
        String[] labels=new String[currentProject.mediaAttachments.size()];
        for(int i=0;i<labels.length;i++){MediaAttachment m=currentProject.mediaAttachments.get(i);labels[i]=m.kind+" • "+m.name;}
        new AlertDialog.Builder(this).setTitle("Medya Ekleri").setItems(labels,(d,which)->{
            MediaAttachment m=currentProject.mediaAttachments.get(which);Uri target=Uri.parse(m.uri);
            if("Belge".equals(m.kind)||CadDocumentSupport.isDocument(m.name,m.mime)){openDocumentViewer(target,m.name);return;}
            try{
                Intent view=new Intent(Intent.ACTION_VIEW,target);view.setType(m.mime);view.addFlags(Intent.FLAG_GRANT_READ_URI_PERMISSION);startActivity(view);
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

    private Set<String> favoriteBlockIds(){
        Set<String> stored=getSharedPreferences("musacad_block_library",MODE_PRIVATE).getStringSet("favorites",Collections.emptySet());
        return new LinkedHashSet<>(stored==null?Collections.emptySet():stored);
    }

    private boolean isFavoriteBlock(String id){return id!=null&&favoriteBlockIds().contains(id);}

    private void toggleFavoriteBlock(String id){
        if(id==null||id.trim().isEmpty())return;
        Set<String> favorites=favoriteBlockIds();
        if(!favorites.add(id))favorites.remove(id);
        getSharedPreferences("musacad_block_library",MODE_PRIVATE).edit().putStringSet("favorites",favorites).apply();
    }

    private void showBlockLibrary(){
        if(currentProject==null||!canEdit()){result.setText("Blok kütüphanesi • Düzenlenebilir bir çizim açın");return;}

        LinearLayout box=new LinearLayout(this);box.setOrientation(LinearLayout.VERTICAL);int p=dp(12);box.setPadding(p,p/2,p,p/2);
        EditText search=new EditText(this);search.setSingleLine(true);search.setHint("Blok ara: pompa, priz, sprinkler…");box.addView(search,new LinearLayout.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT,ViewGroup.LayoutParams.WRAP_CONTENT));

        ArrayList<String> categories=new ArrayList<>();categories.add("Tümü");categories.add("★ Favoriler");categories.addAll(CadBlockLibrary.categories());categories.add("Kullanıcı");
        Spinner category=new Spinner(this);category.setAdapter(new ArrayAdapter<>(this,android.R.layout.simple_spinner_dropdown_item,categories));box.addView(category,new LinearLayout.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT,ViewGroup.LayoutParams.WRAP_CONTENT));

        TextView help=new TextView(this);help.setText("Dokun: yerleştir • Uzun bas: favoriye ekle / çıkar");help.setTextSize(11);help.setPadding(2,dp(4),2,dp(4));box.addView(help);

        ListView list=new ListView(this);ArrayAdapter<String> adapter=new ArrayAdapter<>(this,android.R.layout.simple_list_item_1,new ArrayList<>());list.setAdapter(adapter);box.addView(list,new LinearLayout.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT,dp(360)));

        ArrayList<String> keys=new ArrayList<>();
        final Runnable[] refresh=new Runnable[1];
        refresh[0]=()->{
            String selected=String.valueOf(category.getSelectedItem());
            String query=search.getText()==null?"":search.getText().toString().trim();
            String normalizedQuery=query.toLowerCase(new Locale("tr","TR"));
            Set<String> favorites=favoriteBlockIds();
            adapter.clear();keys.clear();

            boolean favoritesOnly="★ Favoriler".equals(selected);
            boolean usersOnly="Kullanıcı".equals(selected);
            if(!usersOnly){
                String builtInCategory=("Tümü".equals(selected)||favoritesOnly)?null:selected;
                for(CadBlockLibrary.Entry entry:CadBlockLibrary.filter(builtInCategory,query)){
                    if(favoritesOnly&&!favorites.contains(entry.id))continue;
                    boolean fav=favorites.contains(entry.id);
                    adapter.add((fav?"★ ":"☆ ")+entry.name+"  •  "+entry.category);
                    keys.add("B:"+entry.id);
                }
            }

            if(!favoritesOnly&&("Tümü".equals(selected)||usersOnly)){
                for(String name:cad.userBlockNames()){
                    if(name==null||name.startsWith("LIB_"))continue;
                    if(!normalizedQuery.isEmpty()&&!name.toLowerCase(new Locale("tr","TR")).contains(normalizedQuery))continue;
                    adapter.add("◼ "+name+"  •  Kullanıcı");
                    keys.add("U:"+name);
                }
            }
            adapter.notifyDataSetChanged();
            if(keys.isEmpty())help.setText("Bu filtrede blok bulunamadı.");
            else help.setText(keys.size()+" blok • Dokun: yerleştir • Uzun bas: favori");
        };

        search.addTextChangedListener(new android.text.TextWatcher(){
            public void beforeTextChanged(CharSequence s,int start,int count,int after){}
            public void onTextChanged(CharSequence s,int start,int before,int count){}
            public void afterTextChanged(android.text.Editable s){refresh[0].run();}
        });
        category.setOnItemSelectedListener(new android.widget.AdapterView.OnItemSelectedListener(){
            public void onItemSelected(android.widget.AdapterView<?> parent,View view,int position,long id){refresh[0].run();}
            public void onNothingSelected(android.widget.AdapterView<?> parent){}
        });

        AlertDialog dialog=new AlertDialog.Builder(this).setTitle("CAD Blok Kütüphanesi").setView(box).setNegativeButton("KAPAT",null).create();
        list.setOnItemClickListener((parent,view,position,id)->{
            if(position<0||position>=keys.size())return;String key=keys.get(position);
            if(key.startsWith("U:")){dialog.dismiss();showInsertOptions(key.substring(2));return;}
            CadBlockLibrary.Entry entry=CadBlockLibrary.findById(key.substring(2));if(entry==null)return;
            dialog.dismiss();prepareLibraryInsert(entry);
        });
        list.setOnItemLongClickListener((parent,view,position,id)->{
            if(position<0||position>=keys.size())return true;String key=keys.get(position);
            if(!key.startsWith("B:")){Toast.makeText(this,"Kullanıcı blokları zaten proje içinde saklanır",Toast.LENGTH_SHORT).show();return true;}
            String blockId=key.substring(2);CadBlockLibrary.Entry entry=CadBlockLibrary.findById(blockId);if(entry==null)return true;
            boolean was=isFavoriteBlock(blockId);toggleFavoriteBlock(blockId);refresh[0].run();
            Toast.makeText(this,was?"Favorilerden çıkarıldı":"Favorilere eklendi",Toast.LENGTH_SHORT).show();return true;
        });
        dialog.setOnShowListener(d->refresh[0].run());dialog.show();
    }

    private void prepareLibraryInsert(CadBlockLibrary.Entry entry){
        if(entry==null)return;
        if(entry.attributeDefaults.isEmpty()){armLibraryBlock(entry,Collections.emptyMap());return;}

        LinearLayout box=new LinearLayout(this);box.setOrientation(LinearLayout.VERTICAL);int p=dp(14);box.setPadding(p,p/2,p,p/2);
        LinkedHashMap<String,EditText> fields=new LinkedHashMap<>();
        for(Map.Entry<String,String> attr:entry.attributeDefaults.entrySet()){
            TextView label=new TextView(this);label.setText(attr.getKey());label.setTextSize(11);box.addView(label);
            EditText input=new EditText(this);input.setSingleLine(true);input.setText(attr.getValue());input.setSelectAllOnFocus(true);box.addView(input);fields.put(attr.getKey(),input);
        }
        AlertDialog dialog=new AlertDialog.Builder(this).setTitle(entry.name+" • Özellikler").setMessage("Blok etiketi / attribute değerlerini düzenleyin.").setView(box).setPositiveButton("DEVAM",null).setNegativeButton("İPTAL",null).create();
        dialog.setOnShowListener(d->dialog.getButton(AlertDialog.BUTTON_POSITIVE).setOnClickListener(v->{
            LinkedHashMap<String,String> values=new LinkedHashMap<>();
            for(Map.Entry<String,EditText> field:fields.entrySet()){String value=field.getValue().getText().toString().trim();if(value.isEmpty()){field.getValue().setError("Boş bırakılamaz");return;}values.put(field.getKey(),value);}
            dialog.dismiss();armLibraryBlock(entry,values);
        }));dialog.show();
    }

    private void armLibraryBlock(CadBlockLibrary.Entry entry,Map<String,String> attributes){
        try{
            CadBlock.Definition definition=entry.definition(attributes);
            if(!cad.registerBlockDefinition(definition)){result.setText("Blok kütüphanesi • Tanım yüklenemedi");return;}
            showInsertOptions(definition.name,entry.name);
        }catch(Exception e){Toast.makeText(this,"Blok hazırlanamadı: "+e.getMessage(),Toast.LENGTH_LONG).show();}
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

    private void showInsertOptions(String name){showInsertOptions(name,name);}

    private void showInsertOptions(String name,String displayName){
        LinearLayout box=new LinearLayout(this);box.setOrientation(LinearLayout.VERTICAL);int p=dp(16);box.setPadding(p,p/2,p,p/2);
        EditText scale=new EditText(this);scale.setHint("Ölçek");scale.setText("1");scale.setInputType(android.text.InputType.TYPE_CLASS_NUMBER|android.text.InputType.TYPE_NUMBER_FLAG_DECIMAL);box.addView(scale);
        EditText rotation=new EditText(this);rotation.setHint("Döndürme açısı (°)");rotation.setText("0");rotation.setInputType(android.text.InputType.TYPE_CLASS_NUMBER|android.text.InputType.TYPE_NUMBER_FLAG_DECIMAL|android.text.InputType.TYPE_NUMBER_FLAG_SIGNED);box.addView(rotation);
        AlertDialog dialog=new AlertDialog.Builder(this)
            .setTitle("INSERT • "+displayName)
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
                dialog.dismiss();result.setText("INSERT • "+displayName+" • Yerleştirme noktasına dokunun");
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
            "DIV / DIVIDE • Seçili çizgi veya daireyi eşit böl\n"+
            "REVCLOUD • Bulut işareti çiz\n"+
            "MLEADER • Ok ve çoklu açıklama çiz\n"+
            "D / DIMSTYLE • Ölçülendirme stilini ayarla\n"+
            "DLI / DIMLINEAR • Yatay/dikey doğrusal ölçü\n"+
            "DAL / DIMALIGNED • Hizalı ölçü\n"+
            "DAN / DIMANGULAR • Açısal ölçü\n"+
            "DRA / DIMRADIUS • Yarıçap ölçüsü\n"+
            "DDI / DIMDIAMETER • Çap ölçüsü\n"+
            "LA / LAYER • Katman\n"+
            "PR / PROPERTIES / PROP • Özellik/Bilgi\n"+
            "DI / DIST / DISTANCE • Mesafe\n"+
            "AA / AREA • Alan\n"+
            "ANG / ANGLE • Açı ölç\n"+
            "ID • Nokta koordinatı\n"+
            "ARCLEN • Yay uzunluğu\n"+
            "Z E / ZE / ZOOM EXTENTS • Ekrana sığdır\n"+
            "Z / ZOOM • Zoom komutu\n"+
            "3D / 3DORBIT • 3B yüzey görünümü\n"+
            "2D • 2B görünüme dön\n"+
            "U / UNDO • Geri al\n"+
            "REDO • Geri alınan işlemi yeniden uygula\n"+
            "QS / QSAVE / SAVE • Kaydet";
        new AlertDialog.Builder(this)
            .setTitle("MusaCAD komutları")
            .setMessage(text)
            .setPositiveButton("TAMAM",null)
            .show();
    }

    private void noSourceSelection(){Toast.makeText(this,"Önce Seç ile düzenlenebilir bir nesne seçin",Toast.LENGTH_SHORT).show();}

    private void handleBackNavigation(){
        if(pendingPrintWindowSelection){pendingPrintWindowSelection=false;cad.cancelSelection();result.setText("Window yazdırma alanı seçimi iptal edildi");return;}
        if(activeLoad!=null){cancelLoad();Toast.makeText(this,"Devam eden işlem iptal edildi",Toast.LENGTH_SHORT).show();return;}
        if(currentProject!=null){
            requestCloseProject(currentProject);
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
    private void makeDialogButtonsReadable(AlertDialog dialog){
        if(dialog==null)return;
        int[] which={AlertDialog.BUTTON_POSITIVE,AlertDialog.BUTTON_NEUTRAL,AlertDialog.BUTTON_NEGATIVE};
        for(int id:which){
            Button button=dialog.getButton(id);
            if(button==null)continue;
            button.setTextColor(Color.WHITE);
            button.setTextSize(11.5f);
            button.setMinHeight(dp(48));
            button.setSingleLine(false);
            button.setMaxLines(2);
            button.setEllipsize(null);
        }
    }
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
        if(activeDxf==null)return;String missing=activeDxf.fontFallbacks.isEmpty()?"yok":android.text.TextUtils.join(", ",activeDxf.fontFallbacks);String oleTypes=activeDxf.oleTypes.isEmpty()?"yok":android.text.TextUtils.join(", ",activeDxf.oleTypes);String text="Dosya başarıyla açıldı.\n\n"+"Layout: "+activeDxf.activeLayout+" ("+activeDxf.layoutNames.size()+")\n"+"Nesne: "+activeDxf.entityCount+"\n"+"Katman: "+activeDxf.layerCount+"\n"+"Görünür katman: "+activeDxf.visibleLayers.size()+"\n"+"Seçilebilir kaynak nesne: "+activeDxf.editableSourceCount()+"\n"+"OLE / gömülü belge: "+activeDxf.oleObjectCount+" ("+oleTypes+")\n"+"OLE gerçek önizleme: "+activeDxf.olePreviewCount+"\n"+"OLE önizlemesi bulunamayan: "+activeDxf.oleMissingPreviewCount+"\n"+"Düzenleme toplamı: "+cad.editCount()+"\n"+"Kaynak nesne değişikliği: "+cad.sourceModifiedCount()+"\n"+"Eksik / fallback font: "+missing+"\n"+"Kullanıcı TTF/OTF: "+CadFontManager.userFontCount(this)+"\n"+"Complex SHX shape fallback: "+(activeDxf.externalShapeFallback?"var":"yok")+"\n"+"Düzenleme: "+(canEdit()?"açık":"yalnız görüntüleme")+"\n"+"Görüntüleme: vektörel / net yakınlaştırma";
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
        updateHomeFeatured();
        setEditorChromeVisible(false);
        if(welcomePanel!=null){welcomePanel.setVisibility(View.VISIBLE);welcomePanel.setAlpha(1f);}
    }
    private void updateHomeFeatured(){
        View card=findViewById(R.id.homeFeatured);ImageView preview=findViewById(R.id.homeFeaturedPreview);
        Object previous=preview.getTag();preview.setImageDrawable(null);preview.setTag(null);
        if(previous instanceof Bitmap&&!((Bitmap)previous).isRecycled())((Bitmap)previous).recycle();
        List<RecentFileStore.Entry> recents=RecentFileStore.list(this);
        if(recents.isEmpty()){homeFeaturedUri=null;card.setVisibility(View.GONE);return;}
        RecentFileStore.Entry latest=recents.get(0);homeFeaturedUri=Uri.parse(latest.uri);
        ((TextView)findViewById(R.id.homeFeaturedTitle)).setText(latest.name);
        ((TextView)findViewById(R.id.homeFeaturedMeta)).setText(latest.typeLabel()+"  •  "+RecentFileStore.accessLabel(latest.lastAccessMs));
        Bitmap thumb=RecentFileStore.thumbnail(this,latest);
        if(thumb!=null){preview.setImageBitmap(thumb);preview.setTag(thumb);}
        else preview.setImageResource(R.drawable.ic_musacad_mark);
        card.setVisibility(View.VISIBLE);
    }
    private void openFeaturedRecent(){
        if(homeFeaturedUri==null){open();return;}
        try(android.os.ParcelFileDescriptor fd=getContentResolver().openFileDescriptor(homeFeaturedUri,"r")){
            if(fd==null)throw new IOException("Dosya açılamadı");
        }catch(Exception e){
            RecentFileStore.remove(this,homeFeaturedUri.toString());updateHomeFeatured();
            Toast.makeText(this,"Bu dosyaya erişim yok. Yeniden seçin.",Toast.LENGTH_LONG).show();open();return;
        }
        startLoad(homeFeaturedUri);
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
        if(MusaAiVoiceInput.handleActivityResult(r,c,data))return;
        if(r==SAVE_DXF&&c!=RESULT_OK){pendingCloseAfterSave=null;return;}
        if(r==PICK_BOQ&&c!=RESULT_OK){
            MusaAiPanel.Reply reply=pendingBoqReply;pendingBoqReply=null;pendingBoqProject=null;
            if(reply!=null)reply.send("Keşif yükleme iptal edildi.");
            return;
        }
        if(r==PICK_STRUCT_CALC&&c!=RESULT_OK){
            MusaAiPanel.Reply reply=pendingStructuralCalcReply;pendingStructuralCalcReply=null;pendingStructuralCalcProject=null;
            if(reply!=null)reply.send("Statik hesap raporu yükleme iptal edildi.");
            return;
        }
        if(r==OPEN&&c!=RESULT_OK)pendingHomeCategory=0;
        if(c!=RESULT_OK||data==null)return;
        if(r==VIEW_DOCUMENT){handleDocumentImport(data);return;}
        if(data.getData()==null)return;
        if(r==PICK_AUDIO||r==PICK_IMAGE||r==PICK_VIDEO){handleMediaPicked(r,data.getData());return;}
        if(r==PICK_FONT){handleFontPicked(data.getData());return;}
        if(r==PICK_DOCUMENT){handleDocumentPicked(data.getData());return;}
        if(r==PICK_BOQ){handleBoqPicked(data.getData());return;}
        if(r==PICK_STRUCT_CALC){handleStructuralCalcPicked(data.getData());return;}
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
        String candidateName=nameOf(uri);String candidateMime=getContentResolver().getType(uri);
        if(CadDocumentSupport.isDocument(candidateName,candidateMime)){pendingHomeCategory=0;openDocumentViewer(uri,candidateName);return;}
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
                FileTransfer.checkCancelled();loaded.imageOverlays=CadImageOverlayStore.load(getApplicationContext(),uri.toString());loaded.name=nameOf(uri);loaded.dxf=loaded.name.toLowerCase(Locale.ROOT).endsWith(".dxf");loaded.file=File.createTempFile("MusaCAD_acilan_",loaded.dxf?".dxf":".dwg",getCacheDir());
                try(InputStream in=getContentResolver().openInputStream(uri);OutputStream out=new FileOutputStream(loaded.file)){FileTransfer.copy(in,out,512L*1024*1024,bytes->runOnUiThread(()->{if(activeLoad==task&&task.dialog!=null&&task.dialog.isShowing())task.progress.setText(String.format(Locale.getDefault(),"Okunan: %.1f MB",bytes/1048576d));}));}
                FileTransfer.checkCancelled();

                if(loaded.dxf){
                    runOnUiThread(()->{if(activeLoad==task)task.progress.setText("Vektör çizim hazırlanıyor…");});
                    loaded.parsed=DxfParser.render(loaded.file);loaded.workingDxf=loaded.file;loaded.bitmap=loaded.parsed==null?null:loaded.parsed.bitmap;
                    if(loaded.bitmap==null)throw new IOException("Desteklenen DXF geometrisi bulunamadı");
                    Bitmap recentPreview=loaded.bitmap;
                    runOnUiThread(()->{
                        if(activeLoad!=task||isFinishing()||isDestroyed()){loaded.dispose();return;}activeLoad=null;task.dialog.dismiss();
                        ProjectSession project=new ProjectSession();project.sourceUri=loaded.sourceUri;project.file=loaded.file;project.workingDxf=loaded.workingDxf;project.bitmap=loaded.bitmap;project.parsed=loaded.parsed;project.name=loaded.name;project.dxf=true;project.lastAccessMs=System.currentTimeMillis();project.persistedImages.addAll(loaded.imageOverlays);
                        loaded.project=project;loaded.handedOff=true;projects.add(project);activateProject(project);project.savedFingerprint=cad.editFingerprint();project.baselineSet=true;project.dirty=false;refreshProjectTabs();
                    });
                    RecentFileStore.record(getApplicationContext(),uri,loaded.name,recentPreview);
                    return;
                }

                // Do not expose the provisional native DWG scene to the user.
                // Its partial color semantics could make the first visible frame blue
                // and then change after DXF handoff. Keep the loading surface visible
                // until the authoritative vector model is ready so the first drawing
                // frame already has the final CAD colors.
                runOnUiThread(()->{if(activeLoad==task&&task.dialog!=null&&task.dialog.isShowing())task.progress.setText("DWG vektör model hazırlanıyor…");});
                try(NativeCadEngine engine=NativeCadEngine.open(loaded.file)){
                    FileTransfer.checkCancelled();
                    File converted=File.createTempFile("MusaCAD_donusen_",".dxf",getCacheDir());boolean keep=false;
                    try{
                        int status=engine.exportDxf(converted);FileTransfer.checkCancelled();
                        if(converted.length()>512L*1024*1024)throw new IOException("Dönüştürülen çizim 512 MB sınırını aşıyor");
                        // Always build the authoritative DXF-rendered navigation preview.
                        // Native first paint is only a loading bridge; pinch/pan after the full
                        // model is ready must never fall back to re-rendering the whole DWG.
                        DxfParser.Result parsed=DxfParser.render(converted,true);if(parsed==null)throw new IOException("DWG içinde desteklenen 2B nesne bulunamadı");parsed.conversionWarnings=status;
                        loaded.parsed=parsed;loaded.workingDxf=converted;loaded.bitmap=parsed.bitmap;keep=true;
                    }finally{if(!keep)converted.delete();}
                }

                if(loaded.handedOff&&loaded.project!=null){
                    ProjectSession project=loaded.project;DxfParser.Result parsed=loaded.parsed;File working=loaded.workingDxf;
                    runOnUiThread(()->{
                        if(isFinishing()||isDestroyed()){if(working!=null)working.delete();if(parsed.bitmap!=null&&!parsed.bitmap.isRecycled())parsed.bitmap.recycle();return;}
                        if(!projects.contains(project)){if(working!=null)working.delete();if(parsed.bitmap!=null&&!parsed.bitmap.isRecycled())parsed.bitmap.recycle();if(activeLoad==task)activeLoad=null;return;}
                        Bitmap oldPreview=project.bitmap;
                        project.prepareTask=null;project.workingDxf=working;project.parsed=parsed;project.bitmap=parsed.bitmap;project.preparingEditor=false;project.prepareError=null;
                        if(currentProject==project){
                            editingBaseDxf=working;activeDxf=parsed;cad.upgradeNativeDrawing(parsed);snapToggle.setEnabled(parsed.snapPoints.length>0);snapToggle.setChecked(true);cad.setSnapPoints(parsed.snapPoints);updateEditorEnabled(canEdit());updateLayerButtons(true);renderCurrentProjectStatus();
                            project.savedFingerprint=cad.editFingerprint();project.baselineSet=true;project.dirty=false;
                            if(pendingHomeCategory!=0)cad.post(this::showPendingHomeCategory);
                        }else{
                            if(project.nativeScene!=null)project.nativeScene.alignTo(parsed.drawingToContentMatrix());
                            project.viewState=null;
                        }
                        if(oldPreview!=null&&oldPreview!=parsed.bitmap&&!oldPreview.isRecycled())oldPreview.recycle();
                        if(activeLoad==task)activeLoad=null;refreshProjectTabs();
                        // Metadata/thumbnail persistence is secondary to showing the complete model.
                        scheduleRecentMetadataRecord(uri,loaded.name);
                    });
                    return;
                }

                if(loaded.bitmap==null)throw new IOException("DWG içinde görüntülenebilir geometri bulunamadı");
                Bitmap recentPreview=loaded.bitmap;
                runOnUiThread(()->{
                    if(activeLoad!=task||isFinishing()||isDestroyed()){loaded.dispose();return;}activeLoad=null;if(task.dialog!=null)task.dialog.dismiss();
                    ProjectSession project=new ProjectSession();project.sourceUri=loaded.sourceUri;project.file=loaded.file;project.workingDxf=loaded.workingDxf;project.bitmap=loaded.bitmap;project.parsed=loaded.parsed;project.name=loaded.name;project.dxf=false;project.lastAccessMs=System.currentTimeMillis();project.persistedImages.addAll(loaded.imageOverlays);
                    loaded.project=project;loaded.handedOff=true;projects.add(project);activateProject(project);project.savedFingerprint=cad.editFingerprint();project.baselineSet=true;project.dirty=false;refreshProjectTabs();
                });
                RecentFileStore.record(getApplicationContext(),uri,loaded.name,recentPreview);
            }catch(Exception|OutOfMemoryError e){
                if(loaded.handedOff&&loaded.project!=null){
                    ProjectSession project=loaded.project;String message=e.getMessage()==null?"Tam vektör model hazırlanamadı":e.getMessage();
                    if(loaded.workingDxf!=null&&loaded.workingDxf!=project.workingDxf)loaded.workingDxf.delete();
                    runOnUiThread(()->{
                        if(activeLoad==task)activeLoad=null;
                        if(projects.contains(project)){project.prepareTask=null;project.preparingEditor=false;project.prepareError=message;if(currentProject==project){pendingHomeCategory=0;updateEditorEnabled(false);renderCurrentProjectStatus();Toast.makeText(this,"Hızlı görünüm açık; düzenleme modeli hazırlanamadı",Toast.LENGTH_LONG).show();}}
                    });
                }else{
                    loaded.dispose();runOnUiThread(()->{if(activeLoad!=task||isFinishing()||isDestroyed())return;activeLoad=null;if(task.dialog!=null)task.dialog.dismiss();error(e instanceof Exception?(Exception)e:new IOException("Bu çizim için yeterli bellek yok"));});
                }
            }
        });
    }

    private void scheduleRecentNativeRecord(Uri uri,String name,NativeScene scene){
        if(uri==null)return;
        final android.content.Context app=getApplicationContext();
        final String safeName=name==null?"Çizim":name;
        final NativeScene nativeScene=scene;
        recentExecutor.submit(()->{
            android.os.Process.setThreadPriority(android.os.Process.THREAD_PRIORITY_BACKGROUND);
            Bitmap thumb=null;
            try{
                if(nativeScene!=null)thumb=nativeScene.thumbnail(360,240);
                RecentFileStore.record(app,uri,safeName,thumb);
            }catch(Throwable ignored){
                try{RecentFileStore.record(app,uri,safeName,null);}catch(Throwable ignoredAgain){}
            }finally{
                if(thumb!=null&&!thumb.isRecycled())thumb.recycle();
            }
        });
    }

    private void scheduleRecentMetadataRecord(Uri uri,String name){
        if(uri==null)return;
        final android.content.Context app=getApplicationContext();
        final String safeName=name==null?"Çizim":name;
        recentExecutor.submit(()->{
            android.os.Process.setThreadPriority(android.os.Process.THREAD_PRIORITY_BACKGROUND);
            try{RecentFileStore.record(app,uri,safeName,null);}catch(Throwable ignored){}
        });
    }

    private ProjectSession findProject(Uri uri){
        if(uri==null)return null;String key=uri.toString();
        for(ProjectSession p:projects)if(p.sourceUri!=null&&key.equals(p.sourceUri.toString()))return p;
        return null;
    }
    private boolean hasPreparingProject(){for(ProjectSession p:projects)if(p.preparingEditor)return true;return false;}

    private File recoveryDir(){
        File dir=new File(getFilesDir(),"recovery");
        if(!dir.exists())dir.mkdirs();
        return dir;
    }

    private String ensureRecoveryId(ProjectSession project){
        if(project==null)return null;
        if(project.recoveryId!=null&&!project.recoveryId.trim().isEmpty())return project.recoveryId;
        String seed=project.sourceUri==null?"blank:"+(project.name==null?"cizim.dxf":project.name)+":"+project.lastAccessMs:project.sourceUri.toString();
        project.recoveryId=RecoveryStore.idFor(seed);
        return project.recoveryId;
    }

    private void queueRecoveryForCurrent(boolean force){
        final ProjectSession project=currentProject;
        final DxfParser.Result drawing=activeDxf;
        final File base=editingBaseDxf;
        if(project==null||drawing==null||base==null||!base.isFile()||activeLoad!=null||!project.baselineSet)return;

        final long fingerprint=cad.editFingerprint();
        project.dirty=fingerprint!=project.savedFingerprint;
        if(!project.dirty){
            clearRecovery(project);
            project.recoveryFingerprint=fingerprint;
            return;
        }
        if(!force&&project.recoveryFingerprint==fingerprint)return;

        final String id=ensureRecoveryId(project);
        final String displayName=project.name==null?"Kurtarılan çizim.dxf":project.name;
        final String sourceUri=project.sourceUri==null?"":project.sourceUri.toString();
        final String defaultLayer=project.defaultLayer==null?"0":project.defaultLayer;
        final List<CadEdit> additions=new ArrayList<>(cad.getAddedEdits());
        final List<SourceReplacement> replacements=new ArrayList<>(cad.getSourceReplacements());
        final List<SourceRange> removals=new ArrayList<>(cad.getSourceRemovals());
        final List<CadBlock.Definition> blocks=new ArrayList<>(cad.getUserBlocks());
        project.recoveryFingerprint=fingerprint;

        recoveryExecutor.submit(()->{
            try{
                RecoveryStore.write(recoveryDir(),id,displayName,sourceUri,fingerprint,System.currentTimeMillis(),
                    out->DxfWriter.write(base,out,drawing,additions,replacements,removals,blocks,defaultLayer));
                RecoveryStore.prune(recoveryDir(),MAX_OPEN_PROJECTS);
            }catch(Exception e){
                project.recoveryFingerprint=Long.MIN_VALUE;
                android.util.Log.w("MusaCAD","Recovery snapshot failed",e);
            }
        });
    }

    private void clearRecovery(ProjectSession project){
        if(project==null)return;
        String id=project.recoveryId;
        if(id!=null&&!id.trim().isEmpty())RecoveryStore.delete(recoveryDir(),id);
        project.recoveryFingerprint=Long.MIN_VALUE;
    }

    private void offerRecoveryIfIdle(){
        if(recoveryPromptShown||isFinishing()||isDestroyed())return;
        if(activeLoad!=null||!projects.isEmpty()){
            if(activeLoad!=null)recoveryHandler.postDelayed(this::offerRecoveryIfIdle,900L);
            return;
        }
        List<RecoveryStore.Record> records=RecoveryStore.list(recoveryDir());
        if(records.isEmpty())return;
        final RecoveryStore.Record record=records.get(0);
        recoveryPromptShown=true;
        String when=android.text.format.DateFormat.format("dd.MM.yyyy HH:mm",new Date(record.savedAtMs)).toString();
        new AlertDialog.Builder(this)
            .setTitle("Otomatik kurtarma")
            .setMessage(record.displayName+" için kaydedilmemiş bir çalışma bulundu.\n\nSon kurtarma: "+when)
            .setPositiveButton("KURTAR",(d,w)->restoreRecoveryRecord(record))
            .setNeutralButton("SİL",(d,w)->{
                RecoveryStore.delete(recoveryDir(),record.id);
                recoveryPromptShown=false;
                recoveryHandler.post(this::offerRecoveryIfIdle);
            })
            .setNegativeButton("SONRA",null)
            .show();
    }

    private void restoreRecoveryRecord(RecoveryStore.Record record){
        if(record==null||record.dxfFile==null||!record.dxfFile.isFile()||activeLoad!=null)return;
        cancelLoad();
        LoadTask task=new LoadTask();activeLoad=task;
        LinearLayout box=new LinearLayout(this);box.setOrientation(LinearLayout.VERTICAL);int pad=dp(20);box.setPadding(pad,pad,pad,pad);box.addView(new ProgressBar(this));
        task.progress=new TextView(this);task.progress.setText("Kurtarma kopyası hazırlanıyor…");box.addView(task.progress);
        task.dialog=new AlertDialog.Builder(this).setTitle("Çalışma kurtarılıyor").setView(box).setNegativeButton("İPTAL",(d,w)->cancelLoad()).create();
        task.dialog.setOnCancelListener(d->cancelLoad());task.dialog.setCanceledOnTouchOutside(false);task.dialog.show();

        task.future=loader.submit(()->{
            File temp=null;
            try{
                temp=File.createTempFile("MusaCAD_kurtarma_",".dxf",getCacheDir());
                try(InputStream in=new FileInputStream(record.dxfFile);OutputStream out=new FileOutputStream(temp)){
                    FileTransfer.copy(in,out,512L*1024*1024,null);
                }
                FileTransfer.checkCancelled();
                DxfParser.Result parsed=DxfParser.render(temp);
                if(parsed==null||parsed.bitmap==null)throw new IOException("Kurtarma kopyasında görüntülenebilir geometri bulunamadı");
                final File recovered=temp;temp=null;
                runOnUiThread(()->{
                    if(activeLoad!=task||isFinishing()||isDestroyed()){if(parsed.bitmap!=null&&!parsed.bitmap.isRecycled())parsed.bitmap.recycle();recovered.delete();return;}
                    activeLoad=null;task.dialog.dismiss();
                    ProjectSession project=new ProjectSession();
                    project.sourceUri=record.sourceUri==null||record.sourceUri.trim().isEmpty()?null:Uri.parse(record.sourceUri);
                    project.file=recovered;project.workingDxf=recovered;project.bitmap=parsed.bitmap;project.parsed=parsed;project.name=record.displayName;project.dxf=true;project.lastAccessMs=System.currentTimeMillis();
                    project.recoveryId=record.id;project.recoveryFingerprint=record.fingerprint;
                    projects.add(project);activateProject(project);
                    project.savedFingerprint=Long.MIN_VALUE;project.baselineSet=true;project.dirty=true;
                    refreshProjectTabs();
                    result.setText("Kurtarılan çalışma • Kaydet / DXF dışa aktar ile kalıcı dosya oluşturun");
                    Toast.makeText(this,"Kurtarma kopyası açıldı",Toast.LENGTH_LONG).show();
                });
            }catch(Exception|OutOfMemoryError e){
                if(temp!=null)temp.delete();
                final Exception error=e instanceof Exception?(Exception)e:new IOException("Kurtarma için yeterli bellek yok");
                runOnUiThread(()->{
                    if(activeLoad!=task||isFinishing()||isDestroyed())return;
                    activeLoad=null;if(task.dialog!=null)task.dialog.dismiss();error(error);
                });
            }
        });
    }

    @Override protected void onResume(){
        super.onResume();
        recoveryHandler.removeCallbacks(recoveryTicker);
        recoveryHandler.postDelayed(recoveryTicker,RECOVERY_INTERVAL_MS);
    }

    @Override protected void onPause(){
        queueRecoveryForCurrent(true);
        recoveryHandler.removeCallbacks(recoveryTicker);
        super.onPause();
    }

    private void captureCurrentProject(){
        if(currentProject==null)return;
        currentProject.file=currentFile;currentProject.workingDxf=editingBaseDxf;currentProject.parsed=activeDxf;currentProject.name=currentDisplayName;
        currentProject.viewState=cad.captureSessionState();currentProject.dirty=currentProject.baselineSet&&cad.editFingerprint()!=currentProject.savedFingerprint;currentProject.lastAccessMs=System.currentTimeMillis();
        queueRecoveryForCurrent(false);
        // Keep the authoritative navigation preview alive while a project tab is open.
        // It is the zero-jank pinch/pan cache; recycling it here forced large drawings
        // back onto the full vector renderer after every tab switch.
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
        if(project.viewState==null&&!project.persistedImages.isEmpty()){cad.restoreImageOverlays(project.persistedImages);project.persistedImages.clear();}
        hideWelcomePanel();markModeSelected(R.id.panButton);
        snapToggle.setEnabled(project.parsed!=null&&project.parsed.snapPoints.length>0);snapToggle.setChecked(true);if(project.parsed!=null)cad.setSnapPoints(project.parsed.snapPoints);
        updateShareEnabled(true);updateEditorEnabled(canEdit());updateLayerButtons(activeDxf!=null);renderCurrentProjectStatus();refreshProjectTabs();
        if(pendingHomeCategory!=0&&!project.preparingEditor)cad.postDelayed(this::showPendingHomeCategory,220);
    }

    private void renderCurrentProjectStatus(){
        if(currentProject==null){fileName.setText("Henüz proje açılmadı");result.setText("Hazır");return;}
        String editable=canEdit()?"  •  düzenlenebilir":"";
        String mode=currentProject.dxf?"  •  DXF":activeDxf!=null?"  •  DWG":currentProject.nativeScene!=null?"  •  DWG Native":"  •  DWG önizleme";
        fileName.setText(currentDisplayName+mode+editable);
        if(activeDxf!=null){
            String fallback=activeDxf.fontFallbacks.isEmpty()?(activeDxf.externalShapeFallback?"  •  SHX shape fallback":""):"  •  eksik font "+activeDxf.fontFallbacks.size();
            String ole=activeDxf.oleObjectCount>0?"  •  OLE "+activeDxf.olePreviewCount+"/"+activeDxf.oleObjectCount+" önizleme":"";
            result.setText("Hazır  •  "+activeDxf.activeLayout+"  •  "+activeDxf.entityCount+" nesne  •  "+activeDxf.layerCount+" katman  •  "+activeDxf.editableSourceCount()+" seçilebilir"+(canEdit()?"  •  düzenleme açık":"")+ole+fallback);
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
        if(projectCloseDialog!=null&&projectCloseDialog.isShowing())return;

        final boolean dirty=isProjectDirty(project);
        String name=project.name==null||project.name.trim().isEmpty()?"Bu proje":project.name;
        String message=dirty
            ? name+" içinde kaydedilmemiş değişiklikler var.\n\nKaydetmek, kaydetmeden kapatmak veya işlemi iptal etmek için seçim yapın.\n\nİkinci kez geri tuşuna basarsanız proje kaydedilmeden kapanır; MusaCAD açık kalır."
            : name+" kapatılsın mı?\n\nİkinci kez geri tuşuna basarsanız proje kapanır; MusaCAD açık kalır.";

        AlertDialog.Builder builder=new AlertDialog.Builder(this)
            .setTitle(dirty?"Projeyi kapat • Kaydedilmemiş değişiklikler":"Projeyi kapat")
            .setMessage(message)
            .setNegativeButton("İPTAL",null);

        if(dirty){
            builder.setPositiveButton("KAYDET VE KAPAT",(d,w)->{
                pendingCloseAfterSave=project;
                requestEditedDxfSave();
            });
            builder.setNeutralButton("KAYDETMEDEN KAPAT",(d,w)->closeProjectNow(project));
        }else{
            builder.setPositiveButton("KAPAT",(d,w)->closeProjectNow(project));
        }

        projectCloseTarget=project;
        projectCloseDialog=builder.create();
        projectCloseDialog.setCanceledOnTouchOutside(false);
        projectCloseDialog.setOnKeyListener((dialog,keyCode,event)->{
            if(keyCode==KeyEvent.KEYCODE_BACK&&event.getAction()==KeyEvent.ACTION_UP){
                ProjectSession target=projectCloseTarget;
                dialog.dismiss();
                projectCloseDialog=null;
                projectCloseTarget=null;
                if(target!=null&&projects.contains(target))closeProjectNow(target);
                return true;
            }
            return keyCode==KeyEvent.KEYCODE_BACK;
        });
        projectCloseDialog.setOnDismissListener(d->{
            if(projectCloseDialog==d){
                projectCloseDialog=null;
                projectCloseTarget=null;
            }
        });
        projectCloseDialog.show();
        makeDialogButtonsReadable(projectCloseDialog);
    }

    private void closeProjectNow(ProjectSession project){
        if(project==null)return;
        if(projectCloseDialog!=null&&projectCloseDialog.isShowing())projectCloseDialog.dismiss();
        projectCloseDialog=null;projectCloseTarget=null;
        clearRecovery(project);
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

    @Override protected void onDestroy(){recoveryHandler.removeCallbacks(recoveryTicker);recoveryExecutor.shutdownNow();recentExecutor.shutdownNow();aiExecutor.shutdownNow();cancelLoad();ImageView featured=findViewById(R.id.homeFeaturedPreview);if(featured!=null){Object old=featured.getTag();featured.setImageDrawable(null);if(old instanceof Bitmap&&!((Bitmap)old).isRecycled())((Bitmap)old).recycle();}releaseAllProjects();loader.shutdownNow();super.onDestroy();}

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
        LinearLayout box=new LinearLayout(this);box.setOrientation(LinearLayout.VERTICAL);int p=dp(16);box.setPadding(p,p/2,p,p/2);
        EditText input=new EditText(this);input.setHint("Çizime eklenecek yazı");input.setSingleLine(false);input.setMaxLines(3);box.addView(input);
        TextView fontLabel=new TextView(this);fontLabel.setText("Yazı tipi");fontLabel.setPadding(0,p/2,0,0);box.addView(fontLabel);
        List<CadFontManager.Choice> fonts=CadFontManager.choices(this);
        Spinner spinner=new Spinner(this);spinner.setAdapter(new ArrayAdapter<>(this,android.R.layout.simple_spinner_dropdown_item,fonts));box.addView(spinner);
        CadFontManager.Choice preferred=CadFontManager.defaultChoice(this);int selected=0;
        for(int i=0;i<fonts.size();i++)if(fonts.get(i).hint.equalsIgnoreCase(preferred.hint)&&fonts.get(i).shx==preferred.shx){selected=i;break;}
        spinner.setSelection(selected);
        TextView preview=new TextView(this);preview.setText("MusaCAD • AaBbÇçĞğİıÖöŞşÜü 0123");preview.setTextSize(20f);preview.setPadding(0,p/2,0,p/2);box.addView(preview);
        android.widget.AdapterView.OnItemSelectedListener listener=new android.widget.AdapterView.OnItemSelectedListener(){
            public void onItemSelected(android.widget.AdapterView<?> parent,View view,int position,long id){CadFontManager.Choice c=fonts.get(position);preview.setTypeface(CadFontManager.resolveTypeface(c.hint,c.shx,Typeface.NORMAL));}
            public void onNothingSelected(android.widget.AdapterView<?> parent){}
        };spinner.setOnItemSelectedListener(listener);
        AlertDialog dialog=new AlertDialog.Builder(this).setTitle("Yazı ekle").setView(box).setPositiveButton("EKLE",null).setNeutralButton("FONT YÖNETİCİSİ",null).setNegativeButton("İPTAL",null).create();
        dialog.setOnShowListener(d->{
            dialog.getButton(AlertDialog.BUTTON_POSITIVE).setOnClickListener(v->{String text=input.getText().toString().trim();if(text.isEmpty()){input.setError("Bir yazı girin");return;}CadFontManager.Choice choice=fonts.get(Math.max(0,spinner.getSelectedItemPosition()));CadFontManager.setDefaultChoice(this,choice);cad.addTextEdit(x,y,text,choice);dialog.dismiss();});
            dialog.getButton(AlertDialog.BUTTON_NEUTRAL).setOnClickListener(v->{dialog.dismiss();showFontManager();});
        });dialog.show();
    }

    private void showFontManager(){
        List<CadFontManager.Choice> fonts=CadFontManager.choices(this);
        LinearLayout box=new LinearLayout(this);box.setOrientation(LinearLayout.VERTICAL);int p=dp(16);box.setPadding(p,p/2,p,p/2);
        TextView info=new TextView(this);
        String missing=activeDxf==null||activeDxf.fontFallbacks.isEmpty()?"Eksik font: yok":"Eksik / fallback: "+android.text.TextUtils.join(", ",activeDxf.fontFallbacks);
        info.setText("Sistem + kullanıcı TTF/OTF fontları ve yaygın SHX eşlemeleri\n"+missing+"\nKullanıcı fontları: "+CadFontManager.userFontCount(this));box.addView(info);
        Spinner spinner=new Spinner(this);spinner.setAdapter(new ArrayAdapter<>(this,android.R.layout.simple_spinner_dropdown_item,fonts));box.addView(spinner);
        String selectedHint=cad.selectedTextFontHint();CadFontManager.Choice preferred=selectedHint==null?CadFontManager.defaultChoice(this):CadFontManager.findChoice(this,selectedHint,CadFontPolicy.isShx(selectedHint));
        int selected=0;for(int i=0;i<fonts.size();i++)if(fonts.get(i).hint.equalsIgnoreCase(preferred.hint)&&fonts.get(i).shx==preferred.shx){selected=i;break;}spinner.setSelection(selected);
        TextView preview=new TextView(this);preview.setText("Önizleme • MusaCAD 0123456789");preview.setTextSize(22f);preview.setPadding(0,p/2,0,p/2);box.addView(preview);
        spinner.setOnItemSelectedListener(new android.widget.AdapterView.OnItemSelectedListener(){
            public void onItemSelected(android.widget.AdapterView<?> parent,View view,int position,long id){CadFontManager.Choice c=fonts.get(position);preview.setTypeface(CadFontManager.resolveTypeface(c.hint,c.shx,Typeface.NORMAL));}
            public void onNothingSelected(android.widget.AdapterView<?> parent){}
        });
        AlertDialog dialog=new AlertDialog.Builder(this).setTitle("Font Yöneticisi").setView(box).setPositiveButton("UYGULA / VARSAYILAN",null).setNeutralButton("TTF/OTF YÜKLE",null).setNegativeButton("KAPAT",null).create();
        dialog.setOnShowListener(d->{
            dialog.getButton(AlertDialog.BUTTON_POSITIVE).setOnClickListener(v->{CadFontManager.Choice choice=fonts.get(Math.max(0,spinner.getSelectedItemPosition()));CadFontManager.setDefaultChoice(this,choice);boolean applied=cad.updateSelectedTextFont(choice);result.setText(applied?"Font • seçili metne uygulandı • "+choice.displayName:"Font • yeni metinler için varsayılan • "+choice.displayName);dialog.dismiss();});
            dialog.getButton(AlertDialog.BUTTON_NEUTRAL).setOnClickListener(v->{dialog.dismiss();pickFontFile();});
        });dialog.show();
    }

    private void pickFontFile(){
        Intent intent=new Intent(Intent.ACTION_OPEN_DOCUMENT);intent.addCategory(Intent.CATEGORY_OPENABLE);intent.setType("*/*");
        intent.putExtra(Intent.EXTRA_MIME_TYPES,new String[]{"font/ttf","font/otf","application/x-font-ttf","application/x-font-opentype","application/octet-stream"});
        startActivityForResult(intent,PICK_FONT);
    }

    private void handleFontPicked(Uri uri){
        if(uri==null)return;
        new Thread(()->{
            try{
                CadFontManager.Choice choice=CadFontManager.importFont(this,uri);
                runOnUiThread(()->{result.setText("Font yüklendi • "+choice.displayName);showFontManager();});
            }catch(Exception e){runOnUiThread(()->Toast.makeText(this,"Font yüklenemedi: "+e.getMessage(),Toast.LENGTH_LONG).show());}
        },"MusaCAD-font-import").start();
    }

    private void requestEditedDxfSave(){
        if(activeLoad!=null){Toast.makeText(this,"Devam eden işlem bitmeden kaydedilemez",Toast.LENGTH_SHORT).show();return;}if(!canEdit()){Toast.makeText(this,"Bu çizim DXF olarak düzenlenebilir durumda değil",Toast.LENGTH_SHORT).show();return;}
        Intent intent=new Intent(Intent.ACTION_CREATE_DOCUMENT);intent.addCategory(Intent.CATEGORY_OPENABLE);intent.setType("application/octet-stream");String base=currentDisplayName==null?"cizim":currentDisplayName.replaceFirst("(?i)\\.(dwg|dxf)$","");intent.putExtra(Intent.EXTRA_TITLE,base+"_duzenlendi.dxf");startActivityForResult(intent,SAVE_DXF);
    }

    private void saveEditedDxf(Uri uri){
        if(!canEdit()||activeLoad!=null)return;final File base=editingBaseDxf;final DxfParser.Result drawing=activeDxf;final List<CadEdit> additions=cad.getAddedEdits();final List<SourceReplacement> replacements=cad.getSourceReplacements();final List<SourceRange> removals=cad.getSourceRemovals();final List<CadBlock.Definition> blocks=cad.getUserBlocks();final List<CadImageOverlay> rasterOverlays=cad.getImageOverlays();final String defaultLayer=currentProject==null?"0":currentProject.defaultLayer;final String sourceOverlayKey=currentProject==null||currentProject.sourceUri==null?null:currentProject.sourceUri.toString();final String destinationOverlayKey=uri.toString();final int rasterCount=rasterOverlays.size();final int total=additions.size()+removals.size()+blocks.size();
        LoadTask task=new LoadTask();activeLoad=task;LinearLayout box=new LinearLayout(this);box.setOrientation(LinearLayout.VERTICAL);int pad=dp(20);box.setPadding(pad,pad,pad,pad);box.addView(new ProgressBar(this));task.progress=new TextView(this);task.progress.setText("DXF hazırlanıyor…");box.addView(task.progress);task.dialog=new AlertDialog.Builder(this).setTitle("Düzenlenmiş DXF kaydediliyor").setView(box).setNegativeButton("İPTAL",(d,w)->cancelLoad()).create();task.dialog.setOnCancelListener(d->cancelLoad());task.dialog.setCanceledOnTouchOutside(false);task.dialog.show();
        task.future=loader.submit(()->{try(OutputStream out=getContentResolver().openOutputStream(uri,"wt")){if(out==null)throw new IOException("Kaydedilecek dosya açılamadı");DxfWriter.write(base,out,drawing,additions,replacements,removals,blocks,defaultLayer);FileTransfer.checkCancelled();
                    CadImageOverlayStore.save(getApplicationContext(),destinationOverlayKey,rasterOverlays);
                    if(sourceOverlayKey!=null&&!sourceOverlayKey.equals(destinationOverlayKey))CadImageOverlayStore.save(getApplicationContext(),sourceOverlayKey,rasterOverlays);
                    FileTransfer.checkCancelled();runOnUiThread(()->{if(activeLoad!=task||isFinishing()||isDestroyed())return;activeLoad=null;task.dialog.dismiss();
                    if(currentProject!=null){currentProject.savedFingerprint=cad.editFingerprint();currentProject.baselineSet=true;currentProject.dirty=false;currentProject.viewState=cad.captureSessionState();clearRecovery(currentProject);}
                    Toast.makeText(this,rasterCount==0?"DXF kaydedildi • "+total+" düzenleme":"DXF kaydedildi • "+rasterCount+" görüntünün konumu MusaCAD proje verisine kaydedildi (DXF içine gömülmez)",Toast.LENGTH_LONG).show();
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

    private void printDrawing(){printDrawing(false);}

    private void printDrawing(boolean preferWindow){
        if(currentFile==null||!currentFile.exists()){Toast.makeText(this,"Yazdırmak için önce bir DWG veya DXF dosyası açın",Toast.LENGTH_SHORT).show();return;}Bitmap preview=null;
        try{
            RectF displayBounds=cad.visibleContentBounds(),windowBounds=cad.selectedAreaContentBounds();
            if(activeDxf==null){preview=DwgPreview.read(currentFile);if(preview==null)preview=cad.snapshot();}
            CadPrint.show(this,activeDxf,cad.getAddedEdits(),cad.getSourceReplacements(),cad.getHiddenSourceIds(),cad.getUserBlocks(),cad.getImageOverlays(),
                preview,currentDisplayName,displayBounds,windowBounds,preferWindow,()->{
                    pendingPrintWindowSelection=true;
                    if(!cad.beginSelection()){pendingPrintWindowSelection=false;Toast.makeText(this,"Window için çizim alanı seçilemedi",Toast.LENGTH_LONG).show();return;}
                    result.setText("Window • Yazdırılacak alanın iki köşesini sürükleyerek seçin");
                });
        }catch(Exception e){if(preview!=null&&!preview.isRecycled())preview.recycle();error(e);}
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
