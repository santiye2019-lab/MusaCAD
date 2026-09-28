package com.musa.cad;

import android.app.*;
import android.content.*;
import android.graphics.*;
import android.graphics.pdf.PdfDocument;
import android.os.*;
import android.print.*;
import android.print.pdf.PrintedPdfDocument;
import android.view.*;
import android.widget.*;
import java.io.*;
import java.util.*;
import java.util.concurrent.*;

/**
 * Advanced one-page CAD printing.
 *
 * The in-app Print Preview and Android print/PDF spooler use the exact same
 * renderPage(...) path so geometry, layers, colors, text, blocks, tables,
 * placed raster images, dimensions and annotations stay WYSIWYG.
 */
final class CadPrint {
    private static final int MARGIN_MILS=394; // 10 mm
    private static final int[] SCALE_VALUES={0,1,2,5,10,20,25,50,75,100,150,200,-1};
    private static final String[] SCALE_LABELS={
        "Sayfaya sığdır","1:1","1:2","1:5","1:10","1:20","1:25","1:50","1:75","1:100","1:150","1:200","Özel ölçek"
    };
    private static final PrintAttributes.MediaSize[] PAPERS={
        PrintAttributes.MediaSize.ISO_A4,
        PrintAttributes.MediaSize.ISO_A3,
        PrintAttributes.MediaSize.ISO_A2,
        PrintAttributes.MediaSize.ISO_A1,
        PrintAttributes.MediaSize.ISO_A0
    };
    private static final String[] PAPER_LABELS={"A4","A3","A2","A1","A0"};

    private enum AreaMode { EXTENTS, DISPLAY, WINDOW }

    private static final class Spec {
        final AreaMode areaMode;
        final RectF sourceBounds;
        final int denominator;
        final boolean monochrome;
        final PrintAttributes.MediaSize media;
        Spec(AreaMode areaMode,RectF sourceBounds,int denominator,boolean monochrome,PrintAttributes.MediaSize media){
            this.areaMode=areaMode;
            this.sourceBounds=sourceBounds==null?null:new RectF(sourceBounds);
            this.denominator=denominator;
            this.monochrome=monochrome;
            this.media=media;
        }
    }

    static void show(Activity activity,DxfParser.Result drawing,List<CadEdit> additions,List<SourceReplacement> replacements,
                     Set<Integer> hiddenSourceIds,List<CadBlock.Definition> blocks,List<CadImageOverlay> images,
                     Bitmap bitmap,String displayName,RectF displayBounds,RectF windowBounds,boolean preferWindow,
                     Runnable requestWindowSelection){
        if(drawing==null&&bitmap==null){Toast.makeText(activity,"Yazdırılabilir çizim bulunamadı",Toast.LENGTH_SHORT).show();return;}

        final List<CadEdit> editCopy=new ArrayList<>();
        if(additions!=null)for(CadEdit e:additions)if(e!=null)editCopy.add(e.copy());
        final List<SourceReplacement> replacementCopy=new ArrayList<>();
        if(replacements!=null)for(SourceReplacement r:replacements)if(r!=null)replacementCopy.add(r.copy());
        final Set<Integer> hiddenCopy=hiddenSourceIds==null?Collections.emptySet():new LinkedHashSet<>(hiddenSourceIds);
        final LinkedHashMap<String,CadBlock.Definition> blockCopy=new LinkedHashMap<>();
        if(blocks!=null)for(CadBlock.Definition d:blocks)if(d!=null)blockCopy.put(d.name.toUpperCase(Locale.ROOT),new CadBlock.Definition(d.name,d.members));
        final List<CadImageOverlay> imageCopy=new ArrayList<>();
        if(images!=null)for(CadImageOverlay image:images)if(image!=null&&image.bitmap!=null&&!image.bitmap.isRecycled())imageCopy.add(image.copy());

        final RectF extents=drawing==null?null:contentExtents(drawing,editCopy,replacementCopy,blockCopy,imageCopy);
        final RectF display=validBounds(displayBounds)?new RectF(displayBounds):extents==null?null:new RectF(extents);
        final RectF window=validBounds(windowBounds)?new RectF(windowBounds):null;
        final boolean physicalScale=drawing!=null&&drawing.hasPhysicalUnits();
        final boolean[] handedOff={false},closed={false};
        final Bitmap[] previewBitmap={null};
        final Handler mainHandler=new Handler(Looper.getMainLooper());
        final ExecutorService previewExecutor=Executors.newSingleThreadExecutor(r->{
            Thread t=new Thread(r,"MusaCAD-print-preview");
            t.setDaemon(true);
            t.setPriority(Math.max(Thread.MIN_PRIORITY,Thread.NORM_PRIORITY-1));
            return t;
        });
        final int[] previewGeneration={0};
        final Runnable[] pendingPreview={null};

        int pad=Math.round(14*activity.getResources().getDisplayMetrics().density);
        LinearLayout box=new LinearLayout(activity);box.setOrientation(LinearLayout.VERTICAL);box.setPadding(pad,pad/2,pad,pad/2);
        ScrollView scroll=new ScrollView(activity);scroll.setFillViewport(false);scroll.addView(box,new ScrollView.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT,ViewGroup.LayoutParams.WRAP_CONTENT));

