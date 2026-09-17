package com.musa.cad;

import android.content.Intent;
import android.os.Bundle;
import android.os.Handler;
import android.os.Looper;
import android.view.View;
import androidx.appcompat.app.AppCompatActivity;
import androidx.core.graphics.Insets;
import androidx.core.view.ViewCompat;
import androidx.core.view.WindowCompat;
import androidx.core.view.WindowInsetsCompat;

public class SplashActivity extends AppCompatActivity {
    private final Handler handler = new Handler(Looper.getMainLooper());
    private final Runnable launchNext = () -> {
        if (isFinishing() || isDestroyed()) return;
        Intent incoming=new Intent(getIntent());
        boolean fileEntry=Intent.ACTION_VIEW.equals(incoming.getAction())||Intent.ACTION_SEND.equals(incoming.getAction());
        Intent next;
        if(LicenseManager.hasAccess(this)){
            next=fileEntry?incoming.setClass(this,MainActivity.class):new Intent(this,MainActivity.class);
        }else{
            next=new Intent(this,LicenseActivity.class);
            if(fileEntry)next.putExtra(LicenseActivity.EXTRA_PENDING_INTENT,incoming);
        }
        startActivity(next);
        overridePendingTransition(android.R.anim.fade_in, android.R.anim.fade_out);
        finish();
    };

    @Override protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        WindowCompat.setDecorFitsSystemWindows(getWindow(), false);
        setContentView(R.layout.activity_splash);

        View root = findViewById(R.id.splashRoot);
        ViewCompat.setOnApplyWindowInsetsListener(root, (v, insets) -> {
            Insets bars = insets.getInsets(WindowInsetsCompat.Type.systemBars());
            v.setPadding(0, bars.top, 0, bars.bottom);
            return insets;
        });

        View content = findViewById(R.id.splashContent);
        content.setAlpha(0f);
        content.setScaleX(.94f);
        content.setScaleY(.94f);
        content.animate().alpha(1f).scaleX(1f).scaleY(1f).setDuration(480).start();

        handler.postDelayed(launchNext, 1200);
    }

    @Override protected void onDestroy() {
        handler.removeCallbacks(launchNext);
        super.onDestroy();
    }
}
