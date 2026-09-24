package com.musa.cad;

import android.content.*;
import android.os.Bundle;
import android.view.Gravity;
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
    private TextView deviceCode;
    private Intent pendingIntent;
    private volatile boolean trialRequestRunning;
    private boolean stayOnLicense;

    @Override protected void onCreate(Bundle savedInstanceState){
        super.onCreate(savedInstanceState);
        LockedScreenUi.enableImmersive(this);
        setContentView(R.layout.activity_license);

        stayOnLicense=getIntent().getBooleanExtra(EXTRA_STAY_ON_LICENSE,false);
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

            String installationId=LicenseManager.installationId(this);
            deviceCode=new TextView(this);
            deviceCode.setText(DeviceIdentityHash.isValidPublicId(installationId)
                ?"Cihaz Kodu: "+installationId+"\nKopyalamak için dokunun"
                :"Cihaz Kodu: ALINAMADI");
            deviceCode.setTextColor(DeviceIdentityHash.isValidPublicId(installationId)?0xFFE7F2FF:0xFFFF9B9B);
            deviceCode.setTextSize(11f);
            deviceCode.setGravity(Gravity.CENTER_VERTICAL);
            deviceCode.setPadding(dp(12),0,dp(12),0);
            deviceCode.setBackgroundColor(0xB50A2440);
            deviceCode.setTextIsSelectable(true);
            deviceCode.setOnClickListener(v->copyDeviceCode());
            stage.addView(deviceCode);
            LockedScreenUi.position(deviceCode,stage,250,640,545,64);

            licenseCode=new EditText(this);
            licenseCode.setSingleLine(true);
            licenseCode.setTextColor(0xFFFFFFFF);
            licenseCode.setTextSize(15f);
            licenseCode.setHint("Lisans kodunu girin");
            licenseCode.setHintTextColor(0xFF7593B4);
            licenseCode.setPadding(dp(15),0,dp(15),0);
            licenseCode.setBackgroundColor(0xB50A2440);
            stage.addView(licenseCode);
            LockedScreenUi.position(licenseCode,stage,250,718,545,92);

            LockedScreenUi.hotspot(this,stage,90,402,752,150,v->startTrial());
            LockedScreenUi.hotspot(this,stage,128,829,676,88,v->activate());
            LockedScreenUi.hotspot(this,stage,128,932,676,64,v->showTerms(false));
            LockedScreenUi.hotspot(this,stage,96,1246,632,74,v->showTerms(false));
        });

        refresh();
    }

    @Override public void onWindowFocusChanged(boolean hasFocus){
        super.onWindowFocusChanged(hasFocus);
        if(hasFocus)LockedScreenUi.enableImmersive(this);
    }

    private int dp(int v){return Math.round(v*getResources().getDisplayMetrics().density);}

    private void refresh(){
        LicenseManager.State s=LicenseManager.state(this);
        if((s==LicenseManager.State.TRIAL_ACTIVE||s==LicenseManager.State.LICENSED)&&!stayOnLicense)enterApp();
    }

    private void startTrial(){
        if(trialRequestRunning)return;
        String id=LicenseManager.installationId(this);
        if(!DeviceIdentityHash.isValidPublicId(id)){
            Toast.makeText(this,"Kalıcı cihaz kimliği alınamadığı için ücretsiz deneme güvenlik nedeniyle başlatılamaz",Toast.LENGTH_LONG).show();
            return;
        }
        if(!LicenseManager.termsAccepted(this)){showTerms(true);return;}

        trialRequestRunning=true;
        trialExecutor.execute(()->{
            TrialService.Result r=TrialService.start(getApplicationContext());
            runOnUiThread(()->{
                trialRequestRunning=false;
                if(r.status==TrialService.Status.ACTIVATED){
                    Toast.makeText(this,"1 günlük ücretsiz deneme etkinleştirildi",Toast.LENGTH_SHORT).show();
                    enterAfterLicense();
                }else if(r.status==TrialService.Status.ALREADY_USED){
                    Toast.makeText(this,"Bu cihaz ücretsiz denemeyi daha önce kullandı",Toast.LENGTH_LONG).show();
                }else if(r.status==TrialService.Status.NOT_CONFIGURED){
                    Toast.makeText(this,"Deneme hizmeti yapılandırılmamış. Güvenlik nedeniyle yerel deneme başlatılmadı.",Toast.LENGTH_LONG).show();
                }else{
                    Toast.makeText(this,r.message==null?"Deneme başlatılamadı":r.message,Toast.LENGTH_LONG).show();
                }
            });
        });
    }

    private void activate(){
        String id=LicenseManager.installationId(this);
        if(!DeviceIdentityHash.isValidPublicId(id)){
            Toast.makeText(this,"Cihaz kimliği alınamadığı için lisans etkinleştirilemiyor",Toast.LENGTH_LONG).show();
            return;
        }
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

    private void copyDeviceCode(){
        String id=LicenseManager.installationId(this);
        if(!DeviceIdentityHash.isValidPublicId(id)){
            Toast.makeText(this,"Kopyalanabilir cihaz kodu yok",Toast.LENGTH_SHORT).show();
            return;
        }
        ClipboardManager clipboard=(ClipboardManager)getSystemService(Context.CLIPBOARD_SERVICE);
        if(clipboard!=null)clipboard.setPrimaryClip(ClipData.newPlainText("MusaCAD Cihaz Kodu",id));
        Toast.makeText(this,"Cihaz kodu kopyalandı",Toast.LENGTH_SHORT).show();
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
            byte[] b=new byte[4096];int n;
            while((n=in.read(b))!=-1)out.write(b,0,n);
            return out.toString("UTF-8");
        }catch(IOException e){return "Lisans koşulları okunamadı.";}
    }

    private void enterAfterLicense(){enterApp();}

    private void enterApp(){
        Intent next=pendingIntent==null?new Intent(this,MainActivity.class):new Intent(pendingIntent).setClass(this,MainActivity.class);
        next.addFlags(Intent.FLAG_ACTIVITY_CLEAR_TOP);
        startActivity(next);
        overridePendingTransition(android.R.anim.fade_in,android.R.anim.fade_out);
        finish();
    }

    @Override protected void onDestroy(){
        trialExecutor.shutdownNow();
        super.onDestroy();
    }
}
