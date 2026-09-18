package com.musa.cad;

import android.content.Context;
import android.graphics.*;
import android.util.AttributeSet;
import android.view.*;
import java.util.*;

public class CadView extends View {
    public enum Mode { PAN, SELECT_ENTITY, CALIBRATE, DISTANCE, AREA, DRAW_LINE, DRAW_POLYLINE, DRAW_RECTANGLE, DRAW_CIRCLE, DRAW_TEXT }
    public interface Listener {
        void onMeasurement(String value);
        void onCalibrationRequested(double pixelDistance);
        void onSelectionReady();
        void onTextRequested(float contentX,float contentY);
    }

    private final Paint paint=new Paint(Paint.ANTI_ALIAS_FLAG|Paint.FILTER_BITMAP_FLAG);
    private final ArrayList<PointF> points=new ArrayList<>();
    private final ArrayList<PointF> freehandPoints=new ArrayList<>();
    private final ArrayList<CadEdit> edits=new ArrayList<>();
    private final SourceEditSession sourceEdits=new SourceEditSession();
    private final Matrix imageMatrix=new Matrix();
    private final Matrix inverse=new Matrix();
    private Bitmap drawing;
    private DxfParser.Result vectorDrawing;
    private Mode mode=Mode.PAN;
    private Listener listener;
    private float lastX,lastY,scale=1f;
    private double unitsPerImagePixel=1d;
    private String unitName="piksel";
    private boolean multiTouch;
    private float[] snapPoints=new float[0];
    private boolean snapEnabled=true,lastSnapped;
    private boolean moveSelectedArmed,lastActionRegular,fastNavigation;
    private String currentLayer="0",currentTextStyle="STANDARD",currentTextFamily="sans";
    private int currentColorMode=CadEdit.COLOR_BYLAYER,currentColorValue=7;
    private boolean currentTextShx;
    private float currentTextHeight=30f,currentTextWidthFactor=1f;

    private boolean selecting,draggingSelection,exporting;
    private float selectionX,selectionY,selectionEndX,selectionEndY;
    private final ScaleGestureDetector scaleDetector;
    private final GestureDetector gestureDetector;

    private boolean stylusModeDetected,stylusDown,stylusHover;
    private float hoverX,hoverY,stylusPressure=.5f,freehandPressureSum;
    private int freehandPressureSamples;

    public CadView(Context c,AttributeSet a){
        super(c,a);setBackgroundColor(Color.rgb(18,24,30));setFocusable(true);
        scaleDetector=new ScaleGestureDetector(c,new ScaleGestureDetector.SimpleOnScaleGestureListener(){
            @Override public boolean onScaleBegin(ScaleGestureDetector d){fastNavigation=true;return true;}
            public boolean onScale(ScaleGestureDetector d){
                float next=Math.max(.001f,Math.min(200f,scale*d.getScaleFactor()));float f=next/scale;scale=next;
                imageMatrix.postScale(f,f,d.getFocusX(),d.getFocusY());invalidate();return true;
            }
            @Override public void onScaleEnd(ScaleGestureDetector d){fastNavigation=false;invalidate();}
        });
        gestureDetector=new GestureDetector(c,new GestureDetector.SimpleOnGestureListener(){
            @Override public boolean onDoubleTap(MotionEvent e){if(mode!=Mode.PAN||!hasDrawing())return false;fit();invalidate();notifyValue();return true;}
        });
    }

    private boolean hasDrawing(){return drawing!=null||vectorDrawing!=null;}
    private boolean editMode(){return mode==Mode.DRAW_LINE||mode==Mode.DRAW_POLYLINE||mode==Mode.DRAW_RECTANGLE||mode==Mode.DRAW_CIRCLE||mode==Mode.DRAW_TEXT;}
    private int contentWidth(){return vectorDrawing!=null?vectorDrawing.contentWidth():drawing!=null?drawing.getWidth():0;}
    private int contentHeight(){return vectorDrawing!=null?vectorDrawing.contentHeight():drawing!=null?drawing.getHeight():0;}

