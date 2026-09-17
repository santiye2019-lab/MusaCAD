package com.musa.cad.licenseadmin;

import android.app.Activity;
import android.os.Bundle;
import android.content.ClipData;
import android.content.ClipboardManager;
import android.content.Context;
import android.graphics.Color;
import android.graphics.Typeface;
import android.text.InputType;
import android.view.Gravity;
import android.view.ViewGroup;
import android.widget.*;
import java.io.ByteArrayOutputStream;
import java.nio.charset.StandardCharsets;
import java.security.MessageDigest;
import java.util.Locale;

public final class MainActivity extends Activity {
    private static final String PREFIX="MCT1";
    private static final String CONTEXT="MusaCAD-Debug-License-TEST-ONLY-v1";
    private static final char[] URL64="ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_".toCharArray();

    private EditText installationId;
    private Spinner duration;
    private TextView output;

    @Override protected void onCreate(Bundle savedInstanceState){
        super.onCreate(savedInstanceState);
        setTitle("MusaCAD Lisans TEST");

        ScrollView scroll=new ScrollView(this);
        LinearLayout root=new LinearLayout(this);
        root.setOrientation(LinearLayout.VERTICAL);
        root.setPadding(dp(20),dp(26),dp(20),dp(32));
        root.setBackgroundColor(Color.rgb(7,16,21));
        scroll.addView(root,new ScrollView.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT,ViewGroup.LayoutParams.WRAP_CONTENT));

        TextView title=text("MusaCAD Lisans Yönetici",28,Color.WHITE,true);
        root.addView(title);

        TextView warning=text("TEST SÜRÜMÜ • Yalnız birlikte verilen MusaCAD debug APK için kod üretir. Ticari/release sürüm bu kodları kabul etmez.",14,Color.rgb(255,193,7),true);
        warning.setPadding(0,dp(8),0,dp(18));root.addView(warning);

        root.addView(text("Cihaz / Lisans Kimliği",15,Color.LTGRAY,true));
        installationId=new EditText(this);
        installationId.setHint("MusaCAD ekranındaki MC-... kimliğini yapıştırın");
        installationId.setTextColor(Color.WHITE);installationId.setHintTextColor(Color.GRAY);
        installationId.setSingleLine(false);installationId.setMinLines(2);installationId.setMaxLines(4);
        installationId.setInputType(InputType.TYPE_CLASS_TEXT|InputType.TYPE_TEXT_FLAG_NO_SUGGESTIONS);
        installationId.setPadding(dp(12),dp(10),dp(12),dp(10));
        root.addView(installationId,matchWrap());

        Button paste=button("PANODAN KİMLİK AL");
        paste.setOnClickListener(v->pasteId());root.addView(paste,matchWrap());

        TextView durationLabel=text("Lisans süresi",15,Color.LTGRAY,true);durationLabel.setPadding(0,dp(18),0,dp(5));root.addView(durationLabel);
        duration=new Spinner(this);
        String[] values={"1 gün","30 gün","90 gün","365 gün","Süresiz"};
        ArrayAdapter<String> adapter=new ArrayAdapter<>(this,android.R.layout.simple_spinner_dropdown_item,values);
        duration.setAdapter(adapter);root.addView(duration,matchWrap());

        Button issue=button("TEST LİSANS KODU ÜRET");
        issue.setOnClickListener(v->issue());LinearLayout.LayoutParams issueLp=matchWrap();issueLp.topMargin=dp(18);root.addView(issue,issueLp);

        TextView outLabel=text("Üretilen kod",15,Color.LTGRAY,true);outLabel.setPadding(0,dp(18),0,dp(5));root.addView(outLabel);
        output=text("Henüz kod üretilmedi.",13,Color.WHITE,false);
        output.setTypeface(Typeface.MONOSPACE);output.setTextIsSelectable(true);output.setMinLines(6);
        output.setPadding(dp(12),dp(12),dp(12),dp(12));output.setBackgroundColor(Color.rgb(18,48,61));
        root.addView(output,matchWrap());

        Button copy=button("KODU KOPYALA");
        copy.setOnClickListener(v->copyCode());root.addView(copy,matchWrap());

