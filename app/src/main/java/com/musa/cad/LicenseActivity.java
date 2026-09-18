package com.musa.cad;

import android.content.*;
import android.os.Bundle;
import android.view.View;
import android.widget.*;
import androidx.appcompat.app.AlertDialog;
import androidx.appcompat.app.AppCompatActivity;
import androidx.core.graphics.Insets;
import androidx.core.view.ViewCompat;
import androidx.core.view.WindowCompat;
import androidx.core.view.WindowInsetsCompat;
import java.io.*;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;

public class LicenseActivity extends AppCompatActivity {
    public static final String EXTRA_PENDING_INTENT="com.musa.cad.PENDING_INTENT";
    private final ExecutorService trialExecutor=Executors.newSingleThreadExecutor();
    private CheckBox termsCheck;
    private Button trialButton;
    private EditText licenseCode;
    private TextView status,message,installationId;
    private Intent pendingIntent;
    private volatile boolean trialRequestRunning;

    @Override protected void onCreate(Bundle savedInstanceState){
        super.onCreate(savedInstanceState);
        WindowCompat.setDecorFitsSystemWindows(getWindow(),false);
        setContentView(R.layout.activity_license);
        View root=findViewById(R.id.licenseRoot);
        ViewCompat.setOnApplyWindowInsetsListener(root,(v,insets)->{
            Insets bars=insets.getInsets(WindowInsetsCompat.Type.systemBars());
            v.setPadding(0,bars.top,0,bars.bottom);return insets;
        });

        if(android.os.Build.VERSION.SDK_INT>=33)pendingIntent=getIntent().getParcelableExtra(EXTRA_PENDING_INTENT,Intent.class);
        else {
            @SuppressWarnings("deprecation") Intent legacy=getIntent().getParcelableExtra(EXTRA_PENDING_INTENT);pendingIntent=legacy;
        }

        termsCheck=findViewById(R.id.termsCheck);
        trialButton=findViewById(R.id.trialButton);
        licenseCode=findViewById(R.id.licenseCode);
        status=findViewById(R.id.licenseStatus);
        message=findViewById(R.id.licenseMessage);
        installationId=findViewById(R.id.installationId);
        installationId.setText(LicenseManager.installationId(this));
        termsCheck.setChecked(LicenseManager.termsAccepted(this));

        findViewById(R.id.termsButton).setOnClickListener(v->showTerms());
        findViewById(R.id.copyInstallationIdButton).setOnClickListener(v->copyInstallationId());
        trialButton.setOnClickListener(v->startTrial());
        findViewById(R.id.activateButton).setOnClickListener(v->activate());
        refresh();
    }

    private void refresh(){
        LicenseManager.State s=LicenseManager.state(this);
        switch(s){
            case TRIAL_AVAILABLE:
                status.setText("1 gün ücretsiz deneyin");
                boolean configured=TrialService.isConfigured();
                trialButton.setEnabled(!trialRequestRunning);
                trialButton.setAlpha(trialButton.isEnabled()?1f:.45f);
                trialButton.setText(trialRequestRunning?"DENEME DOĞRULANIYOR…":"1 GÜNLÜK ÜCRETSİZ DENEMEYİ BAŞLAT");
                if(!configured)message.setText("1 günlük ücretsiz deneme bu cihazda 24 saatlik olarak başlatılır. Lisans kodu gerekmez.");
                break;
            case TRIAL_EXPIRED:
                status.setText("Ücretsiz deneme sona erdi");
                trialButton.setEnabled(false);trialButton.setAlpha(.45f);
                trialButton.setText("DENEME SÜRESİ KULLANILDI");break;
            case CLOCK_ERROR:
                status.setText("Cihaz saati doğrulanamadı");
                trialButton.setEnabled(false);trialButton.setAlpha(.45f);
                message.setText("Deneme süresi güvenliği için cihaz tarih/saatini otomatik ayara alın.");break;
            case TRIAL_ACTIVE:
            case LICENSED:
                enterApp();break;
        }
    }