    public void setSnapPoints(float[] points){snapPoints=points==null?new float[0]:points.clone();lastSnapped=false;}
    public void setDrawingProperties(String layer,int colorMode,int colorValue,String textStyle,String textFamily,boolean textShx,float textHeight,float textWidthFactor){
        currentLayer=layer==null||layer.trim().isEmpty()?"0":layer.trim();
        currentColorMode=colorMode>=CadEdit.COLOR_BYLAYER&&colorMode<=CadEdit.COLOR_TRUECOLOR?colorMode:CadEdit.COLOR_BYLAYER;
        currentColorValue=currentColorMode==CadEdit.COLOR_ACI?Math.max(1,Math.min(255,colorValue)):currentColorMode==CadEdit.COLOR_TRUECOLOR?(colorValue&0x00FFFFFF):colorValue;
        currentTextStyle=textStyle==null||textStyle.trim().isEmpty()?"STANDARD":textStyle.trim();
        currentTextFamily=textFamily==null||textFamily.trim().isEmpty()?"sans":textFamily.trim();currentTextShx=textShx;
        currentTextHeight=Float.isFinite(textHeight)&&textHeight>0?textHeight:30f;currentTextWidthFactor=Float.isFinite(textWidthFactor)&&textWidthFactor>0?textWidthFactor:1f;
    }
    public String currentLayer(){return currentLayer;}public int currentColorMode(){return currentColorMode;}public int currentColorValue(){return currentColorValue;}
    public String currentTextStyle(){return currentTextStyle;}public String currentTextFamily(){return currentTextFamily;}public boolean currentTextShx(){return currentTextShx;}
    public float currentTextHeight(){return currentTextHeight;}public float currentTextWidthFactor(){return currentTextWidthFactor;}
    private CadEdit styled(CadEdit edit){return edit.withCadProperties(currentLayer,currentColorMode,currentColorValue);}
    public void setSnapEnabled(boolean enabled){snapEnabled=enabled;lastSnapped=false;invalidate();}
    public boolean isStylusModeDetected(){return stylusModeDetected;}

    public boolean beginSelection(){if(!hasDrawing())return false;selecting=true;draggingSelection=false;notifyValue();invalidate();return true;}
    public void cancelSelection(){selecting=false;draggingSelection=false;notifyValue();invalidate();}

    public void replaceVisibleDrawing(DxfParser.Result result){
        if(vectorDrawing==null||result==null)throw new IllegalArgumentException("Vektör çizim bulunamadı");
        vectorDrawing=result;drawing=null;selecting=false;draggingSelection=false;points.clear();freehandPoints.clear();moveSelectedArmed=false;
        int selected=sourceEdits.selectedId();DxfParser.SourceEntity source=result.sourceById(selected);
        if(source==null||!result.isSourceVisible(selected))sourceEdits.clearSelection();
        setSnapPoints(result.snapPoints);notifyValue();invalidate();
    }

    public void setListener(Listener l){listener=l;}

    public void setMode(Mode m){
        lastSnapped=false;selecting=false;draggingSelection=false;moveSelectedArmed=false;
        if(m!=Mode.SELECT_ENTITY)sourceEdits.clearSelection();
        mode=m;points.clear();freehandPoints.clear();notifyValue();invalidate();
    }

    public void setDrawing(Bitmap b){
        snapPoints=new float[0];lastSnapped=false;selecting=false;draggingSelection=false;moveSelectedArmed=false;
        vectorDrawing=null;drawing=b;edits.clear();sourceEdits.clear();unitsPerImagePixel=1;unitName="piksel";mode=Mode.PAN;points.clear();freehandPoints.clear();
        imageMatrix.reset();fit();invalidate();
    }

    public void setVectorDrawing(DxfParser.Result result){
        if(result==null)throw new IllegalArgumentException("Çizim yok");
        snapPoints=result.snapPoints.clone();lastSnapped=false;selecting=false;draggingSelection=false;moveSelectedArmed=false;
        drawing=null;vectorDrawing=result;edits.clear();sourceEdits.clear();unitsPerImagePixel=1;unitName="piksel";mode=Mode.PAN;points.clear();freehandPoints.clear();
        imageMatrix.reset();fit();invalidate();
    }

    public void fitToScreen(){if(hasDrawing()){fit();invalidate();notifyValue();}}
    public void zoomBy(float factor){
        if(!hasDrawing()||!Float.isFinite(factor)||factor<=0f)return;float next=Math.max(.001f,Math.min(200f,scale*factor));float applied=next/scale;scale=next;
        imageMatrix.postScale(applied,applied,getWidth()/2f,getHeight()/2f);invalidate();notifyValue();
    }

    /** All additions plus source replacements, for DXF export compatibility. */
    public List<CadEdit> getEdits(){ArrayList<CadEdit> copy=new ArrayList<>();for(CadEdit e:edits)copy.add(e.copy());copy.addAll(sourceEdits.replacements());return copy;}
    public List<CadEdit> getAddedEdits(){ArrayList<CadEdit> copy=new ArrayList<>();for(CadEdit e:edits)copy.add(e.copy());return copy;}
    public List<SourceReplacement> getSourceReplacements(){return sourceEdits.replacementRecords();}
    public List<SourceRange> getSourceRemovals(){return sourceEdits.removals();}
    public Set<Integer> getHiddenSourceIds(){return sourceEdits.hiddenSourceIds();}
    public int sourceModifiedCount(){return sourceEdits.modifiedCount();}
    public int editCount(){return edits.size()+sourceEdits.modifiedCount();}
    public boolean hasEdits(){return !edits.isEmpty()||sourceEdits.modifiedCount()>0;}

    public List<CadEdit> getVisibleEdits(){
        ArrayList<CadEdit> out=new ArrayList<>();for(CadEdit e:edits)out.add(e.copy());
        if(vectorDrawing==null){out.addAll(sourceEdits.replacements());return out;}
        for(Map.Entry<Integer,CadEdit> e:sourceEdits.replacementMap().entrySet()){
            if(vectorDrawing.isSourceVisible(e.getKey()))out.add(e.getValue().copy());
        }
        return out;
    }

