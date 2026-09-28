package com.musa.cad.licensemanager;

import android.app.KeyguardManager;
import android.content.ClipData;
import android.content.ClipboardManager;
import android.content.Context;
import android.content.Intent;
import android.graphics.Color;
import android.graphics.Typeface;
import android.graphics.drawable.GradientDrawable;
import android.net.Uri;
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
import java.io.ByteArrayOutputStream;
import java.io.InputStream;
import java.io.OutputStream;
import java.nio.charset.StandardCharsets;
import java.util.Arrays;
import java.util.List;

public class MainActivity extends AppCompatActivity {
    private static final int AUTH_ISSUE=71,CREATE_BACKUP=72,OPEN_BACKUP=73;
    private EditText customer,identityField;
    private Spinner duration,mode;
    private TextView tokenView,historyView,keyStatus;
    private String pendingCustomer,pendingIdentity,pendingLabel;
    private int pendingDays;
    private char[] pendingBackupPassword;
    private boolean pendingNewKey,pendingExistingBackup;

    @Override protected void onCreate(Bundle state){
        super.onCreate(state);
        setContentView(buildUi());
        refreshHistory();
        refreshKeyStatus();
    }

    private View buildUi(){
        ScrollView scroll=new ScrollView(this);
        scroll.setFillViewport(true);
        scroll.setClipToPadding(false);
        LinearLayout root=new LinearLayout(this);
        root.setOrientation(LinearLayout.VERTICAL);
        root.setPadding(dp(18),dp(20),dp(18),dp(28));
        root.setBackgroundColor(Color.rgb(7,16,21));
        scroll.addView(root,new ScrollView.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT,ViewGroup.LayoutParams.WRAP_CONTENT));

        root.addView(text("MusaCAD Lisans Yönetici",24,Color.WHITE,true));
        TextView sub=text(BuildConfig.ALLOW_LEGACY_SHORT_LICENSE
            ?"Ticari kullanım için RSA imzalı MC1 lisansı üret. 12 haneli kısa kod yalnız geliştirme/uyumluluk içindir."
            :"Production modu: yalnız RSA imzalı MC1 lisansı üretir.",13,Color.rgb(161,183,196),false);
        sub.setPadding(0,dp(4),0,dp(12));root.addView(sub);

        section(root,"Güvenli imza anahtarı");
        keyStatus=text("",11,Color.rgb(175,205,215),false);
        keyStatus.setTextIsSelectable(true);keyStatus.setLineSpacing(dp(2),1f);keyStatus.setPadding(0,0,0,dp(9));root.addView(keyStatus,matchWrap());

        LinearLayout keyRow1=new LinearLayout(this);keyRow1.setOrientation(LinearLayout.HORIZONTAL);
        Button createKey=button("YENİ ANAHTAR");createKey.setOnClickListener(v->confirmNewKey());
        Button backupKey=button("YEDEKLE");backupKey.setOnClickListener(v->backupExistingKey());
        keyRow1.addView(createKey,new LinearLayout.LayoutParams(0,dp(48),1f));
        keyRow1.addView(space(8),new LinearLayout.LayoutParams(dp(8),1));
        keyRow1.addView(backupKey,new LinearLayout.LayoutParams(0,dp(48),1f));
        root.addView(keyRow1,matchWrap());

        LinearLayout keyRow2=new LinearLayout(this);keyRow2.setOrientation(LinearLayout.HORIZONTAL);
        Button restoreKey=button("YEDEKTEN DÖN");restoreKey.setOnClickListener(v->openBackupForRestore());
        Button publicKey=button("PUBLIC KEY");publicKey.setOnClickListener(v->sharePublicKey());
        keyRow2.addView(restoreKey,new LinearLayout.LayoutParams(0,dp(48),1f));
        keyRow2.addView(space(8),new LinearLayout.LayoutParams(dp(8),1));
        keyRow2.addView(publicKey,new LinearLayout.LayoutParams(0,dp(48),1f));
        LinearLayout.LayoutParams kr2=matchWrap();kr2.topMargin=dp(7);root.addView(keyRow2,kr2);

