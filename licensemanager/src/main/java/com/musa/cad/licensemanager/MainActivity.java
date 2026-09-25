package com.musa.cad.licensemanager;

import android.app.KeyguardManager;
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
    private EditText customer,serialField;
    private Spinner duration;
    private TextView tokenView,historyView;
    private String pendingCustomer,pendingSerial;
    private int pendingDays;
    private String pendingLabel;

    @Override protected void onCreate(Bundle state){
        super.onCreate(state);
        setContentView(buildUi());
        refreshHistory();
    }

    private View buildUi(){
        ScrollView scroll=new ScrollView(this);
        LinearLayout root=new LinearLayout(this);
        root.setOrientation(LinearLayout.VERTICAL);
        root.setPadding(dp(18),dp(20),dp(18),dp(28));
        root.setBackgroundColor(Color.rgb(7,16,21));
        scroll.addView(root,new ScrollView.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT,ViewGroup.LayoutParams.WRAP_CONTENT));

        TextView title=text("MusaCAD Lisans Yönetici",24,Color.WHITE,true);
        root.addView(title);
        TextView sub=text("Serial numarasına göre 12 haneli lisans kodu üret.",13,Color.rgb(161,183,196),false);
        sub.setPadding(0,dp(4),0,dp(16));
        root.addView(sub);

        customer=input("Müşteri adı (isteğe bağlı)");
        root.addView(customer,matchWrap());

        section(root,"Serial");
        serialField=input("MusaCAD'deki 12 haneli Serial");
        serialField.setAllCaps(true);
        serialField.setSingleLine(true);
        serialField.setTextSize(20f);
        root.addView(serialField,matchWrap());

        section(root,"Lisans süresi");
        duration=new Spinner(this);
        ArrayAdapter<String> adapter=new ArrayAdapter<>(this,android.R.layout.simple_spinner_dropdown_item,new String[]{"30 gün","90 gün","365 gün","Süresiz"});
        duration.setAdapter(adapter);
        duration.setSelection(2);
        root.addView(duration,matchWrap());

        Button generate=button("LİSANS KODU ÜRET");
        generate.setBackground(round(Color.rgb(25,181,165),12));
        generate.setOnClickListener(v->requestIssue());
        root.addView(generate,buttonParams());

        section(root,"Lisans Kodu");
        tokenView=text("------------",24,Color.WHITE,true);
        tokenView.setGravity(Gravity.CENTER);
        tokenView.setTextIsSelectable(true);
        tokenView.setPadding(dp(12),dp(18),dp(12),dp(18));
        tokenView.setBackground(round(Color.rgb(15,28,35),10));
        root.addView(tokenView,matchWrap());

        LinearLayout actions=new LinearLayout(this);
        actions.setOrientation(LinearLayout.HORIZONTAL);
        Button copy=button("KOPYALA");
        copy.setOnClickListener(v->copyToken());
        Button share=button("PAYLAŞ");
        share.setOnClickListener(v->shareToken());
        actions.addView(copy,new LinearLayout.LayoutParams(0,dp(50),1f));
        Space spacer=new Space(this);
        actions.addView(spacer,new LinearLayout.LayoutParams(dp(8),1));
        actions.addView(share,new LinearLayout.LayoutParams(0,dp(50),1f));
        LinearLayout.LayoutParams ap=new LinearLayout.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT,ViewGroup.LayoutParams.WRAP_CONTENT);
        ap.topMargin=dp(8);
        root.addView(actions,ap);

        TextView note=text("Kullanım: MusaCAD'den Serial'i kopyala → buraya yapıştır → süreyi seç → lisans kodunu üret → oluşan 12 haneli kodu MusaCAD'deki Lisans Kodu alanına yapıştır.",11,Color.rgb(133,156,169),false);
        note.setPadding(0,dp(18),0,0);
        root.addView(note);

        section(root,"Son işlemler");
        historyView=text("",12,Color.rgb(175,195,205),false);
        historyView.setTextIsSelectable(true);
        root.addView(historyView,matchWrap());

        return scroll;
    }

    private void requestIssue(){
        String serial=serialField.getText().toString().trim();
        try{pendingSerial=ShortLicenseCode.normalizeSerial(serial);}
        catch(Exception e){serialField.setError(e.getMessage());return;}

        int pos=duration.getSelectedItemPosition();
        int[] days={30,90,365,0};
        pendingDays=days[Math.max(0,Math.min(pos,days.length-1))];
        pendingLabel=duration.getSelectedItem().toString();
        pendingCustomer=customer.getText().toString().trim();

        KeyguardManager km=(KeyguardManager)getSystemService(KEYGUARD_SERVICE);
        if(km!=null&&km.isDeviceSecure()){
            Intent auth=km.createConfirmDeviceCredentialIntent("Lisans üretimi","MusaCAD lisans kodu oluşturmak için telefon kilidini doğrulayın.");
            if(auth!=null){startActivityForResult(auth,AUTH);return;}
        }
        issueNow();
    }

    @Override protected void onActivityResult(int requestCode,int resultCode,Intent data){
        super.onActivityResult(requestCode,resultCode,data);
        if(requestCode==AUTH&&resultCode==RESULT_OK)issueNow();
    }

    private void issueNow(){
        try{
            String token=ShortLicenseCode.issue(pendingSerial,pendingDays,System.currentTimeMillis());
            tokenView.setText(token);
            LicenseHistory.add(this,pendingCustomer,pendingSerial,pendingLabel,token);
            refreshHistory();
            Toast.makeText(this,"12 haneli lisans kodu oluşturuldu",Toast.LENGTH_SHORT).show();
        }catch(Exception e){showError("Lisans oluşturulamadı: "+e.getMessage());}
    }

    private void copyToken(){
        String token=tokenView.getText().toString().trim();
        if(!ShortLicenseCode.isCode(token)){Toast.makeText(this,"Önce lisans kodu oluşturun",Toast.LENGTH_SHORT).show();return;}
        copy("MusaCAD Lisans Kodu",token);
        Toast.makeText(this,"Lisans kodu kopyalandı",Toast.LENGTH_SHORT).show();
    }

    private void shareToken(){
        String token=tokenView.getText().toString().trim();
        if(!ShortLicenseCode.isCode(token)){Toast.makeText(this,"Önce lisans kodu oluşturun",Toast.LENGTH_SHORT).show();return;}
        StringBuilder body=new StringBuilder();
        if(pendingCustomer!=null&&!pendingCustomer.isEmpty())body.append("Müşteri: ").append(pendingCustomer).append("\n");
        body.append("Serial: ").append(pendingSerial).append("\n");
        body.append("Lisans Kodu: ").append(token);
        Intent send=new Intent(Intent.ACTION_SEND);
        send.setType("text/plain");
        send.putExtra(Intent.EXTRA_TEXT,body.toString());
        startActivity(Intent.createChooser(send,"Lisansı paylaş"));
    }

    private void refreshHistory(){
        List<String> rows=LicenseHistory.labels(this);
        if(rows.isEmpty()){historyView.setText("Henüz lisans üretilmedi.");return;}
        StringBuilder b=new StringBuilder();
        int n=Math.min(rows.size(),10);
        for(int i=0;i<n;i++){if(i>0)b.append("\n");b.append("• ").append(rows.get(i));}
        historyView.setText(b.toString());
    }

    private void copy(String label,String value){
        android.content.ClipboardManager cm=(android.content.ClipboardManager)getSystemService(CLIPBOARD_SERVICE);
        if(cm!=null)cm.setPrimaryClip(ClipData.newPlainText(label,value));
    }

    private void section(LinearLayout root,String label){
        TextView v=text(label,15,Color.WHITE,true);
        v.setPadding(0,dp(20),0,dp(7));
        root.addView(v);
    }

    private EditText input(String hint){
        EditText e=new EditText(this);
        e.setHint(hint);
        e.setTextColor(Color.WHITE);
        e.setHintTextColor(Color.rgb(115,142,157));
        e.setInputType(InputType.TYPE_CLASS_TEXT);
        e.setBackgroundTintList(android.content.res.ColorStateList.valueOf(Color.rgb(25,181,165)));
        return e;
    }

    private Button button(String label){
        Button b=new Button(this);
        b.setText(label);
        b.setTextColor(Color.WHITE);
        b.setTextSize(12);
        b.setAllCaps(false);
        b.setBackground(round(Color.rgb(11,59,96),12));
        return b;
    }

    private TextView text(String value,float size,int color,boolean bold){
        TextView t=new TextView(this);
        t.setText(value);
        t.setTextSize(size);
        t.setTextColor(color);
        t.setIncludeFontPadding(false);
        if(bold)t.setTypeface(Typeface.DEFAULT,Typeface.BOLD);
        return t;
    }

    private GradientDrawable round(int color,int radius){
        GradientDrawable d=new GradientDrawable();
        d.setColor(color);
        d.setCornerRadius(dp(radius));
        return d;
    }

    private LinearLayout.LayoutParams matchWrap(){
        return new LinearLayout.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT,ViewGroup.LayoutParams.WRAP_CONTENT);
    }

    private LinearLayout.LayoutParams buttonParams(){
        LinearLayout.LayoutParams p=new LinearLayout.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT,dp(52));
        p.topMargin=dp(8);
        return p;
    }

    private int dp(int v){return Math.round(v*getResources().getDisplayMetrics().density);}

    private void showError(String message){
        new androidx.appcompat.app.AlertDialog.Builder(this)
            .setTitle("MusaCAD Lisans Yönetici")
            .setMessage(message)
            .setPositiveButton("TAMAM",null)
            .show();
    }
}
