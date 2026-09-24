package com.musa.cad;

import android.content.*;
import android.os.Bundle;
import android.widget.*;
import androidx.appcompat.app.AlertDialog;
import androidx.appcompat.app.AppCompatActivity;
import java.io.*;
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

    @Override protected void onCreate(Bundle savedInstanceState){
        super.onCreate(savedInstanceState);
        LockedScreenUi.enableImmersive(this);
        setContentView(R.layout.activity_license);

        stayOnLicense=getIntent().getBooleanExtra(EXTRA_STAY_ON_LICENSE,false);
        playBilling=new PlayBillingManager(this,new PlayBillingManager.Listener(){
            @Override public void onProductReady(boolean ready,String displayPrice){
                if(playPurchaseButton==null)return;
                playPurchaseButton.setEnabled(ready);
                playPurchaseButton.setText(ready&&displayPrice!=null&&!displayPrice.isEmpty()
                    ?"GOOGLE PLAY İLE SATIN AL • "+displayPrice
                    :"GOOGLE PLAY İLE SATIN AL");
            }
            @Override public void onEntitlementChanged(boolean active){
                if(active&&!stayOnLicense){
                    Toast.makeText(LicenseActivity.this,"Google Play satın alımı doğrulandı",Toast.LENGTH_SHORT).show();
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

        LockedScreenUi.fillStage(this,root,stage,()->{
            ImageView art=stage.findViewById(R.id.lockedArtwork);
            art.setImageResource(R.drawable.musacad_screen_3);

            // Görseldeki gerçek lisans kodu kutusunun üstündeki canlı EditText
            licenseCode=new EditText(this);
            licenseCode.setSingleLine(true);
            licenseCode.setTextColor(0xFFFFFFFF);
            licenseCode.setTextSize(15f);
            licenseCode.setHint("XXXX-XXXX-XXXX-XXXX");
            licenseCode.setHintTextColor(0xFF7593B4);
            licenseCode.setPadding(dp(15),0,dp(15),0);
            licenseCode.setBackgroundColor(0xB50A2440);
            stage.addView(licenseCode);
            LockedScreenUi.position(licenseCode,stage,250,718,545,92);

            // Lisans üreticide kullanılacak telefona özel kimlik. Dokununca panoya kopyalanır.
            TextView deviceId=new TextView(this);
            String deviceLicenseId=LicenseManager.installationId(this);
            deviceId.setText("Cihaz / Lisans Kimliği (dokun-kopyala)\n"+deviceLicenseId);
            deviceId.setTextColor(0xFFFFFFFF);
            deviceId.setTextSize(12f);
            deviceId.setGravity(android.view.Gravity.CENTER);
            deviceId.setPadding(dp(12),dp(6),dp(12),dp(6));
            deviceId.setBackgroundColor(0xB50A2440);
            deviceId.setTextIsSelectable(true);
            deviceId.setOnClickListener(v->{
                ClipboardManager clipboard=(ClipboardManager)getSystemService(Context.CLIPBOARD_SERVICE);
                if(clipboard!=null){
                    clipboard.setPrimaryClip(ClipData.newPlainText("MusaCAD Cihaz Kimliği",deviceLicenseId));
                    Toast.makeText(this,"Cihaz kimliği kopyalandı",Toast.LENGTH_SHORT).show();
                }
            });
            stage.addView(deviceId);
            LockedScreenUi.position(deviceId,stage,128,1015,676,105);

            playPurchaseButton=new Button(this);
            playPurchaseButton.setText("GOOGLE PLAY İLE SATIN AL");
            playPurchaseButton.setTextSize(13f);
            playPurchaseButton.setAllCaps(false);
            playPurchaseButton.setTextColor(0xFFFFFFFF);
            playPurchaseButton.setBackgroundColor(0xFF1263A8);
            playPurchaseButton.setEnabled(false);
            playPurchaseButton.setOnClickListener(v->{
                if(!LicenseManager.termsAccepted(this)){
                    showTerms(false);
                    Toast.makeText(this,"Satın almadan önce lisans koşullarını kabul edin",Toast.LENGTH_LONG).show();
                    return;
                }
                playBilling.launchPurchase(this);
            });
            stage.addView(playPurchaseButton);
            LockedScreenUi.position(playPurchaseButton,stage,128,1135,676,80);

            // Görseldeki gerçek butonların tam üstündeki şeffaf tıklama katmanları
            LockedScreenUi.hotspot(this,stage,90,402,752,150,v->startTrial());
            LockedScreenUi.hotspot(this,stage,128,829,676,88,v->activate());
            LockedScreenUi.hotspot(this,stage,128,932,676,64,v->showTerms(false));
            LockedScreenUi.hotspot(this,stage,96,1246,632,74,v->showTerms(false));
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
        if(playBilling!=null)playBilling.refresh();
    }

    @Override protected void onDestroy(){
        trialExecutor.shutdownNow();
        if(playBilling!=null)playBilling.close();
        super.onDestroy();
    }
}