        TextView help=text("Kullanım: MusaCAD'da KİMLİĞİ KOPYALA → bu uygulamada PANODAN KİMLİK AL → süreyi seç → kodu üret → KODU KOPYALA → MusaCAD lisans alanına yapıştır.",13,Color.rgb(143,183,197),false);
        help.setPadding(0,dp(18),0,0);root.addView(help);

        setContentView(scroll);
    }

    private void pasteId(){
        ClipboardManager cm=(ClipboardManager)getSystemService(Context.CLIPBOARD_SERVICE);
        if(cm!=null&&cm.hasPrimaryClip()&&cm.getPrimaryClip()!=null&&cm.getPrimaryClip().getItemCount()>0){
            CharSequence value=cm.getPrimaryClip().getItemAt(0).coerceToText(this);
            if(value!=null)installationId.setText(value.toString().trim());
        }
    }

    private void issue(){
        try{
            String id=installationId.getText().toString().trim();
            if(id.isEmpty()){installationId.setError("Cihaz / Lisans Kimliğini girin");return;}
            long now=System.currentTimeMillis(),expiry;
            switch(duration.getSelectedItemPosition()){
                case 0: expiry=now+24L*60L*60L*1000L;break;
                case 1: expiry=now+30L*24L*60L*60L*1000L;break;
                case 2: expiry=now+90L*24L*60L*60L*1000L;break;
                case 3: expiry=now+365L*24L*60L*60L*1000L;break;
                default: expiry=0L;break;
            }
            output.setText(issueToken(id,expiry));
            Toast.makeText(this,"Test lisans kodu üretildi",Toast.LENGTH_SHORT).show();
        }catch(Exception e){Toast.makeText(this,"Kod üretilemedi: "+e.getMessage(),Toast.LENGTH_LONG).show();}
    }

    private void copyCode(){
        String code=output.getText().toString().trim();
        if(!code.startsWith(PREFIX+".")){Toast.makeText(this,"Önce kod üretin",Toast.LENGTH_SHORT).show();return;}
        ClipboardManager cm=(ClipboardManager)getSystemService(Context.CLIPBOARD_SERVICE);
        if(cm!=null)cm.setPrimaryClip(ClipData.newPlainText("MusaCAD Test Lisans",code));
        Toast.makeText(this,"Kod kopyalandı",Toast.LENGTH_SHORT).show();
    }

    private static String issueToken(String installationId,long expiresAtMs)throws Exception{
        if(expiresAtMs<0L)throw new IllegalArgumentException("Süre geçersiz");
        String payload=PREFIX+"|"+installationId.trim()+"|"+expiresAtMs;
        byte[] sig=MessageDigest.getInstance("SHA-256").digest((payload+"|"+CONTEXT).getBytes(StandardCharsets.UTF_8));
        return PREFIX+"."+encode64(payload.getBytes(StandardCharsets.UTF_8))+"."+encode64(sig);
    }

    private static String encode64(byte[] data){
        StringBuilder out=new StringBuilder((data.length*4+2)/3);
        for(int i=0;i<data.length;i+=3){
            int b0=data[i]&255,b1=i+1<data.length?data[i+1]&255:0,b2=i+2<data.length?data[i+2]&255:0;
            out.append(URL64[b0>>>2]).append(URL64[((b0&3)<<4)|(b1>>>4)]);
            if(i+1<data.length)out.append(URL64[((b1&15)<<2)|(b2>>>6)]);
            if(i+2<data.length)out.append(URL64[b2&63]);
        }
        return out.toString();
    }

    private TextView text(String value,float sp,int color,boolean bold){
        TextView t=new TextView(this);t.setText(value);t.setTextSize(sp);t.setTextColor(color);if(bold)t.setTypeface(Typeface.DEFAULT,Typeface.BOLD);return t;
    }
    private Button button(String value){Button b=new Button(this);b.setText(value);b.setTextSize(14);b.setMinHeight(dp(52));b.setAllCaps(false);return b;}
    private LinearLayout.LayoutParams matchWrap(){return new LinearLayout.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT,ViewGroup.LayoutParams.WRAP_CONTENT);}
    private int dp(int value){return Math.round(value*getResources().getDisplayMetrics().density);}
}
