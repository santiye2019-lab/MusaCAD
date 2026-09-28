package com.musa.cad;

import android.content.*;
import android.graphics.Color;
import android.graphics.Typeface;
import android.graphics.drawable.GradientDrawable;
import android.os.Bundle;
import android.text.InputType;
import android.util.TypedValue;
import android.view.Gravity;
import android.view.View;
import android.view.ViewGroup;
import android.widget.*;
import androidx.appcompat.app.AlertDialog;
import androidx.appcompat.app.AppCompatActivity;
import androidx.core.widget.TextViewCompat;
import java.io.*;
import java.text.DateFormat;
import java.util.Date;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;

public class LicenseActivity extends AppCompatActivity {
    public static final String EXTRA_PENDING_INTENT="com.musa.cad.PENDING_INTENT";
    public static final String EXTRA_STAY_ON_LICENSE="com.musa.cad.STAY_ON_LICENSE";

    private final ExecutorService trialExecutor=Executors.newSingleThreadExecutor();
    private EditText licenseCode;
    private Button playPurchaseButton;
    private PlayBillingManager playBilling;
    private Intent pendingIntent;
    private volatile boolean trialRequestRunning;
    private boolean stayOnLicense;
    private boolean playProductReady;
    private String playDisplayPrice="";

    @Override protected void onCreate(Bundle savedInstanceState){
        super.onCreate(savedInstanceState);
        LockedScreenUi.enableImmersive(this);
        setContentView(R.layout.activity_license);

        stayOnLicense=getIntent().getBooleanExtra(EXTRA_STAY_ON_LICENSE,false);
        playBilling=new PlayBillingManager(this,new PlayBillingManager.Listener(){
            @Override public void onProductReady(boolean ready,String displayPrice){
                playProductReady=ready;
                playDisplayPrice=displayPrice==null?"":displayPrice;
                updateRenewalButton();
            }
            @Override public void onEntitlementChanged(boolean active){
                updateRenewalButton();
                if(active&&!stayOnLicense){
                    Toast.makeText(LicenseActivity.this,"Yıllık lisans Google Play ile yenilendi",Toast.LENGTH_SHORT).show();
                    enterAfterLicense();
                }
            }
            @Override public void onBillingMessage(String message){
                if(message!=null&&!message.isEmpty())
                    Toast.makeText(LicenseActivity.this,message,Toast.LENGTH_LONG).show();
            }
        });

        if(android.os.Build.VERSION.SDK_INT>=33){
            pendingIntent=getIntent().getParcelableExtra(EXTRA_PENDING_INTENT,Intent.class);
        }else{
            @SuppressWarnings("deprecation")
            Intent legacy=getIntent().getParcelableExtra(EXTRA_PENDING_INTENT);
            pendingIntent=legacy;
        }

        buildResponsiveLicenseUi();

        refresh();
        playBilling.start();
    }

