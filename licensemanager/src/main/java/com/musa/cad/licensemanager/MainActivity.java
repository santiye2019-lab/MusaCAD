package com.musa.cad.licensemanager;

import android.app.*;
import android.content.*;
import android.graphics.Color;
import android.graphics.Typeface;
import android.graphics.drawable.GradientDrawable;
import android.os.Bundle;
import android.text.InputType;
import android.view.*;
import android.widget.*;
import androidx.appcompat.app.AppCompatActivity;
import java.util.*;

public class MainActivity extends AppCompatActivity {
    private static final int AUTH=71;
    private EditText customer,deviceId;
    private Spinner duration;
    private TextView keyStatus,tokenView,historyView;
    private String pendingCustomer,pendingDevice;
    private int pendingDays;
    private String pendingLabel;

    @Override protected void onCreate(Bundle state){
        super.onCreate(state);
        try{LicenseKeyStore.ensureKey();}catch(Exception e){showError("Anahtar oluşturulamadı: "+e.getMessage());}
        setContentView(buildUi());
        refreshKeyInfo();
        refreshHistory();
    }

    private View buildUi(){
        ScrollView scroll=new ScrollView(this);
        LinearLayout root=new LinearLayout(this);root.setOrientation(LinearLayout.VERTICAL);root.setPadding(dp(18),dp(20),dp(18),dp(24));root.setBackgroundColor(Color.rgb(7,16,21));
        scroll.addView(root,new ScrollView.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT,ViewGroup.LayoutParams.WRAP_CONTENT));

        TextView title=text("MusaCAD Lisans Yönetici",23,Color.WHITE,true);root.addView(title);
        TextView sub=text("Ücretli MusaCAD lisanslarını bu cihazda güvenli şekilde üret.",13,Color.rgb(161,183,196),false);sub.setPadding(0,dp(4),0,dp(18));root.addView(sub);

        keyStatus=text("",12,Color.rgb(25,181,165),false);keyStatus.setPadding(dp(12),dp(10),dp(12),dp(10));keyStatus.setBackground(round(Color.rgb(13,38,48),10));root.addView(keyStatus,matchWrap());

        Button copyKey=button("PUBLIC KEY'İ KOPYALA");copyKey.setOnClickListener(v->copyPublicKey());root.addView(copyKey,buttonParams());

        section(root,"Müşteri / Cihaz");
        customer=input("Müşteri adı (isteğe bağlı)");root.addView(customer,matchWrap());
        deviceId=input("MusaCAD Cihaz / Lisans ID");deviceId.setSingleLine(false);deviceId.setMinLines(2);deviceId.setMaxLines(3);root.addView(deviceId,matchWrap());

        section(root,"Lisans süresi");
        duration=new Spinner(this);
        ArrayAdapter<String> adapter=new ArrayAdapter<>(this,android.R.layout.simple_spinner_dropdown_item,new String[]{"30 gün","90 gün","365 gün","Süresiz"});
        duration.setAdapter(adapter);duration.setSelection(2);root.addView(duration,matchWrap());

        Button generate=button("LİSANS OLUŞTUR");generate.setBackground(round(Color.rgb(25,181,165),12));generate.setOnClickListener(v->requestIssue());root.addView(generate,buttonParams());

        section(root,"Üretilen lisans");
        tokenView=text("Henüz lisans oluşturulmadı.",12,Color.rgb(213,224,230),false);tokenView.setTextIsSelectable(true);tokenView.setPadding(dp(12),dp(12),dp(12),dp(12));tokenView.setBackground(round(Color.rgb(15,28,35),10));root.addView(tokenView,matchWrap());