        TextView keyNote=text("Yeni anahtar oluştururken önce parola korumalı .mlk yedeği kaydedilir; dosya başarıyla yazılmadan anahtar etkinleşmez. Public key, MusaCAD release içindeki MUSACAD-LICENSE-PUBLIC.pem ile aynı olmalıdır.",11,Color.rgb(133,156,169),false);
        keyNote.setPadding(0,dp(8),0,dp(8));root.addView(keyNote);

        customer=input("Müşteri adı (isteğe bağlı)");
        root.addView(customer,matchWrap());

        section(root,"Lisans türü");
        mode=new Spinner(this);
        String[] modes=BuildConfig.ALLOW_LEGACY_SHORT_LICENSE
            ?new String[]{"Güvenli MC1 • Önerilen","12 haneli kısa kod • Uyumluluk"}
            :new String[]{"Güvenli MC1 • Production"};
        mode.setAdapter(new ArrayAdapter<>(this,android.R.layout.simple_spinner_dropdown_item,modes));
        root.addView(mode,matchWrap());

        section(root,"Cihaz kimliği");
        TextView identityHelp=text("MusaCAD > Lisans Bilgisi ekranındaki Güvenli Lisans Kimliğini buraya yapıştırın.",11,Color.rgb(150,174,187),false);
        identityHelp.setPadding(0,0,0,dp(6));root.addView(identityHelp,matchWrap());
        identityField=input("Güvenli Lisans Kimliğini yapıştır");
        identityField.setSingleLine(true);identityField.setTextSize(13f);
        TextViewCompat.setAutoSizeTextTypeUniformWithConfiguration(identityField,10,13,1,TypedValue.COMPLEX_UNIT_SP);
        root.addView(identityField,matchWrap());
        mode.setOnItemSelectedListener(new android.widget.AdapterView.OnItemSelectedListener(){
            @Override public void onItemSelected(android.widget.AdapterView<?> parent,View view,int position,long id){
                boolean secure=position==0;
                identityHelp.setText(secure
                    ?"MusaCAD > Lisans Bilgisi ekranındaki Güvenli Lisans Kimliğini buraya yapıştırın."
                    :"MusaCAD'deki 12 haneli Serial bilgisini buraya girin.");
                identityField.setHint(secure?"Güvenli Lisans Kimliğini yapıştır":"12 haneli Serial");
                tokenView.setTextSize(secure?11f:24f);
            }
            @Override public void onNothingSelected(android.widget.AdapterView<?> parent){}
        });

        section(root,"Lisans süresi");
        duration=new Spinner(this);
        duration.setAdapter(new ArrayAdapter<>(this,android.R.layout.simple_spinner_dropdown_item,
            new String[]{"30 gün","90 gün","365 gün","Süresiz"}));
        duration.setSelection(2);root.addView(duration,matchWrap());

        Button generate=button("LİSANS ÜRET");
        generate.setBackground(round(Color.rgb(25,181,165),12));
        generate.setOnClickListener(v->requestIssue());root.addView(generate,buttonParams());

        section(root,"Lisans");
        tokenView=text("Henüz üretilmedi",11,Color.WHITE,true);
        tokenView.setGravity(Gravity.CENTER);tokenView.setTextIsSelectable(true);
        tokenView.setSingleLine(false);tokenView.setMaxLines(8);
        tokenView.setPadding(dp(12),dp(18),dp(12),dp(18));tokenView.setBackground(round(Color.rgb(15,28,35),10));
        root.addView(tokenView,matchWrap());

        LinearLayout actions=new LinearLayout(this);actions.setOrientation(LinearLayout.HORIZONTAL);
        Button copy=button("KOPYALA");copy.setOnClickListener(v->copyToken());
        Button share=button("PAYLAŞ");share.setOnClickListener(v->shareToken());
        actions.addView(copy,new LinearLayout.LayoutParams(0,dp(50),1f));actions.addView(space(8),new LinearLayout.LayoutParams(dp(8),1));actions.addView(share,new LinearLayout.LayoutParams(0,dp(50),1f));
        LinearLayout.LayoutParams ap=matchWrap();ap.topMargin=dp(8);root.addView(actions,ap);