    private void buildResponsiveLicenseUi(){
        LinearLayout content=findViewById(R.id.licenseContent);
        if(content==null)return;
        content.removeAllViews();

        TextView title=textView("MusaCAD\nLisans ve Aktivasyon",28f,Color.WHITE,true);
        title.setLineSpacing(0f,.92f);
        content.addView(title,matchWrap());

        TextView intro=textView("Güvenli aktivasyon • 1 günlük deneme • cihaz bağlı MC1 lisansı",12f,0xFFB8D0DE,false);
        intro.setPadding(0,dp(4),0,dp(14));
        content.addView(intro,matchWrap());

        LinearLayout trialCard=card();
        trialCard.addView(textView("1 Günlük Ücretsiz Deneme",18f,Color.WHITE,true),matchWrap());
        TextView trialInfo=textView("MusaCAD'i lisans kodu girmeden 24 saat tam özellikli deneyin. Bu hak aynı cihazda yalnız bir kez kullanılabilir.",12f,0xFFC7D7E0,false);
        trialInfo.setPadding(0,dp(6),0,dp(10));trialCard.addView(trialInfo,matchWrap());
        Button trial=actionButton("1 Günlük Ücretsiz Denemeyi Başlat",0xFF118B81);
        trial.setOnClickListener(v->startTrial());trialCard.addView(trial,buttonLp());
        content.addView(trialCard,cardLp());

        LinearLayout activationCard=card();
        activationCard.addView(textView(BuildConfig.ALLOW_LEGACY_SHORT_LICENSE?"Lisans Kodu / MC1":"Güvenli MC1 Lisansı",18f,Color.WHITE,true),matchWrap());
        TextView activationInfo=textView(BuildConfig.ALLOW_LEGACY_SHORT_LICENSE
            ?"MC1 lisansınızı veya uyumluluk lisans kodunu girin."
            :"Bu cihaz için üretilmiş RSA imzalı MC1 lisansını girin.",12f,0xFFC7D7E0,false);
        activationInfo.setPadding(0,dp(6),0,dp(8));activationCard.addView(activationInfo,matchWrap());

        LinearLayout codeRow=new LinearLayout(this);codeRow.setOrientation(LinearLayout.HORIZONTAL);codeRow.setGravity(Gravity.CENTER_VERTICAL);
        licenseCode=new EditText(this);
        licenseCode.setSingleLine(true);
        licenseCode.setInputType(InputType.TYPE_CLASS_TEXT|InputType.TYPE_TEXT_VARIATION_VISIBLE_PASSWORD);
        licenseCode.setTextColor(Color.WHITE);licenseCode.setTextSize(14f);
        TextViewCompat.setAutoSizeTextTypeUniformWithConfiguration(licenseCode,11,14,1,TypedValue.COMPLEX_UNIT_SP);
        licenseCode.setHint(BuildConfig.ALLOW_LEGACY_SHORT_LICENSE?"12 haneli kod veya MC1 lisansı":"Güvenli MC1 lisansı");
        licenseCode.setHintTextColor(0xFF8EA8BD);
        licenseCode.setPadding(dp(12),0,dp(12),0);
        GradientDrawable codeBg=round(0xE60A223A,10,0xFF3AAAF0);
        licenseCode.setBackground(codeBg);
        codeRow.addView(licenseCode,new LinearLayout.LayoutParams(0,dp(54),1f));

        Button paste=actionButton("YAPIŞTIR",0xFF164E73);
        paste.setTextSize(10f);paste.setMinWidth(dp(82));paste.setOnClickListener(v->pasteLicenseCode());
        LinearLayout.LayoutParams pasteLp=new LinearLayout.LayoutParams(dp(88),dp(54));pasteLp.setMarginStart(dp(8));codeRow.addView(paste,pasteLp);
        activationCard.addView(codeRow,matchWrap());

        Button activate=actionButton("ETKİNLEŞTİR",0xFF0E6B9A);
        LinearLayout.LayoutParams activateLp=buttonLp();activateLp.topMargin=dp(10);activationCard.addView(activate,activateLp);
        activate.setOnClickListener(v->activate());
        content.addView(activationCard,cardLp());

        final String secureLicenseId=LicenseManager.installationId(this);
        LinearLayout identityCard=card();
        identityCard.addView(textView("Güvenli Lisans Kimliği",18f,Color.WHITE,true),matchWrap());
        TextView idInfo=textView("MC1 lisansı oluşturulurken kullanılacak cihaz kimliğidir. Dokunarak veya KOPYALA ile panoya alın.",12f,0xFFC7D7E0,false);
        idInfo.setPadding(0,dp(6),0,dp(8));identityCard.addView(idInfo,matchWrap());
        TextView deviceId=textView(secureLicenseId,12f,0xFFE7F7FF,true);
        deviceId.setGravity(Gravity.CENTER);deviceId.setTextIsSelectable(true);deviceId.setPadding(dp(10),dp(12),dp(10),dp(12));
        deviceId.setBackground(round(0xED0A223A,9,0xFF2E9EE8));
        deviceId.setContentDescription("MusaCAD Güvenli Lisans Kimliği. Dokunarak kopyala.");
        deviceId.setOnClickListener(v->copySecureLicenseId(secureLicenseId));
        identityCard.addView(deviceId,matchWrap());
        Button copyId=actionButton("GÜVENLİ KİMLİĞİ KOPYALA",0xFF164E73);
        LinearLayout.LayoutParams copyLp=buttonLp();copyLp.topMargin=dp(8);identityCard.addView(copyId,copyLp);
        copyId.setOnClickListener(v->copySecureLicenseId(secureLicenseId));
        content.addView(identityCard,cardLp());

        LinearLayout distributionCard=card();
        if(BuildConfig.PLAY_DISTRIBUTION){
            distributionCard.addView(textView("Google Play Lisans Yenileme",18f,Color.WHITE,true),matchWrap());
            TextView playInfo=textView("Mevcut veya daha önce etkinleştirilmiş yıllık MusaCAD lisansınızı Google Play üzerinden yenileyebilirsiniz.",12f,0xFFC7D7E0,false);
            playInfo.setPadding(0,dp(6),0,dp(8));distributionCard.addView(playInfo,matchWrap());
            playPurchaseButton=actionButton("Google Play yıllık yenileme hazırlanıyor",0xFF147A55);
            playPurchaseButton.setOnClickListener(v->launchYearlyRenewal());
            distributionCard.addView(playPurchaseButton,buttonLp());
            updateRenewalButton();
        }else{
            distributionCard.addView(textView("DOĞRUDAN APK / KURUMSAL",18f,Color.WHITE,true),matchWrap());
            TextView direct=textView("Güvenli Lisans Kimliği + MC1 lisansı kullanılır. Google Play satın alma bu sürümde kapalıdır.",12f,0xFFC7D7E0,false);
            direct.setPadding(0,dp(6),0,0);distributionCard.addView(direct,matchWrap());
        }
        content.addView(distributionCard,cardLp());

        Button info=actionButton("LİSANS BİLGİSİ",0xFF164E73);info.setOnClickListener(v->showLicenseInfo());
        content.addView(info,buttonLpWithTop(10));
        Button terms=actionButton("LİSANS KOŞULLARI VE GİZLİLİK",0xFF164E73);terms.setOnClickListener(v->showTerms(false));
        content.addView(terms,buttonLpWithTop(8));
    }