    public boolean armMoveSelected(){
        if(mode!=Mode.SELECT_ENTITY||!sourceEdits.hasSelection())return false;moveSelectedArmed=true;notifyValue();invalidate();return true;
    }
    public boolean rotateSelectedEntity(){
        if(mode!=Mode.SELECT_ENTITY||!sourceEdits.rotateSelected(90f))return false;moveSelectedArmed=false;lastActionRegular=false;notifyValue();invalidate();return true;
    }
    public boolean copySelectedEntity(){
        if(mode!=Mode.SELECT_ENTITY||!sourceEdits.hasSelection())return false;float offset=24f*getResources().getDisplayMetrics().density/Math.max(.001f,scale);
        CadEdit copy=sourceEdits.copySelected(offset,-offset);if(copy==null)return false;edits.add(copy);lastActionRegular=true;notifyValue();invalidate();return true;
    }
    public boolean deleteSelectedEntity(){
        if(mode!=Mode.SELECT_ENTITY||!sourceEdits.deleteSelected())return false;moveSelectedArmed=false;lastActionRegular=false;notifyValue();invalidate();return true;
    }

    public boolean finishEdit(){
        if(mode!=Mode.DRAW_POLYLINE||points.size()<2)return false;float[] xy=new float[points.size()*2];
        for(int i=0;i<points.size();i++){xy[i*2]=points.get(i).x;xy[i*2+1]=points.get(i).y;}
        edits.add(styled(CadEdit.polyline(xy)));lastActionRegular=true;points.clear();lastSnapped=false;notifyValue();invalidate();return true;
    }

    public void addTextEdit(float x,float y,String text){addTextEdit(x,y,text,currentTextStyle,currentTextFamily,currentTextShx,currentTextHeight,currentTextWidthFactor);}
    public void addTextEdit(float x,float y,String text,String styleName,String familyHint,boolean shx,float height,float widthFactor){
        if(vectorDrawing==null||text==null||text.trim().isEmpty())return;
        CadEdit edit=CadEdit.styledText(x,y,text.trim(),0f,styleName,familyHint,shx,height,widthFactor,0f,0).withCadProperties(currentLayer,currentColorMode,currentColorValue);
        edits.add(edit);lastActionRegular=true;lastSnapped=false;notifyValue();invalidate();
    }

    public void undo(){
        lastSnapped=false;freehandPoints.clear();
        if(selecting){cancelSelection();return;}
        if(!points.isEmpty())points.remove(points.size()-1);
        else if(mode==Mode.SELECT_ENTITY&&!lastActionRegular&&sourceEdits.undo()){}
        else if(!edits.isEmpty()){edits.remove(edits.size()-1);lastActionRegular=false;}
        else if(sourceEdits.undo()){}
        moveSelectedArmed=false;notifyValue();invalidate();
    }

    public void clearMeasurement(){
        lastSnapped=false;freehandPoints.clear();
        if(selecting){cancelSelection();return;}
        if(mode==Mode.SELECT_ENTITY){sourceEdits.clearSelection();moveSelectedArmed=false;notifyValue();invalidate();return;}
        points.clear();notifyValue();invalidate();
    }

    public void setCalibration(double realDistance,String unit){
        if(!Double.isFinite(realDistance)||realDistance<=0||points.size()!=2)throw new IllegalArgumentException();double px=distance(points.get(0),points.get(1));if(px<=0)throw new IllegalArgumentException();
        unitsPerImagePixel=realDistance/px;unitName=unit;lastSnapped=false;points.clear();mode=Mode.DISTANCE;notifyValue();invalidate();
    }

    private void fit(){
        int width=contentWidth(),height=contentHeight();if(width<=0||height<=0||getWidth()==0||getHeight()==0)return;
        float s=Math.min((float)getWidth()/width,(float)getHeight()/height);imageMatrix.reset();imageMatrix.postScale(s,s);imageMatrix.postTranslate((getWidth()-width*s)/2f,(getHeight()-height*s)/2f);scale=s;
    }

    @Override protected void onSizeChanged(int w,int h,int ow,int oh){if(selecting)cancelSelection();if(ow==0)fit();}