        section(root,"Son işlemler");
        historyView=text("",12,Color.rgb(175,195,205),false);historyView.setTextIsSelectable(true);root.addView(historyView,matchWrap());
        return scroll;
    }

    private void requestIssue(){
        boolean secure=mode.getSelectedItemPosition()==0;
        String raw=identityField.getText().toString().trim();
        try{
            if(secure){
                if(!LicenseKeyStore.hasKey(this))throw new IllegalStateException("Önce güvenli RSA imza anahtarı oluşturun veya yedekten geri yükleyin");
                if(!productionKeyMatches())throw new IllegalStateException("Aktif RSA anahtarı MusaCAD release public key'i ile eşleşmiyor. PUBLIC KEY'i ana uygulamaya aktarın ve License Manager'ı yeniden derleyin.");
                pendingIdentity=LicenseIssuer.normalize(raw);
            }else{
                if(!BuildConfig.ALLOW_LEGACY_SHORT_LICENSE)throw new IllegalStateException("Production License Manager yalnız MC1 lisansı üretir");
                pendingIdentity=ShortLicenseCode.normalizeSerial(raw);
            }
        }catch(Exception e){identityField.setError(e.getMessage());return;}

        int pos=duration.getSelectedItemPosition();int[] days={30,90,365,0};
        pendingDays=days[Math.max(0,Math.min(pos,days.length-1))];
        pendingLabel=duration.getSelectedItem().toString();pendingCustomer=customer.getText().toString().trim();

        KeyguardManager km=(KeyguardManager)getSystemService(KEYGUARD_SERVICE);
        if(km!=null&&km.isDeviceSecure()){
            Intent auth=km.createConfirmDeviceCredentialIntent("Lisans üretimi","MusaCAD lisansı oluşturmak için telefon kilidini doğrulayın.");
            if(auth!=null){startActivityForResult(auth,AUTH_ISSUE);return;}
        }
        issueNow();
    }

    private void issueNow(){
        try{
            boolean secure=mode.getSelectedItemPosition()==0;
            if(!secure&&!BuildConfig.ALLOW_LEGACY_SHORT_LICENSE)throw new IllegalStateException("Production License Manager yalnız MC1 lisansı üretir");
            String token=secure?LicenseIssuer.issue(this,pendingIdentity,pendingDays):ShortLicenseCode.issue(pendingIdentity,pendingDays,System.currentTimeMillis());
            tokenView.setText(token);tokenView.setTextSize(secure?11f:24f);
            LicenseHistory.add(this,pendingCustomer,pendingIdentity,pendingLabel,secure?"MC1":"Kısa kod",token);
            refreshHistory();
            Toast.makeText(this,secure?"RSA imzalı MC1 lisansı oluşturuldu":"12 haneli uyumluluk kodu oluşturuldu",Toast.LENGTH_SHORT).show();
        }catch(Exception e){showError("Lisans oluşturulamadı: "+e.getMessage());}
    }

    private void confirmNewKey(){
        String extra=LicenseKeyStore.hasKey(this)?"\n\nMevcut anahtar değişirse MusaCAD release public key'i de yeni anahtarla güncellenmelidir. Eski anahtar yedeğini kaybetmeyin.":"";
        AlertDialog d=new AlertDialog.Builder(this).setTitle("Yeni RSA imza anahtarı")
            .setMessage("Yeni anahtar yalnız parola korumalı yedek dosyası başarıyla kaydedilirse etkinleşir."+extra)
            .setNegativeButton("İPTAL",null)
            .setPositiveButton("DEVAM",(dialog,w)->askBackupPassword(true))
            .create();
        d.setOnShowListener(x->styleDialogButtons(d));d.show();
    }

    private void backupExistingKey(){
        if(!LicenseKeyStore.hasKey(this)){showError("Yedeklenecek güvenli anahtar yok");return;}
        askBackupPassword(false);
    }

    private void askBackupPassword(boolean newKey){
        LinearLayout box=passwordBox(true);
        EditText p1=(EditText)box.getChildAt(0),p2=(EditText)box.getChildAt(1);
        AlertDialog dialog=new AlertDialog.Builder(this)
            .setTitle(newKey?"Yeni anahtar yedek parolası":"Anahtar yedek parolası")
            .setMessage("En az 12 karakter kullanın. Bu parola kurtarma için zorunludur ve MusaCAD tarafından geri alınamaz.")
            .setView(box).setNegativeButton("İPTAL",null).setPositiveButton("KAYDET",null).create();
        dialog.setOnShowListener(x->{styleDialogButtons(dialog);dialog.getButton(AlertDialog.BUTTON_POSITIVE).setOnClickListener(v->{
            String a=p1.getText().toString(),b=p2.getText().toString();
            if(a.length()<12){p1.setError("En az 12 karakter");return;}
            if(!a.equals(b)){p2.setError("Parolalar eşleşmiyor");return;}
            clearPendingPassword();pendingBackupPassword=a.toCharArray();pendingNewKey=newKey;pendingExistingBackup=!newKey;
            Intent create=new Intent(Intent.ACTION_CREATE_DOCUMENT).addCategory(Intent.CATEGORY_OPENABLE).setType("application/octet-stream");
            create.putExtra(Intent.EXTRA_TITLE,"MusaCAD-Lisans-Anahtari-"+System.currentTimeMillis()+".mlk");
            startActivityForResult(create,CREATE_BACKUP);dialog.dismiss();
        }));
        dialog.show();
    }

    private void openBackupForRestore(){
        Intent open=new Intent(Intent.ACTION_OPEN_DOCUMENT).addCategory(Intent.CATEGORY_OPENABLE).setType("*/*");
        startActivityForResult(open,OPEN_BACKUP);
    }

    private void restoreFromText(String backup){
        LinearLayout box=passwordBox(false);EditText p=(EditText)box.getChildAt(0);
        AlertDialog dialog=new AlertDialog.Builder(this).setTitle("Anahtar yedeğini geri yükle")
            .setMessage("Yedeği oluştururken kullandığınız parolayı girin.")
            .setView(box).setNegativeButton("İPTAL",null).setPositiveButton("GERİ YÜKLE",null).create();
        dialog.setOnShowListener(x->dialog.getButton(AlertDialog.BUTTON_POSITIVE).setOnClickListener(v->{
            char[] password=p.getText().toString().toCharArray();
            try{
                LicenseKeyStore.restore(this,backup,password);refreshKeyStatus();
                Toast.makeText(this,"Güvenli lisans anahtarı geri yüklendi",Toast.LENGTH_LONG).show();dialog.dismiss();
            }catch(Exception e){p.setError("Yedek açılamadı: "+e.getMessage());}
            finally{Arrays.fill(password,'\0');}
        });});dialog.show();
    }

    private void sharePublicKey(){
        if(!LicenseKeyStore.hasKey(this)){showError("Önce güvenli RSA anahtarı oluşturun veya geri yükleyin");return;}
        try{
            String pem=LicenseKeyStore.publicKeyPem(this);
            Intent send=new Intent(Intent.ACTION_SEND);send.setType("text/plain");
            send.putExtra(Intent.EXTRA_SUBJECT,"MusaCAD License Public Key");
            send.putExtra(Intent.EXTRA_TEXT,pem);
            startActivity(Intent.createChooser(send,"Public key paylaş"));
        }catch(Exception e){showError("Public key okunamadı: "+e.getMessage());}
    }

    @Override protected void onActivityResult(int requestCode,int resultCode,Intent data){
        super.onActivityResult(requestCode,resultCode,data);
        if(requestCode==AUTH_ISSUE){if(resultCode==RESULT_OK)issueNow();return;}
        if(requestCode==CREATE_BACKUP){
            if(resultCode==RESULT_OK&&data!=null&&data.getData()!=null&&pendingBackupPassword!=null){
                Uri uri=data.getData();
                try{
                    String backup=pendingNewKey?LicenseKeyStore.generateBackup(pendingBackupPassword):LicenseKeyStore.backup(this,pendingBackupPassword);
                    writeText(uri,backup);
                    if(pendingNewKey)LicenseKeyStore.restore(this,backup,pendingBackupPassword);
                    refreshKeyStatus();
                    Toast.makeText(this,pendingNewKey?"Yeni anahtar yedeklendi ve etkinleştirildi":"Anahtar yedeği kaydedildi",Toast.LENGTH_LONG).show();
                }catch(Exception e){showError("Anahtar yedeği kaydedilemedi; yeni anahtar etkinleştirilmedi: "+e.getMessage());}
            }
            clearBackupOperation();return;
        }
        if(requestCode==OPEN_BACKUP&&resultCode==RESULT_OK&&data!=null&&data.getData()!=null){
            try{restoreFromText(readText(data.getData()));}
            catch(Exception e){showError("Yedek dosyası okunamadı: "+e.getMessage());}
        }
    }

    private void refreshKeyStatus(){
        if(!LicenseKeyStore.hasKey(this)){keyStatus.setText("Durum: Güvenli MC1 imza anahtarı YOK\nİlk kurulum: YENİ ANAHTAR ile .mlk yedeği oluşturun, PUBLIC KEY'i MusaCAD'e aktarın ve iki uygulamayı yeniden production derleyin.\nMusaCAD beklenen fingerprint: "+BuildConfig.EXPECTED_LICENSE_KEY_FINGERPRINT);return;}
        try{
            String current=LicenseKeyStore.fingerprint(this);
            boolean match=current.equalsIgnoreCase(BuildConfig.EXPECTED_LICENSE_KEY_FINGERPRINT);
            keyStatus.setText("Durum: MC1 anahtarı "+(match?"HAZIR / EŞLEŞTİ":"VAR / PUBLIC KEY UYUMSUZ")+
                "\nAktif: "+current+"\nMusaCAD: "+BuildConfig.EXPECTED_LICENSE_KEY_FINGERPRINT);
        }catch(Exception e){keyStatus.setText("Durum: Anahtar okunamadı • "+e.getMessage());}
    }

    private boolean productionKeyMatches(){
        try{
            String expected=BuildConfig.EXPECTED_LICENSE_KEY_FINGERPRINT==null?"":BuildConfig.EXPECTED_LICENSE_KEY_FINGERPRINT.trim();
            return !expected.isEmpty()&&LicenseKeyStore.fingerprint(this).equalsIgnoreCase(expected);
        }catch(Exception e){return false;}
    }

    private void copyToken(){
        String token=tokenView.getText().toString().trim();
        boolean valid=LicenseIssuer.isToken(token)||ShortLicenseCode.isCode(token);
        if(!valid){Toast.makeText(this,"Önce lisans üretin",Toast.LENGTH_SHORT).show();return;}
        ClipboardManager cm=(ClipboardManager)getSystemService(CLIPBOARD_SERVICE);
        if(cm!=null)cm.setPrimaryClip(ClipData.newPlainText("MusaCAD Lisans",token));
        Toast.makeText(this,"Lisans kopyalandı",Toast.LENGTH_SHORT).show();
    }

    private void shareToken(){
        String token=tokenView.getText().toString().trim();
        if(!LicenseIssuer.isToken(token)&&!ShortLicenseCode.isCode(token)){Toast.makeText(this,"Önce lisans üretin",Toast.LENGTH_SHORT).show();return;}
        StringBuilder body=new StringBuilder();
        if(pendingCustomer!=null&&!pendingCustomer.isEmpty())body.append("Müşteri: ").append(pendingCustomer).append("\n");
        body.append("Cihaz: ").append(pendingIdentity).append("\nLisans: ").append(token);
        Intent send=new Intent(Intent.ACTION_SEND);send.setType("text/plain");send.putExtra(Intent.EXTRA_TEXT,body.toString());
        startActivity(Intent.createChooser(send,"Lisansı paylaş"));
    }

    private void refreshHistory(){
        List<String> rows=LicenseHistory.labels(this);
        if(rows.isEmpty()){historyView.setText("Henüz lisans üretilmedi.");return;}
        StringBuilder b=new StringBuilder();for(int i=0;i<Math.min(rows.size(),10);i++){if(i>0)b.append("\n");b.append("• ").append(rows.get(i));}
        historyView.setText(b.toString());
    }

    private LinearLayout passwordBox(boolean confirm){
        LinearLayout box=new LinearLayout(this);box.setOrientation(LinearLayout.VERTICAL);box.setPadding(dp(20),dp(8),dp(20),0);
        EditText p1=input("Yedek parolası");p1.setInputType(InputType.TYPE_CLASS_TEXT|InputType.TYPE_TEXT_VARIATION_PASSWORD);box.addView(p1,matchWrap());
        if(confirm){EditText p2=input("Parolayı tekrar girin");p2.setInputType(InputType.TYPE_CLASS_TEXT|InputType.TYPE_TEXT_VARIATION_PASSWORD);box.addView(p2,matchWrap());}
        return box;
    }

    private void writeText(Uri uri,String value)throws Exception{
        try(OutputStream out=getContentResolver().openOutputStream(uri,"wt")){
            if(out==null)throw new IllegalStateException("Dosya açılamadı");
            out.write(value.getBytes(StandardCharsets.UTF_8));out.flush();
        }
    }

    private String readText(Uri uri)throws Exception{
        try(InputStream in=getContentResolver().openInputStream(uri);ByteArrayOutputStream out=new ByteArrayOutputStream()){
            if(in==null)throw new IllegalStateException("Dosya açılamadı");
            byte[] buf=new byte[4096];int n,total=0;
            while((n=in.read(buf))!=-1){total+=n;if(total>128*1024)throw new IllegalStateException("Yedek dosyası çok büyük");out.write(buf,0,n);}
            return out.toString(StandardCharsets.UTF_8.name());
        }
    }

    private void clearBackupOperation(){clearPendingPassword();pendingNewKey=false;pendingExistingBackup=false;}
    private void clearPendingPassword(){if(pendingBackupPassword!=null){Arrays.fill(pendingBackupPassword,'\0');pendingBackupPassword=null;}}

    @Override protected void onDestroy(){clearBackupOperation();super.onDestroy();}

    private void section(LinearLayout root,String label){TextView v=text(label,15,Color.WHITE,true);v.setPadding(0,dp(18),0,dp(7));root.addView(v);}
    private EditText input(String hint){
        EditText e=new EditText(this);e.setHint(hint);e.setTextColor(Color.WHITE);e.setHintTextColor(Color.rgb(128,151,164));
        e.setTextSize(13f);e.setInputType(InputType.TYPE_CLASS_TEXT);e.setSingleLine(true);
        e.setMinHeight(dp(50));e.setPadding(dp(12),0,dp(12),0);
        GradientDrawable bg=round(Color.rgb(12,31,40),10);bg.setStroke(dp(1),Color.rgb(25,181,165));e.setBackground(bg);
        return e;
    }
    private Button button(String label){
        Button b=new Button(this);b.setText(label);b.setTextColor(Color.WHITE);b.setTextSize(11);b.setAllCaps(false);
        b.setMinHeight(dp(48));b.setSingleLine(false);b.setMaxLines(2);b.setPadding(dp(6),dp(3),dp(6),dp(3));
        TextViewCompat.setAutoSizeTextTypeUniformWithConfiguration(b,9,12,1,TypedValue.COMPLEX_UNIT_SP);
        b.setBackground(round(Color.rgb(11,59,96),12));return b;
    }
    private TextView text(String value,float size,int color,boolean bold){TextView t=new TextView(this);t.setText(value);t.setTextSize(size);t.setTextColor(color);t.setIncludeFontPadding(false);t.setLineSpacing(dp(2),1f);if(bold)t.setTypeface(Typeface.DEFAULT,Typeface.BOLD);return t;}
    private Space space(int dp){return new Space(this);}
    private GradientDrawable round(int color,int radius){GradientDrawable d=new GradientDrawable();d.setColor(color);d.setCornerRadius(dp(radius));return d;}
    private LinearLayout.LayoutParams matchWrap(){return new LinearLayout.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT,ViewGroup.LayoutParams.WRAP_CONTENT);}
    private LinearLayout.LayoutParams buttonParams(){LinearLayout.LayoutParams p=new LinearLayout.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT,dp(52));p.topMargin=dp(8);return p;}
    private int dp(int v){return Math.round(v*getResources().getDisplayMetrics().density);}
    private void showError(String message){AlertDialog d=new AlertDialog.Builder(this).setTitle("MusaCAD Lisans Yönetici").setMessage(message).setPositiveButton("TAMAM",null).create();d.setOnShowListener(x->styleDialogButtons(d));d.show();}
    private void styleDialogButtons(AlertDialog d){
        if(d==null)return;
        int[] ids={AlertDialog.BUTTON_POSITIVE,AlertDialog.BUTTON_NEGATIVE,AlertDialog.BUTTON_NEUTRAL};
        for(int id:ids){Button b=d.getButton(id);if(b==null)continue;b.setTextColor(Color.WHITE);b.setMinHeight(dp(48));b.setSingleLine(false);b.setMaxLines(2);TextViewCompat.setAutoSizeTextTypeUniformWithConfiguration(b,9,12,1,TypedValue.COMPLEX_UNIT_SP);}
    }
}