        TextView targetInfo=new TextView(activity);
        targetInfo.setText("Hedef: FİZİKSEL YAZICI veya PDF\nYAZDIR'a basınca Android yazdırma ekranından fiziksel/ağ yazıcısı veya “PDF olarak kaydet” seçebilirsiniz.");
        targetInfo.setTextColor(Color.WHITE);targetInfo.setTextSize(11);targetInfo.setPadding(0,0,0,pad/2);box.addView(targetInfo);

        Spinner area=spinner(activity,box,"Yazdırma alanı",new String[]{"Extents • Tüm çizim","Display • Ekrandaki görünüm","Window • Seçili alan"});
        if(preferWindow&&window!=null)area.setSelection(2);
        Spinner paper=spinner(activity,box,"Kağıt",PAPER_LABELS);
        Spinner orientation=spinner(activity,box,"Yön",new String[]{"Yatay","Dikey"});
        boolean wide=drawing!=null?drawing.drawingAspectRatio()>=1f:bitmap.getWidth()>=bitmap.getHeight();
        orientation.setSelection(wide?0:1);

        Spinner scale=spinner(activity,box,"Ölçek",physicalScale?SCALE_LABELS:new String[]{"Sayfaya sığdır"});
        scale.setSelection(0);
        EditText customScale=new EditText(activity);customScale.setSingleLine(true);customScale.setHint("Özel ölçek paydası • ör. 125 veya 1:125");customScale.setText("100");
        customScale.setInputType(android.text.InputType.TYPE_CLASS_NUMBER);customScale.setVisibility(View.GONE);box.addView(customScale);

        Spinner color=spinner(activity,box,"Çıktı",new String[]{"Renkli","Siyah-beyaz"});
        TextView info=new TextView(activity);info.setTextSize(11);info.setPadding(0,pad/2,0,pad/2);box.addView(info);