        LinearLayout actions=new LinearLayout(this);actions.setOrientation(LinearLayout.HORIZONTAL);
        Button copy=button("KOPYALA");copy.setOnClickListener(v->copyToken());
        Button share=button("PAYLAŞ");share.setOnClickListener(v->shareToken());
        actions.addView(copy,new LinearLayout.LayoutParams(0,dp(50),1f));LinearLayout.LayoutParams gap=new LinearLayout.LayoutParams(dp(8),1);actions.addView(new Space(this),gap);actions.addView(share,new LinearLayout.LayoutParams(0,dp(50),1f));
        LinearLayout.LayoutParams ap=new LinearLayout.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT,ViewGroup.LayoutParams.WRAP_CONTENT);ap.topMargin=dp(8);root.addView(actions,ap);

        section(root,"Son işlemler");
        historyView=text("",12,Color.rgb(175,195,205),false);historyView.setTextIsSelectable(true);root.addView(historyView,matchWrap());

        TextView note=text("Özel RSA anahtarı Android Keystore içinde kalır ve APK'ya gömülmez. Bu telefon kaybolursa anahtarı geri almak mümkün değildir; final MusaCAD public key'i bu uygulamadaki anahtarla eşleştirilecektir.",11,Color.rgb(133,156,169),false);
        note.setPadding(0,dp(20),0,0);root.addView(note);
        return scroll;
    }

    private void requestIssue(){
        String id=deviceId.getText().toString().trim();
        try{LicenseIssuer.normalize(id);}catch(Exception e){deviceId.setError(e.getMessage());return;}
        int pos=duration.getSelectedItemPosition();
        int[] days={30,90,365,0};pendingDays=days[Math.max(0,Math.min(pos,days.length-1))];
        pendingLabel=duration.getSelectedItem().toString();
        pendingDevice=id;pendingCustomer=customer.getText().toString().trim();

        KeyguardManager km=(KeyguardManager)getSystemService(KEYGUARD_SERVICE);
        if(km!=null&&km.isDeviceSecure()){
            Intent auth=km.createConfirmDeviceCredentialIntent("Lisans üretimi","MusaCAD lisansı oluşturmak için telefon kilidini doğrulayın.");
            if(auth!=null){startActivityForResult(auth,AUTH);return;}
        }
        new androidx.appcompat.app.AlertDialog.Builder(this)
            .setTitle("Ekran kilidi önerilir")
            .setMessage("Bu telefonda güvenli ekran kilidi yok. Lisans üretici anahtarı Android Keystore'da korunuyor; yine de telefon için PIN/parmak izi kullanmanız önerilir.")
            .setPositiveButton("DEVAM",(d,w)->issueNow())
            .setNegativeButton("İPTAL",null).show();
    }

    @Override protected void onActivityResult(int requestCode,int resultCode,Intent data){
        super.onActivityResult(requestCode,resultCode,data);
        if(requestCode==AUTH&&resultCode==RESULT_OK)issueNow();
    }

    private void issueNow(){
        try{
            String token=LicenseIssuer.issue(pendingDevice,pendingDays);
            tokenView.setText(token);
            LicenseHistory.add(this,pendingCustomer,pendingDevice,pendingLabel,token);
            refreshHistory();
            Toast.makeText(this,"Lisans oluşturuldu",Toast.LENGTH_SHORT).show();
        }catch(Exception e){showError("Lisans oluşturulamadı: "+e.getMessage());}
    }

    private void copyToken(){
        String token=tokenView.getText().toString().trim();
        if(!token.startsWith("MC1.")){Toast.makeText(this,"Önce lisans oluşturun",Toast.LENGTH_SHORT).show();return;}
        copy("MusaCAD lisans",token);Toast.makeText(this,"Lisans kopyalandı",Toast.LENGTH_SHORT).show();
    }

    private void shareToken(){
        String token=tokenView.getText().toString().trim();
        if(!token.startsWith("MC1.")){Toast.makeText(this,"Önce lisans oluşturun",Toast.LENGTH_SHORT).show();return;}
        StringBuilder body=new StringBuilder();
        if(pendingCustomer!=null&&!pendingCustomer.isEmpty())body.append("Müşteri: ").append(pendingCustomer).append("\n");
        body.append("MusaCAD lisans kodu:\n\n").append(token);
        Intent send=new Intent(Intent.ACTION_SEND);send.setType("text/plain");send.putExtra(Intent.EXTRA_TEXT,body.toString());
        startActivity(Intent.createChooser(send,"Lisansı paylaş"));
    }

    private void copyPublicKey(){
        try{copy("MusaCAD public key",LicenseKeyStore.publicKeyPem());Toast.makeText(this,"Public key kopyalandı",Toast.LENGTH_SHORT).show();}
        catch(Exception e){showError(e.getMessage());}
    }

    private void refreshKeyInfo(){
        try{keyStatus.setText("🔐 Anahtar hazır  •  Parmak izi: "+LicenseKeyStore.fingerprint());}
        catch(Exception e){keyStatus.setText("Anahtar hatası: "+e.getMessage());}
    }

    private void refreshHistory(){
        List<String> rows=LicenseHistory.labels(this);
        if(rows.isEmpty()){historyView.setText("Henüz lisans üretilmedi.");return;}
        StringBuilder b=new StringBuilder();int n=Math.min(rows.size(),10);
        for(int i=0;i<n;i++){if(i>0)b.append("\n");b.append("• ").append(rows.get(i));}
        historyView.setText(b.toString());
    }

    private void copy(String label,String value){
        android.content.ClipboardManager cm=(android.content.ClipboardManager)getSystemService(CLIPBOARD_SERVICE);
        if(cm!=null)cm.setPrimaryClip(ClipData.newPlainText(label,value));
    }

    private void section(LinearLayout root,String label){
        TextView v=text(label,13,Color.WHITE,true);v.setPadding(0,dp(20),0,dp(7));root.addView(v);
    }
    private EditText input(String hint){
        EditText e=new EditText(this);e.setHint(hint);e.setTextColor(Color.WHITE);e.setHintTextColor(Color.rgb(115,142,157));e.setInputType(InputType.TYPE_CLASS_TEXT);e.setBackgroundTintList(android.content.res.ColorStateList.valueOf(Color.rgb(25,181,165)));return e;
    }
    private Button button(String label){
        Button b=new Button(this);b.setText(label);b.setTextColor(Color.WHITE);b.setTextSize(12);b.setAllCaps(false);b.setBackground(round(Color.rgb(11,59,96),12));return b;
    }
    private TextView text(String value,float size,int color,boolean bold){
        TextView t=new TextView(this);t.setText(value);t.setTextSize(size);t.setTextColor(color);t.setIncludeFontPadding(false);if(bold)t.setTypeface(Typeface.DEFAULT,Typeface.BOLD);return t;
    }
    private GradientDrawable round(int color,int radius){GradientDrawable d=new GradientDrawable();d.setColor(color);d.setCornerRadius(dp(radius));return d;}
    private LinearLayout.LayoutParams matchWrap(){return new LinearLayout.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT,ViewGroup.LayoutParams.WRAP_CONTENT);}
    private LinearLayout.LayoutParams buttonParams(){LinearLayout.LayoutParams p=new LinearLayout.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT,dp(52));p.topMargin=dp(8);return p;}
    private int dp(int v){return Math.round(v*getResources().getDisplayMetrics().density);}
    private void showError(String message){new androidx.appcompat.app.AlertDialog.Builder(this).setTitle("MusaCAD Lisans Yönetici").setMessage(message).setPositiveButton("TAMAM",null).show();}
}
