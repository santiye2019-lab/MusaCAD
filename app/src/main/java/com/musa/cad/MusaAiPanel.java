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
import android.widget.Toast;
import android.os.Build;
import android.widget.TextView;
import android.widget.ProgressBar;
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
        /** Report only measured units; never infer HTTP/model completion. */
        default void progressMeasured(String task,int completed,int total){progress(task);}
        /** Server/model response time cannot be measured as a percentage. */
        default void progressWaiting(String task){progress(task);}
        /** Reports a successful/failed actual cloud model answer, not just /health. */
        default void cloudStatus(boolean modelAnswered){}
        /** Recent failure reason for the LED; no secrets, tokens or CAD are logged. */
        default void cloudFailure(String reason){cloudStatus(false);}
    }

    public interface Host {
        String contextLabel();
        void onPrompt(String prompt,Reply reply);
        default void onPrompt(String prompt,String recentContext,Reply reply){
            onPrompt(prompt,reply);
        }
        default void onViewPdf(String answer,Reply reply) {
            reply.send("PDF görüntüleme bu sürümde yapılandırılmadı.");
        }
        default void onImportPriceBook(Reply reply){
            reply.send("Poz kitabı seçimi bu sürümde yapılandırılmadı.");
        }
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

        // An actual /health HTTP 200 is required before showing the online LED.
        // A configured URL, developer role, or stale consent is not connectivity.
        final boolean cloudConfigured=MusaAiCloudHealth.configured();
        final AtomicBoolean modelVerified=new AtomicBoolean(false);
        final AtomicBoolean lastHealthOk=new AtomicBoolean(false);
        final java.util.concurrent.atomic.AtomicInteger failedHealthChecks=
            new java.util.concurrent.atomic.AtomicInteger(0);
        final String[] lastCloudFailure={""};
        TextView state=text(activity,cloudConfigured?"● Denetleniyor":"● AI ayarsız",10f,
            cloudConfigured?0xFFFFD184:0xFFB0BDC6,true);
        state.setGravity(Gravity.CENTER);
        state.setPadding(dp(activity,8),dp(activity,5),dp(activity,8),dp(activity,5));
        state.setBackground(round(activity,0xFF233541,16,0));
        header.addView(state,new LinearLayout.LayoutParams(ViewGroup.LayoutParams.WRAP_CONTENT,ViewGroup.LayoutParams.WRAP_CONTENT));
        state.setOnClickListener(v->Toast.makeText(activity,
            !lastCloudFailure[0].isEmpty()?lastCloudFailure[0]:
            !lastHealthOk.get()
                ?"Bağlantı denetimi geçici olarak yanıt vermedi. Son AI yanıtı başarılı olabilir; yeni soruda oturum yeniden doğrulanır."
                :modelVerified.get()
                    ?"Son AI isteği başarıyla yanıtlandı. Bir sonraki soru yeni güvenli HTTPS isteği açar."
                    :"AI sunucusuna erişiliyor. Model yanıtı ve lisans yetkisi ayrı doğrulanır.",
            Toast.LENGTH_LONG).show());
        root.addView(header,new LinearLayout.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT,ViewGroup.LayoutParams.WRAP_CONTENT));

        TextView context=text(activity,host.contextLabel(),11f,0xFF9FC1D1,false);
        context.setPadding(0,dp(activity,3),0,dp(activity,8));
        context.setMaxLines(2);
        root.addView(context,new LinearLayout.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT,ViewGroup.LayoutParams.WRAP_CONTENT));

        // Separate explicit book import from free-form Gandalf prompts and PDF view.
        LinearLayout topActions=new LinearLayout(activity);
        topActions.setOrientation(LinearLayout.HORIZONTAL);
        Button pdfPreview=new Button(activity);
        pdfPreview.setText("PDF görüntüle");
        pdfPreview.setAllCaps(false);
        pdfPreview.setTextColor(Color.WHITE);
        pdfPreview.setTextSize(11f);
        pdfPreview.setMinHeight(dp(activity,44));
        pdfPreview.setBackground(round(activity,0xFF0C594F,12,0xFF16B8A6));
        LinearLayout.LayoutParams pdfLp=new LinearLayout.LayoutParams(0,dp(activity,44),1f);
        topActions.addView(pdfPreview,pdfLp);
        Button importBook=new Button(activity);
        importBook.setText("Poz kitabı yükle");
        importBook.setAllCaps(false);
        importBook.setTextColor(Color.WHITE);
        importBook.setTextSize(11f);
        importBook.setMinHeight(dp(activity,44));
        importBook.setBackground(round(activity,0xFF12384C,12,0xFF246C88));
        LinearLayout.LayoutParams bookLp=new LinearLayout.LayoutParams(0,dp(activity,44),1f);
        bookLp.setMarginStart(dp(activity,7));
        topActions.addView(importBook,bookLp);
        LinearLayout.LayoutParams actionsLp=new LinearLayout.LayoutParams(
            ViewGroup.LayoutParams.MATCH_PARENT,ViewGroup.LayoutParams.WRAP_CONTENT);
        actionsLp.bottomMargin=dp(activity,5);
        root.addView(topActions,actionsLp);

        ScrollView messagesScroll=new ScrollView(activity);
        messagesScroll.setFillViewport(true);
        messagesScroll.setVerticalScrollBarEnabled(true);
        messagesScroll.setScrollbarFadingEnabled(false);
        messagesScroll.setScrollBarStyle(View.SCROLLBARS_INSIDE_INSET);
        if(Build.VERSION.SDK_INT>=29){
            GradientDrawable scrollbar=new GradientDrawable();
            scrollbar.setColor(0xFF27BEBC);
            scrollbar.setCornerRadius(dp(activity,3));
            messagesScroll.setVerticalScrollbarThumbDrawable(scrollbar);
        }
        LinearLayout messages=new LinearLayout(activity);
        messages.setOrientation(LinearLayout.VERTICAL);
        messages.setPadding(0,dp(activity,8),0,dp(activity,8));
        messagesScroll.addView(messages,new ScrollView.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT,ViewGroup.LayoutParams.WRAP_CONTENT));
        LinearLayout.LayoutParams messagesLp=new LinearLayout.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT,0,1f);
        messagesLp.topMargin=dp(activity,4);
        root.addView(messagesScroll,messagesLp);

        appendBubble(activity,messages,false,
            developer
                ?"Gandalf Developer • Sorunuzu doğal cümleyle yazın. Bulut AI, izin verdiğinizde yerel CAD verileriyle birlikte çalışır; çizim değişiklikleri için onay gerekir."
                :"Gandalf • Sorunuzu kendi kelimelerinizle yazın. Yerel DWG araçları ölçer ve kontrol eder; izinli bağlantıda çevrim içi AI bunları yorumlar. Bağlantı olmadığında yerel yanıt gösterilir.");

        LinearLayout composer=new LinearLayout(activity);
        composer.setOrientation(LinearLayout.VERTICAL);
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
        // Full-width editor avoids the narrow multi-line column seen on phones.
        composer.addView(input,new LinearLayout.LayoutParams(
            ViewGroup.LayoutParams.MATCH_PARENT,ViewGroup.LayoutParams.WRAP_CONTENT));
        LinearLayout actionRow=new LinearLayout(activity);
        actionRow.setOrientation(LinearLayout.HORIZONTAL);
        actionRow.setGravity(Gravity.END|Gravity.CENTER_VERTICAL);
        LinearLayout.LayoutParams actionLp=new LinearLayout.LayoutParams(
            ViewGroup.LayoutParams.MATCH_PARENT,ViewGroup.LayoutParams.WRAP_CONTENT);
        actionLp.topMargin=dp(activity,6);
        composer.addView(actionRow,actionLp);

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
        actionRow.addView(voice,voiceLp);

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
        actionRow.addView(send,sendLp);
        root.addView(composer,new LinearLayout.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT,ViewGroup.LayoutParams.WRAP_CONTENT));

        final String[] latestAssistantAnswer={""};
        // Session-only short history: never written to device storage.
        final java.util.ArrayDeque<String> recentTurns=new java.util.ArrayDeque<>();

        Runnable submit=()->{
            String prompt=input.getText().toString().trim();
            if(prompt.isEmpty())return;
            input.setText("");
            appendBubble(activity,messages,true,prompt);
            TextView pending=appendBubble(activity,messages,false,"İşleniyor…");
            // Each request owns its own progress view. Concurrent commands must
            // never overwrite the stage or percentage of another request.
            LinearLayout taskProgress=new LinearLayout(activity);
            taskProgress.setOrientation(LinearLayout.VERTICAL);
            taskProgress.setPadding(dp(activity,10),dp(activity,6),dp(activity,10),dp(activity,7));
            taskProgress.setBackground(round(activity,0xFF102B3C,10,0xFF245D79));
            LinearLayout.LayoutParams progressLp=new LinearLayout.LayoutParams(
                Math.round(activity.getResources().getDisplayMetrics().widthPixels*.84f),
                ViewGroup.LayoutParams.WRAP_CONTENT);
            progressLp.gravity=Gravity.START;
            progressLp.topMargin=dp(activity,3);
            messages.addView(taskProgress,progressLp);
            TextView taskName=text(activity,"Hazırlanıyor • ilerleme ölçülüyor",11f,0xFFD4E7EE,true);
            taskProgress.addView(taskName);
            ProgressBar taskBar=new ProgressBar(activity,null,android.R.attr.progressBarStyleHorizontal);
            taskBar.setMax(100);
            taskBar.setIndeterminate(true);
            if(Build.VERSION.SDK_INT>=21){
                taskBar.setProgressTintList(android.content.res.ColorStateList.valueOf(0xFF26D1C0));
                taskBar.setIndeterminateTintList(android.content.res.ColorStateList.valueOf(0xFF26D1C0));
            }
            LinearLayout.LayoutParams barLp=new LinearLayout.LayoutParams(
                ViewGroup.LayoutParams.MATCH_PARENT,dp(activity,5));
            barLp.topMargin=dp(activity,6);
            taskProgress.addView(taskBar,barLp);
            TextView taskElapsed=text(activity,"0 sn • ilerleme bekleniyor",10f,0xFF9FC1D1,false);
            LinearLayout.LayoutParams elapsedLp=new LinearLayout.LayoutParams(
                ViewGroup.LayoutParams.MATCH_PARENT,ViewGroup.LayoutParams.WRAP_CONTENT);
            elapsedLp.topMargin=dp(activity,4);
            taskProgress.addView(taskElapsed,elapsedLp);
            scrollBottom(messagesScroll);
            final AtomicBoolean completed=new AtomicBoolean(false);
            final StringBuilder previousTurns=new StringBuilder();
            for(String turn:recentTurns){
                if(previousTurns.length()+turn.length()>2600)continue;
                previousTurns.append(turn).append("\\n");
            }
            // The 12-minute cap belongs exclusively to an explicit DWG visual
            // engineering review. Spoken definitions use an ordinary chat cap.
            final boolean imageReview=!MusaAiConversationalIntent.isInformationalQuestion(prompt)&&
                (MusaAiAnalysisIntent.isVisionFirstReview(prompt)||
                 MusaAiAnalysisIntent.isReview(prompt)||
                 MusaAiAnalysisIntent.isCombinedVisualReview(prompt));
            final long hardDeadlineMs=imageReview
                ?MusaAiVisionTimeBudget.PANEL_HARD_DEADLINE_MS
                :MusaAiVisionTimeBudget.PANEL_CHAT_DEADLINE_MS;
            final long idleDelayMs=imageReview
                ?MusaAiVisionTimeBudget.IDLE_NOTICE_MS
                :MusaAiVisionTimeBudget.CHAT_IDLE_NOTICE_MS;
            final Handler timeoutHandler=new Handler(Looper.getMainLooper());
            final long startedMs=android.os.SystemClock.elapsedRealtime();
            final long[] lastProgressMs={startedMs};
            final String[] taskStage={"Hazırlanıyor"};
            final boolean[] measured={false};
            final Runnable[] progressTicker=new Runnable[1];
            progressTicker[0]=()->{
                if(completed.get()||!sheet.isShowing())return;
                long now=android.os.SystemClock.elapsedRealtime();
                long elapsed=Math.max(0,(now-startedMs)/1000);
                long stageWait=Math.max(0,(now-lastProgressMs[0])/1000);
                String diagnostic=measured[0]
                    ?"Doğrulanmış adımlar; sıradaki işlem bekleniyor"
                    :stageWait>=45
                        ?"Yeni yanıt yok • ağ/model gecikmiş olabilir"
                        :"İşlem izleniyor • yüzde yalnız ölçülen adımlardır";
                taskElapsed.setText(elapsed+" sn geçti • "+diagnostic);
                timeoutHandler.postDelayed(progressTicker[0],2000L);
            };
            timeoutHandler.postDelayed(progressTicker[0],2000L);
            // One Qwen image call can legitimately take 90s, plus CAD rendering.
            // An idle notification is NOT a terminal failure; accept a late reply.
            final Runnable idleNotice=()->{
                if(completed.get())return;
                // Never overwrite the last confirmed progress with a deceptive
                // generic "working" claim: the Worker may actually be stalled.
                if(!measured[0])taskName.setText(taskStage[0]+" • yanıt henüz doğrulanmadı");
                taskElapsed.setText(Math.max(0,(android.os.SystemClock.elapsedRealtime()-startedMs)/1000)+
                    " sn geçti • Son işlemden yeni yanıt alınamadı");
            };
            final Runnable timeout=()->{
                if(!completed.compareAndSet(false,true))return;
                timeoutHandler.removeCallbacks(idleNotice);
                timeoutHandler.removeCallbacks(progressTicker[0]);
                taskProgress.setVisibility(View.GONE);
                if(imageReview){
                    pending.setText("Gandalf • 12 dakikalık toplam analiz sınırına ulaşıldı. "+
                        "Bu mesaj 9/9 görsel incelemenin tamamlandığını göstermez. "+
                        "Çizime müdahale edilmedi. Eksik bölgeler doğrulanmış kabul edilmez.");
                }else{
                    pending.setText("Gandalf • Sohbet yanıtı 2 dakikalık süre sınırını aştı. "+
                        "DWG görsel taraması yapılmadı. Sunucu yanıtı gecikti; yeniden deneyebilirsiniz.");
                }
                // A green /health from an earlier call does not prove THIS
                // model request returned successfully.
                lastCloudFailure[0]="Yanıt zaman aşımı: "+(imageReview?"görsel inceleme":"genel sohbet");
                state.setText("● İstek zaman aşımı");
                state.setTextColor(0xFFFFA7A7);
                state.setBackground(round(activity,0xFF4C2428,16,0));
                scrollBottom(messagesScroll);
            };
            timeoutHandler.postDelayed(timeout,hardDeadlineMs);
            timeoutHandler.postDelayed(idleNotice,idleDelayMs);
            Reply requestReply=new Reply(){
                @Override public void send(String text){
                    activity.runOnUiThread(()->{
                        if(!completed.compareAndSet(false,true))return;
                        timeoutHandler.removeCallbacks(timeout);
                        timeoutHandler.removeCallbacks(idleNotice);
                        timeoutHandler.removeCallbacks(progressTicker[0]);
                        taskProgress.setVisibility(View.GONE);
                        String answer=text==null||text.trim().isEmpty()?"Yanıt oluşturulamadı.":text.trim();
                        pending.setText(answer);
                        latestAssistantAnswer[0]=answer;
                        recentTurns.addLast("Kullanıcı: "+prompt.substring(0,Math.min(300,prompt.length()))+
                            "\\nGandalf: "+answer.substring(0,Math.min(500,answer.length())));
                        while(recentTurns.size()>4)recentTurns.removeFirst();
                        Linkify.addLinks(pending,Linkify.WEB_URLS);
                        pending.setMovementMethod(LinkMovementMethod.getInstance());
                        pending.setLinksClickable(true);
                        scrollBottom(messagesScroll);
                    });
                }
                @Override public void cloudStatus(boolean answered){
                    activity.runOnUiThread(()->{
                        // A late result from an expired request must not make
                        // its stale status LED green after a visible timeout.
                        if(completed.get())return;
                        modelVerified.set(answered);
                        if(answered){
                            lastCloudFailure[0]="";
                            // A real model response supersedes earlier failed health probes.
                            lastHealthOk.set(true);
                            failedHealthChecks.set(0);
                            // The session was refreshed by this successful model call.
                            title.setText(MusaAiSessionService.developerCached()
                                ?"Gandalf • Developer":"Gandalf • MusaCAD AI");
                        }
                        state.setText(answered?"● AI bağlı":"● AI erişilemedi");
                        state.setTextColor(answered?0xFF78F2C7:0xFFFFA7A7);
                        state.setBackground(round(activity,answered?0xFF15493F:0xFF4C2428,16,0));
                    });
                }
                @Override public void cloudFailure(String reason){
                    activity.runOnUiThread(()->{
                        if(completed.get())return;
                        modelVerified.set(false);
                        String diagnostic=reason==null?"AI isteği başarısız.":reason.trim();
                        if(diagnostic.length()>280)diagnostic=diagnostic.substring(0,280);
                        lastCloudFailure[0]=diagnostic;
                        String lowered=diagnostic.toLowerCase(new java.util.Locale("tr","TR"));
                        String stateName=lowered.contains("kota")||lowered.contains("429")
                            ?"● AI kota doldu":lowered.contains("oturum")||lowered.contains("lisans")||
                                lowered.contains("401")||lowered.contains("403")
                            ?"● AI oturum hatası":lowered.contains("süre")||lowered.contains("zaman")||
                                lowered.contains("504")
                            ?"● AI zaman aşımı":"● AI erişilemedi";
                        state.setText(stateName);
                        state.setTextColor(0xFFFFA7A7);
                        state.setBackground(round(activity,0xFF4C2428,16,0));
                    });
                }
                private void applyStage(String stage,boolean known,int done,int total){
                    if(completed.get())return;
                    timeoutHandler.removeCallbacks(idleNotice);
                    timeoutHandler.postDelayed(idleNotice,idleDelayMs);
                    taskStage[0]=stage==null?"İşleniyor":stage.trim();
                    measured[0]=known;
                    lastProgressMs[0]=android.os.SystemClock.elapsedRealtime();
                    if(known&&total>0){
                        int count=Math.max(0,Math.min(done,total));
                        int percent=(int)((100L*count)/total);
                        taskBar.setIndeterminate(false);
                        taskBar.setProgress(percent);
                        taskName.setText(taskStage[0]+" • "+count+"/"+total+" • %"+percent);
                    }else{
                        taskBar.setIndeterminate(true);
                        taskName.setText(taskStage[0]+" • yanıt bekleniyor (yüzde ölçülemiyor)");
                    }
                    pending.setText("Gandalf • "+taskStage[0]);
                    taskElapsed.setText(Math.max(0,
                        (android.os.SystemClock.elapsedRealtime()-startedMs)/1000)+
                        " sn geçti • "+(known?"Doğrulanmış ilerleme":"Sonucun gelmesi bekleniyor"));
                    scrollBottom(messagesScroll);
                }
                @Override public void progress(String text){
                    activity.runOnUiThread(()->applyStage(text,false,0,0));
                }
                @Override public void progressMeasured(String task,int done,int total){
                    activity.runOnUiThread(()->applyStage(task,total>0,done,total));
                }
                @Override public void progressWaiting(String task){
                    activity.runOnUiThread(()->applyStage(task,false,0,0));
                }
            };
            try{host.onPrompt(prompt,previousTurns.toString(),requestReply);}
            catch(Exception e){requestReply.send("Gandalf komutu işlenirken hata oluştu. Tekrar deneyin.");}
        };

        importBook.setOnClickListener(v->{
            TextView statusBubble=appendBubble(activity,messages,false,
                "2025 ÇŞİDB poz kitabı seçiliyor…");
            scrollBottom(messagesScroll);
            host.onImportPriceBook(new Reply(){
                @Override public void send(String message){
                    activity.runOnUiThread(()->{
                        statusBubble.setText(message==null?"Dosya seçme işlemi tamamlandı.":message);
                        scrollBottom(messagesScroll);
                    });
                }
                @Override public void progress(String message){
                    activity.runOnUiThread(()->{
                        if(message!=null&&!message.trim().isEmpty())statusBubble.setText(message);
                        scrollBottom(messagesScroll);
                    });
                }
            });
        });
        pdfPreview.setOnClickListener(v->{
            String latest=latestAssistantAnswer[0];
            if(latest==null||latest.trim().isEmpty()){
                appendBubble(activity,messages,false,
                    "Önce bir soru sorun veya rapor oluşturun; ardından son yanıtı PDF açabilirsiniz.");
                scrollBottom(messagesScroll);
                return;
            }
            host.onViewPdf(latest,new Reply(){
                @Override public void send(String message){
                    activity.runOnUiThread(()->{
                        appendBubble(activity,messages,false,message);
                        scrollBottom(messagesScroll);
                    });
                }
            });
        });
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
        final Handler connectionHandler=new Handler(Looper.getMainLooper());
        final AtomicBoolean connectionClosed=new AtomicBoolean(false);
        final AtomicBoolean connectionCheckRunning=new AtomicBoolean(false);
        final Runnable[] connectionProbe=new Runnable[1];
        connectionProbe[0]=()->{
            if(connectionClosed.get()||!cloudConfigured||!connectionCheckRunning.compareAndSet(false,true))
                return;
            MusaAiCloudHealth.check(online->{
                connectionCheckRunning.set(false);
                if(connectionClosed.get()||activity.isFinishing()||activity.isDestroyed())return;
                lastHealthOk.set(online);
                int failed=online?0:failedHealthChecks.incrementAndGet();
                if(online)failedHealthChecks.set(0);
                if(!lastCloudFailure[0].isEmpty()){
                    // Keep an actual model or authorization error visible.
                    // A gateway /health response cannot override the failure.
                }else if(!online){
                    // One short /health timeout is not proof that a validated
                    // AI session disconnected. Preserve the earlier answer
                    // and never label a health-only miss as "Erişim yok".
                    state.setText(failed>=3?"● Bağlantı denetlenemedi":
                        modelVerified.get()?"● Son AI yanıtı başarılı":"● Bağlantı denetleniyor");
                }else{
                    state.setText(modelVerified.get()?"● AI bağlı":"● Çevrimiçi");
                }
                boolean actualError=!lastCloudFailure[0].isEmpty();
                int indicatorColor=actualError||(!online&&failed>=3)
                    ?0xFFFFA7A7:online?0xFF78F2C7:0xFFFFD184;
                int indicatorBackground=actualError||(!online&&failed>=3)
                    ?0xFF4C2428:online?0xFF15493F:0xFF4A3A21;
                state.setTextColor(indicatorColor);
                state.setBackground(round(activity,indicatorBackground,16,0));
                connectionHandler.postDelayed(connectionProbe[0],20_000L);
            });
        };
        if(cloudConfigured)connectionHandler.post(connectionProbe[0]);
        sheet.setOnDismissListener(d->{
            connectionClosed.set(true);
            connectionHandler.removeCallbacks(connectionProbe[0]);
            MusaAiVoiceInput.clear(voiceCallback);
        });
        sheet.show();
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
