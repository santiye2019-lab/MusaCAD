package com.musa.cad;

import android.app.*;
import android.content.*;
import android.graphics.*;
import android.graphics.pdf.PdfDocument;
import android.os.*;
import android.print.*;
import android.print.pdf.PrintedPdfDocument;
import android.view.ViewGroup;
import android.widget.*;
import java.io.*;
import java.util.*;

/** Native Android one-page CAD printing with A4/A3, orientation, color and engineering scale. */
final class CadPrint {
    private static final int MARGIN_MILS=394;
    private static final int[] SCALE_VALUES={0,20,25,50,75,100,200,500,1000};

    static void show(Activity activity,DxfParser.Result drawing,List<CadEdit> additions,List<SourceReplacement> replacements,
                     Set<Integer> hiddenSourceIds,List<CadBlock.Definition> blocks,Bitmap bitmap,String displayName){
        if(drawing==null&&bitmap==null){Toast.makeText(activity,"Yazdırılabilir çizim bulunamadı",Toast.LENGTH_SHORT).show();return;}
        final List<CadEdit> editCopy=new ArrayList<>();if(additions!=null)for(CadEdit e:additions)editCopy.add(e.copy());
        final List<SourceReplacement> replacementCopy=new ArrayList<>();if(replacements!=null)for(SourceReplacement r:replacements)replacementCopy.add(r.copy());
        final Set<Integer> hiddenCopy=hiddenSourceIds==null?Collections.emptySet():new LinkedHashSet<>(hiddenSourceIds);
        final LinkedHashMap<String,CadBlock.Definition> blockCopy=new LinkedHashMap<>();if(blocks!=null)for(CadBlock.Definition d:blocks)if(d!=null)blockCopy.put(d.name.toUpperCase(Locale.ROOT),new CadBlock.Definition(d.name,d.members));
        final boolean[] handedOff={false};

        LinearLayout box=new LinearLayout(activity);box.setOrientation(LinearLayout.VERTICAL);int pad=Math.round(18*activity.getResources().getDisplayMetrics().density);box.setPadding(pad,pad/2,pad,pad/2);
        Spinner paper=spinner(activity,box,"Kağıt",new String[]{"A4","A3"});Spinner orientation=spinner(activity,box,"Yön",new String[]{"Yatay","Dikey"});
        boolean wide=drawing!=null?drawing.drawingAspectRatio()>=1f:bitmap.getWidth()>=bitmap.getHeight();orientation.setSelection(wide?0:1);

        String[] scaleLabels;
        if(drawing!=null&&drawing.hasPhysicalUnits()){scaleLabels=new String[SCALE_VALUES.length];scaleLabels[0]="Sayfaya sığdır";for(int i=1;i<SCALE_VALUES.length;i++)scaleLabels[i]="1:"+SCALE_VALUES[i];}
        else scaleLabels=new String[]{"Sayfaya sığdır"};
        Spinner scale=spinner(activity,box,"Ölçek",scaleLabels);if(scaleLabels.length>1)scale.setSelection(5);
        Spinner color=spinner(activity,box,"Çıktı",new String[]{"Renkli","Siyah-beyaz"});TextView info=new TextView(activity);info.setPadding(0,pad/2,0,0);
        if(drawing!=null&&drawing.hasPhysicalUnits())info.setText("Çizim birimi: "+drawing.drawingUnitName()+" • gerçek 1: ölçek kullanılabilir");else info.setText("Çizim birimi tanımsız/önizleme • sayfaya sığdır kullanılacak");
        box.addView(info,new LinearLayout.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT,ViewGroup.LayoutParams.WRAP_CONTENT));

