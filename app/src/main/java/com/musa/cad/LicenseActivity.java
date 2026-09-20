package com.musa.cad;

import android.content.*;
import android.os.Bundle;
import android.view.View;
import android.widget.*;
import androidx.appcompat.app.AlertDialog;
import androidx.appcompat.app.AppCompatActivity;
import androidx.core.view.WindowCompat;
import java.io.*;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;

public class LicenseActivity extends AppCompatActivity {
    public static final String EXTRA_PENDING_INTENT="com.musa.cad.PENDING_INTENT";
    public static final String EXTRA_STAY_ON_LICENSE="com.musa.cad.STAY_ON_LICENSE";
    private final ExecutorService trialExecutor=Executors.newSingleThreadExecutor();
    private EditText licenseCode; private Intent pendingIntent; private volatile boolean trialRequestRunning;
    private boolean stayOnLicense;
    @Override protected void onCreate(Bundle savedInstanceState){
        super.onCreate(savedInstanceState);
        WindowCompat.setDecorFitsSystemWindows(getWindow(),false);
        setContentView(R.layout.activity_license);
        stayOnLicense=getIntent().getBooleanExtra(EXTRA_STAY_ON_LICENSE,false);
        if(android.os.Build.VERSION.SDK_INT>=33)pendingIntent=getIntent().getParcelableExtra(EXTRA_PENDING_INTENT,Intent.class);
        else { @SuppressWarnings("deprecation") Intent legacy=getIntent().getParcelableExtra(EXTRA_PENDING_INTENT);pendingIntent=legacy; }

        FrameLayout root=findViewById(R.id.licenseRoot),stage=findViewById(R.id.artworkStage);
        LockedScreenUi.fitStage(this,root,stage,()->{
            ImageView art=stage.findViewById(R.id.lockedArtwork);
            LockedScreenUi.loadArtwork(this,art,"locked/screen3",R.drawable.splash_scene_reference);

            licenseCode=new EditText(this);
            licenseCode.setSingleLine(true);
            licenseCode.setTextColor(0xFFFFFFFF); licenseCode.setTextSize(15f);
            licenseCode.setHint("Lisans Kodunu Girin"); licenseCode.setHintTextColor(0xFF7891A8);
            licenseCode.setPadding(dp(18),0,dp(18),0);
            licenseCode.setBackgroundColor(0xD9082036);
            stage.addView(licenseCode);
            LockedScreenUi.position(licenseCode,stage,132,714,666,92);

            LockedScreenUi.hotspot(this,stage,108,428,714,128,v->startTrial());
            LockedScreenUi.hotspot(this,stage,128,570,688,122,v->showInstallationId());
            LockedScreenUi.hotspot(this,stage,128,829,688,84,v->activate());
            LockedScreenUi.hotspot(this,stage,128,930,688,62,v->showTerms(false));
            LockedScreenUi.hotspot(this,stage,116,1245,620,58,v->showTerms(false));
            LockedScreenUi.hotspot(this,stage,65,1445,185,135,v->enterAppIfAllowed());
            LockedScreenUi.hotspot(this,stage,273,1445,185,135,v->enterAppIfAllowed());
            LockedScreenUi.hotspot(this,stage,482,1445,185,135,v->enterAppIfAllowed());
            LockedScreenUi.hotspot(this,stage,690,1445,185,135,v->enterAppIfAllowed());
        });
        refresh();
    }

