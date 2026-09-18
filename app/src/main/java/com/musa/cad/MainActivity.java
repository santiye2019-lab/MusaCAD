package com.musa.cad;

import android.app.*;
import android.content.*;
import android.content.res.ColorStateList;
import android.graphics.*;
import android.graphics.pdf.PdfDocument;
import android.graphics.drawable.GradientDrawable;
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
    private static final int MAX_OPEN_DOCUMENTS=4;
    private static final String[] LINE_WEIGHT_LABELS={"BYLAYER","BYBLOCK","DEFAULT","0.00 mm","0.05 mm","0.09 mm","0.13 mm","0.15 mm","0.18 mm","0.20 mm","0.25 mm","0.30 mm","0.35 mm","0.40 mm","0.50 mm","0.53 mm","0.60 mm","0.70 mm","0.80 mm","0.90 mm","1.00 mm","1.06 mm","1.20 mm","1.40 mm","1.58 mm","2.00 mm","2.11 mm"};
    private static final int[] LINE_WEIGHT_VALUES={DxfLineStyle.LW_BYLAYER,DxfLineStyle.LW_BYBLOCK,DxfLineStyle.LW_DEFAULT,0,5,9,13,15,18,20,25,30,35,40,50,53,60,70,80,90,100,106,120,140,158,200,211};
    private interface ColorPickListener { void onPick(int mode,int value); }
    private static final int MENU_OPEN=1,MENU_LAYERS=2,MENU_FIT=3,MENU_SHARE=4,MENU_INFO=5,MENU_ABOUT=6,MENU_SAVE_DXF=7,MENU_PRINT=8,MENU_LAYOUTS=9;
    private final ExecutorService loader=Executors.newSingleThreadExecutor();
    private LoadTask activeLoad;

    private static final class LoadTask {Future<?> future;AlertDialog dialog;TextView progress;}
    private static final class Loaded {
        File file,workingDxf;Bitmap bitmap;DxfParser.Result parsed;String name;boolean dxf;
        void dispose(){if(bitmap!=null&&!bitmap.isRecycled())bitmap.recycle();if(workingDxf!=null&&workingDxf!=file)workingDxf.delete();if(file!=null)file.delete();}
    }
    private static final class DocumentSession {
        File file,workingDxf;Bitmap bitmap;DxfParser.Result parsed;String name;boolean dxf;
        CadView.SessionState viewState;boolean dirty;long savedEditSignature;
        void dispose(){
            Bitmap owned=parsed!=null?parsed.bitmap:bitmap;if(owned!=null&&!owned.isRecycled())owned.recycle();
            if(workingDxf!=null&&workingDxf!=file)workingDxf.delete();if(file!=null)file.delete();
            file=null;workingDxf=null;bitmap=null;parsed=null;viewState=null;
        }
    }

    private CheckBox snapToggle;
    private DxfParser.Result activeDxf;
    private CadView cad;
    private TextView fileName,result;
    private EditText commandInput;
    private File currentFile,editingBaseDxf;
    private String currentDisplayName="cizim.dwg";
    private View[] modeButtons;
    private View welcomePanel,shareButton,shareToolButton,documentTabScroll;
    private LinearLayout documentTabs;
    private final ArrayList<DocumentSession> documents=new ArrayList<>();
    private int activeDocumentIndex=-1,backCloseStage=0;
    private long backCloseStageAt;
    private String lastCommandRaw="";
    private boolean closeActiveAfterSave;

    @Override protected void onCreate(Bundle b){
        super.onCreate(b);WindowCompat.setDecorFitsSystemWindows(getWindow(),false);setContentView(R.layout.activity_main);
        View root=findViewById(R.id.mainRoot);
        ViewCompat.setOnApplyWindowInsetsListener(root,(v,insets)->{Insets bars=insets.getInsets(WindowInsetsCompat.Type.systemBars());v.setPadding(0,bars.top,0,bars.bottom+dp(6));return insets;});

        cad=findViewById(R.id.cadView);fileName=findViewById(R.id.fileName);result=findViewById(R.id.resultText);welcomePanel=findViewById(R.id.welcomePanel);commandInput=findViewById(R.id.commandInput);
        shareButton=findViewById(R.id.shareButton);shareToolButton=findViewById(R.id.shareToolButton);documentTabs=findViewById(R.id.documentTabs);documentTabScroll=findViewById(R.id.documentTabScroll);
        cad.setListener(new CadView.Listener(){
            public void onMeasurement(String v){result.setText(v);}
            public void onCalibrationRequested(double px){showCalibration();}
            public void onSelectionReady(){previewSelection();}
            public void onTextRequested(float x,float y){showTextEditor(x,y);}
            public void onDocumentChanged(){refreshDocumentTabs();}
            public void onCadPropertiesChanged(){refreshPropertyButtons();}
        });

        snapToggle=findViewById(R.id.snapToggle);snapToggle.setOnCheckedChangeListener((button,checked)->cad.setSnapEnabled(checked));
        modeButtons=new View[]{findViewById(R.id.panButton),findViewById(R.id.selectEntityButton),findViewById(R.id.calibrateButton),findViewById(R.id.distanceButton),findViewById(R.id.areaButton),findViewById(R.id.lineButton),findViewById(R.id.polylineButton),findViewById(R.id.rectangleButton),findViewById(R.id.circleButton),findViewById(R.id.textButton)};
        markModeSelected(R.id.panButton);

        int[] interactive={R.id.menuButton,R.id.openButton,R.id.shareButton,R.id.quickOpenButton,R.id.layersButton,R.id.propertiesButton,R.id.colorButton,R.id.lineWeightButton,R.id.snapToggle,R.id.panButton,R.id.selectEntityButton,R.id.moveEntityButton,R.id.rotateEntityButton,R.id.copyEntityButton,R.id.deleteEntityButton,R.id.calibrateButton,R.id.distanceButton,R.id.areaButton,R.id.lineButton,R.id.polylineButton,R.id.rectangleButton,R.id.circleButton,R.id.textButton,R.id.finishEditButton,R.id.saveDxfButton,R.id.zoomInButton,R.id.zoomOutButton,R.id.fitButton,R.id.undoButton,R.id.clearButton,R.id.shareToolButton,R.id.commandSendButton};
        for(int id:interactive)installInteractiveFeedback(findViewById(id));

        findViewById(R.id.menuButton).setOnClickListener(this::showMainMenu);findViewById(R.id.appTitle).setOnClickListener(this::showMainMenu);
        findViewById(R.id.layersButton).setOnClickListener(v->showLayers());findViewById(R.id.propertiesButton).setOnClickListener(v->showDrawingProperties());findViewById(R.id.colorButton).setOnClickListener(v->showQuickColor());findViewById(R.id.lineWeightButton).setOnClickListener(v->showQuickLineWeight());findViewById(R.id.openButton).setOnClickListener(v->open());findViewById(R.id.quickOpenButton).setOnClickListener(v->open());
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
        findViewById(R.id.fitButton).setOnClickListener(v->{v.performHapticFeedback(HapticFeedbackConstants.KEYBOARD_TAP);cad.fitToScreen();});
        findViewById(R.id.undoButton).setOnClickListener(v->{v.performHapticFeedback(HapticFeedbackConstants.KEYBOARD_TAP);cad.undo();});
        findViewById(R.id.clearButton).setOnClickListener(v->{v.performHapticFeedback(HapticFeedbackConstants.KEYBOARD_TAP);cad.clearMeasurement();});
        shareButton.setOnClickListener(v->showShare());shareToolButton.setOnClickListener(v->showShare());
        findViewById(R.id.commandSendButton).setOnClickListener(v->executeCommand());
        commandInput.setOnEditorActionListener((v,action,event)->{if(action==android.view.inputmethod.EditorInfo.IME_ACTION_DONE||action==android.view.inputmethod.EditorInfo.IME_ACTION_GO||(event!=null&&event.getKeyCode()==KeyEvent.KEYCODE_ENTER&&event.getAction()==KeyEvent.ACTION_DOWN)){executeCommand();return true;}return false;});
        commandInput.setOnKeyListener((v,keyCode,event)->{if(keyCode==KeyEvent.KEYCODE_ENTER&&event.getAction()==KeyEvent.ACTION_DOWN){executeCommand();return true;}return false;});
        updateShareEnabled(false);updateEditorEnabled(false);refreshPropertyButtons();refreshDocumentTabs();handleIncomingIntent(getIntent());
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
        menu.add(0,MENU_OPEN,0,"Dosya aç");menu.add(0,MENU_LAYERS,1,"Katmanlar").setEnabled(activeDxf!=null);menu.add(0,MENU_LAYOUTS,2,"Model / Layout").setEnabled(activeDxf!=null&&activeDxf.layoutNames.size()>1);menu.add(0,MENU_FIT,3,"Ekrana sığdır").setEnabled(currentFile!=null);menu.add(0,MENU_SAVE_DXF,4,"Düzenlenmiş DXF kaydet").setEnabled(canEdit());menu.add(0,MENU_PRINT,5,"Yazdır").setEnabled(currentFile!=null);menu.add(0,MENU_SHARE,6,"Paylaş").setEnabled(currentFile!=null);menu.add(0,MENU_INFO,7,"Çizim bilgileri").setEnabled(activeDxf!=null);menu.add(0,MENU_ABOUT,8,"MusaCAD hakkında");
        popup.setOnMenuItemClickListener(item->{switch(item.getItemId()){case MENU_OPEN:open();return true;case MENU_LAYERS:showLayers();return true;case MENU_LAYOUTS:showLayouts();return true;case MENU_FIT:cad.fitToScreen();return true;case MENU_SAVE_DXF:requestEditedDxfSave();return true;case MENU_PRINT:printDrawing();return true;case MENU_SHARE:showShare();return true;case MENU_INFO:showDrawingInfo();return true;case MENU_ABOUT:showLicense();return true;default:return false;}});popup.show();
    }

    private void executeCommand(){
        if(commandInput==null)return;
        String raw=commandInput.getText().toString().trim();commandInput.setText("");
        android.view.inputmethod.InputMethodManager imm=(android.view.inputmethod.InputMethodManager)getSystemService(INPUT_METHOD_SERVICE);
        if(imm!=null)imm.hideSoftInputFromWindow(commandInput.getWindowToken(),0);
        if(raw.isEmpty()){
            if(cad.confirmCurrentCommand()){result.setText("Komut tamamlandı • Enter ile onaylandı");refreshDocumentTabs();return;}
            if(lastCommandRaw.isEmpty()){result.setText("Komut bekleniyor");return;}
            raw=lastCommandRaw;result.setText("Son komut tekrarlandı • "+CadCommand.canonical(raw));
        }else lastCommandRaw=raw;
        CadCommand.Action action=CadCommand.parse(raw);
        switch(action){
            case LINE:selectEditMode(R.id.lineButton,CadView.Mode.DRAW_LINE);result.setText("LINE • İlk noktayı seçin");break;
            case POLYLINE:selectEditMode(R.id.polylineButton,CadView.Mode.DRAW_POLYLINE);result.setText("PLINE • Noktaları seçin • Bitir ile tamamlayın");break;
            case CIRCLE:selectEditMode(R.id.circleButton,CadView.Mode.DRAW_CIRCLE);result.setText("CIRCLE • Merkez ve yarıçap noktası seçin");break;
            case RECTANGLE:selectEditMode(R.id.rectangleButton,CadView.Mode.DRAW_RECTANGLE);result.setText("RECTANG • İki köşe seçin");break;
            case TEXT:selectEditMode(R.id.textButton,CadView.Mode.DRAW_TEXT);result.setText("TEXT/MTEXT • Yazı konumuna dokunun");break;
            case SELECT:selectEditMode(R.id.selectEntityButton,CadView.Mode.SELECT_ENTITY);result.setText("SELECT • Nesne seçin");break;
            case PAN:selectMode(R.id.panButton,CadView.Mode.PAN);result.setText("PAN • Çizimi sürükleyin");break;
            case MOVE:if(!cad.armMoveSelected()){selectEditMode(R.id.selectEntityButton,CadView.Mode.SELECT_ENTITY);result.setText("MOVE • Önce nesne seçin, sonra M yazın");}break;
            case COPY:if(!cad.copySelectedEntity()){selectEditMode(R.id.selectEntityButton,CadView.Mode.SELECT_ENTITY);result.setText("COPY • Önce nesne seçin, sonra CO yazın");}break;
            case ROTATE:if(!cad.rotateSelectedEntity()){selectEditMode(R.id.selectEntityButton,CadView.Mode.SELECT_ENTITY);result.setText("ROTATE • Önce nesne seçin, sonra RO yazın");}break;
            case ERASE:if(!cad.deleteSelectedEntity()){selectEditMode(R.id.selectEntityButton,CadView.Mode.SELECT_ENTITY);result.setText("ERASE • Önce nesne seçin, sonra E yazın");}break;
            case LAYER:showLayers();break;
            case PROPERTIES:showDrawingProperties();break;
            case COLOR:showQuickColor();break;
            case LINEWEIGHT:showQuickLineWeight();break;
            case LINETYPE:showQuickLineType();break;
            case DISTANCE:selectMode(R.id.distanceButton,CadView.Mode.DISTANCE);result.setText("DIST • İki nokta seçin");break;
            case AREA:selectMode(R.id.areaButton,CadView.Mode.AREA);result.setText("AREA • Sınır noktalarını seçin");break;
            case ZOOM:result.setText("ZOOM • Extents için Z E veya ZE kullanın; yakınlaştırma için üst araçları kullanın");break;
            case ZOOM_EXTENTS:cad.fitToScreen();result.setText("ZOOM EXTENTS • Çizim ekrana sığdırıldı");break;
            case UNDO:cad.undo();break;
            case SAVE:requestEditedDxfSave();break;
            case UNSUPPORTED:result.setText(CadCommand.canonical(raw)+" • AutoCAD komutu tanındı, MusaCAD motoru henüz desteklemiyor");break;
            case HELP:new AlertDialog.Builder(this).setTitle("AutoCAD uyumlu komutlar").setMessage("L / LINE • Çizgi\nPL / PLINE • Polyline\nC / CIRCLE • Daire\nREC / RECTANG • Dörtgen\nDT / TEXT / T / MTEXT • Yazı\nSEL / SELECT • Seç\nP / PAN • Gezin\nM / MOVE • Taşı\nCO / CP / COPY • Kopya\nRO / ROTATE • Döndür\nE / ERASE • Sil\nLA / LAYER • Katman\nPR / PROPERTIES • Özellikler\nDI / DIST • Mesafe\nAA / AREA • Alan\nZ E / ZE • Zoom Extents\nU / UNDO • Geri al\nQS / QSAVE / SAVE • Kaydet\n\nTanınıyor fakat henüz motoru yok: S/STRETCH, TR/TRIM, EX/EXTEND, O/OFFSET, SC/SCALE, MI/MIRROR, F/FILLET, H/HATCH, A/ARC, X/EXPLODE ve diğerleri.").setPositiveButton("TAMAM",null).show();break;
            default:result.setText("Bilinmeyen komut: "+raw+"  •  ? yazarak komutları görün");break;
        }
    }

    private int lineWeightIndex(int value){
        for(int i=0;i<LINE_WEIGHT_VALUES.length;i++)if(LINE_WEIGHT_VALUES[i]==value)return i;
        return 10;
    }
    private String lineWeightShort(int value){
        if(value==DxfLineStyle.LW_BYLAYER)return "BYL";
        if(value==DxfLineStyle.LW_BYBLOCK)return "BYB";
        if(value==DxfLineStyle.LW_DEFAULT)return "DEF";
        return String.format(Locale.US,"%.2f",Math.max(0,value)/100f);
    }
    private void refreshPropertyButtons(){
        Button color=findViewById(R.id.colorButton),weight=findViewById(R.id.lineWeightButton);
        if(color!=null){
            int mode=cad==null?CadEdit.COLOR_BYLAYER:cad.currentColorMode(),value=cad==null?7:cad.currentColorValue();String layerName=cad==null?"0":cad.currentLayer();
            String label=mode==CadEdit.COLOR_BYLAYER?"BYL":mode==CadEdit.COLOR_BYBLOCK?"BYB":mode==CadEdit.COLOR_ACI?"A"+value:"RGB";
            color.setText("Renk");color.setContentDescription("Renk "+label);int swatch=previewColor(mode,value,layerName);int lum=(Color.red(swatch)*299+Color.green(swatch)*587+Color.blue(swatch)*114)/1000;
            color.setBackgroundTintList(ColorStateList.valueOf(swatch));color.setTextColor(lum>150?Color.rgb(16,32,40):Color.WHITE);
        }
        if(weight!=null){String lw=cad==null?"BYL":lineWeightShort(cad.currentLineWeight());weight.setText("LW "+lw);weight.setContentDescription("LineWeight "+lw);}
    }
    private void setActiveColor(int mode,int value){
        cad.setDrawingProperties(cad.currentLayer(),mode,value,cad.currentLineType(),cad.currentLineWeight(),cad.currentTextStyle(),cad.currentTextFamily(),cad.currentTextShx(),cad.currentTextHeight(),cad.currentTextWidthFactor());
        boolean selected=cad.applyCurrentPropertiesToSelected();refreshPropertyButtons();
        result.setText((selected?"Seçili nesne":"Aktif çizim")+" rengi güncellendi");
    }
    private void showQuickColor(){
        if(activeDxf==null)return;
        showColorPalette(cad.currentColorMode(),cad.currentColorValue(),cad.currentLayer(),this::setActiveColor);
    }
    private void setActiveLineType(String type){
        String value=DxfLineStyle.normalizeName(type);
        cad.setDrawingProperties(cad.currentLayer(),cad.currentColorMode(),cad.currentColorValue(),value,cad.currentLineWeight(),cad.currentTextStyle(),cad.currentTextFamily(),cad.currentTextShx(),cad.currentTextHeight(),cad.currentTextWidthFactor());
        boolean selected=cad.applyCurrentPropertiesToSelected();
        result.setText((selected?"Seçili nesne":"Aktif çizim")+" çizgi tipi: "+value);
    }
    private void showQuickLineType(){
        if(activeDxf==null)return;
        LinkedHashSet<String> values=new LinkedHashSet<>();values.add(DxfLineStyle.BYLAYER);values.add(DxfLineStyle.BYBLOCK);values.addAll(activeDxf.lineTypeNames());
        String[] items=values.toArray(new String[0]);int checked=0;for(int i=0;i<items.length;i++)if(items[i].equalsIgnoreCase(cad.currentLineType())){checked=i;break;}
        new AlertDialog.Builder(this).setTitle("Çizgi tipi • Linetype").setSingleChoiceItems(items,checked,(dialog,which)->{setActiveLineType(items[which]);dialog.dismiss();}).setNegativeButton("İPTAL",null).show();
    }

    private void setActiveLineWeight(int weight){
        cad.setDrawingProperties(cad.currentLayer(),cad.currentColorMode(),cad.currentColorValue(),cad.currentLineType(),weight,cad.currentTextStyle(),cad.currentTextFamily(),cad.currentTextShx(),cad.currentTextHeight(),cad.currentTextWidthFactor());
        boolean selected=cad.applyCurrentPropertiesToSelected();refreshPropertyButtons();
        result.setText((selected?"Seçili nesne":"Aktif çizim")+" LineWeight: "+LINE_WEIGHT_LABELS[lineWeightIndex(weight)]);
    }
    private void showQuickLineWeight(){
        if(activeDxf==null)return;
        new AlertDialog.Builder(this).setTitle("Çizgi kalınlığı • LineWeight").setSingleChoiceItems(LINE_WEIGHT_LABELS,lineWeightIndex(cad.currentLineWeight()),(dialog,which)->{setActiveLineWeight(LINE_WEIGHT_VALUES[which]);dialog.dismiss();}).setNegativeButton("İPTAL",null).show();
    }

    private int previewColor(int mode,int value,String layerName){
        if(mode==CadEdit.COLOR_BYLAYER&&activeDxf!=null)return activeDxf.layerColor(layerName);
        if(mode==CadEdit.COLOR_BYBLOCK)return Color.rgb(120,140,150);
        if(mode==CadEdit.COLOR_ACI)return DxfColor.aciArgb(value);
        if(mode==CadEdit.COLOR_TRUECOLOR)return 0xFF000000|(value&0x00FFFFFF);
        return Color.WHITE;
    }
    private void updateColorPreview(TextView preview,int mode,int value,String layerName){
        if(preview==null)return;int color=previewColor(mode,value,layerName);
        GradientDrawable bg=new GradientDrawable();bg.setColor(color);bg.setCornerRadius(dp(6));bg.setStroke(dp(1),Color.rgb(210,220,225));preview.setBackground(bg);
        String label=mode==CadEdit.COLOR_BYLAYER?"BYLAYER":mode==CadEdit.COLOR_BYBLOCK?"BYBLOCK":mode==CadEdit.COLOR_ACI?"ACI "+value:String.format(Locale.US,"#%06X",value&0xFFFFFF);
        preview.setText(label);int lum=(Color.red(color)*299+Color.green(color)*587+Color.blue(color)*114)/1000;preview.setTextColor(lum>150?Color.BLACK:Color.WHITE);
    }
    private void showColorPalette(int currentMode,int currentValue,String layerName,ColorPickListener listener){
        LinearLayout root=new LinearLayout(this);root.setOrientation(LinearLayout.VERTICAL);root.setPadding(dp(10),dp(6),dp(10),dp(8));
        LinearLayout top=new LinearLayout(this);top.setOrientation(LinearLayout.HORIZONTAL);
        Button byLayer=new Button(this);byLayer.setText("BYLAYER");Button byBlock=new Button(this);byBlock.setText("BYBLOCK");Button trueColor=new Button(this);trueColor.setText("TRUECOLOR");
        for(Button b:new Button[]{byLayer,byBlock,trueColor}){LinearLayout.LayoutParams lp=new LinearLayout.LayoutParams(0,dp(46),1f);lp.setMargins(dp(2),0,dp(2),0);top.addView(b,lp);}
        root.addView(top,new LinearLayout.LayoutParams(LinearLayout.LayoutParams.MATCH_PARENT,LinearLayout.LayoutParams.WRAP_CONTENT));
        TextView caption=new TextView(this);caption.setText("AutoCAD Color Index • ACI 1–255");caption.setTextSize(13);caption.setPadding(dp(4),dp(10),0,dp(6));root.addView(caption);
        GridLayout grid=new GridLayout(this);grid.setColumnCount(8);grid.setAlignmentMode(GridLayout.ALIGN_BOUNDS);
        ScrollView scroll=new ScrollView(this);scroll.addView(grid);root.addView(scroll,new LinearLayout.LayoutParams(LinearLayout.LayoutParams.MATCH_PARENT,dp(430)));
        AlertDialog dialog=new AlertDialog.Builder(this).setTitle("Renk paleti").setView(root).setNegativeButton("İPTAL",null).create();
        byLayer.setOnClickListener(v->{listener.onPick(CadEdit.COLOR_BYLAYER,7);dialog.dismiss();});
        byBlock.setOnClickListener(v->{listener.onPick(CadEdit.COLOR_BYBLOCK,7);dialog.dismiss();});
        trueColor.setOnClickListener(v->{
            EditText input=new EditText(this);input.setSingleLine(true);input.setHint("#RRGGBB");input.setText(currentMode==CadEdit.COLOR_TRUECOLOR?String.format(Locale.US,"#%06X",currentValue&0xFFFFFF):"#FFFFFF");
            AlertDialog hex=new AlertDialog.Builder(this).setTitle("TrueColor RGB").setView(input).setPositiveButton("UYGULA",null).setNegativeButton("İPTAL",null).create();
            hex.setOnShowListener(x->hex.getButton(AlertDialog.BUTTON_POSITIVE).setOnClickListener(w->{try{String raw=input.getText().toString().trim().replace("#","");if(raw.length()!=6)throw new IllegalArgumentException();listener.onPick(CadEdit.COLOR_TRUECOLOR,Integer.parseInt(raw,16));hex.dismiss();dialog.dismiss();}catch(Exception ex){input.setError("#RRGGBB biçiminde girin");}}));hex.show();
        });
        for(int aci=1;aci<=255;aci++){
            final int value=aci;int color=DxfColor.aciArgb(aci);TextView cell=new TextView(this);cell.setText(Integer.toString(aci));cell.setGravity(Gravity.CENTER);cell.setTextSize(7);cell.setContentDescription("ACI "+aci);
            int lum=(Color.red(color)*299+Color.green(color)*587+Color.blue(color)*114)/1000;cell.setTextColor(lum>150?Color.BLACK:Color.WHITE);
            GradientDrawable swatch=new GradientDrawable();swatch.setColor(color);swatch.setCornerRadius(dp(3));swatch.setStroke(dp(1),Color.rgb(70,80,85));cell.setBackground(swatch);
            GridLayout.LayoutParams lp=new GridLayout.LayoutParams();lp.width=dp(39);lp.height=dp(39);lp.setMargins(dp(2),dp(2),dp(2),dp(2));grid.addView(cell,lp);
            cell.setOnClickListener(v->{listener.onPick(CadEdit.COLOR_ACI,value);dialog.dismiss();});
        }
        dialog.show();
    }
    private String textStyleLabel(DxfTextStyle.Style style){
        if(style==null)return "STANDARD";
        StringBuilder out=new StringBuilder(style.name);if(!style.fontFile.isEmpty())out.append("  •  ").append(style.fontFile);if(!style.bigFontFile.isEmpty())out.append(" + ").append(style.bigFontFile);if(style.usesShx())out.append("  •  SHX");return out.toString();
    }

    private void showDrawingProperties(){
        if(activeDxf==null){Toast.makeText(this,"Özellikler için önce çizim açın",Toast.LENGTH_SHORT).show();return;}
        LinearLayout box=new LinearLayout(this);box.setOrientation(LinearLayout.VERTICAL);int pad=dp(16);box.setPadding(pad,pad/2,pad,pad/2);

        TextView l1=new TextView(this);l1.setText("Katman");box.addView(l1);
        Spinner layer=new Spinner(this);String[] layers=activeDxf.layerNames.toArray(new String[0]);ArrayAdapter<String> la=new ArrayAdapter<>(this,android.R.layout.simple_spinner_dropdown_item,layers);layer.setAdapter(la);for(int i=0;i<layers.length;i++)if(layers[i].equals(cad.currentLayer()))layer.setSelection(i);box.addView(layer);

        TextView l2=new TextView(this);l2.setText("Renk");l2.setPadding(0,dp(10),0,0);box.addView(l2);
        Spinner colorMode=new Spinner(this);String[] modes={"BYLAYER","BYBLOCK","ACI renk paleti","True Color RGB"};colorMode.setAdapter(new ArrayAdapter<>(this,android.R.layout.simple_spinner_dropdown_item,modes));colorMode.setSelection(cad.currentColorMode());box.addView(colorMode);
        EditText colorValue=new EditText(this);colorValue.setSingleLine(true);colorValue.setHint("ACI: 1-255 veya RGB: #RRGGBB");colorValue.setText(cad.currentColorMode()==CadEdit.COLOR_TRUECOLOR?String.format(Locale.US,"#%06X",cad.currentColorValue()&0xFFFFFF):Integer.toString(cad.currentColorValue()));box.addView(colorValue);

        LinearLayout colorRow=new LinearLayout(this);colorRow.setOrientation(LinearLayout.HORIZONTAL);colorRow.setGravity(Gravity.CENTER_VERTICAL);colorRow.setPadding(0,dp(6),0,dp(4));
        TextView colorPreview=new TextView(this);colorPreview.setGravity(Gravity.CENTER);colorPreview.setTextSize(11);colorPreview.setTypeface(Typeface.DEFAULT_BOLD);
        Button paletteButton=new Button(this);paletteButton.setText("RENK PALETİ • 1–255");paletteButton.setTextSize(11);paletteButton.setAllCaps(false);
        LinearLayout.LayoutParams previewLp=new LinearLayout.LayoutParams(dp(112),dp(44));previewLp.setMargins(0,0,dp(8),0);colorRow.addView(colorPreview,previewLp);colorRow.addView(paletteButton,new LinearLayout.LayoutParams(0,dp(48),1f));box.addView(colorRow);
        updateColorPreview(colorPreview,cad.currentColorMode(),cad.currentColorValue(),cad.currentLayer());

        colorMode.setOnItemSelectedListener(new AdapterView.OnItemSelectedListener(){
            public void onNothingSelected(AdapterView<?> parent){}
            public void onItemSelected(AdapterView<?> parent,View view,int position,long id){
                boolean explicit=position==CadEdit.COLOR_ACI||position==CadEdit.COLOR_TRUECOLOR;colorValue.setVisibility(explicit?View.VISIBLE:View.GONE);
                if(position==CadEdit.COLOR_ACI&&colorValue.getText().toString().trim().startsWith("#"))colorValue.setText("7");
                if(position==CadEdit.COLOR_TRUECOLOR&&!colorValue.getText().toString().trim().startsWith("#"))colorValue.setText("#FFFFFF");
                int value=7;try{String raw=colorValue.getText().toString().trim();value=position==CadEdit.COLOR_TRUECOLOR?Integer.parseInt(raw.replace("#",""),16):Integer.parseInt(raw);}catch(Exception ignored){}
                String layerName=layers.length==0?"0":String.valueOf(layer.getSelectedItem());updateColorPreview(colorPreview,position,value,layerName);
            }
        });
        paletteButton.setOnClickListener(v->{
            int mode=colorMode.getSelectedItemPosition(),value=7;try{String raw=colorValue.getText().toString().trim();value=mode==CadEdit.COLOR_TRUECOLOR?Integer.parseInt(raw.replace("#",""),16):Integer.parseInt(raw);}catch(Exception ignored){}
            String layerName=layers.length==0?"0":String.valueOf(layer.getSelectedItem());
            showColorPalette(mode,value,layerName,(pickedMode,pickedValue)->{
                colorMode.setSelection(pickedMode);colorValue.setText(pickedMode==CadEdit.COLOR_TRUECOLOR?String.format(Locale.US,"#%06X",pickedValue&0xFFFFFF):Integer.toString(pickedValue));
                colorValue.setVisibility(pickedMode==CadEdit.COLOR_ACI||pickedMode==CadEdit.COLOR_TRUECOLOR?View.VISIBLE:View.GONE);
                updateColorPreview(colorPreview,pickedMode,pickedValue,layerName);
            });
        });

        TextView ltLabel=new TextView(this);ltLabel.setText("Çizgi tipi");ltLabel.setPadding(0,dp(10),0,0);box.addView(ltLabel);
        LinkedHashSet<String> typeSet=new LinkedHashSet<>();typeSet.add(DxfLineStyle.BYLAYER);typeSet.add(DxfLineStyle.BYBLOCK);typeSet.addAll(activeDxf.lineTypeNames());String[] lineTypes=typeSet.toArray(new String[0]);
        Spinner lineType=new Spinner(this);lineType.setAdapter(new ArrayAdapter<>(this,android.R.layout.simple_spinner_dropdown_item,lineTypes));for(int i=0;i<lineTypes.length;i++)if(lineTypes[i].equalsIgnoreCase(cad.currentLineType()))lineType.setSelection(i);box.addView(lineType);

        TextView lwLabel=new TextView(this);lwLabel.setText("Çizgi kalınlığı • LineWeight");lwLabel.setPadding(0,dp(10),0,0);box.addView(lwLabel);
        Spinner lineWeight=new Spinner(this);lineWeight.setAdapter(new ArrayAdapter<>(this,android.R.layout.simple_spinner_dropdown_item,LINE_WEIGHT_LABELS));lineWeight.setSelection(lineWeightIndex(cad.currentLineWeight()));box.addView(lineWeight);

        TextView l3=new TextView(this);l3.setText("Yazı tipi / DXF text style");l3.setPadding(0,dp(10),0,0);box.addView(l3);
        ArrayList<String> styleNamesList=new ArrayList<>(activeDxf.textStyleNames());if(styleNamesList.isEmpty())styleNamesList.add(DxfTextStyle.STANDARD);
        String[] styleNames=styleNamesList.toArray(new String[0]);String[] styleLabels=new String[styleNames.length];for(int i=0;i<styleNames.length;i++)styleLabels[i]=textStyleLabel(activeDxf.textStyle(styleNames[i]));
        Spinner font=new Spinner(this);
        ArrayAdapter<String> styleAdapter=new ArrayAdapter<String>(this,android.R.layout.simple_spinner_dropdown_item,styleLabels){
            private View styleRow(View view,int position){
                TextView text=(TextView)super.getDropDownView(position,view,font);DxfTextStyle.Style st=activeDxf.textStyle(styleNames[Math.max(0,Math.min(position,styleNames.length-1))]);
                text.setTypeface(st.usesShx()?Typeface.MONOSPACE:Typeface.create(st.familyHint(),Typeface.NORMAL));text.setTextSize(18);text.setPadding(dp(12),dp(10),dp(12),dp(10));return text;
            }
            @Override public View getDropDownView(int position,View convertView,ViewGroup parent){return styleRow(convertView,position);}
            @Override public View getView(int position,View convertView,ViewGroup parent){
                TextView text=(TextView)super.getView(position,convertView,parent);DxfTextStyle.Style st=activeDxf.textStyle(styleNames[Math.max(0,Math.min(position,styleNames.length-1))]);
                text.setTypeface(st.usesShx()?Typeface.MONOSPACE:Typeface.create(st.familyHint(),Typeface.NORMAL));return text;
            }
        };
        font.setAdapter(styleAdapter);int styleSelection=0;for(int i=0;i<styleNames.length;i++)if(styleNames[i].equalsIgnoreCase(cad.currentTextStyle())){styleSelection=i;break;}font.setSelection(styleSelection);box.addView(font);
        TextView styleInfo=new TextView(this);styleInfo.setText("Çizimde tanımlı "+styleNames.length+" yazı stili listeleniyor. SHX/TTF dosya adı varsa yanında gösterilir.");styleInfo.setTextSize(10);styleInfo.setTextColor(Color.LTGRAY);styleInfo.setPadding(0,dp(3),0,dp(3));box.addView(styleInfo);

        EditText textHeight=new EditText(this);textHeight.setSingleLine(true);textHeight.setInputType(android.text.InputType.TYPE_CLASS_NUMBER|android.text.InputType.TYPE_NUMBER_FLAG_DECIMAL);textHeight.setHint("Yazı yüksekliği");textHeight.setText(String.format(Locale.US,"%.2f",cad.currentTextHeight()));box.addView(textHeight);
        EditText widthFactor=new EditText(this);widthFactor.setSingleLine(true);widthFactor.setInputType(android.text.InputType.TYPE_CLASS_NUMBER|android.text.InputType.TYPE_NUMBER_FLAG_DECIMAL);widthFactor.setHint("Yazı genişlik katsayısı");widthFactor.setText(String.format(Locale.US,"%.2f",cad.currentTextWidthFactor()));box.addView(widthFactor);

        ScrollView scroll=new ScrollView(this);scroll.addView(box);
        AlertDialog dialog=new AlertDialog.Builder(this).setTitle("Çizim / Nesne özellikleri").setView(scroll).setPositiveButton("UYGULA",null).setNegativeButton("İPTAL",null).create();
        dialog.setOnShowListener(d->dialog.getButton(AlertDialog.BUTTON_POSITIVE).setOnClickListener(v->{
            try{
                String layerName=layers.length==0?"0":String.valueOf(layer.getSelectedItem());
                int mode=colorMode.getSelectedItemPosition(),value=7;String raw=colorValue.getText().toString().trim();
                if(mode==CadEdit.COLOR_ACI){value=Integer.parseInt(raw);if(value<1||value>255)throw new IllegalArgumentException("ACI renk numarası 1-255 olmalı");}
                else if(mode==CadEdit.COLOR_TRUECOLOR){String hex=raw.replace("#","").trim();if(hex.length()!=6)throw new IllegalArgumentException("RGB renk #RRGGBB biçiminde olmalı");value=Integer.parseInt(hex,16);}
                String selectedType=lineTypes[Math.max(0,lineType.getSelectedItemPosition())];int selectedWeight=LINE_WEIGHT_VALUES[Math.max(0,lineWeight.getSelectedItemPosition())];
                int fi=Math.max(0,font.getSelectedItemPosition());String styleName=styleNames[Math.min(fi,styleNames.length-1)];DxfTextStyle.Style selectedStyle=activeDxf.textStyle(styleName);
                String family=selectedStyle.familyHint();boolean shx=selectedStyle.usesShx();
                float h=Float.parseFloat(textHeight.getText().toString().trim()),wf=Float.parseFloat(widthFactor.getText().toString().trim());if(!(h>0)||!(wf>0))throw new IllegalArgumentException("Yazı ölçüleri sıfırdan büyük olmalı");
                cad.setDrawingProperties(layerName,mode,value,selectedType,selectedWeight,selectedStyle.name,family,shx,h,wf);
                boolean selected=cad.applyCurrentPropertiesToSelected();refreshPropertyButtons();
                result.setText((selected?"Seçili nesne":"Aktif çizim")+" • "+layerName+" • "+modes[mode]+" • "+selectedStyle.name+" • LW "+lineWeightShort(selectedWeight));
                dialog.dismiss();
            }catch(Exception ex){Toast.makeText(this,ex.getMessage()==null?"Özellik değeri geçersiz":ex.getMessage(),Toast.LENGTH_LONG).show();}
        }));dialog.show();
    }

    private void showDrawingInfo(){
        if(activeDxf==null)return;String shx=activeDxf.fontFallbacks.isEmpty()?"yok":android.text.TextUtils.join(", ",activeDxf.fontFallbacks);String text="Dosya başarıyla açıldı.\n\n"+"Layout: "+activeDxf.activeLayout+" ("+activeDxf.layoutNames.size()+")\n"+"Nesne: "+activeDxf.entityCount+"\n"+"Katman: "+activeDxf.layerCount+"\n"+"Görünür katman: "+activeDxf.visibleLayers.size()+"\n"+"Seçilebilir kaynak nesne: "+activeDxf.editableSourceCount()+"\n"+"Düzenleme toplamı: "+cad.editCount()+"\n"+"Kaynak nesne değişikliği: "+cad.sourceModifiedCount()+"\n"+"SHX metin fallback: "+shx+"\n"+"Complex SHX shape fallback: "+(activeDxf.externalShapeFallback?"var":"yok")+"\n"+"Düzenleme: "+(canEdit()?"açık":"yalnız görüntüleme")+"\n"+"Görüntüleme: vektörel / net yakınlaştırma";
        new AlertDialog.Builder(this).setTitle("Çizim bilgileri").setMessage(text).setPositiveButton("TAMAM",null).show();
    }

    private void updateShareEnabled(boolean enabled){shareButton.setEnabled(enabled);shareButton.setAlpha(enabled?1f:.45f);shareToolButton.setEnabled(enabled);shareToolButton.setAlpha(enabled?1f:.55f);}
    private void updateEditorEnabled(boolean enabled){
        int[] ids={R.id.propertiesButton,R.id.colorButton,R.id.lineWeightButton,R.id.selectEntityButton,R.id.moveEntityButton,R.id.rotateEntityButton,R.id.copyEntityButton,R.id.deleteEntityButton,R.id.lineButton,R.id.polylineButton,R.id.rectangleButton,R.id.circleButton,R.id.textButton,R.id.finishEditButton,R.id.saveDxfButton};
        for(int id:ids){View v=findViewById(id);v.setEnabled(enabled);v.setAlpha(enabled?1f:.45f);}
    }
    private void selectMode(int id,CadView.Mode mode){View button=findViewById(id);if(button!=null)button.performHapticFeedback(HapticFeedbackConstants.KEYBOARD_TAP);cad.setMode(mode);markModeSelected(id);}
    private void selectEditMode(int id,CadView.Mode mode){if(!canEdit()){Toast.makeText(this,"Bu çizim düzenleme için vektörel olarak açılamadı",Toast.LENGTH_SHORT).show();return;}selectMode(id,mode);}
    private void markModeSelected(int id){if(modeButtons==null)return;for(View button:modeButtons)button.setSelected(button.getId()==id);}
    private void hideWelcomePanel(){if(welcomePanel==null||welcomePanel.getVisibility()!=View.VISIBLE)return;welcomePanel.animate().alpha(0f).setDuration(180).withEndAction(()->{welcomePanel.setVisibility(View.GONE);welcomePanel.setAlpha(1f);}).start();}

    private DocumentSession activeSession(){
        return activeDocumentIndex>=0&&activeDocumentIndex<documents.size()?documents.get(activeDocumentIndex):null;
    }

    private void captureActiveSession(){
        DocumentSession session=activeSession();if(session==null||currentFile==null)return;
        session.file=currentFile;session.workingDxf=editingBaseDxf;session.parsed=activeDxf;session.name=currentDisplayName;
        if(activeDxf!=null)session.bitmap=activeDxf.bitmap;
        session.viewState=cad.captureSessionState();session.dirty=currentEditSignature()!=session.savedEditSignature;
    }

    private void addLoadedDocument(Loaded loaded){
        captureActiveSession();
        DocumentSession session=new DocumentSession();session.file=loaded.file;session.workingDxf=loaded.workingDxf;session.bitmap=loaded.bitmap;session.parsed=loaded.parsed;session.name=loaded.name;session.dxf=loaded.dxf;
        documents.add(session);activeDocumentIndex=documents.size()-1;applyDocumentSession(session);resetBackCloseFlow();refreshDocumentTabs();
    }

    private void activateDocument(int index){
        if(index<0||index>=documents.size()||index==activeDocumentIndex||activeLoad!=null)return;
        captureActiveSession();activeDocumentIndex=index;applyDocumentSession(documents.get(index));resetBackCloseFlow();refreshDocumentTabs();
    }

    private void applyDocumentSession(DocumentSession session){
        if(session==null)return;
        currentFile=session.file;editingBaseDxf=session.workingDxf;currentDisplayName=session.name==null?"cizim.dwg":session.name;activeDxf=session.parsed;
        hideWelcomePanel();updateShareEnabled(currentFile!=null);updateEditorEnabled(canEdit());findViewById(R.id.layersButton).setEnabled(activeDxf!=null);
        cad.restoreSession(activeDxf,session.bitmap,session.viewState);markModeSelected(R.id.panButton);refreshPropertyButtons();
        snapToggle.setEnabled(activeDxf!=null&&activeDxf.snapPoints.length>0);snapToggle.setChecked(activeDxf!=null&&(session.viewState==null||session.viewState.snapEnabled));
        String editable=canEdit()?"  •  düzenlenebilir":"";
        fileName.setText(currentDisplayName+(session.dxf?"  •  DXF":activeDxf!=null?"  •  DWG":"  •  DWG önizleme")+editable);
        if(activeDxf!=null){String fallback=(activeDxf.fontFallbacks.isEmpty()&&!activeDxf.externalShapeFallback)?"":"  •  SHX fallback";result.setText("Hazır  •  "+activeDxf.activeLayout+"  •  "+activeDxf.entityCount+" nesne  •  "+activeDxf.layerCount+" katman  •  "+activeDxf.editableSourceCount()+" seçilebilir"+editable+fallback);}
        else result.setText("Hazır  •  DWG önizleme modu");
    }

    private void closeActiveDocument(){
        if(activeDocumentIndex<0||activeDocumentIndex>=documents.size())return;
        captureActiveSession();int closing=activeDocumentIndex;DocumentSession session=documents.remove(closing);session.dispose();
        if(documents.isEmpty()){activeDocumentIndex=-1;clearActiveDocumentUi();}
        else{activeDocumentIndex=Math.min(closing,documents.size()-1);applyDocumentSession(documents.get(activeDocumentIndex));}
        resetBackCloseFlow();refreshDocumentTabs();
    }

    private void requestCloseDocument(int index){
        if(index<0||index>=documents.size())return;if(index!=activeDocumentIndex)activateDocument(index);
        if(isActiveDocumentDirty()){
            new AlertDialog.Builder(this).setTitle("Projeyi kapat").setMessage("Bu projede kaydedilmemiş değişiklikler var.")
                .setPositiveButton("KAYDET VE KAPAT",(d,w)->{closeActiveAfterSave=true;requestEditedDxfSave();})
                .setNegativeButton("KAYDETMEDEN KAPAT",(d,w)->{closeActiveAfterSave=false;closeActiveDocument();})
                .setNeutralButton("İPTAL",null).show();
        }else closeActiveDocument();
    }

    private void clearActiveDocumentUi(){
        currentFile=null;editingBaseDxf=null;activeDxf=null;currentDisplayName="cizim.dwg";closeActiveAfterSave=false;
        cad.clearDocument();updateShareEnabled(false);updateEditorEnabled(false);findViewById(R.id.layersButton).setEnabled(false);snapToggle.setEnabled(false);snapToggle.setChecked(false);
        fileName.setText("Henüz proje açılmadı");result.setText("Hazır");refreshPropertyButtons();if(welcomePanel!=null){welcomePanel.setVisibility(View.VISIBLE);welcomePanel.setAlpha(1f);}
    }

    private String shortDocumentName(String name){
        if(name==null||name.trim().isEmpty())return "Çizim";
        String n=name.trim();return n.length()<=22?n:n.substring(0,19)+"…";
    }

    private void refreshDocumentTabs(){
        if(documentTabs==null||documentTabScroll==null)return;
        documentTabs.removeAllViews();
        if(documents.isEmpty()){documentTabScroll.setVisibility(View.GONE);return;}
        documentTabScroll.setVisibility(View.VISIBLE);
        DocumentSession current=activeSession();if(current!=null&&currentFile!=null)current.dirty=currentEditSignature()!=current.savedEditSignature;
        for(int i=0;i<documents.size();i++){
            final int index=i;DocumentSession session=documents.get(i);boolean active=i==activeDocumentIndex;
            LinearLayout tab=new LinearLayout(this);tab.setOrientation(LinearLayout.HORIZONTAL);tab.setGravity(Gravity.CENTER_VERTICAL);tab.setPadding(dp(9),0,dp(3),0);
            LinearLayout.LayoutParams tabLp=new LinearLayout.LayoutParams(LinearLayout.LayoutParams.WRAP_CONTENT,dp(32));tabLp.setMargins(dp(2),dp(2),dp(2),dp(2));tab.setLayoutParams(tabLp);tab.setBackgroundColor(active?Color.rgb(15,116,128):Color.rgb(20,48,61));
            TextView label=new TextView(this);label.setSingleLine(true);label.setText(shortDocumentName(session.name)+(session.dirty?" •":""));label.setTextColor(Color.WHITE);label.setTextSize(10);label.setPadding(0,0,dp(5),0);tab.addView(label,new LinearLayout.LayoutParams(LinearLayout.LayoutParams.WRAP_CONTENT,LinearLayout.LayoutParams.MATCH_PARENT));
            TextView close=new TextView(this);close.setText("×");close.setTextSize(18);close.setGravity(Gravity.CENTER);close.setTextColor(active?Color.WHITE:Color.rgb(160,184,194));close.setContentDescription("Projeyi kapat");tab.addView(close,new LinearLayout.LayoutParams(dp(28),LinearLayout.LayoutParams.MATCH_PARENT));
            label.setOnClickListener(v->activateDocument(index));close.setOnClickListener(v->requestCloseDocument(index));
            documentTabs.addView(tab);
        }
        TextView add=new TextView(this);add.setText("+");add.setTextSize(22);add.setTextColor(Color.rgb(111,227,215));add.setGravity(Gravity.CENTER);add.setContentDescription("Yeni proje aç");
        add.setBackgroundColor(Color.rgb(12,38,49));add.setOnClickListener(v->open());documentTabs.addView(add,new LinearLayout.LayoutParams(dp(42),dp(32)));
    }

    private boolean isActiveDocumentDirty(){
        DocumentSession session=activeSession();return session!=null&&currentEditSignature()!=session.savedEditSignature;
    }

    private long currentEditSignature(){
        List<CadEdit> additions=cad.getAddedEdits();List<SourceReplacement> replacements=cad.getSourceReplacements();List<SourceRange> removals=cad.getSourceRemovals();
        if(additions.isEmpty()&&replacements.isEmpty()&&removals.isEmpty())return 0L;
        long h=1469598103934665603L;
        for(CadEdit edit:additions)h=hashEdit(h,edit);
        for(SourceReplacement r:replacements){h=mix(h,r.sourceId);h=mix(h,r.range.startLine);h=mix(h,r.range.endLineExclusive);h=mix(h,r.colorMode);h=mix(h,r.colorValue);h=mix(h,r.lineWeight);h=mix(h,Double.doubleToLongBits(r.lineTypeScale));h=mix(h,r.layer.hashCode());h=mix(h,r.lineType.hashCode());h=hashEdit(h,r.edit);}
        for(SourceRange r:removals){h=mix(h,r.sourceId);h=mix(h,r.startLine);h=mix(h,r.endLineExclusive);}
        return h;
    }
    private static long hashEdit(long h,CadEdit e){
        if(e==null)return mix(h,0);h=mix(h,e.type.ordinal());h=mix(h,e.closed?1:0);h=mix(h,Float.floatToIntBits(e.strokeWidth));h=mix(h,Float.floatToIntBits(e.rotationDegrees));
        h=mix(h,e.text==null?0:e.text.hashCode());h=mix(h,e.layerName.hashCode());h=mix(h,e.colorMode);h=mix(h,e.colorValue);h=mix(h,e.lineTypeName.hashCode());h=mix(h,e.lineWeight);h=mix(h,e.textStyleName.hashCode());h=mix(h,e.textFamilyHint.hashCode());
        h=mix(h,e.textShx?1:0);h=mix(h,Float.floatToIntBits(e.textHeight));h=mix(h,Float.floatToIntBits(e.textWidthFactor));h=mix(h,Float.floatToIntBits(e.textOblique));h=mix(h,e.textGenerationFlags);
        for(float v:e.xy)h=mix(h,Float.floatToIntBits(v));return h;
    }
    private static long mix(long h,long value){h^=value;return h*1099511628211L;}

    private void resetBackCloseFlow(){backCloseStage=0;backCloseStageAt=0L;closeActiveAfterSave=false;}

    @Override public void onBackPressed(){
        if(activeLoad!=null){Toast.makeText(this,"Devam eden işlem tamamlanıyor; kapatma için tekrar deneyin",Toast.LENGTH_SHORT).show();return;}
        if(activeSession()==null){super.onBackPressed();return;}
        long now=System.currentTimeMillis();if(backCloseStage==1&&now-backCloseStageAt>6000L)backCloseStage=0;
        if(backCloseStage==0){
            backCloseStage=1;backCloseStageAt=now;result.setText("Projeden çıkmak istiyor musunuz? • Geri tuşuna tekrar basın");
            Toast.makeText(this,"Projeyi kapatma: tekrar Geri'ye basın",Toast.LENGTH_SHORT).show();return;
        }
        if(backCloseStage==1){
            backCloseStage=2;
            if(isActiveDocumentDirty()){
                AlertDialog dialog=new AlertDialog.Builder(this).setTitle("Değişiklikler kaydedilsin mi?")
                    .setMessage("Projede kaydedilmemiş düzenlemeler var. Üçüncü Geri tuşunda bu proje sekmesi kapanacak.")
                    .setPositiveButton("KAYDET",(d,w)->{requestEditedDxfSave();result.setText("Kaydetme seçildi • ardından Geri = proje sekmesini kapat");})
                    .setNegativeButton("KAYDETMEDEN DEVAM",(d,w)->result.setText("Kaydetmeden devam • Geri = proje sekmesini kapat"))
                    .setNeutralButton("İPTAL",(d,w)->resetBackCloseFlow()).create();
                dialog.setOnCancelListener(d->resetBackCloseFlow());dialog.show();
            }else result.setText("Kaydedilmemiş değişiklik yok • Geri = proje sekmesini kapat");
            return;
        }
        closeActiveDocument();
    }

    private void releaseAllDocuments(){
        captureActiveSession();for(DocumentSession session:documents)session.dispose();documents.clear();activeDocumentIndex=-1;currentFile=null;editingBaseDxf=null;activeDxf=null;
    }

    private void showLicense(){
        String license;try(InputStream in=getAssets().open("COPYING-LibreDWG.txt")){ByteArrayOutputStream out=new ByteArrayOutputStream();byte[] bytes=new byte[4096];int n;while((n=in.read(bytes))!=-1)out.write(bytes,0,n);license=out.toString("UTF-8");}catch(IOException e){license="GPL-3.0-or-later";}
        TextView text=new TextView(this);text.setPadding(24,16,24,16);text.setText("MusaCAD — LibreDWG ile çevrimdışı DWG okuma\nKaynak kod: https://github.com/santiye2019-lab/MusaCAD\n\n"+license);android.text.util.Linkify.addLinks(text,android.text.util.Linkify.WEB_URLS);text.setMovementMethod(android.text.method.LinkMovementMethod.getInstance());ScrollView scroll=new ScrollView(this);scroll.addView(text);new AlertDialog.Builder(this).setTitle("Lisans ve kaynak kod").setView(scroll).setPositiveButton("KAPAT",null).show();
    }

    private void open(){
        if(activeLoad!=null){Toast.makeText(this,"Devam eden işlem bitmeden yeni dosya açılamaz",Toast.LENGTH_SHORT).show();return;}
        if(documents.size()>=MAX_OPEN_DOCUMENTS){new AlertDialog.Builder(this).setTitle("Açık proje sınırı").setMessage("Telefon belleğini korumak için aynı anda en fazla "+MAX_OPEN_DOCUMENTS+" proje açık tutuluyor. Yeni proje açmak için bir sekmeyi kapatın.").setPositiveButton("TAMAM",null).show();return;}
        startActivityForResult(new Intent(this,RecentFilesActivity.class),OPEN);
    }
    @Override protected void onActivityResult(int r,int c,Intent data){
        super.onActivityResult(r,c,data);
        if(c!=RESULT_OK||data==null||data.getData()==null){if(r==SAVE_DXF)closeActiveAfterSave=false;return;}
        if(r==OPEN)startLoad(data.getData());else if(r==SAVE_DXF)saveEditedDxf(data.getData());
    }
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
                    if(activeLoad!=task||isFinishing()||isDestroyed()){loaded.dispose();return;}
                    activeLoad=null;task.dialog.dismiss();addLoadedDocument(loaded);
                });
            }catch(Exception|OutOfMemoryError e){loaded.dispose();runOnUiThread(()->{if(activeLoad!=task||isFinishing()||isDestroyed())return;activeLoad=null;task.dialog.dismiss();error(e instanceof Exception?(Exception)e:new IOException("Bu çizim için yeterli bellek yok"));});}
        });
    }

    @Override protected void onDestroy(){cancelLoad();releaseAllDocuments();loader.shutdownNow();super.onDestroy();}

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
        LinearLayout box=new LinearLayout(this);box.setOrientation(LinearLayout.VERTICAL);int p=dp(16);box.setPadding(p,p/2,p,p/2);
        EditText input=new EditText(this);input.setHint("Çizime eklenecek yazı");input.setSingleLine(false);input.setMaxLines(3);box.addView(input);
        TextView info=new TextView(this);info.setPadding(0,dp(8),0,0);info.setText("Aktif stil: "+cad.currentTextStyle()+" • "+String.format(Locale.getDefault(),"%.2f",cad.currentTextHeight())+" • Katman: "+cad.currentLayer());box.addView(info);
        Button props=new Button(this);props.setText("YAZI / RENK / KATMAN ÖZELLİKLERİ");props.setOnClickListener(v->showDrawingProperties());box.addView(props);
        AlertDialog dialog=new AlertDialog.Builder(this).setTitle("Yazı ekle").setView(box).setPositiveButton("EKLE",null).setNegativeButton("İPTAL",null).create();
        dialog.setOnShowListener(d->dialog.getButton(AlertDialog.BUTTON_POSITIVE).setOnClickListener(v->{String text=input.getText().toString().trim();if(text.isEmpty()){input.setError("Bir yazı girin");return;}cad.addTextEdit(x,y,text);dialog.dismiss();}));dialog.show();
    }

    private void requestEditedDxfSave(){
        if(activeLoad!=null){Toast.makeText(this,"Devam eden işlem bitmeden kaydedilemez",Toast.LENGTH_SHORT).show();return;}if(!canEdit()){Toast.makeText(this,"Bu çizim DXF olarak düzenlenebilir durumda değil",Toast.LENGTH_SHORT).show();return;}
        Intent intent=new Intent(Intent.ACTION_CREATE_DOCUMENT);intent.addCategory(Intent.CATEGORY_OPENABLE);intent.setType("application/octet-stream");String base=currentDisplayName==null?"cizim":currentDisplayName.replaceFirst("(?i)\\.(dwg|dxf)$","");intent.putExtra(Intent.EXTRA_TITLE,base+"_duzenlendi.dxf");startActivityForResult(intent,SAVE_DXF);
    }

    private void saveEditedDxf(Uri uri){
        if(!canEdit()||activeLoad!=null)return;final File base=editingBaseDxf;final DxfParser.Result drawing=activeDxf;final List<CadEdit> additions=cad.getAddedEdits();final List<SourceReplacement> replacements=cad.getSourceReplacements();final List<SourceRange> removals=cad.getSourceRemovals();final int total=additions.size()+removals.size();
        LoadTask task=new LoadTask();activeLoad=task;LinearLayout box=new LinearLayout(this);box.setOrientation(LinearLayout.VERTICAL);int pad=dp(20);box.setPadding(pad,pad,pad,pad);box.addView(new ProgressBar(this));task.progress=new TextView(this);task.progress.setText("DXF hazırlanıyor…");box.addView(task.progress);task.dialog=new AlertDialog.Builder(this).setTitle("Düzenlenmiş DXF kaydediliyor").setView(box).setNegativeButton("İPTAL",(d,w)->cancelLoad()).create();task.dialog.setOnCancelListener(d->cancelLoad());task.dialog.setCanceledOnTouchOutside(false);task.dialog.show();
        task.future=loader.submit(()->{try(OutputStream out=getContentResolver().openOutputStream(uri,"wt")){if(out==null)throw new IOException("Kaydedilecek dosya açılamadı");DxfWriter.write(base,out,drawing,additions,replacements,removals);FileTransfer.checkCancelled();runOnUiThread(()->{if(activeLoad!=task||isFinishing()||isDestroyed())return;activeLoad=null;task.dialog.dismiss();DocumentSession session=activeSession();if(session!=null){session.savedEditSignature=currentEditSignature();session.dirty=false;}refreshDocumentTabs();Toast.makeText(this,"DXF kaydedildi • "+total+" düzenleme",Toast.LENGTH_LONG).show();if(closeActiveAfterSave){closeActiveAfterSave=false;closeActiveDocument();}});}catch(Exception e){runOnUiThread(()->{if(activeLoad!=task||isFinishing()||isDestroyed())return;activeLoad=null;task.dialog.dismiss();closeActiveAfterSave=false;error(e);});}});
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