    private LinearLayout card(){
        LinearLayout card=new LinearLayout(this);card.setOrientation(LinearLayout.VERTICAL);
        card.setPadding(dp(16),dp(14),dp(16),dp(14));
        card.setBackground(round(0xE6122638,14,0xFF245D79));
        return card;
    }

    private TextView textView(String value,float size,int color,boolean bold){
        TextView t=new TextView(this);t.setText(value);t.setTextColor(color);t.setTextSize(size);
        t.setIncludeFontPadding(false);t.setLineSpacing(dp(2),1f);
        if(bold)t.setTypeface(Typeface.DEFAULT,Typeface.BOLD);
        return t;
    }

    private Button actionButton(String label,int color){
        Button b=new Button(this);b.setText(label);b.setAllCaps(false);b.setTextColor(Color.WHITE);b.setTextSize(12.5f);
        TextViewCompat.setAutoSizeTextTypeUniformWithConfiguration(b,10,13,1,TypedValue.COMPLEX_UNIT_SP);
        b.setGravity(Gravity.CENTER);b.setMinHeight(dp(48));b.setPadding(dp(10),dp(4),dp(10),dp(4));
        b.setStateListAnimator(null);b.setBackground(round(color,11,0x5538B9E8));
        return b;
    }

    private GradientDrawable round(int fill,int radius,int stroke){
        GradientDrawable d=new GradientDrawable();d.setColor(fill);d.setCornerRadius(dp(radius));
        if(stroke!=0)d.setStroke(dp(1),stroke);return d;
    }