    private int dp(int v){return Math.round(v*getResources().getDisplayMetrics().density);}
    private void refresh(){
        LicenseManager.State s=LicenseManager.state(this);
        if((s==LicenseManager.State.TRIAL_ACTIVE||s==LicenseManager.State.LICENSED)&&!stayOnLicense)enterApp();
    }
    private void startTrial(){
        if(trialRequestRunning)return;
        if(!LicenseManager.termsAccepted(this)){showTerms(true);return;}
        trialRequestRunning=true;
        trialExecutor.execute(()->{
            TrialService.Result r=TrialService.start(getApplicationContext());
            runOnUiThread(()->{
                trialRequestRunning=false;
                if(r.status==TrialService.Status.ACTIVATED){Toast.makeText(this,"1 günlük ücretsiz deneme etkinleştirildi",Toast.LENGTH_SHORT).show();enterAfterLicense();}
                else if(r.status==TrialService.Status.NOT_CONFIGURED)startLocalTrialFallback();
                else if(r.status==TrialService.Status.ALREADY_USED)Toast.makeText(this,"Bu cihaz ücretsiz denemeyi daha önce kullandı",Toast.LENGTH_LONG).show();
                else Toast.makeText(this,r.message==null?"Deneme başlatılamadı":r.message,Toast.LENGTH_LONG).show();
            });
        });
    }
    private void startLocalTrialFallback(){
        if(LicenseManager.startTrial(this)){Toast.makeText(this,"1 günlük ücretsiz deneme başlatıldı",Toast.LENGTH_SHORT).show();enterAfterLicense();}
        else Toast.makeText(this,"Bu cihazdaki ücretsiz deneme daha önce kullanılmış veya sona ermiş",Toast.LENGTH_LONG).show();
    }
    private void activate(){
        if(!LicenseManager.termsAccepted(this)){showTerms(false);Toast.makeText(this,"Lisans koşullarını kabul ettikten sonra tekrar Etkinleştir'e basın",Toast.LENGTH_LONG).show();return;}
        String code=licenseCode==null?"":licenseCode.getText().toString().trim();
        LicenseManager.ActivationResult r=LicenseManager.activateCode(this,code);
        if(r==LicenseManager.ActivationResult.ACTIVATED){Toast.makeText(this,"Lisans etkinleştirildi",Toast.LENGTH_SHORT).show();enterAfterLicense();}
        else { if(licenseCode!=null)licenseCode.setError("Kod geçersiz, süresi dolmuş veya bu cihaza ait değil"); }
    }
    private void showInstallationId(){
        final String id=LicenseManager.installationId(this);
        new AlertDialog.Builder(this).setTitle("Cihaz / Lisans Kimliği").setMessage(id)
            .setPositiveButton("KOPYALA",(d,w)->{
                ClipboardManager c=(ClipboardManager)getSystemService(Context.CLIPBOARD_SERVICE);
                c.setPrimaryClip(ClipData.newPlainText("MusaCAD Lisans Kimliği",id));
                Toast.makeText(this,"Lisans kimliği kopyalandı",Toast.LENGTH_SHORT).show();
            }).setNegativeButton("KAPAT",null).show();
    }
    private void showTerms(boolean startAfterAccept){
        TextView text=new TextView(this);int p=dp(18);text.setPadding(p,p,p,p);text.setText(readAsset("MUSACAD-LICENSE-TERMS-TR.txt"));text.setTextIsSelectable(true);text.setTextSize(12f);
        ScrollView scroll=new ScrollView(this);scroll.addView(text);
        new AlertDialog.Builder(this).setTitle("MusaCAD lisans koşulları").setView(scroll)
            .setPositiveButton("KABUL EDİYORUM",(d,w)->{LicenseManager.acceptTerms(this);if(startAfterAccept)startTrial();})
            .setNegativeButton("KAPAT",null).show();
    }
    private String readAsset(String name){
        try(InputStream in=getAssets().open(name);ByteArrayOutputStream out=new ByteArrayOutputStream()){
            byte[] b=new byte[4096];int n;while((n=in.read(b))!=-1)out.write(b,0,n);return out.toString("UTF-8");
        }catch(IOException e){return "Lisans koşulları okunamadı.";}
    }
    private void enterAfterLicense(){
        if(pendingIntent!=null){enterApp();return;}
        Intent about=new Intent(this,AboutActivity.class);about.putExtra(AboutActivity.EXTRA_CONTINUE_TO_APP,true);
        startActivity(about);overridePendingTransition(android.R.anim.fade_in,android.R.anim.fade_out);finish();
    }
    private void enterAppIfAllowed(){if(LicenseManager.hasAccess(this))enterApp();else Toast.makeText(this,"Önce denemeyi başlatın veya lisansı etkinleştirin",Toast.LENGTH_SHORT).show();}
    private void enterApp(){
        Intent next=pendingIntent==null?new Intent(this,MainActivity.class):new Intent(pendingIntent).setClass(this,MainActivity.class);
        next.addFlags(Intent.FLAG_ACTIVITY_CLEAR_TOP);startActivity(next);overridePendingTransition(android.R.anim.fade_in,android.R.anim.fade_out);finish();
    }
    @Override protected void onDestroy(){trialExecutor.shutdownNow();super.onDestroy();}
}