    private void startTrial(){
        if(trialRequestRunning)return;
        if(!termsCheck.isChecked()){
            Toast.makeText(this,"Önce lisans ve deneme koşullarını kabul edin",Toast.LENGTH_LONG).show();return;
        }
        LicenseManager.acceptTerms(this);
        if(!TrialService.isConfigured()){
            if(LicenseManager.startTrial(this)){
                Toast.makeText(this,"1 günlük ücretsiz deneme etkinleştirildi",Toast.LENGTH_SHORT).show();enterApp();return;
            }
            LicenseManager.State state=LicenseManager.state(this);
            if(state==LicenseManager.State.TRIAL_ACTIVE){enterApp();return;}
            message.setText(state==LicenseManager.State.TRIAL_EXPIRED?"Bu cihaz 1 günlük ücretsiz denemeyi daha önce kullandı.":"1 günlük ücretsiz deneme başlatılamadı. Lütfen yeniden deneyin.");
            refresh();return;
        }
        trialRequestRunning=true;message.setText("Ücretsiz deneme cihaz için doğrulanıyor…");refresh();
        trialExecutor.execute(()->{
            TrialService.Result r=TrialService.start(getApplicationContext());
            runOnUiThread(()->{
                if(isFinishing()||isDestroyed())return;trialRequestRunning=false;
                switch(r.status){
                    case ACTIVATED:Toast.makeText(this,"1 günlük ücretsiz deneme etkinleştirildi",Toast.LENGTH_SHORT).show();enterApp();return;
                    case ALREADY_USED:message.setText("Bu cihaz 1 günlük ücretsiz denemeyi daha önce kullandı.");refresh();return;
                    case NOT_CONFIGURED:
                        if(LicenseManager.startTrial(this)){Toast.makeText(this,"1 günlük ücretsiz deneme etkinleştirildi",Toast.LENGTH_SHORT).show();enterApp();return;}
                        message.setText("1 günlük ücretsiz deneme başlatılamadı. Lütfen yeniden deneyin.");break;
                    case NETWORK_ERROR:message.setText("Ücretsiz denemeyi başlatmak için internet bağlantısını kontrol edin ve yeniden deneyin.");break;
                    case DENIED:message.setText(r.message==null?"Ücretsiz deneme isteği reddedildi.":r.message);break;
                    case INVALID_RESPONSE:message.setText("Deneme doğrulaması tamamlanamadı. Lütfen yeniden deneyin.");break;
                }
                refresh();
            });
        });
    }

    private void activate(){
        if(!termsCheck.isChecked()){
            Toast.makeText(this,"Önce lisans koşullarını kabul edin",Toast.LENGTH_LONG).show();return;
        }
        String code=licenseCode.getText().toString().trim();
        if(code.isEmpty()){
            licenseCode.setError("Lisans kodunu yapıştırın");
            licenseCode.requestFocus();
            message.setText("Lisans kodu, bu ekrandaki Cihaz / Lisans Kimliği için üretilmelidir.");
            return;
        }
        LicenseManager.ActivationResult r=LicenseManager.activateCode(this,code);
        if(r==LicenseManager.ActivationResult.ACTIVATED){LicenseManager.acceptTerms(this);Toast.makeText(this,"Lisans etkinleştirildi",Toast.LENGTH_SHORT).show();enterApp();return;}
        licenseCode.setError("Kod geçersiz, süresi dolmuş veya bu cihaza ait değil");
        message.setText("Lisans kodu bu ekrandaki Cihaz/Lisans Kimliği için üretilmelidir.");
    }

    private void copyInstallationId(){
        String id=LicenseManager.installationId(this);
        ClipboardManager clipboard=(ClipboardManager)getSystemService(Context.CLIPBOARD_SERVICE);
        clipboard.setPrimaryClip(ClipData.newPlainText("MusaCAD Lisans Kimliği",id));
        Toast.makeText(this,"Lisans kimliği kopyalandı",Toast.LENGTH_SHORT).show();
    }

    private void enterApp(){
        Intent next=pendingIntent==null?new Intent(this,MainActivity.class):new Intent(pendingIntent).setClass(this,MainActivity.class);
        next.addFlags(Intent.FLAG_ACTIVITY_CLEAR_TOP);startActivity(next);
        overridePendingTransition(android.R.anim.fade_in,android.R.anim.fade_out);finish();
    }

    private void showTerms(){
        TextView text=new TextView(this);int p=Math.round(18*getResources().getDisplayMetrics().density);text.setPadding(p,p,p,p);
        text.setText(readAsset("MUSACAD-LICENSE-TERMS-TR.txt"));text.setTextIsSelectable(true);text.setTextSize(12f);
        ScrollView scroll=new ScrollView(this);scroll.addView(text);
        new AlertDialog.Builder(this).setTitle("MusaCAD lisans koşulları").setView(scroll)
            .setPositiveButton("KABUL EDİYORUM",(d,w)->{termsCheck.setChecked(true);LicenseManager.acceptTerms(this);})
            .setNegativeButton("KAPAT",null).show();
    }

    private String readAsset(String name){
        try(InputStream in=getAssets().open(name);ByteArrayOutputStream out=new ByteArrayOutputStream()){
            byte[] b=new byte[4096];int n;while((n=in.read(b))!=-1)out.write(b,0,n);return out.toString("UTF-8");
        }catch(IOException e){return "Lisans koşulları okunamadı.";}
    }

    @Override protected void onDestroy(){trialExecutor.shutdownNow();super.onDestroy();}
}
