package com.musa.cad;

import android.app.Activity;
import android.os.Handler;
import android.os.Looper;
import java.util.concurrent.atomic.AtomicBoolean;
import android.graphics.Color;
import android.graphics.Typeface;
import android.graphics.drawable.GradientDrawable;
import android.view.Gravity;
import android.view.View;
import android.view.ViewGroup;
import android.view.WindowManager;
import android.view.inputmethod.EditorInfo;
import android.text.util.Linkify;
import android.text.method.LinkMovementMethod;
import android.widget.Button;
import android.widget.EditText;
import android.widget.HorizontalScrollView;
import android.widget.LinearLayout;
import android.widget.ScrollView;
import android.widget.TextView;
import androidx.core.widget.TextViewCompat;
import com.google.android.material.bottomsheet.BottomSheetDialog;

/**
 * Shared MusaCAD AI conversation surface.
 *
 * The Host callback is deliberately asynchronous so local CAD tools, on-device
 * models and an optional cloud model can all use the same UI without changing
 * the editor screen again.
 */
public final class MusaAiPanel {
    public interface Reply {
        void send(String text);
        default void progress(String text){}
    }

    public interface Host {
        String contextLabel();
        void onPrompt(String prompt,Reply reply);
    }

    public static void show(Activity activity,Host host){
        if(activity==null||host==null)return;

        BottomSheetDialog sheet=new BottomSheetDialog(activity);
        LinearLayout root=new LinearLayout(activity);
        root.setOrientation(LinearLayout.VERTICAL);
        int pad=dp(activity,14);
        root.setPadding(pad,dp(activity,10),pad,dp(activity,12));
        root.setBackgroundColor(Color.rgb(5,21,32));

        LinearLayout header=new LinearLayout(activity);
        header.setOrientation(LinearLayout.HORIZONTAL);
        header.setGravity(Gravity.CENTER_VERTICAL);

        boolean developer=MusaAiSessionService.developerCached();
        TextView title=text(activity,developer?"Gandalf • Developer":"Gandalf • MusaCAD AI",20f,Color.WHITE,true);
        header.addView(title,new LinearLayout.LayoutParams(0,ViewGroup.LayoutParams.WRAP_CONTENT,1f));

        TextView state=text(activity,developer?"DEV":"AI",10f,0xFFBFFAF4,true);
        state.setGravity(Gravity.CENTER);
        state.setPadding(dp(activity,10),dp(activity,5),dp(activity,10),dp(activity,5));
        state.setBackground(round(activity,0xFF0C594F,16,0xFF16B8A6));
        header.addView(state,new LinearLayout.LayoutParams(ViewGroup.LayoutParams.WRAP_CONTENT,ViewGroup.LayoutParams.WRAP_CONTENT));
        root.addView(header,new LinearLayout.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT,ViewGroup.LayoutParams.WRAP_CONTENT));