    @Override protected void onDraw(Canvas c){
        super.onDraw(c);
        if(vectorDrawing!=null){
            if(fastNavigation&&!exporting&&vectorDrawing.bitmap!=null&&!vectorDrawing.bitmap.isRecycled())c.drawBitmap(vectorDrawing.bitmap,imageMatrix,paint);
            else vectorDrawing.drawVector(c,imageMatrix,sourceEdits.hiddenSourceIds(),false);
        }else if(drawing!=null)c.drawBitmap(drawing,imageMatrix,paint);else drawWelcome(c);
        drawEdits(c);drawLiveFreehand(c);

        paint.setStrokeWidth(4);paint.setStyle(Paint.Style.STROKE);paint.setColor(editMode()?Color.rgb(255,193,7):Color.rgb(25,181,165));
        ArrayList<PointF> screen=new ArrayList<>();for(PointF point:points){float[] xy={point.x,point.y};imageMatrix.mapPoints(xy);screen.add(new PointF(xy[0],xy[1]));}
        if(screen.size()>1){Path p=new Path();p.moveTo(screen.get(0).x,screen.get(0).y);for(int i=1;i<screen.size();i++)p.lineTo(screen.get(i).x,screen.get(i).y);if(mode==Mode.AREA&&screen.size()>2)p.close();c.drawPath(p,paint);}
        paint.setStyle(Paint.Style.FILL);for(PointF p:screen)c.drawCircle(p.x,p.y,8,paint);

        if(lastSnapped&&!screen.isEmpty()&&!exporting){PointF point=screen.get(screen.size()-1);paint.setStyle(Paint.Style.STROKE);paint.setColor(Color.WHITE);paint.setStrokeWidth(2);c.drawRect(point.x-12,point.y-12,point.x+12,point.y+12,paint);paint.setStyle(Paint.Style.FILL);}
        if(selecting&&draggingSelection&&!exporting){paint.setStyle(Paint.Style.STROKE);paint.setColor(Color.YELLOW);paint.setStrokeWidth(3);c.drawRect(Math.min(selectionX,selectionEndX),Math.min(selectionY,selectionEndY),Math.max(selectionX,selectionEndX),Math.max(selectionY,selectionEndY),paint);paint.setStyle(Paint.Style.FILL);}
        if(mode==Mode.SELECT_ENTITY&&sourceEdits.hasSelection()&&!exporting)drawSelectedSource(c);
        if(stylusHover&&!exporting)drawStylusCursor(c);
    }

    private void drawEdits(Canvas c){
        for(CadEdit edit:edits)drawEdit(c,edit,editColor(edit));
        if(vectorDrawing!=null){for(SourceReplacement replacement:sourceEdits.replacementRecords())vectorDrawing.drawSourceReplacement(c,imageMatrix,replacement,false,false);}
        resetTextPaint();paint.setPathEffect(null);paint.setStrokeWidth(3f);
    }

    private int editColor(CadEdit edit){
        if(edit.colorMode==CadEdit.COLOR_ACI)return DxfColor.aciArgb(edit.colorValue);
        if(edit.colorMode==CadEdit.COLOR_TRUECOLOR)return 0xFF000000|(edit.colorValue&0x00FFFFFF);
        return edit.colorMode==CadEdit.COLOR_BYBLOCK?Color.CYAN:Color.WHITE;
    }

    private void drawEdit(Canvas c,CadEdit edit,int color){
        paint.setPathEffect(null);paint.setColor(color);paint.setStyle(Paint.Style.STROKE);paint.setStrokeWidth(edit.strokeWidth);
        switch(edit.type){
            case LINE:{float[] v=map(edit.xy);c.drawLine(v[0],v[1],v[2],v[3],paint);break;}
            case RECTANGLE:{float[] v=map(edit.xy);c.drawRect(Math.min(v[0],v[2]),Math.min(v[1],v[3]),Math.max(v[0],v[2]),Math.max(v[1],v[3]),paint);break;}
            case CIRCLE:{float[] v=map(edit.xy);float r=(float)Math.hypot(v[2]-v[0],v[3]-v[1]);c.drawCircle(v[0],v[1],r,paint);break;}
            case POLYLINE:{float[] v=map(edit.xy);if(v.length>=4){Path p=new Path();p.moveTo(v[0],v[1]);for(int i=2;i+1<v.length;i+=2)p.lineTo(v[i],v[i+1]);if(edit.closed)p.close();c.drawPath(p,paint);}break;}
            case TEXT:{
                float[] v=map(edit.xy);paint.setStyle(Paint.Style.FILL);int save=c.save();c.rotate(edit.rotationDegrees,v[0],v[1]);
                if(edit.hasTextStyle()){
                    paint.setTypeface(edit.textShx?Typeface.MONOSPACE:Typeface.create(edit.textFamilyHint,Typeface.NORMAL));
                    paint.setTextSize(Math.max(1f,edit.textHeight*scale));paint.setTextScaleX(edit.textWidthFactor);paint.setTextSkewX((float)-Math.tan(Math.toRadians(edit.textOblique)));
                    c.scale((edit.textGenerationFlags&2)!=0?-1f:1f,(edit.textGenerationFlags&4)!=0?-1f:1f,v[0],v[1]);
                }else paint.setTextSize(Math.max(14f*getResources().getDisplayMetrics().scaledDensity,30f*scale));
                c.drawText(edit.text==null?"":edit.text,v[0],v[1],paint);c.restoreToCount(save);resetTextPaint();paint.setStyle(Paint.Style.STROKE);break;
            }
        }
    }