        ImageView preview=new ImageView(activity);preview.setAdjustViewBounds(true);preview.setScaleType(ImageView.ScaleType.FIT_CENTER);preview.setBackgroundColor(Color.rgb(48,52,55));
        int screenHeight=activity.getResources().getDisplayMetrics().heightPixels;
        int previewHeight=Math.max(dp(activity,140),Math.min(dp(activity,240),screenHeight/4));
        box.addView(preview,new LinearLayout.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT,previewHeight));

        final Runnable[] refresh={null};
        refresh[0]=()->{
            if(closed[0])return;
            if(pendingPreview[0]!=null){mainHandler.removeCallbacks(pendingPreview[0]);pendingPreview[0]=null;}
            final int generation=++previewGeneration[0];

            boolean custom=physicalScale&&scale.getSelectedItemPosition()==SCALE_VALUES.length-1;
            customScale.setVisibility(custom?View.VISIBLE:View.GONE);
            int denominator=readDenominator(scale,customScale,physicalScale);
            AreaMode mode=areaMode(area.getSelectedItemPosition());
            RectF source=sourceFor(mode,extents,display,window);
            PrintAttributes.MediaSize media=mediaFor(paper.getSelectedItemPosition(),orientation.getSelectedItemPosition());
            boolean mono=color.getSelectedItemPosition()==1;

            String scaleText=denominator>0?"1:"+denominator:"Sayfaya sığdır";
            String unit=drawing!=null&&drawing.hasPhysicalUnits()?drawing.drawingUnitName():"birimsiz / raster";
            if(mode==AreaMode.WINDOW&&source==null){
                info.setText("Window • Önce YAZDIR düğmesine basıp çizim üzerinde alan seçin. • Ölçek: "+scaleText);
                replacePreview(preview,previewBitmap,placeholderPreview(activity,media,"WINDOW","Çizim üzerinde alan seçin"));
                return;
            }
            if(custom&&denominator<=0){
                info.setText("Özel ölçek için pozitif bir payda girin. Örnek: 125 veya 1:125");
                replacePreview(preview,previewBitmap,placeholderPreview(activity,media,"ÖZEL ÖLÇEK","Geçerli ölçek girin"));
                return;
            }

            final Spec spec=new Spec(mode,source,denominator,mono,media);
            info.setText(areaLabel(mode)+" • "+PAPER_LABELS[Math.max(0,Math.min(PAPER_LABELS.length-1,paper.getSelectedItemPosition()))]+" • "+
                (orientation.getSelectedItemPosition()==0?"Yatay":"Dikey")+" • "+scaleText+" • Birim: "+unit+" • Önizleme hazırlanıyor…");
            replacePreview(preview,previewBitmap,placeholderPreview(activity,media,"ÖNİZLEME","Hazırlanıyor…"));

            Runnable kickoff=()->previewExecutor.submit(()->{
                if(closed[0]||generation!=previewGeneration[0])return;
                Bitmap rendered=null;String problem=null;
                try{rendered=renderPreview(activity,drawing,editCopy,replacementCopy,hiddenCopy,blockCopy,imageCopy,bitmap,spec,420,560);}
                catch(Exception e){problem=safeMessage(e);}
                final Bitmap page=rendered;final String error=problem;
                mainHandler.post(()->{
                    if(closed[0]||generation!=previewGeneration[0]){
                        if(page!=null&&!page.isRecycled())page.recycle();
                        return;
                    }
                    if(page!=null)replacePreview(preview,previewBitmap,page);
                    else replacePreview(preview,previewBitmap,placeholderPreview(activity,spec.media,"ÖNİZLEME","Hazırlanamadı"));
                    if(error!=null)info.setText("Print Preview hazırlanamadı: "+error);
                    else info.setText(areaLabel(spec.areaMode)+" • "+
                        PAPER_LABELS[Math.max(0,Math.min(PAPER_LABELS.length-1,paper.getSelectedItemPosition()))]+" • "+
                        (orientation.getSelectedItemPosition()==0?"Yatay":"Dikey")+" • "+scaleText+" • Birim: "+unit);
                });
            });
            pendingPreview[0]=kickoff;
            mainHandler.postDelayed(kickoff,100L);
        };

        AdapterView.OnItemSelectedListener changed=new AdapterView.OnItemSelectedListener(){
            public void onItemSelected(AdapterView<?> parent,View view,int position,long id){refresh[0].run();}
            public void onNothingSelected(AdapterView<?> parent){}
        };
        area.setOnItemSelectedListener(changed);paper.setOnItemSelectedListener(changed);orientation.setOnItemSelectedListener(changed);scale.setOnItemSelectedListener(changed);color.setOnItemSelectedListener(changed);
        customScale.addTextChangedListener(new android.text.TextWatcher(){
            public void beforeTextChanged(CharSequence s,int start,int count,int after){}
            public void onTextChanged(CharSequence s,int start,int before,int count){}
            public void afterTextChanged(android.text.Editable s){if(customScale.getVisibility()==View.VISIBLE)refresh[0].run();}
        });

        AlertDialog dialog=new AlertDialog.Builder(activity)
            .setTitle("Gelişmiş Yazdırma")
            .setView(scroll)
            .setPositiveButton("YAZDIR",null)
            .setNeutralButton("ÖNİZLEME",null)
            .setNegativeButton("İPTAL",null)
            .create();

        dialog.setOnShowListener(d->{
            fitDialogToPhone(activity,dialog);
            styleDialogButtons(activity,dialog);
            refresh[0].run();

            dialog.getButton(AlertDialog.BUTTON_POSITIVE).setOnClickListener(v->{
                AreaMode mode=areaMode(area.getSelectedItemPosition());
                if(mode==AreaMode.WINDOW&&window==null){
                    dialog.dismiss();
                    if(requestWindowSelection!=null)requestWindowSelection.run();
                    else Toast.makeText(activity,"Window için önce çizim üzerinde alan seçin",Toast.LENGTH_LONG).show();
                    return;
                }
                int denominator=readDenominator(scale,customScale,physicalScale);
                boolean custom=physicalScale&&scale.getSelectedItemPosition()==SCALE_VALUES.length-1;
                if(custom&&denominator<=0){customScale.setError("Pozitif bir ölçek paydası girin");return;}
                RectF source=sourceFor(mode,extents,display,window);
                if(source==null){Toast.makeText(activity,"Yazdırma alanı hazırlanamadı",Toast.LENGTH_LONG).show();return;}
                boolean monochrome=color.getSelectedItemPosition()==1;
                PrintAttributes.MediaSize media=mediaFor(paper.getSelectedItemPosition(),orientation.getSelectedItemPosition());
                Spec spec=new Spec(mode,source,denominator,monochrome,media);
                PrintAttributes attributes=attributesFor(spec);
                PrintManager manager=(PrintManager)activity.getSystemService(Context.PRINT_SERVICE);
                if(manager==null){Toast.makeText(activity,"Android yazdırma hizmeti bulunamadı",Toast.LENGTH_LONG).show();return;}
                handedOff[0]=true;String base=cleanName(displayName);
                manager.print("MusaCAD - "+base,new Adapter(activity,drawing,editCopy,replacementCopy,hiddenCopy,blockCopy,imageCopy,bitmap,base,spec),attributes);
                dialog.dismiss();
            });

            dialog.getButton(AlertDialog.BUTTON_NEUTRAL).setOnClickListener(v->{
                AreaMode mode=areaMode(area.getSelectedItemPosition());RectF source=sourceFor(mode,extents,display,window);
                if(mode==AreaMode.WINDOW&&source==null){Toast.makeText(activity,"Önizleme için önce Window alanını seçin",Toast.LENGTH_SHORT).show();return;}
                int denominator=readDenominator(scale,customScale,physicalScale);boolean custom=physicalScale&&scale.getSelectedItemPosition()==SCALE_VALUES.length-1;
                if(custom&&denominator<=0){customScale.setError("Pozitif bir ölçek paydası girin");return;}
                Spec spec=new Spec(mode,source,denominator,color.getSelectedItemPosition()==1,mediaFor(paper.getSelectedItemPosition(),orientation.getSelectedItemPosition()));
                if(pendingPreview[0]!=null){mainHandler.removeCallbacks(pendingPreview[0]);pendingPreview[0]=null;}
                final int generation=++previewGeneration[0];
                Button previewButton=dialog.getButton(AlertDialog.BUTTON_NEUTRAL);
                previewButton.setEnabled(false);previewButton.setText("HAZIRLANIYOR…");
                previewExecutor.submit(()->{
                    Bitmap page=null;String problem=null;
                    try{page=renderPreview(activity,drawing,editCopy,replacementCopy,hiddenCopy,blockCopy,imageCopy,bitmap,spec,900,1200);}
                    catch(Exception e){problem=safeMessage(e);}
                    final Bitmap rendered=page;final String error=problem;
                    mainHandler.post(()->{
                        if(closed[0]||generation!=previewGeneration[0]){
                            if(rendered!=null&&!rendered.isRecycled())rendered.recycle();
                            return;
                        }
                        previewButton.setEnabled(true);previewButton.setText("ÖNİZLEME");
                        if(rendered!=null)showLargePreview(activity,rendered,spec);
                        else Toast.makeText(activity,"Print Preview hazırlanamadı: "+(error==null?"Bilinmeyen hata":error),Toast.LENGTH_LONG).show();
                    });
                });
            });
        });

        dialog.setOnDismissListener(d->{
            closed[0]=true;++previewGeneration[0];
            if(pendingPreview[0]!=null)mainHandler.removeCallbacks(pendingPreview[0]);
            preview.setImageDrawable(null);
            if(previewBitmap[0]!=null&&!previewBitmap[0].isRecycled())previewBitmap[0].recycle();
            previewBitmap[0]=null;
            if(!handedOff[0]&&bitmap!=null&&!bitmap.isRecycled()){
                try{previewExecutor.submit(()->{if(!bitmap.isRecycled())bitmap.recycle();});}catch(RejectedExecutionException ignored){}
            }
            previewExecutor.shutdown();
        });
        dialog.show();
    }

    private static void showLargePreview(Activity activity,Bitmap page,Spec spec){
        if(page==null||page.isRecycled())return;
        ImageView image=new ImageView(activity);image.setImageBitmap(page);image.setAdjustViewBounds(true);image.setScaleType(ImageView.ScaleType.FIT_CENTER);image.setBackgroundColor(Color.rgb(45,48,50));
        ScrollView scroll=new ScrollView(activity);scroll.addView(image,new ScrollView.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT,ViewGroup.LayoutParams.WRAP_CONTENT));
        AlertDialog dialog=new AlertDialog.Builder(activity).setTitle("Print Preview • "+areaLabel(spec.areaMode)).setView(scroll).setPositiveButton("KAPAT",null).create();
        dialog.setOnShowListener(d->{fitDialogToPhone(activity,dialog);styleDialogButtons(activity,dialog);});
        dialog.setOnDismissListener(d->{image.setImageDrawable(null);if(!page.isRecycled())page.recycle();});dialog.show();
    }

    private static void styleDialogButtons(Activity activity,AlertDialog dialog){
        if(dialog==null)return;
        int[] ids={AlertDialog.BUTTON_POSITIVE,AlertDialog.BUTTON_NEUTRAL,AlertDialog.BUTTON_NEGATIVE};
        for(int id:ids){
            Button button=dialog.getButton(id);
            if(button==null)continue;
            button.setTextColor(Color.WHITE);
            button.setTextSize(12f);
            button.setMinHeight(dp(activity,48));
            button.setSingleLine(false);
            button.setMaxLines(2);
            button.setEllipsize(null);
        }
    }

    private static void fitDialogToPhone(Activity activity,AlertDialog dialog){
        Window window=dialog==null?null:dialog.getWindow();if(window==null)return;
        android.util.DisplayMetrics dm=activity.getResources().getDisplayMetrics();
        int width=Math.max(dp(activity,280),dm.widthPixels-dp(activity,16));
        int height=Math.max(dp(activity,360),Math.min(dm.heightPixels-dp(activity,24),Math.round(dm.heightPixels*.92f)));
        window.setLayout(width,height);
    }

    private static PrintAttributes attributesFor(Spec spec){
        return new PrintAttributes.Builder()
            .setMediaSize(spec.media)
            .setMinMargins(new PrintAttributes.Margins(MARGIN_MILS,MARGIN_MILS,MARGIN_MILS,MARGIN_MILS))
            .setColorMode(spec.monochrome?PrintAttributes.COLOR_MODE_MONOCHROME:PrintAttributes.COLOR_MODE_COLOR)
            .build();
    }

    private static Bitmap renderPreview(Activity activity,DxfParser.Result drawing,List<CadEdit> additions,List<SourceReplacement> replacements,
                                        Set<Integer> hidden,Map<String,CadBlock.Definition> blocks,List<CadImageOverlay> images,Bitmap bitmap,
                                        Spec spec,int maxWidth,int maxHeight){
        float pageW=Math.max(1f,spec.media.getWidthMils()*72f/1000f),pageH=Math.max(1f,spec.media.getHeightMils()*72f/1000f);
        float factor=Math.min(maxWidth/pageW,maxHeight/pageH);factor=Math.max(.08f,Math.min(1.6f,factor));
        int width=Math.max(1,Math.round(pageW*factor)),height=Math.max(1,Math.round(pageH*factor));
        Bitmap out=Bitmap.createBitmap(width,height,Bitmap.Config.ARGB_8888);Canvas canvas=new Canvas(out);canvas.drawColor(Color.rgb(48,52,55));
        int save=canvas.save();canvas.scale(factor,factor);
        float margin=MARGIN_MILS*72f/1000f;RectF content=new RectF(margin,margin,pageW-margin,pageH-margin);
        renderPage(canvas,content,drawing,additions,replacements,hidden,blocks,images,bitmap,spec);
        canvas.restoreToCount(save);
        return out;
    }

    private static Bitmap placeholderPreview(Activity activity,PrintAttributes.MediaSize media,String title,String message){
        float pageW=Math.max(1f,media.getWidthMils()*72f/1000f),pageH=Math.max(1f,media.getHeightMils()*72f/1000f),factor=Math.min(520f/pageW,720f/pageH);
        int width=Math.max(1,Math.round(pageW*factor)),height=Math.max(1,Math.round(pageH*factor));
        Bitmap out=Bitmap.createBitmap(width,height,Bitmap.Config.ARGB_8888);Canvas c=new Canvas(out);c.drawColor(Color.WHITE);
        Paint p=new Paint(Paint.ANTI_ALIAS_FLAG);p.setColor(Color.DKGRAY);p.setTextAlign(Paint.Align.CENTER);p.setTextSize(Math.max(16f,24f*factor));
        c.drawText(title,width/2f,height*.46f,p);p.setTextSize(Math.max(12f,16f*factor));c.drawText(message,width/2f,height*.54f,p);return out;
    }

    private static void replacePreview(ImageView view,Bitmap[] holder,Bitmap next){
        Bitmap old=holder[0];holder[0]=next;view.setImageBitmap(next);if(old!=null&&old!=next&&!old.isRecycled())old.recycle();
    }

    private static int readDenominator(Spinner scale,EditText custom,boolean physical){
        if(!physical)return 0;int index=scale.getSelectedItemPosition();
        if(index<0||index>=SCALE_VALUES.length)return 0;
        int value=SCALE_VALUES[index];return value==-1?CadPrintMath.parseScaleDenominator(custom.getText().toString()):value;
    }

    private static AreaMode areaMode(int position){return position==1?AreaMode.DISPLAY:position==2?AreaMode.WINDOW:AreaMode.EXTENTS;}
    private static String areaLabel(AreaMode mode){return mode==AreaMode.DISPLAY?"Display":mode==AreaMode.WINDOW?"Window":"Extents";}
    private static RectF sourceFor(AreaMode mode,RectF extents,RectF display,RectF window){
        RectF source=mode==AreaMode.DISPLAY?display:mode==AreaMode.WINDOW?window:extents;return validBounds(source)?new RectF(source):null;
    }

    private static PrintAttributes.MediaSize mediaFor(int paper,int orientation){
        PrintAttributes.MediaSize base=PAPERS[Math.max(0,Math.min(PAPERS.length-1,paper))];
        return orientation==0?base.asLandscape():base.asPortrait();
    }

    private static Spinner spinner(Activity activity,LinearLayout box,String title,String[] values){
        TextView label=new TextView(activity);label.setText(title);label.setPadding(0,10,0,2);box.addView(label);
        Spinner spinner=new Spinner(activity);spinner.setAdapter(new ArrayAdapter<>(activity,android.R.layout.simple_spinner_dropdown_item,values));box.addView(spinner);return spinner;
    }

    private static int dp(Activity activity,int value){return Math.round(value*activity.getResources().getDisplayMetrics().density);}
    private static String cleanName(String name){String base=name==null?"cizim":name.replaceFirst("(?i)\\.(dwg|dxf)$","");base=base.replaceAll("[^\\p{L}\\p{N}._ -]+","_").trim();return base.isEmpty()?"cizim":base;}
    private static String safeMessage(Throwable e){return e==null||e.getMessage()==null||e.getMessage().trim().isEmpty()?"Bilinmeyen hata":e.getMessage();}

    private static final class Adapter extends PrintDocumentAdapter {
        private final Context context;
        private final DxfParser.Result drawing;
        private final List<CadEdit> additions;
        private final List<SourceReplacement> replacements;
        private final Set<Integer> hidden;
        private final Map<String,CadBlock.Definition> blocks;
        private final List<CadImageOverlay> images;
        private final Bitmap bitmap;
        private final String name;
        private final Spec spec;
        private PrintAttributes attributes;

        Adapter(Context context,DxfParser.Result drawing,List<CadEdit> additions,List<SourceReplacement> replacements,Set<Integer> hidden,
                Map<String,CadBlock.Definition> blocks,List<CadImageOverlay> images,Bitmap bitmap,String name,Spec spec){
            this.context=context;this.drawing=drawing;this.additions=additions;this.replacements=replacements;this.hidden=hidden;this.blocks=blocks;this.images=images;this.bitmap=bitmap;this.name=name;this.spec=spec;
        }

        @Override public void onLayout(PrintAttributes oldAttributes,PrintAttributes newAttributes,CancellationSignal signal,LayoutResultCallback callback,Bundle extras){
            attributes=newAttributes;if(signal.isCanceled()){callback.onLayoutCancelled();return;}
            PrintDocumentInfo info=new PrintDocumentInfo.Builder(name+".pdf").setContentType(PrintDocumentInfo.CONTENT_TYPE_DOCUMENT).setPageCount(1).build();
            callback.onLayoutFinished(info,oldAttributes==null||!newAttributes.equals(oldAttributes));
        }

        @Override public void onWrite(PageRange[] pages,ParcelFileDescriptor destination,CancellationSignal signal,WriteResultCallback callback){
            if(signal.isCanceled()){callback.onWriteCancelled();return;}if(!containsPage(pages,0)){callback.onWriteFinished(new PageRange[0]);return;}
            PrintedPdfDocument document=null;
            try{
                if(attributes==null)throw new IOException("Yazdırma özellikleri hazırlanamadı");
                document=new PrintedPdfDocument(context,attributes);PdfDocument.Page page=document.startPage(0);
                Rect content=document.getPageContentRect();Spec actual=new Spec(spec.areaMode,spec.sourceBounds,spec.denominator,spec.monochrome||attributes.getColorMode()==PrintAttributes.COLOR_MODE_MONOCHROME,attributes.getMediaSize()==null?spec.media:attributes.getMediaSize());
                renderPage(page.getCanvas(),new RectF(content),drawing,additions,replacements,hidden,blocks,images,bitmap,actual);
                document.finishPage(page);
                if(signal.isCanceled()){callback.onWriteCancelled();return;}
                try(FileOutputStream out=new FileOutputStream(destination.getFileDescriptor())){document.writeTo(out);out.flush();}
                callback.onWriteFinished(new PageRange[]{PageRange.ALL_PAGES});
            }catch(Exception e){callback.onWriteFailed(e.getMessage()==null?"Yazdırma belgesi oluşturulamadı":e.getMessage());}
            finally{if(document!=null)document.close();}
        }

        @Override public void onFinish(){if(bitmap!=null&&!bitmap.isRecycled())bitmap.recycle();}
    }

    /** Single WYSIWYG renderer shared by Print Preview, physical printing and system PDF output. */
    private static void renderPage(Canvas canvas,RectF target,DxfParser.Result drawing,List<CadEdit> additions,List<SourceReplacement> replacements,
                                   Set<Integer> hidden,Map<String,CadBlock.Definition> blocks,List<CadImageOverlay> images,Bitmap bitmap,Spec spec){
        canvas.drawColor(Color.WHITE);if(target==null||target.width()<=0f||target.height()<=0f)return;
        int save=canvas.save();canvas.clipRect(target);
        if(drawing!=null){
            Matrix matrix=drawing.printMatrix(target,spec.denominator,spec.sourceBounds);
            drawing.drawVectorForPrint(canvas,matrix,spec.monochrome,hidden);
            drawImageOverlays(canvas,images,matrix,spec.monochrome);
            drawEdits(canvas,additions,matrix,spec.monochrome,blocks);
            if(replacements!=null)for(SourceReplacement replacement:replacements)if(replacement!=null)drawing.drawSourceReplacement(canvas,matrix,replacement,true,spec.monochrome);
        }else renderBitmap(canvas,bitmap,target,spec.monochrome);
        canvas.restoreToCount(save);
    }

    private static void drawImageOverlays(Canvas canvas,List<CadImageOverlay> images,Matrix matrix,boolean monochrome){
        if(images==null||images.isEmpty())return;
        Paint paint=new Paint(Paint.ANTI_ALIAS_FLAG|Paint.FILTER_BITMAP_FLAG);
        if(monochrome){ColorMatrix cm=new ColorMatrix();cm.setSaturation(0f);paint.setColorFilter(new ColorMatrixColorFilter(cm));}
        for(CadImageOverlay image:images){
            if(image==null||image.bitmap==null||image.bitmap.isRecycled())continue;
            float[] center={image.centerX(),image.centerY()};matrix.mapPoints(center);
            float[] vectors={image.width(),0f,0f,image.height()};matrix.mapVectors(vectors);
            float w=(float)Math.hypot(vectors[0],vectors[1]),h=(float)Math.hypot(vectors[2],vectors[3]);
            if(w<=0f||h<=0f)continue;
            int save=canvas.save();canvas.rotate(image.rotationDegrees()+matrixRotation(matrix),center[0],center[1]);
            canvas.drawBitmap(image.bitmap,null,new RectF(center[0]-w*.5f,center[1]-h*.5f,center[0]+w*.5f,center[1]+h*.5f),paint);canvas.restoreToCount(save);
        }
    }

    private static void renderBitmap(Canvas canvas,Bitmap bitmap,RectF target,boolean monochrome){
        if(bitmap==null||bitmap.isRecycled())return;float scale=Math.min(target.width()/bitmap.getWidth(),target.height()/bitmap.getHeight());
        float w=bitmap.getWidth()*scale,h=bitmap.getHeight()*scale;RectF dst=new RectF(target.centerX()-w/2f,target.centerY()-h/2f,target.centerX()+w/2f,target.centerY()+h/2f);
        Paint paint=new Paint(Paint.ANTI_ALIAS_FLAG|Paint.FILTER_BITMAP_FLAG);
        if(monochrome){ColorMatrix cm=new ColorMatrix();cm.setSaturation(0f);paint.setColorFilter(new ColorMatrixColorFilter(cm));}
        canvas.drawBitmap(bitmap,null,dst,paint);
    }

    private static void drawEdits(Canvas canvas,List<CadEdit> edits,Matrix matrix,boolean monochrome,Map<String,CadBlock.Definition> blocks){
        if(edits==null||edits.isEmpty())return;
        int color=monochrome?Color.BLACK:Color.rgb(255,193,7);
        Paint paint=new Paint(Paint.ANTI_ALIAS_FLAG);paint.setColor(color);paint.setStrokeWidth(.9f);paint.setStyle(Paint.Style.STROKE);
        for(CadEdit edit:edits)drawEdit(canvas,edit,matrix,paint,blocks,0);
    }

    private static void drawEdit(Canvas canvas,CadEdit edit,Matrix matrix,Paint paint,Map<String,CadBlock.Definition> blocks,int depth){
        if(edit==null||depth>8)return;float[] v=edit.xy.clone();matrix.mapPoints(v);
        switch(edit.type){
            case LINE:canvas.drawLine(v[0],v[1],v[2],v[3],paint);break;
            case RECTANGLE:canvas.drawRect(Math.min(v[0],v[2]),Math.min(v[1],v[3]),Math.max(v[0],v[2]),Math.max(v[1],v[3]),paint);break;
            case CIRCLE:canvas.drawCircle(v[0],v[1],(float)Math.hypot(v[2]-v[0],v[3]-v[1]),paint);break;
            case ARC:{
                if(v.length<8)break;float r=(float)Math.hypot(v[0]-v[6],v[1]-v[7]);float start=angle(v[0]-v[6],v[1]-v[7]),mid=angle(v[2]-v[6],v[3]-v[7]),end=angle(v[4]-v[6],v[5]-v[7]),sweep=arcSweep(start,mid,end);
                canvas.drawArc(new RectF(v[6]-r,v[7]-r,v[6]+r,v[7]+r),start,sweep,false,paint);break;
            }
            case ELLIPSE:{
                if(v.length<6)break;float a=(float)Math.hypot(v[2]-v[0],v[3]-v[1]),b=(float)Math.hypot(v[4]-v[0],v[5]-v[1]),rot=(float)Math.toDegrees(Math.atan2(v[3]-v[1],v[2]-v[0]));
                int save=canvas.save();canvas.rotate(rot,v[0],v[1]);canvas.drawOval(new RectF(v[0]-a,v[1]-b,v[0]+a,v[1]+b),paint);canvas.restoreToCount(save);break;
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
                if(v.length<2)break;
                Paint.Style oldStyle=paint.getStyle();float oldSize=paint.getTextSize(),oldScaleX=paint.getTextScaleX(),oldSkew=paint.getTextSkewX();Typeface oldTypeface=paint.getTypeface();
                paint.setStyle(Paint.Style.FILL);float size=edit.hasTextStyle()?Math.max(3f,edit.textHeight*matrixScale(matrix)):9f;paint.setTextSize(size);
                paint.setTypeface(CadFontManager.resolveTypeface(edit.textFamilyHint,edit.textShx,Typeface.NORMAL));
                paint.setTextScaleX(edit.textWidthFactor);paint.setTextSkewX((float)-Math.tan(Math.toRadians(edit.textOblique)));
                int save=canvas.save();canvas.rotate(edit.rotationDegrees,v[0],v[1]);canvas.drawText(edit.text==null?"":edit.text,v[0],v[1],paint);canvas.restoreToCount(save);
                paint.setTypeface(oldTypeface);paint.setTextSkewX(oldSkew);paint.setTextScaleX(oldScaleX);paint.setTextSize(oldSize);paint.setStyle(oldStyle);break;
            }
        }
    }

    private static void drawHatch(Canvas canvas,CadEdit edit,float[] v,Paint paint,float scale){
        if(v.length<6)return;Path path=new Path();path.moveTo(v[0],v[1]);for(int i=2;i+1<v.length;i+=2)path.lineTo(v[i],v[i+1]);path.close();
        Paint.Style old=paint.getStyle();int oldAlpha=paint.getAlpha();
        if("SOLID".equalsIgnoreCase(edit.text)){paint.setStyle(Paint.Style.FILL);paint.setAlpha(80);canvas.drawPath(path,paint);}
        else{
            RectF b=new RectF();path.computeBounds(b,true);int save=canvas.save();canvas.clipPath(path);float cx=b.centerX(),cy=b.centerY(),diag=(float)Math.hypot(b.width(),b.height())*1.5f,spacing=Math.max(4f,edit.textHeight*Math.max(.001f,scale));
            canvas.rotate(45f+edit.rotationDegrees,cx,cy);paint.setStyle(Paint.Style.STROKE);paint.setAlpha(180);
            for(float y=cy-diag;y<=cy+diag;y+=spacing)canvas.drawLine(cx-diag,y,cx+diag,y,paint);canvas.restoreToCount(save);
        }
        paint.setStyle(Paint.Style.STROKE);paint.setAlpha(oldAlpha);canvas.drawPath(path,paint);paint.setStyle(old);
    }

    private static RectF contentExtents(DxfParser.Result drawing,List<CadEdit> additions,List<SourceReplacement> replacements,
                                        Map<String,CadBlock.Definition> blocks,List<CadImageOverlay> images){
        RectF out=drawing.printableContentBounds();
        if(additions!=null)for(CadEdit edit:additions)includeEditBounds(out,edit,blocks,0);
        if(replacements!=null)for(SourceReplacement replacement:replacements)if(replacement!=null)includeEditBounds(out,replacement.edit,blocks,0);
        if(images!=null)for(CadImageOverlay image:images)if(image!=null)out.union(image.axisAlignedBounds());
        if(out.width()<=0f){out.left-=.5f;out.right+=.5f;}if(out.height()<=0f){out.top-=.5f;out.bottom+=.5f;}
        return out;
    }

    private static void includeEditBounds(RectF out,CadEdit edit,Map<String,CadBlock.Definition> blocks,int depth){
        if(out==null||edit==null||depth>8)return;
        if(edit.type==CadEdit.Type.INSERT&&edit.text!=null&&blocks!=null){
            CadBlock.Definition def=blocks.get(edit.text.toUpperCase(Locale.ROOT));
            if(def!=null){for(CadEdit member:def.members){CadEdit placed=member.scaled(edit.insertScale(),0f,0f).rotated(edit.rotationDegrees,0f,0f).translated(edit.xy[0],edit.xy[1]);includeEditBounds(out,placed,blocks,depth+1);}return;}
        }
        float l=edit.minX(),t=edit.minY(),r=edit.maxX(),b=edit.maxY();
        if(edit.type==CadEdit.Type.CIRCLE&&edit.xy.length>=4){float radius=(float)Math.hypot(edit.xy[2]-edit.xy[0],edit.xy[3]-edit.xy[1]);l=edit.xy[0]-radius;r=edit.xy[0]+radius;t=edit.xy[1]-radius;b=edit.xy[1]+radius;}
        else if(edit.type==CadEdit.Type.ELLIPSE&&edit.xy.length>=6){float a=(float)Math.hypot(edit.xy[2]-edit.xy[0],edit.xy[3]-edit.xy[1]),bb=(float)Math.hypot(edit.xy[4]-edit.xy[0],edit.xy[5]-edit.xy[1]),radius=Math.max(a,bb);l=edit.xy[0]-radius;r=edit.xy[0]+radius;t=edit.xy[1]-radius;b=edit.xy[1]+radius;}
        else if(edit.type==CadEdit.Type.TEXT&&edit.xy.length>=2){float h=edit.hasTextStyle()?edit.textHeight:18f,w=Math.max(h,(edit.text==null?1:edit.text.length())*h*.62f);l=edit.xy[0];r=edit.xy[0]+w;t=edit.xy[1]-h;b=edit.xy[1]+h*.25f;}
        if(Float.isFinite(l)&&Float.isFinite(t)&&Float.isFinite(r)&&Float.isFinite(b))out.union(Math.min(l,r),Math.min(t,b),Math.max(l,r),Math.max(t,b));
    }

    private static boolean validBounds(RectF b){return b!=null&&Float.isFinite(b.left)&&Float.isFinite(b.top)&&Float.isFinite(b.right)&&Float.isFinite(b.bottom)&&b.width()>0f&&b.height()>0f;}
    private static boolean containsPage(PageRange[] ranges,int page){if(ranges==null)return false;for(PageRange range:ranges)if(page>=range.getStart()&&page<=range.getEnd())return true;return false;}
    private static float matrixScale(Matrix matrix){float[] v={0f,0f,1f,0f};matrix.mapPoints(v);return (float)Math.hypot(v[2]-v[0],v[3]-v[1]);}
    private static float matrixRotation(Matrix matrix){float[] v={0f,0f,1f,0f};matrix.mapPoints(v);return (float)Math.toDegrees(Math.atan2(v[3]-v[1],v[2]-v[0]));}
    private static float angle(float x,float y){float a=(float)Math.toDegrees(Math.atan2(y,x));return a<0f?a+360f:a;}
    private static float ccw(float from,float to){float d=to-from;while(d<0f)d+=360f;while(d>=360f)d-=360f;return d;}
    private static float arcSweep(float start,float mid,float end){float se=ccw(start,end),sm=ccw(start,mid);return sm<=se+1e-4f?se:-(360f-se);}

    private CadPrint(){}
}
