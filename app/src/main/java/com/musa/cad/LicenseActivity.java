package com.musa.cad;

import android.content.*;
import android.graphics.Color;
import android.graphics.drawable.GradientDrawable;
import android.os.Bundle;
import android.view.Gravity;
import android.widget.*;
import androidx.appcompat.app.AlertDialog;
import androidx.appcompat.app.AppCompatActivity;
import java.io.*;
import java.text.DateFormat;
import java.util.Date;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;

public class LicenseActivity extends AppCompatActivity {
    public static final String EXTRA_PENDING_INTENT="com.musa.cad.PENDING_INTENT";
    public static final String EXTRA_STAY_ON_LICENSE="com.musa.cad.STAY_ON_LICENSE";
    private static final float SCREEN_W=600f,SCREEN_H=1535f;

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

        FrameLayout root=findViewById(R.id.licenseRoot);
        FrameLayout stage=findViewById(R.id.artworkStage);

        LockedScreenUi.fillStage(this,root,stage,SCREEN_W,SCREEN_H,()->{
            ImageView art=stage.findViewById(R.id.lockedArtwork);
            art.setImageResource(R.drawable.musacad_screen_3_v2);
            art.setContentDescription("MusaCAD lisans ve aktivasyon ekranı");

            // 1) Ücretsiz deneme
            LockedScreenUi.hotspot(this,stage,60,344,482,132,SCREEN_W,SCREEN_H,v->startTrial());

            // 2) Lisans kodu alanı
            licenseCode=new EditText(this);
            licenseCode.setSingleLine(true);
            licenseCode.setTextColor(Color.WHITE);
            licenseCode.setTextSize(14f);
            licenseCode.setHint("XXXX-XXXX-XXXX-XXXX");
            licenseCode.setHintTextColor(0xFF7890A8);
            licenseCode.setPadding(dp(12),0,dp(12),0);
            GradientDrawable codeBg=new GradientDrawable();
            codeBg.setColor(0xD20A223A);
            codeBg.setCornerRadius(dp(10));
            codeBg.setStroke(dp(1),0xFF3AAAF0);
            licenseCode.setBackground(codeBg);
            stage.addView(licenseCode);
            LockedScreenUi.position(licenseCode,stage,83,598,369,56,SCREEN_W,SCREEN_H);

            // Lisans kodu yanındaki pano simgesi: panodan yapıştır.
            LockedScreenUi.hotspot(this,stage,458,597,64,58,SCREEN_W,SCREEN_H,v->pasteLicenseCode());
            LockedScreenUi.hotspot(this,stage,78,663,445,67,SCREEN_W,SCREEN_H,v->activate());

            // 3) Telefona özel cihaz/lisans kimliği
            final String deviceLicenseId=LicenseManager.installationId(this);
            TextView deviceId=new TextView(this);
            deviceId.setSingleLine(true);
            deviceId.setEllipsize(android.text.TextUtils.TruncateAt.END);
            deviceId.setText(deviceLicenseId);
            deviceId.setTextColor(0xFFE7F7FF);
            deviceId.setTextSize(11f);
            deviceId.setGravity(Gravity.CENTER);
            deviceId.setPadding(dp(5),0,dp(5),0);
            GradientDrawable idBg=new GradientDrawable();
            idBg.setColor(0xD30A223A);
            idBg.setCornerRadius(dp(8));
            idBg.setStroke(dp(1),0xFF2E9EE8);
            deviceId.setBackground(idBg);
            deviceId.setContentDescription("MusaCAD cihaz lisans kimliği. Dokunarak kopyala.");
            deviceId.setOnClickListener(v->copyDeviceId(deviceLicenseId));
            stage.addView(deviceId);
            LockedScreenUi.position(deviceId,stage,176,818,284,44,SCREEN_W,SCREEN_H);
            LockedScreenUi.hotspot(this,stage,458,817,65,50,SCREEN_W,SCREEN_H,v->copyDeviceId(deviceLicenseId));

            // 4) Google Play: yalnızca yıllık lisans yenileme
            LockedScreenUi.hotspot(this,stage,60,930,482,139,SCREEN_W,SCREEN_H,v->launchYearlyRenewal());

            playPurchaseButton=new Button(this);
            playPurchaseButton.setAllCaps(false);
            playPurchaseButton.setTextSize(10.5f);
            playPurchaseButton.setTextColor(Color.WHITE);
            playPurchaseButton.setGravity(Gravity.CENTER);
            playPurchaseButton.setPadding(dp(4),0,dp(4),0);
            playPurchaseButton.setStateListAnimator(null);
            GradientDrawable renewBg=new GradientDrawable();
            renewBg.setColor(0xD20D6D4A);
            renewBg.setCornerRadius(dp(10));
            renewBg.setStroke(dp(1),0xFF20E39A);
            playPurchaseButton.setBackground(renewBg);
            playPurchaseButton.setOnClickListener(v->launchYearlyRenewal());
            stage.addView(playPurchaseButton);
            LockedScreenUi.position(playPurchaseButton,stage,179,1024,276,39,SCREEN_W,SCREEN_H);
            updateRenewalButton();

            // 5) Lisans bilgisi ve sözleşme
            LockedScreenUi.hotspot(this,stage,60,1080,482,146,SCREEN_W,SCREEN_H,v->showLicenseInfo());
            LockedScreenUi.hotspot(this,stage,60,1236,482,62,SCREEN_W,SCREEN_H,v->showTerms(false));
        });

        refresh();
        playBilling.start();
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

    private void copyDeviceId(String value){
        ClipboardManager clipboard=(ClipboardManager)getSystemService(Context.CLIPBOARD_SERVICE);
        if(clipboard!=null){
            clipboard.setPrimaryClip(ClipData.newPlainText("MusaCAD Cihaz Kimliği",value));
            Toast.makeText(this,"Cihaz / lisans kimliği kopyalandı",Toast.LENGTH_SHORT).show();
        }
    }

    private void showLicenseInfo(){
        LicenseManager.State state=LicenseManager.state(this);
        StringBuilder msg=new StringBuilder();
        msg.append("Durum: ").append(stateLabel(state));
        if(state==LicenseManager.State.TRIAL_ACTIVE){
            msg.append("\n").append(LicenseManager.remainingLabel(this));
        }
        long playExpiry=LicenseManager.playExpiryAtMs(this);
        if(playExpiry>System.currentTimeMillis()){
            msg.append("\nGoogle Play yıllık lisans bitişi: ")
               .append(DateFormat.getDateInstance(DateFormat.MEDIUM).format(new Date(playExpiry)));
        }
        msg.append("\n\nCihaz / Lisans Kimliği:\n").append(LicenseManager.installationId(this));
        new AlertDialog.Builder(this)
            .setTitle("MusaCAD Lisans Bilgisi")
            .setMessage(msg.toString())
            .setPositiveButton("TAMAM",null)
            .show();
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
            licenseCode.setError("Kod geçersiz, süresi dolmuş veya bu cihaza ait değil");
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

        new AlertDialog.Builder(this)
            .setTitle("MusaCAD lisans koşulları")
            .setView(scroll)
            .setPositiveButton("KABUL EDİYORUM",(d,w)->{
                LicenseManager.acceptTerms(this);
                if(startAfterAccept)startTrial();
            })
            .setNegativeButton("KAPAT",null)
            .show();
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
