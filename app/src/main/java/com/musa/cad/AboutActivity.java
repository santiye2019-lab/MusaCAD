package com.musa.cad;

import android.os.Bundle;
import android.view.View;
import android.widget.ImageView;
import android.widget.ScrollView;
import android.widget.TextView;
import androidx.appcompat.app.AlertDialog;
import androidx.appcompat.app.AppCompatActivity;
import androidx.core.graphics.Insets;
import androidx.core.view.ViewCompat;
import androidx.core.view.WindowCompat;
import androidx.core.view.WindowInsetsCompat;
import java.io.ByteArrayOutputStream;
import java.io.IOException;
import java.io.InputStream;

public class AboutActivity extends AppCompatActivity {
    public static final String EXTRA_CONTINUE_TO_APP="com.musa.cad.CONTINUE_TO_APP";
    @Override protected void onCreate(Bundle savedInstanceState){
        super.onCreate(savedInstanceState);
        WindowCompat.setDecorFitsSystemWindows(getWindow(),false);
        setContentView(R.layout.activity_about);

        View root=findViewById(R.id.aboutRoot);
        ViewCompat.setOnApplyWindowInsetsListener(root,(v,insets)->{
            Insets bars=insets.getInsets(WindowInsetsCompat.Type.systemBars());
            v.setPadding(0,bars.top,0,bars.bottom);
            return insets;
        });

        ImageView photo=findViewById(R.id.developerPhoto);
        EmbeddedProfilePhoto.loadInto(this,photo);
        ((TextView)findViewById(R.id.versionText)).setText("MusaCAD • Sürüm "+BuildConfig.VERSION_NAME);

        findViewById(R.id.openSourceButton).setOnClickListener(v->showOpenSource());
        findViewById(R.id.aboutBackButton).setOnClickListener(v->{if(getIntent().getBooleanExtra(EXTRA_CONTINUE_TO_APP,false))openMusaCad();else finish();});
        findViewById(R.id.closeAboutButton).setOnClickListener(v->openMusaCad());
    }

    private void openMusaCad(){
        android.content.Intent next;
        if(LicenseManager.hasAccess(this))next=new android.content.Intent(this,MainActivity.class);
        else next=new android.content.Intent(this,LicenseActivity.class);
        next.addFlags(android.content.Intent.FLAG_ACTIVITY_CLEAR_TOP);
        startActivity(next);
        overridePendingTransition(android.R.anim.fade_in,android.R.anim.fade_out);
        if(getIntent().getBooleanExtra(EXTRA_CONTINUE_TO_APP,false))finish();
    }

    private void showOpenSource(){
        String license=readAsset("COPYING-LibreDWG.txt","GPL-3.0-or-later");
        TextView text=new TextView(this);
        int p=Math.round(18*getResources().getDisplayMetrics().density);
        text.setPadding(p,p,p,p);
        text.setText("MusaCAD\nKaynak kod: https://github.com/santiye2019-lab/MusaCAD\n\nLibreDWG lisansı:\n\n"+license);
        text.setTextColor(0xFFE7F4F8);
        text.setTextSize(12f);
        text.setTextIsSelectable(true);
        android.text.util.Linkify.addLinks(text,android.text.util.Linkify.WEB_URLS);
        text.setMovementMethod(android.text.method.LinkMovementMethod.getInstance());
        ScrollView scroll=new ScrollView(this);
        scroll.setBackgroundColor(0xFF102631);
        scroll.addView(text);
        new AlertDialog.Builder(this)
            .setTitle("Açık kaynak ve lisanslar")
            .setView(scroll)
            .setPositiveButton("KAPAT",null)
            .show();
    }

    private String readAsset(String name,String fallback){
        try(InputStream in=getAssets().open(name);ByteArrayOutputStream out=new ByteArrayOutputStream()){
            byte[] b=new byte[4096];int n;while((n=in.read(b))!=-1)out.write(b,0,n);
            return out.toString("UTF-8");
        }catch(IOException e){return fallback;}
    }
}
