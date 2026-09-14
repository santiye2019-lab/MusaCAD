package com.musa.cad;

import android.net.Uri;
import android.os.Bundle;
import android.widget.Toast;
import java.lang.reflect.Method;

/**
 * Entry point used by Android's "Open with" chooser for DWG/DXF documents.
 * It reuses MainActivity's normal viewer UI and forwards the selected document
 * into the same loading pipeline used by MusaCAD's own file picker.
 */
public class OpenWithActivity extends MainActivity {
    @Override protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        openIncomingDocument();
    }

    @Override protected void onNewIntent(android.content.Intent intent) {
        super.onNewIntent(intent);
        setIntent(intent);
        openIncomingDocument();
    }

    private void openIncomingDocument() {
        Uri uri = getIntent() == null ? null : getIntent().getData();
        if (uri == null) return;
        try {
            Method loader = MainActivity.class.getDeclaredMethod("startLoad", Uri.class);
            loader.setAccessible(true);
            loader.invoke(this, uri);
        } catch (Exception e) {
            Toast.makeText(this, "Dosya MusaCAD ile açılamadı: " + e.getMessage(), Toast.LENGTH_LONG).show();
        }
    }
}