    private LinearLayout.LayoutParams matchWrap(){
        return new LinearLayout.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT,ViewGroup.LayoutParams.WRAP_CONTENT);
    }

    private LinearLayout.LayoutParams cardLp(){
        LinearLayout.LayoutParams lp=matchWrap();lp.bottomMargin=dp(10);return lp;
    }

    private LinearLayout.LayoutParams buttonLp(){
        return new LinearLayout.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT,dp(52));
    }

    private LinearLayout.LayoutParams buttonLpWithTop(int top){
        LinearLayout.LayoutParams lp=buttonLp();lp.topMargin=dp(top);return lp;
    }

    private void styleDialogButtons(AlertDialog dialog){
        if(dialog==null)return;
        int[] ids={AlertDialog.BUTTON_POSITIVE,AlertDialog.BUTTON_NEUTRAL,AlertDialog.BUTTON_NEGATIVE};
        for(int id:ids){
            Button b=dialog.getButton(id);if(b==null)continue;
            b.setTextColor(Color.WHITE);b.setMinHeight(dp(48));b.setSingleLine(false);b.setMaxLines(2);
            TextViewCompat.setAutoSizeTextTypeUniformWithConfiguration(b,9,12,1,TypedValue.COMPLEX_UNIT_SP);
        }
    }

    @Override public void onWindowFocusChanged(boolean hasFocus){
        super.onWindowFocusChanged(hasFocus);
        if(hasFocus)LockedScreenUi.enableImmersive(this);
    }

    private int dp(int v){
        return Math.round(v*getResources().getDisplayMetrics().density);
    }

    private void updateRenewalButton(){
        if(playPurchaseButton==null)return;
        boolean eligible=LicenseManager.eligibleForPlayYearlyRenewal(this);
        playPurchaseButton.setEnabled(true);
        if(!eligible){
            playPurchaseButton.setText("Yıllık yenileme için mevcut lisans gerekli");
        }else if(playProductReady){
            playPurchaseButton.setText(playDisplayPrice.isEmpty()
                ?"Google Play ile yıllık lisansı yenile"
                :"Yıllık lisansı yenile • "+playDisplayPrice);
        }else{
            playPurchaseButton.setText("Google Play yıllık yenileme hazırlanıyor");
        }
    }

    private void launchYearlyRenewal(){
        if(!BuildConfig.PLAY_DISTRIBUTION){
            Toast.makeText(this,
                "Bu doğrudan APK / kurumsal MusaCAD sürümünde Google Play yenilemesi yoktur. Güvenli Lisans Kimliği için üretilmiş MC1 lisansını kullanın.",
                Toast.LENGTH_LONG).show();
            return;
        }
        if(!LicenseManager.eligibleForPlayYearlyRenewal(this)){
            Toast.makeText(this,
                "Google Play yalnızca mevcut veya daha önce etkinleştirilmiş yıllık MusaCAD lisansını yenilemek içindir. İlk aktivasyon için lisans kodunu kullanın.",
                Toast.LENGTH_LONG).show();
            return;
        }
        if(!LicenseManager.termsAccepted(this)){
            showTerms(false);
            Toast.makeText(this,"Yenilemeden önce lisans koşullarını kabul edin",Toast.LENGTH_LONG).show();
            return;
        }
        playBilling.launchPurchase(this);
    }

    private void pasteLicenseCode(){
        ClipboardManager clipboard=(ClipboardManager)getSystemService(Context.CLIPBOARD_SERVICE);
        if(clipboard==null||!clipboard.hasPrimaryClip())return;
        ClipData clip=clipboard.getPrimaryClip();
        if(clip==null||clip.getItemCount()==0)return;
        CharSequence text=clip.getItemAt(0).coerceToText(this);
        if(text!=null&&licenseCode!=null){
            licenseCode.setText(text.toString().trim());
            licenseCode.setSelection(licenseCode.length());
        }
    }

    private void copySerial(String value){
        ClipboardManager clipboard=(ClipboardManager)getSystemService(Context.CLIPBOARD_SERVICE);
        if(clipboard!=null){
            clipboard.setPrimaryClip(ClipData.newPlainText("MusaCAD Serial",value));
            Toast.makeText(this,"Serial kopyalandı",Toast.LENGTH_SHORT).show();
        }
    }

    private void copySecureLicenseId(String value){
        ClipboardManager clipboard=(ClipboardManager)getSystemService(Context.CLIPBOARD_SERVICE);
        if(clipboard!=null){
            clipboard.setPrimaryClip(ClipData.newPlainText("MusaCAD Güvenli Lisans Kimliği",value));
            Toast.makeText(this,"Güvenli lisans kimliği kopyalandı",Toast.LENGTH_SHORT).show();
        }
    }

    private void showLicenseInfo(){
        LicenseManager.State state=LicenseManager.state(this);
        StringBuilder msg=new StringBuilder();
        msg.append("Durum: ").append(stateLabel(state));
        msg.append("\nDağıtım: ").append(BuildConfig.PLAY_DISTRIBUTION
            ?"Google Play"
            :"Doğrudan APK / Kurumsal");
        if(state==LicenseManager.State.TRIAL_ACTIVE){
            msg.append("\n").append(LicenseManager.remainingLabel(this));
        }
        long playExpiry=LicenseManager.playExpiryAtMs(this);
        if(BuildConfig.PLAY_DISTRIBUTION&&playExpiry>System.currentTimeMillis()){
            msg.append("\nGoogle Play yıllık lisans bitişi: ")
               .append(DateFormat.getDateInstance(DateFormat.MEDIUM).format(new Date(playExpiry)));
        }
        final String secureLicenseId=LicenseManager.installationId(this);
        msg.append("\n\nSerial: ").append(LicenseManager.serialId(this));
        msg.append("\nGüvenli Lisans Kimliği: ").append(secureLicenseId);
        msg.append("\nOffline / kurumsal aktivasyon: Güvenli Lisans Kimliği için RSA imzalı MC1 lisansı kullanılır. Production sürümünde 12 haneli kısa kod kabul edilmez.");
        String stored=getSharedPreferences("musacad_license_state",MODE_PRIVATE).getString("license_token_v1",null);
        if(BuildConfig.ALLOW_LEGACY_SHORT_LICENSE&&stored!=null&&ShortLicenseCode.looksLikeShortCode(stored)){
            long shortExpiry=ShortLicenseCode.expiryAtMs(stored);
            if(shortExpiry==0L)msg.append("\nKısa lisans: Süresiz");
            else msg.append("\nKısa lisans bitişi: ").append(DateFormat.getDateInstance(DateFormat.MEDIUM).format(new Date(shortExpiry)));
        }
        AlertDialog dialog=new AlertDialog.Builder(this)
            .setTitle("MusaCAD Lisans Bilgisi")
            .setMessage(msg.toString())
            .setNeutralButton("GÜVENLİ KİMLİĞİ KOPYALA",(d,which)->copySecureLicenseId(secureLicenseId))
            .setPositiveButton("TAMAM",null)
            .create();
        dialog.setOnShowListener(d->styleDialogButtons(dialog));
        dialog.show();
    }

    private String stateLabel(LicenseManager.State state){
        switch(state){
            case LICENSED:return "Lisanslı";
            case TRIAL_ACTIVE:return "Ücretsiz deneme aktif";
            case TRIAL_AVAILABLE:return "Ücretsiz deneme kullanılabilir";
            case TRIAL_EXPIRED:return "Deneme süresi sona erdi";
            case CLOCK_ERROR:return "Cihaz saatinde tutarsızlık";
            default:return state.name();
        }
    }

    private void refresh(){
        LicenseManager.State s=LicenseManager.state(this);
        if((s==LicenseManager.State.TRIAL_ACTIVE||s==LicenseManager.State.LICENSED)&&!stayOnLicense){
            enterApp();
        }
    }

    private void startTrial(){
        if(trialRequestRunning)return;
        if(!LicenseManager.termsAccepted(this)){
            showTerms(true);
            return;
        }

        trialRequestRunning=true;
        trialExecutor.execute(()->{
            TrialService.Result r=TrialService.start(getApplicationContext());
            runOnUiThread(()->{
                trialRequestRunning=false;
                if(r.status==TrialService.Status.ACTIVATED){
                    Toast.makeText(this,"1 günlük ücretsiz deneme etkinleştirildi",Toast.LENGTH_SHORT).show();
                    enterAfterLicense();
                }else if(r.status==TrialService.Status.NOT_CONFIGURED){
                    if(BuildConfig.DEBUG){
                        startLocalTrialFallback();
                    }else{
                        Toast.makeText(this,"Ücretsiz deneme servisi yapılandırılmadı. Lütfen daha sonra tekrar deneyin.",Toast.LENGTH_LONG).show();
                    }
                }else if(r.status==TrialService.Status.ALREADY_USED){
                    Toast.makeText(this,"Bu cihaz ücretsiz denemeyi daha önce kullandı",Toast.LENGTH_LONG).show();
                }else{
                    Toast.makeText(this,r.message==null?"Deneme başlatılamadı":r.message,Toast.LENGTH_LONG).show();
                }
            });
        });
    }

    private void startLocalTrialFallback(){
        if(LicenseManager.startTrial(this)){
            Toast.makeText(this,"1 günlük ücretsiz deneme başlatıldı",Toast.LENGTH_SHORT).show();
            enterAfterLicense();
        }else{
            Toast.makeText(this,"Bu cihazdaki ücretsiz deneme daha önce kullanılmış veya sona ermiş",Toast.LENGTH_LONG).show();
        }
    }

    private void activate(){
        if(!LicenseManager.termsAccepted(this)){
            showTerms(false);
            Toast.makeText(this,"Lisans koşullarını kabul ettikten sonra tekrar Etkinleştir'e basın",Toast.LENGTH_LONG).show();
            return;
        }
        String code=licenseCode==null?"":licenseCode.getText().toString().trim();
        LicenseManager.ActivationResult r=LicenseManager.activateCode(this,code);
        if(r==LicenseManager.ActivationResult.ACTIVATED){
            Toast.makeText(this,"Lisans etkinleştirildi",Toast.LENGTH_SHORT).show();
            updateRenewalButton();
            enterAfterLicense();
        }else if(licenseCode!=null){
            if(ShortLicenseCode.looksLikeShortCode(code)){
                licenseCode.setError(BuildConfig.ALLOW_LEGACY_SHORT_LICENSE
                    ?"12 haneli uyumluluk kodu bu Serial ile eşleşmiyor veya süresi dolmuş"
                    :"Production sürümünde 12 haneli kısa kod kabul edilmez; MC1 lisansı kullanın.");
            }else if(BuildConfig.DEBUG&&code.startsWith("MCT1.")){
                licenseCode.setError("TEST kodu bu cihaza ait değil veya süresi dolmuş");
            }else{
                licenseCode.setError("Kod geçersiz, süresi dolmuş veya bu cihaza ait değil");
            }
        }
    }

    private void showTerms(boolean startAfterAccept){
        TextView text=new TextView(this);
        int p=dp(18);
        text.setPadding(p,p,p,p);
        text.setText(readAsset("MUSACAD-LICENSE-TERMS-TR.txt"));
        text.setTextIsSelectable(true);
        text.setTextSize(12f);

        ScrollView scroll=new ScrollView(this);
        scroll.addView(text);

        AlertDialog dialog=new AlertDialog.Builder(this)
            .setTitle("MusaCAD lisans koşulları")
            .setView(scroll)
            .setPositiveButton("KABUL EDİYORUM",(d,w)->{
                LicenseManager.acceptTerms(this);
                if(startAfterAccept)startTrial();
            })
            .setNegativeButton("KAPAT",null)
            .create();
        dialog.setOnShowListener(d->styleDialogButtons(dialog));
        dialog.show();
    }

    private String readAsset(String name){
        try(InputStream in=getAssets().open(name);ByteArrayOutputStream out=new ByteArrayOutputStream()){
            byte[] b=new byte[4096];
            int n;
            while((n=in.read(b))!=-1)out.write(b,0,n);
            return out.toString("UTF-8");
        }catch(IOException e){
            return "Lisans koşulları okunamadı.";
        }
    }

    private void enterAfterLicense(){
        enterApp();
    }

    private void enterApp(){
        Intent next=pendingIntent==null
            ?new Intent(this,MainActivity.class)
            :new Intent(pendingIntent).setClass(this,MainActivity.class);
        next.addFlags(Intent.FLAG_ACTIVITY_CLEAR_TOP);
        startActivity(next);
        overridePendingTransition(android.R.anim.fade_in,android.R.anim.fade_out);
        finish();
    }

    @Override protected void onResume(){
        super.onResume();
        updateRenewalButton();
        if(playBilling!=null)playBilling.refresh();
    }

    @Override protected void onDestroy(){
        trialExecutor.shutdownNow();
        if(playBilling!=null)playBilling.close();
        super.onDestroy();
    }
}