        TextView context=text(activity,host.contextLabel(),11f,0xFF9FC1D1,false);
        context.setPadding(0,dp(activity,3),0,dp(activity,8));
        context.setMaxLines(2);
        root.addView(context,new LinearLayout.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT,ViewGroup.LayoutParams.WRAP_CONTENT));

        HorizontalScrollView quickScroll=new HorizontalScrollView(activity);
        quickScroll.setHorizontalScrollBarEnabled(false);
        LinearLayout quickRow=new LinearLayout(activity);
        quickRow.setOrientation(LinearLayout.HORIZONTAL);
        quickScroll.addView(quickRow,new HorizontalScrollView.LayoutParams(ViewGroup.LayoutParams.WRAP_CONTENT,ViewGroup.LayoutParams.WRAP_CONTENT));
        root.addView(quickScroll,new LinearLayout.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT,ViewGroup.LayoutParams.WRAP_CONTENT));

        ScrollView messagesScroll=new ScrollView(activity);
        messagesScroll.setFillViewport(true);
        LinearLayout messages=new LinearLayout(activity);
        messages.setOrientation(LinearLayout.VERTICAL);
        messages.setPadding(0,dp(activity,8),0,dp(activity,8));
        messagesScroll.addView(messages,new ScrollView.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT,ViewGroup.LayoutParams.WRAP_CONTENT));
        LinearLayout.LayoutParams messagesLp=new LinearLayout.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT,0,1f);
        messagesLp.topMargin=dp(activity,4);
        root.addView(messagesScroll,messagesLp);

        appendBubble(activity,messages,false,
            developer
                ?"Gandalf Developer modu aktif. Yerel MusaCAD AI özelliklerinin yanında tam bulut analiz yüzeyi kullanılabilir. Çizim değişiklikleri yine önizleme ve açık kullanıcı onayı olmadan uygulanmaz."
                :"Merhaba. Ben MusaCAD AI. Yerel mod çevrimdışı çizim soruları, metraj ve proje kontrolünü yapar. Gandalf Cloud AI ise kullanıcı onayıyla sınırlı CAD-JSON bağlamını kullanarak daha derin mühendislik analizi ve güncel kaynak araştırması yapabilir.");

        LinearLayout composer=new LinearLayout(activity);
        composer.setOrientation(LinearLayout.HORIZONTAL);
        composer.setGravity(Gravity.BOTTOM);

        EditText input=new EditText(activity);
        input.setSingleLine(false);
        input.setMaxLines(4);
        input.setMinHeight(dp(activity,48));
        input.setHint("Gandalf'a yazın veya sesli komut verin…");
        input.setTextColor(Color.WHITE);
        input.setHintTextColor(0xFF718A98);
        input.setTextSize(13f);
        input.setPadding(dp(activity,12),dp(activity,7),dp(activity,12),dp(activity,7));
        input.setBackground(round(activity,0xFF0A2638,12,0xFF245D79));
        input.setImeOptions(EditorInfo.IME_ACTION_SEND);
        composer.addView(input,new LinearLayout.LayoutParams(0,ViewGroup.LayoutParams.WRAP_CONTENT,1f));

        Button voice=new Button(activity);
        voice.setText("Ses");
        voice.setAllCaps(false);
        voice.setTextColor(0xFFEAFBFF);
        voice.setTextSize(11f);
        voice.setMinHeight(dp(activity,48));
        voice.setPadding(dp(activity,6),0,dp(activity,6),0);
        voice.setBackground(round(activity,0xFF12384C,12,0xFF246C88));
        LinearLayout.LayoutParams voiceLp=new LinearLayout.LayoutParams(dp(activity,64),ViewGroup.LayoutParams.WRAP_CONTENT);
        voiceLp.setMarginStart(dp(activity,8));
        composer.addView(voice,voiceLp);

        Button send=new Button(activity);
        send.setText("Gönder");
        send.setAllCaps(false);
        send.setTextColor(Color.WHITE);
        send.setTextSize(11f);
        send.setMinHeight(dp(activity,48));
        send.setPadding(dp(activity,8),0,dp(activity,8),0);
        send.setBackground(round(activity,0xFF087E75,12,0xFF26D1C0));
        LinearLayout.LayoutParams sendLp=new LinearLayout.LayoutParams(dp(activity,82),ViewGroup.LayoutParams.WRAP_CONTENT);
        sendLp.setMarginStart(dp(activity,8));
        composer.addView(send,sendLp);
        root.addView(composer,new LinearLayout.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT,ViewGroup.LayoutParams.WRAP_CONTENT));

        String[][] prompts={
            {"Çizime sor","Bu çizimde neler var?"},
            {"Mühendislik Analizi","Projeyi mühendislik açısından analiz et"},
            {"Pis Su Kontrolü","Pis su tesisatındaki çapları ve hat metrajını analiz et"},
            {"Havalandırma","Havalandırma kanal kesitlerini analiz et"},
            {"Yangın Sistemi","Yangın pompası ve yangın tesisatı etiketlerini kontrol et"},
            {"Komut ver","Ekrana sığdır"},
            {"Metraj","Bu projede metraj çıkar"},
            {"Keşif","Keşif yükle"},
            {"Keşif Oluştur","Projeden keşif oluştur"},
            {"Kıyas","Keşif karşılaştır"},
            {"Mekanik","AI_MEKANIK_KONTROL"},
            {"Elektrik","Elektrik projesini kontrol et"},
            {"Statik","Statik teknik rapor oluştur"},
            {"Tam Denetim","Tam proje denetim raporu oluştur"},
            {"Uzmanlar","Disiplin AI yardım"},
            {"MEKAI","MEKAI_FULL"},
            {"STATIKAI","STATIKAI_FULL"},
            {"ELKAI","ELKAI_FULL"},
            {"Kontrol","Projeyi kontrol et"},
            {"Rapor","Proje raporu oluştur"}
        };
        for(String[] item:prompts)addQuickPrompt(activity,quickRow,input,item[0],item[1]);
        addQuickPrompt(activity,quickRow,input,"G-MEKAI",
            "GMEKAI_FULL projeyi derin analiz et, hata raporu hazırla ve gerekli düzeltmeleri öner");
        addQuickPrompt(activity,quickRow,input,"G-STATİK",
            "GSTATIKAI_FULL statik projeyi derin analiz et, koordinasyon hatalarını ve eksik verileri raporla");

        Runnable submit=()->{
            String prompt=input.getText().toString().trim();
            if(prompt.isEmpty())return;
            input.setText("");
            appendBubble(activity,messages,true,prompt);
            TextView pending=appendBubble(activity,messages,false,"İşleniyor…");
            scrollBottom(messagesScroll);
            final AtomicBoolean completed=new AtomicBoolean(false);
            final Handler timeoutHandler=new Handler(Looper.getMainLooper());
            final long startedMs=android.os.SystemClock.elapsedRealtime();
            final long maxRequestMs=240_000L;
            final Runnable timeout=()->{
                if(!completed.compareAndSet(false,true))return;
                pending.setText("Gandalf isteği ilerleme veya toplam süre sınırına ulaştı. Çizime müdahale edilmedi. İsteği yeniden başlatabilir ya da yerel analiz kullanabilirsiniz.");
                scrollBottom(messagesScroll);
            };
            timeoutHandler.postDelayed(timeout,75_000L);
            Reply requestReply=new Reply(){
                @Override public void send(String text){
                    activity.runOnUiThread(()->{
                        if(!completed.compareAndSet(false,true))return;
                        timeoutHandler.removeCallbacks(timeout);
                        pending.setText(text==null||text.trim().isEmpty()?"Yanıt oluşturulamadı.":text.trim());
                        Linkify.addLinks(pending,Linkify.WEB_URLS);
                        pending.setMovementMethod(LinkMovementMethod.getInstance());
                        pending.setLinksClickable(true);
                        scrollBottom(messagesScroll);
                    });
                }
                @Override public void progress(String text){
                    activity.runOnUiThread(()->{
                        if(completed.get())return;
                        // A multi-region vision sweep has bounded provider calls; restart
                        // the idle watchdog only on actual progress, with a hard total cap.
                        long elapsed=android.os.SystemClock.elapsedRealtime()-startedMs;
                        timeoutHandler.removeCallbacks(timeout);
                        timeoutHandler.postDelayed(timeout,Math.max(1L,Math.min(75_000L,maxRequestMs-elapsed)));
                        if(text!=null&&!text.trim().isEmpty())pending.setText(text.trim());
                        scrollBottom(messagesScroll);
                    });
                }
            };
            try{host.onPrompt(prompt,requestReply);}
            catch(Exception e){requestReply.send("Gandalf komutu işlenirken hata oluştu. Tekrar deneyin.");}
        };

        // Gandalf is an action, not a passive text preset: one tap starts the
        // central agent immediately. Voice commands use the same host/router.
        addQuickPromptAuto(activity,quickRow,input,"Gandalf",
            "Bu projeyi tüm disiplinlerde kontrol et, önemli bulguları ve yapılacakları raporla",submit);
        MusaAiVoiceInput.Callback voiceCallback=new MusaAiVoiceInput.Callback(){
            @Override public void onText(String text){
                activity.runOnUiThread(()->{
                    input.setText(text);
                    input.setSelection(input.length());
                    appendBubble(activity,messages,false,"Ses algılandı • "+text);
                    scrollBottom(messagesScroll);
                    submit.run();
                });
            }
            @Override public void onStatus(String text){
                activity.runOnUiThread(()->{
                    appendBubble(activity,messages,false,text);
                    scrollBottom(messagesScroll);
                });
            }
        };
        voice.setOnClickListener(v->{
            appendBubble(activity,messages,false,
                "Gandalf dinliyor • Android konuşma tanıma hizmeti açılıyor. MusaCAD ses kaydı saklamaz.");
            scrollBottom(messagesScroll);
            MusaAiVoiceInput.launch(activity,voiceCallback);
        });
        send.setOnClickListener(v->submit.run());
        input.setOnEditorActionListener((v,actionId,event)->{
            if(actionId==EditorInfo.IME_ACTION_SEND){submit.run();return true;}
            return false;
        });

        sheet.setContentView(root);
        sheet.setOnShowListener(d->{
            View bottom=sheet.findViewById(com.google.android.material.R.id.design_bottom_sheet);
            if(bottom!=null){
                bottom.getLayoutParams().height=Math.min(
                    Math.round(activity.getResources().getDisplayMetrics().heightPixels*.82f),
                    dp(activity,720)
                );
                bottom.requestLayout();
            }
            if(sheet.getWindow()!=null)sheet.getWindow().setSoftInputMode(WindowManager.LayoutParams.SOFT_INPUT_ADJUST_RESIZE);
        });
        sheet.setOnDismissListener(d->MusaAiVoiceInput.clear(voiceCallback));
        sheet.show();
    }

    private static void addQuickPrompt(Activity activity,LinearLayout row,EditText input,String label,String prompt){
        Button chip=new Button(activity);
        chip.setText(label);
        chip.setAllCaps(false);
        chip.setTextColor(0xFFEAFBFF);
        chip.setTextSize(10f);
        chip.setMinHeight(dp(activity,36));
        chip.setPadding(dp(activity,12),0,dp(activity,12),0);
        chip.setBackground(round(activity,0xFF12384C,18,0xFF246C88));
        LinearLayout.LayoutParams lp=new LinearLayout.LayoutParams(ViewGroup.LayoutParams.WRAP_CONTENT,dp(activity,36));
        lp.setMarginEnd(dp(activity,7));
        row.addView(chip,lp);
        chip.setOnClickListener(v->{input.setText(prompt);input.setSelection(input.length());input.requestFocus();});
    }

    private static void addQuickPromptAuto(Activity activity,LinearLayout row,EditText input,String label,String prompt,Runnable submit){
        Button chip=new Button(activity);
        chip.setText(label);
        chip.setAllCaps(false);
        chip.setTextColor(0xFFEAFBFF);
        chip.setTextSize(10f);
        chip.setMinHeight(dp(activity,36));
        chip.setPadding(dp(activity,12),0,dp(activity,12),0);
        chip.setBackground(round(activity,0xFF0C594F,18,0xFF16B8A6));
        LinearLayout.LayoutParams lp=new LinearLayout.LayoutParams(ViewGroup.LayoutParams.WRAP_CONTENT,dp(activity,36));
        lp.setMarginEnd(dp(activity,7));
        row.addView(chip,lp);
        chip.setOnClickListener(v->{
            input.setText(prompt);
            input.setSelection(input.length());
            submit.run();
        });
    }

    private static TextView appendBubble(Activity activity,LinearLayout messages,boolean user,String value){
        TextView bubble=text(activity,value,12.5f,user?Color.WHITE:0xFFE6F4FA,false);
        bubble.setTextIsSelectable(true);
        Linkify.addLinks(bubble,Linkify.WEB_URLS);
        bubble.setMovementMethod(LinkMovementMethod.getInstance());
        bubble.setLinksClickable(true);
        bubble.setPadding(dp(activity,12),dp(activity,9),dp(activity,12),dp(activity,9));
        bubble.setBackground(round(activity,user?0xFF0D716B:0xFF102B3C,12,user?0xFF20B8AA:0xFF245D79));
        LinearLayout.LayoutParams lp=new LinearLayout.LayoutParams(
            Math.round(activity.getResources().getDisplayMetrics().widthPixels*.84f),
            ViewGroup.LayoutParams.WRAP_CONTENT
        );
        lp.gravity=user?Gravity.END:Gravity.START;
        lp.topMargin=dp(activity,6);
        messages.addView(bubble,lp);
        return bubble;
    }

    private static TextView text(Activity activity,String value,float size,int color,boolean bold){
        TextView t=new TextView(activity);
        t.setText(value);
        t.setTextColor(color);
        t.setTextSize(size);
        t.setIncludeFontPadding(false);
        t.setLineSpacing(dp(activity,2),1f);
        if(bold)t.setTypeface(Typeface.DEFAULT,Typeface.BOLD);
        return t;
    }

    private static GradientDrawable round(Activity activity,int fill,int radius,int stroke){
        GradientDrawable d=new GradientDrawable();
        d.setColor(fill);
        d.setCornerRadius(dp(activity,radius));
        if(stroke!=0)d.setStroke(dp(activity,1),stroke);
        return d;
    }

    private static void scrollBottom(ScrollView scroll){
        scroll.post(()->scroll.fullScroll(View.FOCUS_DOWN));
    }

    private static int dp(Activity activity,int value){
        return Math.round(value*activity.getResources().getDisplayMetrics().density);
    }

    private MusaAiPanel(){}
}
