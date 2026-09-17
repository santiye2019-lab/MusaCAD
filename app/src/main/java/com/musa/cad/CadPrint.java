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
                     Set<Integer> hiddenSourceIds,Bitmap bitmap,String displayName){
        if(drawing==null&&bitmap==null){Toast.makeText(activity,"Yazdırılabilir çizim bulunamadı",Toast.LENGTH_SHORT).show();return;}
        final List<CadEdit> editCopy=new ArrayList<>();if(additions!=null)for(CadEdit e:additions)editCopy.add(e.copy());
        final List<SourceReplacement> replacementCopy=new ArrayList<>();if(replacements!=null)for(SourceReplacement r:replacements)replacementCopy.add(r.copy());
        final Set<Integer> hiddenCopy=hiddenSourceIds==null?Collections.emptySet():new LinkedHashSet<>(hiddenSourceIds);
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
                handedOff[0]=true;String base=cleanName(displayName);manager.print("MusaCAD - "+base,new Adapter(activity,drawing,editCopy,replacementCopy,hiddenCopy,bitmap,base,denominator,monochrome),attributes);
            }).setNegativeButton("İPTAL",null).create();
        dialog.setOnDismissListener(d->{if(!handedOff[0]&&bitmap!=null&&!bitmap.isRecycled())bitmap.recycle();});dialog.show();
    }

    private static Spinner spinner(Activity activity,LinearLayout box,String title,String[] values){TextView label=new TextView(activity);label.setText(title);label.setPadding(0,12,0,2);box.addView(label);Spinner spinner=new Spinner(activity);spinner.setAdapter(new ArrayAdapter<>(activity,android.R.layout.simple_spinner_dropdown_item,values));box.addView(spinner);return spinner;}
    private static String cleanName(String name){String base=name==null?"cizim":name.replaceFirst("(?i)\\.(dwg|dxf)$","");base=base.replaceAll("[^\\p{L}\\p{N}._ -]+","_").trim();return base.isEmpty()?"cizim":base;}

    private static final class Adapter extends PrintDocumentAdapter {
        private final Context context;private final DxfParser.Result drawing;private final List<CadEdit> additions;private final List<SourceReplacement> replacements;private final Set<Integer> hidden;
        private final Bitmap bitmap;private final String name;private final int denominator;private final boolean forceMonochrome;private PrintAttributes attributes;
        Adapter(Context context,DxfParser.Result drawing,List<CadEdit> additions,List<SourceReplacement> replacements,Set<Integer> hidden,Bitmap bitmap,String name,int denominator,boolean forceMonochrome){
            this.context=context;this.drawing=drawing;this.additions=additions;this.replacements=replacements;this.hidden=hidden;this.bitmap=bitmap;this.name=name;this.denominator=denominator;this.forceMonochrome=forceMonochrome;
        }
        @Override public void onLayout(PrintAttributes oldAttributes,PrintAttributes newAttributes,CancellationSignal signal,LayoutResultCallback callback,Bundle extras){attributes=newAttributes;if(signal.isCanceled()){callback.onLayoutCancelled();return;}PrintDocumentInfo info=new PrintDocumentInfo.Builder(name+".pdf").setContentType(PrintDocumentInfo.CONTENT_TYPE_DOCUMENT).setPageCount(1).build();callback.onLayoutFinished(info,oldAttributes==null||!newAttributes.equals(oldAttributes));}
        @Override public void onWrite(PageRange[] pages,ParcelFileDescriptor destination,CancellationSignal signal,WriteResultCallback callback){
            if(signal.isCanceled()){callback.onWriteCancelled();return;}if(!containsPage(pages,0)){callback.onWriteFinished(new PageRange[0]);return;}PrintedPdfDocument document=null;
            try{
                if(attributes==null)throw new IOException("Yazdırma özellikleri hazırlanamadı");document=new PrintedPdfDocument(context,attributes);PdfDocument.Page page=document.startPage(0);Canvas canvas=page.getCanvas();canvas.drawColor(Color.WHITE);Rect content=document.getPageContentRect();RectF target=new RectF(content);int save=canvas.save();canvas.clipRect(target);
                boolean mono=forceMonochrome||attributes.getColorMode()==PrintAttributes.COLOR_MODE_MONOCHROME;
                if(drawing!=null){Matrix matrix=drawing.printMatrix(target,denominator);drawing.drawVectorForPrint(canvas,matrix,mono,hidden);drawEdits(canvas,additions,matrix,mono,null);for(SourceReplacement replacement:replacements)if(drawing.visibleLayers.contains(replacement.layer))drawEdits(canvas,Collections.singletonList(replacement.edit),matrix,mono,replacement.color);}
                else renderBitmap(canvas,bitmap,target,mono);
                canvas.restoreToCount(save);document.finishPage(page);if(signal.isCanceled()){callback.onWriteCancelled();return;}try(FileOutputStream out=new FileOutputStream(destination.getFileDescriptor())){document.writeTo(out);out.flush();}callback.onWriteFinished(new PageRange[]{PageRange.ALL_PAGES});
            }catch(Exception e){callback.onWriteFailed(e.getMessage()==null?"Yazdırma belgesi oluşturulamadı":e.getMessage());}finally{if(document!=null)document.close();}
        }
        @Override public void onFinish(){if(bitmap!=null&&!bitmap.isRecycled())bitmap.recycle();}
    }

    private static boolean containsPage(PageRange[] ranges,int page){if(ranges==null)return false;for(PageRange range:ranges)if(page>=range.getStart()&&page<=range.getEnd())return true;return false;}
    private static void renderBitmap(Canvas canvas,Bitmap bitmap,RectF target,boolean monochrome){if(bitmap==null||bitmap.isRecycled())return;float scale=Math.min(target.width()/bitmap.getWidth(),target.height()/bitmap.getHeight());float w=bitmap.getWidth()*scale,h=bitmap.getHeight()*scale;RectF dst=new RectF(target.centerX()-w/2f,target.centerY()-h/2f,target.centerX()+w/2f,target.centerY()+h/2f);Paint paint=new Paint(Paint.ANTI_ALIAS_FLAG|Paint.FILTER_BITMAP_FLAG);if(monochrome){ColorMatrix cm=new ColorMatrix();cm.setSaturation(0f);paint.setColorFilter(new ColorMatrixColorFilter(cm));}canvas.drawBitmap(bitmap,null,dst,paint);}

    private static void drawEdits(Canvas canvas,List<CadEdit> edits,Matrix matrix,boolean monochrome,Integer sourceColor){
        if(edits==null||edits.isEmpty())return;int color=monochrome?Color.BLACK:(sourceColor==null?Color.rgb(230,126,34):paperColor(sourceColor));Paint paint=new Paint(Paint.ANTI_ALIAS_FLAG);paint.setColor(color);paint.setStrokeWidth(.9f);paint.setStyle(Paint.Style.STROKE);
        for(CadEdit edit:edits){float[] v=edit.xy.clone();matrix.mapPoints(v);switch(edit.type){
            case LINE:canvas.drawLine(v[0],v[1],v[2],v[3],paint);break;
            case RECTANGLE:canvas.drawRect(Math.min(v[0],v[2]),Math.min(v[1],v[3]),Math.max(v[0],v[2]),Math.max(v[1],v[3]),paint);break;
            case CIRCLE:canvas.drawCircle(v[0],v[1],(float)Math.hypot(v[2]-v[0],v[3]-v[1]),paint);break;
            case POLYLINE:if(v.length>=4){Path path=new Path();path.moveTo(v[0],v[1]);for(int i=2;i+1<v.length;i+=2)path.lineTo(v[i],v[i+1]);if(edit.closed)path.close();canvas.drawPath(path,paint);}break;
            case TEXT:paint.setStyle(Paint.Style.FILL);paint.setTextSize(9f);int save=canvas.save();canvas.rotate(edit.rotationDegrees,v[0],v[1]);canvas.drawText(edit.text==null?"":edit.text,v[0],v[1],paint);canvas.restoreToCount(save);paint.setStyle(Paint.Style.STROKE);break;
        }}
    }
    private static int paperColor(int color){int r=Color.red(color),g=Color.green(color),b=Color.blue(color);return r>=245&&g>=245&&b>=245?Color.BLACK:Color.rgb(r,g,b);}
    private CadPrint(){}
}