    private void resetTextPaint(){paint.setTypeface(null);paint.setTextScaleX(1f);paint.setTextSkewX(0f);}

    private void drawSelectedSource(Canvas c){
        CadEdit edit=sourceEdits.currentSelected();if(edit==null)return;float[] v=map(edit.xy);float left=Float.POSITIVE_INFINITY,top=Float.POSITIVE_INFINITY,right=Float.NEGATIVE_INFINITY,bottom=Float.NEGATIVE_INFINITY;
        if(edit.type==CadEdit.Type.CIRCLE&&v.length>=4){float r=(float)Math.hypot(v[2]-v[0],v[3]-v[1]);left=v[0]-r;right=v[0]+r;top=v[1]-r;bottom=v[1]+r;}
        else {for(int i=0;i+1<v.length;i+=2){left=Math.min(left,v[i]);right=Math.max(right,v[i]);top=Math.min(top,v[i+1]);bottom=Math.max(bottom,v[i+1]);}}
        if(!Float.isFinite(left)){return;}float pad=Math.max(10f,8f*getResources().getDisplayMetrics().density);left-=pad;right+=pad;top-=pad;bottom+=pad;
        paint.setPathEffect(null);paint.setStyle(Paint.Style.STROKE);paint.setStrokeWidth(2.5f);paint.setColor(moveSelectedArmed?Color.CYAN:Color.YELLOW);c.drawRect(left,top,right,bottom,paint);paint.setStyle(Paint.Style.FILL);
    }

    private void drawLiveFreehand(Canvas c){
        if(freehandPoints.size()<2)return;float[] xy=new float[freehandPoints.size()*2];for(int i=0;i<freehandPoints.size();i++){xy[i*2]=freehandPoints.get(i).x;xy[i*2+1]=freehandPoints.get(i).y;}imageMatrix.mapPoints(xy);
        Path p=new Path();p.moveTo(xy[0],xy[1]);for(int i=2;i+1<xy.length;i+=2)p.lineTo(xy[i],xy[i+1]);paint.setPathEffect(null);paint.setStyle(Paint.Style.STROKE);paint.setStrokeCap(Paint.Cap.ROUND);paint.setStrokeJoin(Paint.Join.ROUND);paint.setColor(Color.rgb(255,193,7));paint.setStrokeWidth(pressureWidth(stylusPressure));c.drawPath(p,paint);paint.setStrokeCap(Paint.Cap.BUTT);paint.setStrokeJoin(Paint.Join.MITER);
    }

    private void drawStylusCursor(Canvas c){float r=9f*getResources().getDisplayMetrics().density;paint.setPathEffect(null);paint.setStyle(Paint.Style.STROKE);paint.setStrokeWidth(1.5f);paint.setColor(Color.WHITE);c.drawCircle(hoverX,hoverY,r,paint);c.drawLine(hoverX-r*1.5f,hoverY,hoverX+r*1.5f,hoverY,paint);c.drawLine(hoverX,hoverY-r*1.5f,hoverX,hoverY+r*1.5f,paint);paint.setStyle(Paint.Style.FILL);}
    private float pressureWidth(float pressure){float p=Math.max(0f,Math.min(1f,pressure));return 2f+5f*p;}
    private float[] map(float[] source){float[] target=source.clone();imageMatrix.mapPoints(target);return target;}

    private void drawWelcome(Canvas c){paint.setTextAlign(Paint.Align.CENTER);paint.setColor(Color.LTGRAY);paint.setTextSize(36);c.drawText("MusaCAD",getWidth()/2f,getHeight()/2f-26,paint);paint.setTextSize(22);c.drawText("DWG / DXF dosyası açın",getWidth()/2f,getHeight()/2f+18,paint);paint.setTextSize(16);paint.setColor(Color.GRAY);c.drawText("İki parmak: yakınlaştır • Çift dokun: sığdır",getWidth()/2f,getHeight()/2f+54,paint);paint.setTextAlign(Paint.Align.LEFT);}

    @Override public boolean onGenericMotionEvent(MotionEvent e){
        if(isStylus(e)){stylusModeDetected=true;int action=e.getActionMasked();if(action==MotionEvent.ACTION_HOVER_ENTER||action==MotionEvent.ACTION_HOVER_MOVE){stylusHover=true;hoverX=e.getX();hoverY=e.getY();stylusPressure=e.getPressure();invalidate();return true;}if(action==MotionEvent.ACTION_HOVER_EXIT){stylusHover=false;invalidate();return true;}}
        return super.onGenericMotionEvent(e);
    }

