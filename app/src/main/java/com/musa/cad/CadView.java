package com.musa.cad;

import android.content.Context;
import android.graphics.*;
import android.util.AttributeSet;
import android.view.*;
import java.util.*;

public class CadView extends View {
    public enum Mode { PAN, SELECT_ENTITY, CALIBRATE, DISTANCE, AREA, DRAW_LINE, DRAW_POLYLINE, DRAW_RECTANGLE, DRAW_CIRCLE, DRAW_ARC, DRAW_ELLIPSE, DRAW_POINT, DRAW_XLINE, DRAW_TEXT }
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
    private final ArrayDeque<CadEdit> redoEdits=new ArrayDeque<>();
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
    private boolean moveSelectedArmed,lastActionRegular;
    private enum PairCommand { NONE, TRIM, EXTEND, FILLET, CHAMFER, MATCHPROP, JOIN }
    private PairCommand pairCommand=PairCommand.NONE;
    private float pairValue;
    private boolean breakArmed,lastUndoWasRegular;
    private float selectedPickX,selectedPickY;

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
            public boolean onScale(ScaleGestureDetector d){
                float next=Math.max(.001f,Math.min(200f,scale*d.getScaleFactor()));float f=next/scale;scale=next;
                imageMatrix.postScale(f,f,d.getFocusX(),d.getFocusY());invalidate();return true;
            }
        });
        gestureDetector=new GestureDetector(c,new GestureDetector.SimpleOnGestureListener(){
            @Override public boolean onDoubleTap(MotionEvent e){if(mode!=Mode.PAN||!hasDrawing())return false;fit();invalidate();notifyValue();return true;}
        });
    }

    private boolean hasDrawing(){return drawing!=null||vectorDrawing!=null;}
    private boolean editMode(){return mode==Mode.DRAW_LINE||mode==Mode.DRAW_POLYLINE||mode==Mode.DRAW_RECTANGLE||mode==Mode.DRAW_CIRCLE||mode==Mode.DRAW_ARC||mode==Mode.DRAW_ELLIPSE||mode==Mode.DRAW_POINT||mode==Mode.DRAW_XLINE||mode==Mode.DRAW_TEXT;}
    private int contentWidth(){return vectorDrawing!=null?vectorDrawing.contentWidth():drawing!=null?drawing.getWidth():0;}
    private int contentHeight(){return vectorDrawing!=null?vectorDrawing.contentHeight():drawing!=null?drawing.getHeight():0;}

    public void setSnapPoints(float[] points){snapPoints=points==null?new float[0]:points.clone();lastSnapped=false;}
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

    public boolean confirmCurrentCommand(){return finishEdit();}

    public void setMode(Mode m){
        lastSnapped=false;selecting=false;draggingSelection=false;moveSelectedArmed=false;pairCommand=PairCommand.NONE;breakArmed=false;
        if(m!=Mode.SELECT_ENTITY)sourceEdits.clearSelection();
        mode=m;points.clear();freehandPoints.clear();notifyValue();invalidate();
    }

    public void setDrawing(Bitmap b){
        snapPoints=new float[0];lastSnapped=false;selecting=false;draggingSelection=false;moveSelectedArmed=false;
        vectorDrawing=null;drawing=b;edits.clear();redoEdits.clear();sourceEdits.clear();unitsPerImagePixel=1;unitName="piksel";mode=Mode.PAN;points.clear();freehandPoints.clear();
        imageMatrix.reset();fit();invalidate();
    }

    public void setVectorDrawing(DxfParser.Result result){
        if(result==null)throw new IllegalArgumentException("Çizim yok");
        snapPoints=result.snapPoints.clone();lastSnapped=false;selecting=false;draggingSelection=false;moveSelectedArmed=false;
        drawing=null;vectorDrawing=result;edits.clear();redoEdits.clear();sourceEdits.clear();unitsPerImagePixel=1;unitName="piksel";mode=Mode.PAN;points.clear();freehandPoints.clear();
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

    private void addRegularEdit(CadEdit edit){if(edit==null)return;edits.add(edit);redoEdits.clear();lastUndoWasRegular=false;}
    private void addRegularEdits(Collection<CadEdit> list){if(list==null||list.isEmpty())return;for(CadEdit edit:list)if(edit!=null)edits.add(edit);redoEdits.clear();lastUndoWasRegular=false;}

    public boolean hasSelectedEntity(){return mode==Mode.SELECT_ENTITY&&sourceEdits.hasSelection();}

    public boolean armTrimSelected(){
        CadEdit selected=sourceEdits.currentSelected();
        if(mode!=Mode.SELECT_ENTITY||selected==null||selected.type!=CadEdit.Type.LINE)return false;
        pairCommand=PairCommand.TRIM;moveSelectedArmed=false;notifyValue();invalidate();return true;
    }
    public boolean armExtendSelected(){
        CadEdit selected=sourceEdits.currentSelected();
        if(mode!=Mode.SELECT_ENTITY||selected==null||selected.type!=CadEdit.Type.LINE)return false;
        pairCommand=PairCommand.EXTEND;moveSelectedArmed=false;notifyValue();invalidate();return true;
    }

    public boolean armFilletSelected(float radius){
        CadEdit selected=sourceEdits.currentSelected();
        if(mode!=Mode.SELECT_ENTITY||selected==null||selected.type!=CadEdit.Type.LINE||!Float.isFinite(radius)||radius<=0f)return false;
        pairValue=radius;pairCommand=PairCommand.FILLET;breakArmed=false;moveSelectedArmed=false;notifyValue();invalidate();return true;
    }
    public boolean armChamferSelected(float distance){
        CadEdit selected=sourceEdits.currentSelected();
        if(mode!=Mode.SELECT_ENTITY||selected==null||selected.type!=CadEdit.Type.LINE||!Float.isFinite(distance)||distance<=0f)return false;
        pairValue=distance;pairCommand=PairCommand.CHAMFER;breakArmed=false;moveSelectedArmed=false;notifyValue();invalidate();return true;
    }

    public boolean armMatchProperties(){
        if(mode!=Mode.SELECT_ENTITY||!sourceEdits.hasSelection())return false;
        pairCommand=PairCommand.MATCHPROP;breakArmed=false;moveSelectedArmed=false;notifyValue();invalidate();return true;
    }
    public boolean armJoinSelected(){
        CadEdit selected=sourceEdits.currentSelected();
        if(mode!=Mode.SELECT_ENTITY||selected==null||(selected.type!=CadEdit.Type.LINE&&selected.type!=CadEdit.Type.POLYLINE)||selected.closed)return false;
        pairCommand=PairCommand.JOIN;breakArmed=false;moveSelectedArmed=false;notifyValue();invalidate();return true;
    }

    public boolean armBreakSelected(){
        CadEdit selected=sourceEdits.currentSelected();
        if(mode!=Mode.SELECT_ENTITY||selected==null||selected.type!=CadEdit.Type.LINE)return false;
        breakArmed=true;pairCommand=PairCommand.NONE;moveSelectedArmed=false;notifyValue();invalidate();return true;
    }
    public boolean toggleSelectedPolylineClosed(){
        if(mode!=Mode.SELECT_ENTITY||!sourceEdits.hasSelection())return false;
        CadEdit selected=sourceEdits.currentSelected();if(selected==null||selected.type!=CadEdit.Type.POLYLINE)return false;
        boolean changed=sourceEdits.replaceSelected(selected.withClosed(!selected.closed));
        if(changed){lastActionRegular=false;notifyValue();invalidate();}
        return changed;
    }
    public String selectedEntityInfo(){
        if(mode!=Mode.SELECT_ENTITY||!sourceEdits.hasSelection())return null;
        CadEdit e=sourceEdits.currentSelected();DxfParser.SourceEntity source=vectorDrawing==null?null:vectorDrawing.sourceById(sourceEdits.selectedId());
        if(e==null)return null;
        StringBuilder b=new StringBuilder();
        b.append("Tür: ").append(source==null?e.type.name():source.type);
        if(source!=null){b.append("\nKatman: ").append(source.layer);b.append("\nÇizgi tipi: ").append(source.lineType);b.append("\nRenk: ").append(source.color);b.append("\nÇizgi kalınlığı: ").append(source.lineWeight);}
        b.append(String.format(Locale.getDefault(),"\nMerkez: %.3f, %.3f",e.centerX(),e.centerY()));
        b.append(String.format(Locale.getDefault(),"\nSınır: [%.3f, %.3f] - [%.3f, %.3f]",e.minX(),e.minY(),e.maxX(),e.maxY()));
        if(e.type==CadEdit.Type.LINE&&e.xy.length>=4)b.append(String.format(Locale.getDefault(),"\nUzunluk: %.3f",Math.hypot(e.xy[2]-e.xy[0],e.xy[3]-e.xy[1])));
        if(e.type==CadEdit.Type.CIRCLE&&e.xy.length>=4)b.append(String.format(Locale.getDefault(),"\nYarıçap: %.3f",Math.hypot(e.xy[2]-e.xy[0],e.xy[3]-e.xy[1])));
        if(e.type==CadEdit.Type.POLYLINE)b.append("\nKapalı: ").append(e.closed?"Evet":"Hayır");
        return b.toString();
    }

    public boolean armMoveSelected(){
        if(mode!=Mode.SELECT_ENTITY||!sourceEdits.hasSelection())return false;moveSelectedArmed=true;notifyValue();invalidate();return true;
    }
    public boolean rotateSelectedEntity(){
        if(mode!=Mode.SELECT_ENTITY||!sourceEdits.rotateSelected(90f))return false;moveSelectedArmed=false;lastActionRegular=false;notifyValue();invalidate();return true;
    }
    public boolean scaleSelectedEntity(float factor){
        if(mode!=Mode.SELECT_ENTITY||!sourceEdits.scaleSelected(factor))return false;moveSelectedArmed=false;lastActionRegular=false;notifyValue();invalidate();return true;
    }
    public boolean mirrorSelectedEntity(boolean verticalAxis){
        if(mode!=Mode.SELECT_ENTITY||!sourceEdits.mirrorSelected(verticalAxis))return false;moveSelectedArmed=false;lastActionRegular=false;notifyValue();invalidate();return true;
    }
    public boolean offsetSelectedEntity(float distance){
        if(mode!=Mode.SELECT_ENTITY||!sourceEdits.hasSelection())return false;CadEdit offset=sourceEdits.offsetSelected(distance);if(offset==null)return false;
        edits.add(offset);lastActionRegular=true;notifyValue();invalidate();return true;
    }
    public boolean copySelectedEntity(){
        if(mode!=Mode.SELECT_ENTITY||!sourceEdits.hasSelection())return false;float offset=24f*getResources().getDisplayMetrics().density/Math.max(.001f,scale);
        CadEdit copy=sourceEdits.copySelected(offset,-offset);if(copy==null)return false;edits.add(copy);lastActionRegular=true;notifyValue();invalidate();return true;
    }
    public int arraySelectedEntity(int rows,int columns,float columnSpacing,float rowSpacing){
        if(mode!=Mode.SELECT_ENTITY||!sourceEdits.hasSelection()||rows<1||columns<1)return 0;
        int added=0;
        for(int r=0;r<rows;r++){
            for(int c=0;c<columns;c++){
                if(r==0&&c==0)continue;
                CadEdit copy=sourceEdits.copySelected(c*columnSpacing,r*rowSpacing);
                if(copy!=null){edits.add(copy);added++;}
            }
        }
        if(added>0){lastActionRegular=true;notifyValue();invalidate();}
        return added;
    }
    public boolean explodeSelectedEntity(){
        if(mode!=Mode.SELECT_ENTITY||!sourceEdits.hasSelection())return false;
        CadEdit selected=sourceEdits.currentSelected();if(selected==null)return false;
        ArrayList<CadEdit> parts=new ArrayList<>();
        if(selected.type==CadEdit.Type.RECTANGLE&&selected.xy.length>=4){
            float x1=selected.xy[0],y1=selected.xy[1],x2=selected.xy[2],y2=selected.xy[3];
            parts.add(CadEdit.line(x1,y1,x2,y1));parts.add(CadEdit.line(x2,y1,x2,y2));
            parts.add(CadEdit.line(x2,y2,x1,y2));parts.add(CadEdit.line(x1,y2,x1,y1));
        }else if(selected.type==CadEdit.Type.POLYLINE&&selected.xy.length>=4){
            for(int i=2;i+1<selected.xy.length;i+=2)parts.add(CadEdit.line(selected.xy[i-2],selected.xy[i-1],selected.xy[i],selected.xy[i+1]));
            if(selected.closed&&selected.xy.length>=6)parts.add(CadEdit.line(selected.xy[selected.xy.length-2],selected.xy[selected.xy.length-1],selected.xy[0],selected.xy[1]));
        }else return false;
        if(parts.isEmpty()||!sourceEdits.deleteSelected())return false;
        edits.addAll(parts);lastActionRegular=true;moveSelectedArmed=false;notifyValue();invalidate();return true;
    }
    public void regenerate(){invalidate();notifyValue();}
    public boolean deleteSelectedEntity(){
        if(mode!=Mode.SELECT_ENTITY||!sourceEdits.deleteSelected())return false;moveSelectedArmed=false;lastActionRegular=false;notifyValue();invalidate();return true;
    }

    public boolean finishEdit(){
        if(mode!=Mode.DRAW_POLYLINE||points.size()<2)return false;float[] xy=new float[points.size()*2];
        for(int i=0;i<points.size();i++){xy[i*2]=points.get(i).x;xy[i*2+1]=points.get(i).y;}
        edits.add(CadEdit.polyline(xy));lastActionRegular=true;points.clear();lastSnapped=false;notifyValue();invalidate();return true;
    }

    public void addTextEdit(float x,float y,String text){
        if(vectorDrawing==null||text==null||text.trim().isEmpty())return;edits.add(CadEdit.text(x,y,text.trim()));lastActionRegular=true;lastSnapped=false;notifyValue();invalidate();
    }

    public void undo(){
        lastSnapped=false;freehandPoints.clear();
        if(selecting){cancelSelection();return;}
        if(!points.isEmpty()){points.remove(points.size()-1);lastUndoWasRegular=false;}
        else if(mode==Mode.SELECT_ENTITY&&!lastActionRegular&&sourceEdits.undo()){lastUndoWasRegular=false;}
        else if(!edits.isEmpty()){CadEdit removed=edits.remove(edits.size()-1);redoEdits.addFirst(removed.copy());lastActionRegular=false;lastUndoWasRegular=true;}
        else if(sourceEdits.undo()){lastUndoWasRegular=false;}
        moveSelectedArmed=false;notifyValue();invalidate();
    }
    public boolean redo(){
        lastSnapped=false;freehandPoints.clear();boolean changed=false;
        if(lastUndoWasRegular&&!redoEdits.isEmpty()){CadEdit edit=redoEdits.removeFirst();edits.add(edit.copy());lastActionRegular=true;changed=true;}
        else if(sourceEdits.redo()){lastActionRegular=false;changed=true;}
        if(changed){moveSelectedArmed=false;notifyValue();invalidate();}
        return changed;
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
        if(vectorDrawing!=null)vectorDrawing.drawVector(c,imageMatrix,sourceEdits.hiddenSourceIds());
        else if(drawing!=null)c.drawBitmap(drawing,imageMatrix,paint);else drawWelcome(c);
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
        for(CadEdit edit:edits)drawEdit(c,edit,Color.rgb(255,193,7));
        if(vectorDrawing!=null){for(SourceReplacement replacement:sourceEdits.replacementRecords())vectorDrawing.drawSourceReplacement(c,imageMatrix,replacement,false,false);}
        resetTextPaint();paint.setPathEffect(null);paint.setStrokeWidth(3f);
    }

    private void drawEdit(Canvas c,CadEdit edit,int color){
        paint.setPathEffect(null);paint.setColor(color);paint.setStyle(Paint.Style.STROKE);paint.setStrokeWidth(edit.strokeWidth);
        switch(edit.type){
            case LINE:{float[] v=map(edit.xy);c.drawLine(v[0],v[1],v[2],v[3],paint);break;}
            case RECTANGLE:{float[] v=map(edit.xy);c.drawRect(Math.min(v[0],v[2]),Math.min(v[1],v[3]),Math.max(v[0],v[2]),Math.max(v[1],v[3]),paint);break;}
            case CIRCLE:{float[] v=map(edit.xy);float r=(float)Math.hypot(v[2]-v[0],v[3]-v[1]);c.drawCircle(v[0],v[1],r,paint);break;}
            case ARC:{float[] v=map(edit.xy);if(v.length>=8){float r=(float)Math.hypot(v[0]-v[6],v[1]-v[7]);float start=angleDeg(v[0]-v[6],v[1]-v[7]);float mid=angleDeg(v[2]-v[6],v[3]-v[7]);float end=angleDeg(v[4]-v[6],v[5]-v[7]);float sweep=arcSweep(start,mid,end);RectF oval=new RectF(v[6]-r,v[7]-r,v[6]+r,v[7]+r);c.drawArc(oval,start,sweep,false,paint);}break;}
            case ELLIPSE:{float[] v=map(edit.xy);if(v.length>=6){float a=(float)Math.hypot(v[2]-v[0],v[3]-v[1]),b=(float)Math.hypot(v[4]-v[0],v[5]-v[1]),angle=(float)Math.toDegrees(Math.atan2(v[3]-v[1],v[2]-v[0]));int save=c.save();c.rotate(angle,v[0],v[1]);c.drawOval(new RectF(v[0]-a,v[1]-b,v[0]+a,v[1]+b),paint);c.restoreToCount(save);}break;}
            case POINT:{float[] v=map(edit.xy);if(v.length>=2){float r=Math.max(5f,5f*getResources().getDisplayMetrics().density);c.drawLine(v[0]-r,v[1],v[0]+r,v[1],paint);c.drawLine(v[0],v[1]-r,v[0],v[1]+r,paint);c.drawCircle(v[0],v[1],r*.55f,paint);}break;}
            case XLINE:{float[] v=map(edit.xy);if(v.length>=4){float dx=v[2]-v[0],dy=v[3]-v[1],len=(float)Math.hypot(dx,dy);if(len>1e-6f){dx/=len;dy/=len;float reach=(float)Math.hypot(getWidth(),getHeight())*2f;c.drawLine(v[0]-dx*reach,v[1]-dy*reach,v[0]+dx*reach,v[1]+dy*reach,paint);}}break;}
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
    private static float angleDeg(float x,float y){float a=(float)Math.toDegrees(Math.atan2(y,x));return a<0f?a+360f:a;}
    private static float ccwDelta(float from,float to){float d=to-from;while(d<0f)d+=360f;while(d>=360f)d-=360f;return d;}
    private static float arcSweep(float start,float mid,float end){float se=ccwDelta(start,end),sm=ccwDelta(start,mid);return sm<=se+1e-4f?se:-(360f-se);}

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
        if(e.getActionMasked()==MotionEvent.ACTION_MOVE&&mode==Mode.PAN){float dx=e.getX()-lastX,dy=e.getY()-lastY;imageMatrix.postTranslate(dx,dy);lastX=e.getX();lastY=e.getY();invalidate();return true;}
        if(e.getActionMasked()==MotionEvent.ACTION_UP&&mode!=Mode.PAN)return addCadPoint(e.getX(),e.getY());return true;
    }

    private boolean sourceEditTouch(MotionEvent e){
        if(vectorDrawing==null)return true;int action=e.getActionMasked();if(action==MotionEvent.ACTION_DOWN)multiTouch=false;if(e.getPointerCount()>1)multiTouch=true;scaleDetector.onTouchEvent(e);if(multiTouch)return true;
        if(action==MotionEvent.ACTION_UP){PointF point=screenToContent(e.getX(),e.getY());if(point==null)return true;
            if(moveSelectedArmed&&sourceEdits.hasSelection()){if(sourceEdits.moveSelectedTo(point.x,point.y)){lastActionRegular=false;performHapticFeedback(HapticFeedbackConstants.KEYBOARD_TAP);}moveSelectedArmed=false;notifyValue();invalidate();return true;}
            if(breakArmed&&sourceEdits.hasSelection()){applyBreak(point.x,point.y);return true;}
            if(pairCommand!=PairCommand.NONE&&sourceEdits.hasSelection()){applyPairCommand(point.x,point.y);return true;}
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
        selectedPickX=x;selectedPickY=y;moveSelectedArmed=false;pairCommand=PairCommand.NONE;breakArmed=false;performHapticFeedback(HapticFeedbackConstants.KEYBOARD_TAP);notifyValue();invalidate();
    }

    private void applyBreak(float x,float y){
        CadEdit target=sourceEdits.currentSelected();
        if(target==null||target.type!=CadEdit.Type.LINE||target.xy.length<4){breakArmed=false;notifyValue();invalidate();return;}
        float t=projectionParameter(x,y,target.xy[0],target.xy[1],target.xy[2],target.xy[3]);
        if(t<=.001f||t>=.999f){if(listener!=null)listener.onMeasurement("BREAK • Çizginin uçlarından uzakta bir nokta seçin");return;}
        float px=target.xy[0]+(target.xy[2]-target.xy[0])*t,py=target.xy[1]+(target.xy[3]-target.xy[1])*t;
        CadEdit first=CadEdit.line(target.xy[0],target.xy[1],px,py),second=CadEdit.line(px,py,target.xy[2],target.xy[3]);
        if(!sourceEdits.deleteSelected()){breakArmed=false;return;}
        edits.add(first);edits.add(second);breakArmed=false;lastActionRegular=true;performHapticFeedback(HapticFeedbackConstants.KEYBOARD_TAP);invalidate();
        if(listener!=null)listener.onMeasurement("BREAK • Çizgi iki parçaya bölündü");
    }

    private void applyPairCommand(float x,float y){
        CadEdit target=sourceEdits.currentSelected();
        if(target==null||target.type!=CadEdit.Type.LINE||target.xy.length<4){pairCommand=PairCommand.NONE;notifyValue();invalidate();return;}
        PairCommand command=pairCommand;
        DxfParser.SourceEntity boundary=(command==PairCommand.MATCHPROP||command==PairCommand.JOIN)?findEditableSourceAt(x,y):findBoundaryLineAt(x,y);
        if(boundary==null){if(listener!=null)listener.onMeasurement(pairCommandName(command)+" • İkinci nesneye dokunun");return;}
        CadEdit edge=sourceEdits.currentFor(boundary.sourceId);if(edge==null)edge=boundary.prototype();
        if(command==PairCommand.MATCHPROP){
            if(sourceEdits.matchSelectedPropertiesToOther(boundary.sourceId,boundary.range,boundary.prototype(),boundary.layer,boundary.color,boundary.lineType,boundary.lineTypeScale,boundary.lineWeight)){
                pairCommand=PairCommand.NONE;lastActionRegular=false;lastUndoWasRegular=false;performHapticFeedback(HapticFeedbackConstants.KEYBOARD_TAP);invalidate();
                if(listener!=null)listener.onMeasurement("MATCHPROP • Özellikler hedef nesneye aktarıldı");
            }
            return;
        }
        if(command==PairCommand.JOIN){
            CadEdit joined=joinGeometry(target,edge,30f*getResources().getDisplayMetrics().density/Math.max(.001f,scale));
            if(joined==null){if(listener!=null)listener.onMeasurement("JOIN • Açık LINE/POLYLINE uçları birbirine yeterince yakın olmalı");return;}
            if(sourceEdits.replaceSelectedAndDeleteOther(boundary.sourceId,boundary.range,boundary.prototype(),boundary.layer,boundary.color,boundary.lineType,boundary.lineTypeScale,boundary.lineWeight,joined)){
                pairCommand=PairCommand.NONE;lastActionRegular=false;lastUndoWasRegular=false;performHapticFeedback(HapticFeedbackConstants.KEYBOARD_TAP);invalidate();
                if(listener!=null)listener.onMeasurement("JOIN • İki nesne tek polyline olarak birleştirildi");
            }
            return;
        }
        if(edge.type!=CadEdit.Type.LINE||edge.xy.length<4){if(listener!=null)listener.onMeasurement(pairCommandName(command)+" • İkinci nesne LINE olmalı");return;}
        float[] hit=lineIntersection(target.xy[0],target.xy[1],target.xy[2],target.xy[3],edge.xy[0],edge.xy[1],edge.xy[2],edge.xy[3]);
        if(hit==null){if(listener!=null)listener.onMeasurement(pairCommandName(command)+" • Çizgiler paralel veya çakışık");return;}
        float ix=hit[0],iy=hit[1],t=hit[2],u=hit[3];

        if(command==PairCommand.TRIM||command==PairCommand.EXTEND){
            CadEdit result=null;
            if(command==PairCommand.TRIM){
                if(t<=1e-4f||t>=.9999f||u<-.0001f||u>1.0001f){if(listener!=null)listener.onMeasurement("TRIM • Kesişim hedef çizginin içinde ve sınır çizgisi üzerinde olmalı");return;}
                float pickT=projectionParameter(selectedPickX,selectedPickY,target.xy[0],target.xy[1],target.xy[2],target.xy[3]);
                result=pickT<t?CadEdit.line(ix,iy,target.xy[2],target.xy[3]):CadEdit.line(target.xy[0],target.xy[1],ix,iy);
            }else{
                if(u<-.0001f||u>1.0001f){if(listener!=null)listener.onMeasurement("EXTEND • Uzatma sınır çizgisinin üzerine ulaşmalı");return;}
                if(t<0f)result=CadEdit.line(ix,iy,target.xy[2],target.xy[3]);
                else if(t>1f)result=CadEdit.line(target.xy[0],target.xy[1],ix,iy);
                else {if(listener!=null)listener.onMeasurement("EXTEND • Hedef çizgi sınırı zaten kesiyor");return;}
            }
            if(result!=null&&sourceEdits.replaceSelected(result)){
                pairCommand=PairCommand.NONE;lastActionRegular=false;performHapticFeedback(HapticFeedbackConstants.KEYBOARD_TAP);invalidate();
                if(listener!=null)listener.onMeasurement(pairCommandName(command)+" • İşlem tamamlandı");
            }
            return;
        }

        float[] d1=rayFromIntersection(target,ix,iy,selectedPickX,selectedPickY);
        float[] d2=rayFromIntersection(edge,ix,iy,x,y);
        if(d1==null||d2==null){if(listener!=null)listener.onMeasurement(pairCommandName(command)+" • Çizgi yönleri belirlenemedi");return;}
        float dot=Math.max(-1f,Math.min(1f,d1[0]*d2[0]+d1[1]*d2[1]));
        float theta=(float)Math.acos(dot);
        if(theta<Math.toRadians(1d)||theta>Math.toRadians(179d)){if(listener!=null)listener.onMeasurement(pairCommandName(command)+" • Uygun bir köşe açısı seçin");return;}

        float distance=pairValue;
        CadEdit connector;
        float tangentDistance=distance;
        if(command==PairCommand.FILLET){
            tangentDistance=(float)(distance/Math.tan(theta/2d));
            if(!Float.isFinite(tangentDistance)||tangentDistance<=0f){if(listener!=null)listener.onMeasurement("FILLET • Yarıçap bu açı için uygulanamadı");return;}
        }
        float p1x=ix+d1[0]*tangentDistance,p1y=iy+d1[1]*tangentDistance;
        float p2x=ix+d2[0]*tangentDistance,p2y=iy+d2[1]*tangentDistance;
        CadEdit first=keptLineToPoint(target,ix,iy,d1,p1x,p1y);
        CadEdit second=keptLineToPoint(edge,ix,iy,d2,p2x,p2y);
        if(first==null||second==null){if(listener!=null)listener.onMeasurement(pairCommandName(command)+" • Çizgiler düzenlenemedi");return;}

        if(command==PairCommand.CHAMFER){
            connector=CadEdit.line(p1x,p1y,p2x,p2y);
        }else{
            float bx=d1[0]+d2[0],by=d1[1]+d2[1],bl=(float)Math.hypot(bx,by);
            if(bl<1e-6f){if(listener!=null)listener.onMeasurement("FILLET • Açıortay oluşturulamadı");return;}
            bx/=bl;by/=bl;
            float centerDistance=(float)(distance/Math.sin(theta/2d));
            float cx=ix+bx*centerDistance,cy=iy+by*centerDistance;
            float vx=ix-cx,vy=iy-cy,vl=(float)Math.hypot(vx,vy);
            if(vl<1e-6f){if(listener!=null)listener.onMeasurement("FILLET • Yay merkezi oluşturulamadı");return;}
            float mx=cx+vx/vl*distance,my=cy+vy/vl*distance;
            connector=CadEdit.arc(p1x,p1y,mx,my,p2x,p2y);
            if(connector==null){if(listener!=null)listener.onMeasurement("FILLET • Yay geometrisi oluşturulamadı");return;}
        }

        boolean replaced=sourceEdits.replaceSelectedAndOther(
            boundary.sourceId,boundary.range,boundary.prototype(),boundary.layer,boundary.color,boundary.lineType,boundary.lineTypeScale,boundary.lineWeight,
            first,second);
        if(!replaced){if(listener!=null)listener.onMeasurement(pairCommandName(command)+" • İki çizgi birlikte güncellenemedi");return;}
        edits.add(connector);lastActionRegular=true;pairCommand=PairCommand.NONE;performHapticFeedback(HapticFeedbackConstants.KEYBOARD_TAP);invalidate();
        if(listener!=null)listener.onMeasurement(pairCommandName(command)+" • İşlem tamamlandı");
    }

    private static String pairCommandName(PairCommand command){
        switch(command){case TRIM:return "TRIM";case EXTEND:return "EXTEND";case FILLET:return "FILLET";case CHAMFER:return "CHAMFER";case MATCHPROP:return "MATCHPROP";case JOIN:return "JOIN";default:return "KOMUT";}
    }

    private static float[] rayFromIntersection(CadEdit line,float ix,float iy,float pickX,float pickY){
        if(line==null||line.xy.length<4)return null;
        float t=projectionParameter(pickX,pickY,line.xy[0],line.xy[1],line.xy[2],line.xy[3]);
        float px=line.xy[0]+(line.xy[2]-line.xy[0])*t,py=line.xy[1]+(line.xy[3]-line.xy[1])*t;
        float vx=px-ix,vy=py-iy,len=(float)Math.hypot(vx,vy);
        if(len<1e-5f){
            float d1=(float)Math.hypot(line.xy[0]-pickX,line.xy[1]-pickY),d2=(float)Math.hypot(line.xy[2]-pickX,line.xy[3]-pickY);
            float ex=d1<=d2?line.xy[0]:line.xy[2],ey=d1<=d2?line.xy[1]:line.xy[3];vx=ex-ix;vy=ey-iy;len=(float)Math.hypot(vx,vy);
        }
        if(len<1e-6f)return null;return new float[]{vx/len,vy/len};
    }

    private static CadEdit keptLineToPoint(CadEdit line,float ix,float iy,float[] dir,float tx,float ty){
        if(line==null||line.xy.length<4||dir==null)return null;
        float a=(line.xy[0]-ix)*dir[0]+(line.xy[1]-iy)*dir[1],b=(line.xy[2]-ix)*dir[0]+(line.xy[3]-iy)*dir[1];
        float ex=a>=b?line.xy[0]:line.xy[2],ey=a>=b?line.xy[1]:line.xy[3];
        if(Math.hypot(ex-tx,ey-ty)<1e-6)return null;
        return CadEdit.line(ex,ey,tx,ty);
    }

    private DxfParser.SourceEntity findEditableSourceAt(float x,float y){
        if(vectorDrawing==null)return null;float tolerance=20f*getResources().getDisplayMetrics().density/Math.max(.001f,scale),best=tolerance;DxfParser.SourceEntity found=null;int selectedId=sourceEdits.selectedId();
        int replacementId=sourceEdits.findReplacement(x,y,tolerance);if(replacementId>=0&&replacementId!=selectedId){DxfParser.SourceEntity replacement=vectorDrawing.sourceById(replacementId);if(replacement!=null&&vectorDrawing.isSourceVisible(replacementId))return replacement;}
        Set<Integer> hidden=sourceEdits.hiddenSourceIds();
        for(DxfParser.SourceEntity candidate:vectorDrawing.editableSources()){
            if(candidate.sourceId==selectedId||hidden.contains(candidate.sourceId)||!vectorDrawing.isSourceVisible(candidate.sourceId))continue;
            float d=candidate.prototype().hitDistance(x,y);if(d<=best){best=d;found=candidate;}
        }
        return found;
    }
    private static CadEdit joinGeometry(CadEdit a,CadEdit b,float tolerance){
        float[] pa=openPath(a),pb=openPath(b);if(pa==null||pb==null)return null;
        float[] ar=reversePath(pa),br=reversePath(pb);float[][] aa={pa,ar},bb={pb,br};float best=Float.POSITIVE_INFINITY;float[] left=null,right=null;
        for(float[] x:aa)for(float[] y:bb){float d=(float)Math.hypot(x[x.length-2]-y[0],x[x.length-1]-y[1]);if(d<best){best=d;left=x;right=y;}}
        if(left==null||right==null||best>Math.max(1e-4f,tolerance))return null;
        int skip=best<1e-4f?2:0;float[] out=new float[left.length+right.length-skip];System.arraycopy(left,0,out,0,left.length);System.arraycopy(right,skip,out,left.length,right.length-skip);
        return CadEdit.polyline(out,false);
    }
    private static float[] openPath(CadEdit e){
        if(e==null||e.closed)return null;if(e.type==CadEdit.Type.LINE&&e.xy.length>=4)return e.xy.clone();if(e.type==CadEdit.Type.POLYLINE&&e.xy.length>=4)return e.xy.clone();return null;
    }
    private static float[] reversePath(float[] p){float[] out=new float[p.length];int n=p.length/2;for(int i=0;i<n;i++){out[i*2]=p[(n-1-i)*2];out[i*2+1]=p[(n-1-i)*2+1];}return out;}

    private DxfParser.SourceEntity findBoundaryLineAt(float x,float y){
        if(vectorDrawing==null)return null;float tolerance=20f*getResources().getDisplayMetrics().density/Math.max(.001f,scale),best=tolerance;DxfParser.SourceEntity found=null;
        Set<Integer> hidden=sourceEdits.hiddenSourceIds();int selectedId=sourceEdits.selectedId();
        for(DxfParser.SourceEntity candidate:vectorDrawing.editableSources()){
            if(candidate.sourceId==selectedId||hidden.contains(candidate.sourceId)||!vectorDrawing.isSourceVisible(candidate.sourceId))continue;
            CadEdit p=candidate.prototype();if(p.type!=CadEdit.Type.LINE)continue;float d=p.hitDistance(x,y);if(d<=best){best=d;found=candidate;}
        }
        return found;
    }

    private static float[] lineIntersection(float ax,float ay,float bx,float by,float cx,float cy,float dx,float dy){
        float rX=bx-ax,rY=by-ay,sX=dx-cx,sY=dy-cy,den=rX*sY-rY*sX;if(Math.abs(den)<1e-7f)return null;
        float qx=cx-ax,qy=cy-ay,t=(qx*sY-qy*sX)/den,u=(qx*rY-qy*rX)/den;
        return new float[]{ax+t*rX,ay+t*rY,t,u};
    }
    private static float projectionParameter(float px,float py,float ax,float ay,float bx,float by){
        float dx=bx-ax,dy=by-ay,len=dx*dx+dy*dy;if(len<1e-12f)return 0f;return ((px-ax)*dx+(py-ay)*dy)/len;
    }

    private boolean fingerNavigationTouch(MotionEvent e){
        if(stylusDown)return true;if(e.getActionMasked()==MotionEvent.ACTION_DOWN)multiTouch=false;if(e.getPointerCount()>1)multiTouch=true;scaleDetector.onTouchEvent(e);if(multiTouch)return true;
        if(e.getActionMasked()==MotionEvent.ACTION_DOWN){lastX=e.getX();lastY=e.getY();return true;}if(e.getActionMasked()==MotionEvent.ACTION_MOVE){imageMatrix.postTranslate(e.getX()-lastX,e.getY()-lastY);lastX=e.getX();lastY=e.getY();invalidate();return true;}return true;
    }

    private boolean directPanTouch(MotionEvent e){int action=e.getActionMasked();if(action==MotionEvent.ACTION_DOWN){stylusDown=true;lastX=e.getX();lastY=e.getY();return true;}if(action==MotionEvent.ACTION_MOVE){imageMatrix.postTranslate(e.getX()-lastX,e.getY()-lastY);lastX=e.getX();lastY=e.getY();invalidate();return true;}if(action==MotionEvent.ACTION_UP||action==MotionEvent.ACTION_CANCEL){stylusDown=false;return true;}return true;}

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
        float avg=freehandPressureSamples==0?.5f:freehandPressureSum/freehandPressureSamples;edits.add(CadEdit.freehand(xy,pressureWidth(avg)));lastActionRegular=true;freehandPoints.clear();
    }

    private boolean addCadPoint(float screenX,float screenY){
        if(editMode()&&vectorDrawing==null){notifyValue();return true;}if(mode==Mode.CALIBRATE&&points.size()>=2)points.clear();PointF point=screenToContent(screenX,screenY);if(point==null)return true;float[] xy={point.x,point.y};
        int snapped=snapEnabled?SnapPoints.nearest(snapPoints,xy[0],xy[1],scale,18*getResources().getDisplayMetrics().density):-1;lastSnapped=snapped>=0;if(lastSnapped){xy[0]=snapPoints[snapped];xy[1]=snapPoints[snapped+1];}
        if(mode==Mode.DRAW_TEXT){if(listener!=null)listener.onTextRequested(xy[0],xy[1]);lastSnapped=false;notifyValue();invalidate();return true;}
        points.add(new PointF(xy[0],xy[1]));
        if(mode==Mode.DRAW_LINE&&points.size()==2){PointF a=points.get(0),b=points.get(1);edits.add(CadEdit.line(a.x,a.y,b.x,b.y));lastActionRegular=true;points.clear();lastSnapped=false;}
        else if(mode==Mode.DRAW_RECTANGLE&&points.size()==2){PointF a=points.get(0),b=points.get(1);edits.add(CadEdit.rectangle(a.x,a.y,b.x,b.y));lastActionRegular=true;points.clear();lastSnapped=false;}
        else if(mode==Mode.DRAW_CIRCLE&&points.size()==2){PointF a=points.get(0),b=points.get(1);edits.add(CadEdit.circle(a.x,a.y,b.x,b.y));lastActionRegular=true;points.clear();lastSnapped=false;}
        else if(mode==Mode.DRAW_ARC&&points.size()==3){PointF a=points.get(0),m=points.get(1),b=points.get(2);CadEdit arc=CadEdit.arc(a.x,a.y,m.x,m.y,b.x,b.y);if(arc==null){points.clear();lastSnapped=false;if(listener!=null)listener.onMeasurement("ARC • Üç nokta aynı doğru üzerinde olamaz");invalidate();return true;}edits.add(arc);lastActionRegular=true;points.clear();lastSnapped=false;}
        else if(mode==Mode.DRAW_ELLIPSE&&points.size()==3){PointF c=points.get(0),a=points.get(1),b=points.get(2);CadEdit ellipse=CadEdit.ellipse(c.x,c.y,a.x,a.y,b.x,b.y);if(ellipse==null){points.clear();lastSnapped=false;if(listener!=null)listener.onMeasurement("ELLIPSE • Geçerli ana ve kısa eksen seçin");invalidate();return true;}edits.add(ellipse);lastActionRegular=true;points.clear();lastSnapped=false;}
        else if(mode==Mode.DRAW_POINT&&points.size()==1){PointF p=points.get(0);edits.add(CadEdit.point(p.x,p.y));lastActionRegular=true;points.clear();lastSnapped=false;}
        else if(mode==Mode.DRAW_XLINE&&points.size()==2){PointF a=points.get(0),b=points.get(1);CadEdit xline=CadEdit.xline(a.x,a.y,b.x,b.y);if(xline==null){points.clear();lastSnapped=false;if(listener!=null)listener.onMeasurement("XLINE • İki farklı nokta seçin");invalidate();return true;}edits.add(xline);lastActionRegular=true;points.clear();lastSnapped=false;}
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
            if(breakArmed){listener.onMeasurement("BREAK • Çizgiyi böleceğiniz noktaya dokunun");return;}
            if(pairCommand==PairCommand.TRIM){listener.onMeasurement("TRIM • Kesme sınırı olacak ikinci çizgiye dokunun");return;}
            if(pairCommand==PairCommand.EXTEND){listener.onMeasurement("EXTEND • Uzatma sınırı olacak ikinci çizgiye dokunun");return;}
            if(pairCommand==PairCommand.FILLET){listener.onMeasurement(String.format(Locale.getDefault(),"FILLET • R=%.3f • İkinci çizgiye dokunun",pairValue));return;}
            if(pairCommand==PairCommand.CHAMFER){listener.onMeasurement(String.format(Locale.getDefault(),"CHAMFER • D=%.3f • İkinci çizgiye dokunun",pairValue));return;}
            if(pairCommand==PairCommand.MATCHPROP){listener.onMeasurement("MATCHPROP • Özelliklerin aktarılacağı hedef nesneye dokunun");return;}
            if(pairCommand==PairCommand.JOIN){listener.onMeasurement("JOIN • Birleştirilecek ikinci LINE/POLYLINE nesnesine dokunun");return;}
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
        else if(mode==Mode.DRAW_ARC)listener.onMeasurement("Yay: başlangıç, yay üzeri ve bitiş olmak üzere 3 nokta seçin • Nokta: "+points.size());
        else if(mode==Mode.DRAW_ELLIPSE)listener.onMeasurement("Elips: merkez, ana eksen ucu ve kısa eksen yönü seçin • Nokta: "+points.size());
        else if(mode==Mode.DRAW_POINT)listener.onMeasurement("POINT: noktanın yerini seçin");
        else if(mode==Mode.DRAW_XLINE)listener.onMeasurement("XLINE: doğrultuyu belirlemek için iki nokta seçin • Nokta: "+points.size());
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