        AlertDialog dialog=new AlertDialog.Builder(activity).setTitle("Yazdırma ayarları").setView(box)
            .setPositiveButton("YAZDIR",(d,w)->{
                int denominator=scale.getSelectedItemPosition()<SCALE_VALUES.length?SCALE_VALUES[scale.getSelectedItemPosition()]:0;if(!(drawing!=null&&drawing.hasPhysicalUnits()))denominator=0;
                boolean monochrome=color.getSelectedItemPosition()==1;PrintAttributes.MediaSize media=paper.getSelectedItemPosition()==0?PrintAttributes.MediaSize.ISO_A4:PrintAttributes.MediaSize.ISO_A3;media=orientation.getSelectedItemPosition()==0?media.asLandscape():media.asPortrait();
                PrintAttributes attributes=new PrintAttributes.Builder().setMediaSize(media).setMinMargins(new PrintAttributes.Margins(MARGIN_MILS,MARGIN_MILS,MARGIN_MILS,MARGIN_MILS)).setColorMode(monochrome?PrintAttributes.COLOR_MODE_MONOCHROME:PrintAttributes.COLOR_MODE_COLOR).build();
                PrintManager manager=(PrintManager)activity.getSystemService(Context.PRINT_SERVICE);if(manager==null){Toast.makeText(activity,"Android yazdırma hizmeti bulunamadı",Toast.LENGTH_LONG).show();return;}
                handedOff[0]=true;String base=cleanName(displayName);manager.print("MusaCAD - "+base,new Adapter(activity,drawing,editCopy,replacementCopy,hiddenCopy,blockCopy,bitmap,base,denominator,monochrome),attributes);
            }).setNegativeButton("İPTAL",null).create();
        dialog.setOnDismissListener(d->{if(!handedOff[0]&&bitmap!=null&&!bitmap.isRecycled())bitmap.recycle();});dialog.show();
    }

    private static Spinner spinner(Activity activity,LinearLayout box,String title,String[] values){TextView label=new TextView(activity);label.setText(title);label.setPadding(0,12,0,2);box.addView(label);Spinner spinner=new Spinner(activity);spinner.setAdapter(new ArrayAdapter<>(activity,android.R.layout.simple_spinner_dropdown_item,values));box.addView(spinner);return spinner;}
    private static String cleanName(String name){String base=name==null?"cizim":name.replaceFirst("(?i)\\.(dwg|dxf)$","");base=base.replaceAll("[^\\p{L}\\p{N}._ -]+","_").trim();return base.isEmpty()?"cizim":base;}

    private static final class Adapter extends PrintDocumentAdapter {
        private final Context context;private final DxfParser.Result drawing;private final List<CadEdit> additions;private final List<SourceReplacement> replacements;private final Set<Integer> hidden;private final Map<String,CadBlock.Definition> blocks;
        private final Bitmap bitmap;private final String name;private final int denominator;private final boolean forceMonochrome;private PrintAttributes attributes;
        Adapter(Context context,DxfParser.Result drawing,List<CadEdit> additions,List<SourceReplacement> replacements,Set<Integer> hidden,Map<String,CadBlock.Definition> blocks,Bitmap bitmap,String name,int denominator,boolean forceMonochrome){
            this.context=context;this.drawing=drawing;this.additions=additions;this.replacements=replacements;this.hidden=hidden;this.blocks=blocks;this.bitmap=bitmap;this.name=name;this.denominator=denominator;this.forceMonochrome=forceMonochrome;
        }
        @Override public void onLayout(PrintAttributes oldAttributes,PrintAttributes newAttributes,CancellationSignal signal,LayoutResultCallback callback,Bundle extras){attributes=newAttributes;if(signal.isCanceled()){callback.onLayoutCancelled();return;}PrintDocumentInfo info=new PrintDocumentInfo.Builder(name+".pdf").setContentType(PrintDocumentInfo.CONTENT_TYPE_DOCUMENT).setPageCount(1).build();callback.onLayoutFinished(info,oldAttributes==null||!newAttributes.equals(oldAttributes));}
        @Override public void onWrite(PageRange[] pages,ParcelFileDescriptor destination,CancellationSignal signal,WriteResultCallback callback){
            if(signal.isCanceled()){callback.onWriteCancelled();return;}if(!containsPage(pages,0)){callback.onWriteFinished(new PageRange[0]);return;}PrintedPdfDocument document=null;
            try{
                if(attributes==null)throw new IOException("Yazdırma özellikleri hazırlanamadı");document=new PrintedPdfDocument(context,attributes);PdfDocument.Page page=document.startPage(0);Canvas canvas=page.getCanvas();canvas.drawColor(Color.WHITE);Rect content=document.getPageContentRect();RectF target=new RectF(content);int save=canvas.save();canvas.clipRect(target);
                boolean mono=forceMonochrome||attributes.getColorMode()==PrintAttributes.COLOR_MODE_MONOCHROME;
                if(drawing!=null){
                    Matrix matrix=drawing.printMatrix(target,denominator);drawing.drawVectorForPrint(canvas,matrix,mono,hidden);drawEdits(canvas,additions,matrix,mono,blocks);
                    for(SourceReplacement replacement:replacements)drawing.drawSourceReplacement(canvas,matrix,replacement,true,mono);
                }else renderBitmap(canvas,bitmap,target,mono);
                canvas.restoreToCount(save);document.finishPage(page);if(signal.isCanceled()){callback.onWriteCancelled();return;}try(FileOutputStream out=new FileOutputStream(destination.getFileDescriptor())){document.writeTo(out);out.flush();}callback.onWriteFinished(new PageRange[]{PageRange.ALL_PAGES});
            }catch(Exception e){callback.onWriteFailed(e.getMessage()==null?"Yazdırma belgesi oluşturulamadı":e.getMessage());}finally{if(document!=null)document.close();}
        }
        @Override public void onFinish(){if(bitmap!=null&&!bitmap.isRecycled())bitmap.recycle();}
    }

    private static boolean containsPage(PageRange[] ranges,int page){if(ranges==null)return false;for(PageRange range:ranges)if(page>=range.getStart()&&page<=range.getEnd())return true;return false;}
    private static void renderBitmap(Canvas canvas,Bitmap bitmap,RectF target,boolean monochrome){if(bitmap==null||bitmap.isRecycled())return;float scale=Math.min(target.width()/bitmap.getWidth(),target.height()/bitmap.getHeight());float w=bitmap.getWidth()*scale,h=bitmap.getHeight()*scale;RectF dst=new RectF(target.centerX()-w/2f,target.centerY()-h/2f,target.centerX()+w/2f,target.centerY()+h/2f);Paint paint=new Paint(Paint.ANTI_ALIAS_FLAG|Paint.FILTER_BITMAP_FLAG);if(monochrome){ColorMatrix cm=new ColorMatrix();cm.setSaturation(0f);paint.setColorFilter(new ColorMatrixColorFilter(cm));}canvas.drawBitmap(bitmap,null,dst,paint);}

    private static void drawEdits(Canvas canvas,List<CadEdit> edits,Matrix matrix,boolean monochrome,Map<String,CadBlock.Definition> blocks){
        if(edits==null||edits.isEmpty())return;int color=monochrome?Color.BLACK:Color.rgb(230,126,34);Paint paint=new Paint(Paint.ANTI_ALIAS_FLAG);paint.setColor(color);paint.setStrokeWidth(.9f);paint.setStyle(Paint.Style.STROKE);
        for(CadEdit edit:edits)drawEdit(canvas,edit,matrix,paint,blocks,0);
    }

    private static void drawEdit(Canvas canvas,CadEdit edit,Matrix matrix,Paint paint,Map<String,CadBlock.Definition> blocks,int depth){
        if(edit==null||depth>8)return;float[] v=edit.xy.clone();matrix.mapPoints(v);
        switch(edit.type){
            case LINE:canvas.drawLine(v[0],v[1],v[2],v[3],paint);break;
            case RECTANGLE:canvas.drawRect(Math.min(v[0],v[2]),Math.min(v[1],v[3]),Math.max(v[0],v[2]),Math.max(v[1],v[3]),paint);break;
            case CIRCLE:canvas.drawCircle(v[0],v[1],(float)Math.hypot(v[2]-v[0],v[3]-v[1]),paint);break;
            case ARC:{
                if(v.length<8)break;float r=(float)Math.hypot(v[0]-v[6],v[1]-v[7]);float start=angle(v[0]-v[6],v[1]-v[7]),mid=angle(v[2]-v[6],v[3]-v[7]),end=angle(v[4]-v[6],v[5]-v[7]);float sweep=arcSweep(start,mid,end);
                canvas.drawArc(new RectF(v[6]-r,v[7]-r,v[6]+r,v[7]+r),start,sweep,false,paint);break;
            }
            case ELLIPSE:{
                if(v.length<6)break;float a=(float)Math.hypot(v[2]-v[0],v[3]-v[1]),b=(float)Math.hypot(v[4]-v[0],v[5]-v[1]),rot=(float)Math.toDegrees(Math.atan2(v[3]-v[1],v[2]-v[0]));int save=canvas.save();canvas.rotate(rot,v[0],v[1]);canvas.drawOval(new RectF(v[0]-a,v[1]-b,v[0]+a,v[1]+b),paint);canvas.restoreToCount(save);break;
            }
            case POINT:{
                if(v.length<2)break;float r=3f;canvas.drawLine(v[0]-r,v[1],v[0]+r,v[1],paint);canvas.drawLine(v[0],v[1]-r,v[0],v[1]+r,paint);canvas.drawCircle(v[0],v[1],r*.5f,paint);break;
            }
            case XLINE:{
                if(v.length<4)break;float dx=v[2]-v[0],dy=v[3]-v[1],len=(float)Math.hypot(dx,dy);if(len<1e-6f)break;dx/=len;dy/=len;float reach=(float)Math.hypot(canvas.getWidth(),canvas.getHeight())*2f;canvas.drawLine(v[0]-dx*reach,v[1]-dy*reach,v[0]+dx*reach,v[1]+dy*reach,paint);break;
            }
            case POLYLINE:{
                if(v.length>=4){Path path=new Path();path.moveTo(v[0],v[1]);for(int i=2;i+1<v.length;i+=2)path.lineTo(v[i],v[i+1]);if(edit.closed)path.close();canvas.drawPath(path,paint);}break;
            }
            case HATCH:drawHatch(canvas,edit,v,paint,matrixScale(matrix));break;
            case INSERT:{
                if(v.length<2||edit.text==null||blocks==null)break;CadBlock.Definition def=blocks.get(edit.text.toUpperCase(Locale.ROOT));if(def==null)break;
                for(CadEdit member:def.members){CadEdit placed=member.scaled(edit.insertScale(),0f,0f).rotated(edit.rotationDegrees,0f,0f).translated(edit.xy[0],edit.xy[1]);drawEdit(canvas,placed,matrix,paint,blocks,depth+1);}break;
            }
            case TEXT:{
                if(v.length<2)break;Paint.Style old=paint.getStyle();float oldSize=paint.getTextSize();paint.setStyle(Paint.Style.FILL);
                float size=edit.hasTextStyle()?Math.max(3f,edit.textHeight*matrixScale(matrix)):9f;paint.setTextSize(size);
                int save=canvas.save();canvas.rotate(edit.rotationDegrees,v[0],v[1]);canvas.drawText(edit.text==null?"":edit.text,v[0],v[1],paint);canvas.restoreToCount(save);paint.setTextSize(oldSize);paint.setStyle(old);break;
            }
        }
    }

    private static void drawHatch(Canvas canvas,CadEdit edit,float[] v,Paint paint,float scale){
        if(v.length<6)return;Path path=new Path();path.moveTo(v[0],v[1]);for(int i=2;i+1<v.length;i+=2)path.lineTo(v[i],v[i+1]);path.close();Paint.Style old=paint.getStyle();int oldAlpha=paint.getAlpha();
        if("SOLID".equalsIgnoreCase(edit.text)){paint.setStyle(Paint.Style.FILL);paint.setAlpha(80);canvas.drawPath(path,paint);}
        else{
            RectF b=new RectF();path.computeBounds(b,true);int save=canvas.save();canvas.clipPath(path);float cx=b.centerX(),cy=b.centerY(),diag=(float)Math.hypot(b.width(),b.height())*1.5f,spacing=Math.max(4f,edit.textHeight*Math.max(.001f,scale));
            canvas.rotate(45f+edit.rotationDegrees,cx,cy);paint.setStyle(Paint.Style.STROKE);paint.setAlpha(180);for(float y=cy-diag;y<=cy+diag;y+=spacing)canvas.drawLine(cx-diag,y,cx+diag,y,paint);canvas.restoreToCount(save);
        }
        paint.setStyle(Paint.Style.STROKE);paint.setAlpha(oldAlpha);canvas.drawPath(path,paint);paint.setStyle(old);
    }

    private static float matrixScale(Matrix matrix){float[] v={0f,0f,1f,0f};matrix.mapPoints(v);return (float)Math.hypot(v[2]-v[0],v[3]-v[1]);}
    private static float angle(float x,float y){float a=(float)Math.toDegrees(Math.atan2(y,x));return a<0f?a+360f:a;}
    private static float ccw(float from,float to){float d=to-from;while(d<0f)d+=360f;while(d>=360f)d-=360f;return d;}
    private static float arcSweep(float start,float mid,float end){float se=ccw(start,end),sm=ccw(start,mid);return sm<=se+1e-4f?se:-(360f-se);}

    private CadPrint(){}
}