    @Override public boolean onTouchEvent(MotionEvent e){
        if(!hasDrawing())return true;boolean stylus=isStylus(e);if(stylus){stylusModeDetected=true;stylusPressure=e.getPressure();stylusHover=false;}if(selecting)return selectionTouch(e);
        if(stylus){
            if(isStylusErase(e)){if(e.getActionMasked()==MotionEvent.ACTION_DOWN){performHapticFeedback(HapticFeedbackConstants.KEYBOARD_TAP);undo();}return true;}
            boolean buttonPan=(e.getButtonState()&MotionEvent.BUTTON_STYLUS_PRIMARY)!=0;if(buttonPan)return directPanTouch(e);
        }
        if(mode==Mode.SELECT_ENTITY)return sourceEditTouch(e);
        if(stylusModeDetected&&!stylus)return fingerNavigationTouch(e);
        if(stylus&&mode==Mode.PAN)return stylusFreehandTouch(e);

        if(mode==Mode.PAN)gestureDetector.onTouchEvent(e);if(e.getActionMasked()==MotionEvent.ACTION_DOWN)multiTouch=false;if(e.getPointerCount()>1)multiTouch=true;scaleDetector.onTouchEvent(e);if(multiTouch)return true;
        if(e.getActionMasked()==MotionEvent.ACTION_DOWN){lastX=e.getX();lastY=e.getY();return true;}
        if(e.getActionMasked()==MotionEvent.ACTION_MOVE&&mode==Mode.PAN){fastNavigation=true;float dx=e.getX()-lastX,dy=e.getY()-lastY;imageMatrix.postTranslate(dx,dy);lastX=e.getX();lastY=e.getY();invalidate();return true;}
        if(e.getActionMasked()==MotionEvent.ACTION_UP){if(mode==Mode.PAN){fastNavigation=false;invalidate();return true;}return addCadPoint(e.getX(),e.getY());}return true;
    }

    private boolean sourceEditTouch(MotionEvent e){
        if(vectorDrawing==null)return true;int action=e.getActionMasked();if(action==MotionEvent.ACTION_DOWN)multiTouch=false;if(e.getPointerCount()>1)multiTouch=true;scaleDetector.onTouchEvent(e);if(multiTouch)return true;
        if(action==MotionEvent.ACTION_UP){PointF point=screenToContent(e.getX(),e.getY());if(point==null)return true;
            if(moveSelectedArmed&&sourceEdits.hasSelection()){if(sourceEdits.moveSelectedTo(point.x,point.y)){lastActionRegular=false;performHapticFeedback(HapticFeedbackConstants.KEYBOARD_TAP);}moveSelectedArmed=false;notifyValue();invalidate();return true;}
            selectSourceAt(point.x,point.y);return true;}
        return true;
    }

    private void selectSourceAt(float x,float y){
        float tolerance=20f*getResources().getDisplayMetrics().density/Math.max(.001f,scale);DxfParser.SourceEntity source=null;
        int replacementId=sourceEdits.findReplacement(x,y,tolerance);
        if(replacementId>=0){DxfParser.SourceEntity candidate=vectorDrawing.sourceById(replacementId);if(candidate!=null&&vectorDrawing.isSourceVisible(replacementId))source=candidate;}
        if(source==null)source=vectorDrawing.findEditableSource(x,y,tolerance,sourceEdits.hiddenSourceIds());
        if(source==null){sourceEdits.clearSelection();moveSelectedArmed=false;notifyValue();invalidate();return;}
        sourceEdits.select(source.sourceId,source.range,source.prototype(),source.layer,source.color,source.lineType,source.lineTypeScale,source.lineWeight);
        moveSelectedArmed=false;performHapticFeedback(HapticFeedbackConstants.KEYBOARD_TAP);notifyValue();invalidate();
    }

    private boolean fingerNavigationTouch(MotionEvent e){
        if(stylusDown)return true;if(e.getActionMasked()==MotionEvent.ACTION_DOWN)multiTouch=false;if(e.getPointerCount()>1)multiTouch=true;scaleDetector.onTouchEvent(e);if(multiTouch)return true;
        if(e.getActionMasked()==MotionEvent.ACTION_DOWN){lastX=e.getX();lastY=e.getY();return true;}if(e.getActionMasked()==MotionEvent.ACTION_MOVE){imageMatrix.postTranslate(e.getX()-lastX,e.getY()-lastY);lastX=e.getX();lastY=e.getY();invalidate();return true;}return true;
    }

    private boolean directPanTouch(MotionEvent e){int action=e.getActionMasked();if(action==MotionEvent.ACTION_DOWN){stylusDown=true;lastX=e.getX();lastY=e.getY();return true;}if(action==MotionEvent.ACTION_MOVE){fastNavigation=true;imageMatrix.postTranslate(e.getX()-lastX,e.getY()-lastY);lastX=e.getX();lastY=e.getY();invalidate();return true;}if(action==MotionEvent.ACTION_UP||action==MotionEvent.ACTION_CANCEL){stylusDown=false;fastNavigation=false;invalidate();return true;}return true;}

    private boolean stylusFreehandTouch(MotionEvent e){
        int action=e.getActionMasked();if(action==MotionEvent.ACTION_DOWN){stylusDown=true;freehandPoints.clear();freehandPressureSum=0f;freehandPressureSamples=0;addFreehandSample(e.getX(),e.getY(),e.getPressure());invalidate();return true;}
        if(action==MotionEvent.ACTION_MOVE){int history=e.getHistorySize();for(int i=0;i<history;i++)addFreehandSample(e.getHistoricalX(i),e.getHistoricalY(i),e.getHistoricalPressure(i));addFreehandSample(e.getX(),e.getY(),e.getPressure());invalidate();return true;}
        if(action==MotionEvent.ACTION_UP){addFreehandSample(e.getX(),e.getY(),e.getPressure());commitFreehand();stylusDown=false;notifyValue();invalidate();return true;}if(action==MotionEvent.ACTION_CANCEL){freehandPoints.clear();stylusDown=false;invalidate();return true;}return true;
    }

    private void addFreehandSample(float screenX,float screenY,float pressure){
        PointF p=screenToContent(screenX,screenY);if(p==null)return;if(!freehandPoints.isEmpty()){PointF last=freehandPoints.get(freehandPoints.size()-1);float min=Math.max(.4f,1.5f/Math.max(.01f,scale));if(distance(last,p)<min)return;}
        freehandPoints.add(p);freehandPressureSum+=Math.max(0f,Math.min(1f,pressure));freehandPressureSamples++;stylusPressure=pressure;
    }

    private void commitFreehand(){
        if(freehandPoints.size()<2){freehandPoints.clear();return;}float[] xy=new float[freehandPoints.size()*2];for(int i=0;i<freehandPoints.size();i++){xy[i*2]=freehandPoints.get(i).x;xy[i*2+1]=freehandPoints.get(i).y;}
        float avg=freehandPressureSamples==0?.5f:freehandPressureSum/freehandPressureSamples;edits.add(styled(CadEdit.freehand(xy,pressureWidth(avg))));lastActionRegular=true;freehandPoints.clear();
    }

    private boolean addCadPoint(float screenX,float screenY){
        if(editMode()&&vectorDrawing==null){notifyValue();return true;}if(mode==Mode.CALIBRATE&&points.size()>=2)points.clear();PointF point=screenToContent(screenX,screenY);if(point==null)return true;float[] xy={point.x,point.y};
        int snapped=snapEnabled?SnapPoints.nearest(snapPoints,xy[0],xy[1],scale,18*getResources().getDisplayMetrics().density):-1;lastSnapped=snapped>=0;if(lastSnapped){xy[0]=snapPoints[snapped];xy[1]=snapPoints[snapped+1];}
        if(mode==Mode.DRAW_TEXT){if(listener!=null)listener.onTextRequested(xy[0],xy[1]);lastSnapped=false;notifyValue();invalidate();return true;}
        points.add(new PointF(xy[0],xy[1]));
        if(mode==Mode.DRAW_LINE&&points.size()==2){PointF a=points.get(0),b=points.get(1);edits.add(styled(CadEdit.line(a.x,a.y,b.x,b.y)));lastActionRegular=true;points.clear();lastSnapped=false;}
        else if(mode==Mode.DRAW_RECTANGLE&&points.size()==2){PointF a=points.get(0),b=points.get(1);edits.add(styled(CadEdit.rectangle(a.x,a.y,b.x,b.y)));lastActionRegular=true;points.clear();lastSnapped=false;}
        else if(mode==Mode.DRAW_CIRCLE&&points.size()==2){PointF a=points.get(0),b=points.get(1);edits.add(styled(CadEdit.circle(a.x,a.y,b.x,b.y)));lastActionRegular=true;points.clear();lastSnapped=false;}
        else if(mode==Mode.CALIBRATE&&points.size()==2&&listener!=null)listener.onCalibrationRequested(distance(points.get(0),points.get(1)));
        notifyValue();invalidate();return true;
    }

    private PointF screenToContent(float screenX,float screenY){float[] xy={screenX,screenY};if(!imageMatrix.invert(inverse))return null;inverse.mapPoints(xy);if(xy[0]<0||xy[1]<0||xy[0]>contentWidth()||xy[1]>contentHeight())return null;return new PointF(xy[0],xy[1]);}
    private boolean isStylus(MotionEvent e){if(e==null||e.getPointerCount()<1)return false;int type=e.getToolType(0);return type==MotionEvent.TOOL_TYPE_STYLUS||type==MotionEvent.TOOL_TYPE_ERASER;}
    private boolean isStylusErase(MotionEvent e){if(e.getToolType(0)==MotionEvent.TOOL_TYPE_ERASER)return true;return (e.getButtonState()&MotionEvent.BUTTON_STYLUS_SECONDARY)!=0;}

    private void notifyValue(){
        if(listener==null)return;if(selecting){listener.onMeasurement("Alanı sürükleyerek seçin • İptal: GERİ");return;}
        if(mode==Mode.SELECT_ENTITY){
            if(vectorDrawing==null){listener.onMeasurement("Kaynak nesne düzenleme yalnız vektörel DXF/DWG çizimlerinde kullanılabilir");return;}
            if(moveSelectedArmed){listener.onMeasurement("Taşı: seçili nesnenin yeni merkez noktasına dokunun");return;}
            if(sourceEdits.hasSelection()){DxfParser.SourceEntity s=vectorDrawing.sourceById(sourceEdits.selectedId());listener.onMeasurement("Seçili: "+(s==null?"nesne":s.type)+" • Taşı / Döndür / Kopya / Sil");return;}
            listener.onMeasurement("Nesne seçin • Düzenlenebilir: "+vectorDrawing.editableSourceCount()+" • LINE / Çoklu / Daire / Yazı");return;
        }
        if(mode==Mode.CALIBRATE)listener.onMeasurement(points.size()<2?"Bilinen uzunluğun iki ucunu seçin":"Gerçek uzunluğu girin");
        else if(mode==Mode.DISTANCE){double sum=0;for(int i=1;i<points.size();i++)sum+=distance(points.get(i-1),points.get(i));listener.onMeasurement(points.size()<2?"Mesafe için en az 2 nokta seçin":String.format(Locale.getDefault(),"Mesafe: %.3f %s",sum*unitsPerImagePixel,unitName));}
        else if(mode==Mode.AREA){double a=0;if(points.size()>2){for(int i=0;i<points.size();i++){PointF p=points.get(i),q=points.get((i+1)%points.size());a+=p.x*q.y-q.x*p.y;}a=Math.abs(a)/2*unitsPerImagePixel*unitsPerImagePixel;}listener.onMeasurement(points.size()<3?"Alan için en az 3 nokta seçin":String.format(Locale.getDefault(),"Alan: %.3f %s²",a,unitName));}
        else if(mode==Mode.DRAW_LINE)listener.onMeasurement("Çizgi: iki nokta seçin • Eklenen: "+edits.size());
        else if(mode==Mode.DRAW_POLYLINE)listener.onMeasurement("Çoklu çizgi: noktaları seçin • Bitir ile tamamlayın • Nokta: "+points.size());
        else if(mode==Mode.DRAW_RECTANGLE)listener.onMeasurement("Dikdörtgen: iki köşe seçin • Eklenen: "+edits.size());
        else if(mode==Mode.DRAW_CIRCLE)listener.onMeasurement("Daire: merkez ve yarıçap noktası seçin • Eklenen: "+edits.size());
        else if(mode==Mode.DRAW_TEXT)listener.onMeasurement("Yazı: yerleştirmek istediğiniz noktaya dokunun • Eklenen: "+edits.size());
        else if(stylusModeDetected)listener.onMeasurement("Kalem: serbest çizim • Kalem tuşu: gezin • Silgi/2. tuş: geri al • Parmak: gezin/zoom");
        else listener.onMeasurement("Sürükle: gez • İki parmak: yakınlaştır • Çift dokun: sığdır");
    }

    private boolean selectionTouch(MotionEvent e){
        int action=e.getActionMasked();if(action==MotionEvent.ACTION_DOWN){selectionX=selectionEndX=e.getX();selectionY=selectionEndY=e.getY();draggingSelection=true;invalidate();return true;}
        if(action==MotionEvent.ACTION_CANCEL||e.getPointerCount()>1){draggingSelection=false;invalidate();return true;}if(!draggingSelection)return true;selectionEndX=e.getX();selectionEndY=e.getY();invalidate();
        if(action==MotionEvent.ACTION_UP){if(selectionBounds()==null){draggingSelection=false;if(listener!=null)listener.onMeasurement("Çizim üzerinde daha geniş bir alan seçin");}else if(listener!=null)listener.onSelectionReady();}return true;
    }

    private SelectionBounds selectionBounds(){
        if(!selecting||!draggingSelection||!hasDrawing())return null;SelectionBounds b=SelectionBounds.clip(selectionX,selectionY,selectionEndX,selectionEndY,getWidth(),getHeight(),Math.max(8,(int)(8*getResources().getDisplayMetrics().density)));if(b==null)return null;
        RectF visible=new RectF(0,0,contentWidth(),contentHeight());imageMatrix.mapRect(visible);return RectF.intersects(visible,new RectF(b.left,b.top,b.left+b.width,b.top+b.height))?b:null;
    }

    public Bitmap selectionSnapshot(){
        SelectionBounds bounds=selectionBounds();if(bounds==null)throw new IllegalStateException("Önce çizim üzerinde alan seçin");Bitmap b=Bitmap.createBitmap(bounds.width,bounds.height,Bitmap.Config.ARGB_8888);Canvas canvas=new Canvas(b);canvas.translate(-bounds.left,-bounds.top);exporting=true;try{draw(canvas);}finally{exporting=false;}return b;
    }

    private double distance(PointF a,PointF b){return Math.hypot(a.x-b.x,a.y-b.y);}

    public Bitmap snapshot(){
        if(!hasDrawing())throw new IllegalStateException("Önce çizim açın");Bitmap b=Bitmap.createBitmap(getWidth(),getHeight(),Bitmap.Config.ARGB_8888);exporting=true;try{draw(new Canvas(b));}finally{exporting=false;}return b;
    }
}
