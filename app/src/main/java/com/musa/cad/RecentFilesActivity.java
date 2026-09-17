package com.musa.cad;

import android.content.*;
import android.graphics.*;
import android.graphics.drawable.GradientDrawable;
import android.net.Uri;
import android.os.Bundle;
import android.view.*;
import android.widget.*;
import androidx.appcompat.app.AppCompatActivity;
import androidx.core.graphics.Insets;
import androidx.core.view.ViewCompat;
import androidx.core.view.WindowCompat;
import androidx.core.view.WindowInsetsCompat;
import java.io.FileDescriptor;
import java.util.*;

/** MusaCAD file-opening screen: most-recent drawings first, then the Android document picker. */
public class RecentFilesActivity extends AppCompatActivity {
    private static final int PICK_FILE=91;
    private LinearLayout listBox;

    @Override protected void onCreate(Bundle savedInstanceState){
        super.onCreate(savedInstanceState);
        WindowCompat.setDecorFitsSystemWindows(getWindow(),false);
        setContentView(buildUi());
        ViewCompat.setOnApplyWindowInsetsListener(findViewById(android.R.id.content),(v,insets)->{
            Insets bars=insets.getInsets(WindowInsetsCompat.Type.systemBars());
            v.setPadding(0,bars.top,0,bars.bottom);return insets;
        });
        renderRecents();
    }

    private View buildUi(){
        LinearLayout root=new LinearLayout(this);root.setId(android.R.id.content);root.setOrientation(LinearLayout.VERTICAL);
        root.setBackgroundColor(Color.rgb(7,16,21));root.setPadding(dp(16),dp(14),dp(16),dp(14));

        LinearLayout header=new LinearLayout(this);header.setOrientation(LinearLayout.HORIZONTAL);header.setGravity(Gravity.CENTER_VERTICAL);
        TextView back=new TextView(this);back.setText("‹");back.setTextSize(34);back.setTextColor(Color.WHITE);back.setGravity(Gravity.CENTER);back.setContentDescription("Geri");
        header.addView(back,new LinearLayout.LayoutParams(dp(44),dp(48)));back.setOnClickListener(v->finish());

        LinearLayout titles=new LinearLayout(this);titles.setOrientation(LinearLayout.VERTICAL);
        TextView title=text("Son Açılanlar",22,Color.WHITE,true);TextView sub=text("En son erişilen çizimler üstte",12,Color.rgb(160,181,194),false);
        titles.addView(title);titles.addView(sub);header.addView(titles,new LinearLayout.LayoutParams(0,ViewGroup.LayoutParams.WRAP_CONTENT,1f));
        root.addView(header);

        ScrollView scroll=new ScrollView(this);scroll.setFillViewport(true);scroll.setClipToPadding(false);scroll.setPadding(0,dp(12),0,dp(12));
        listBox=new LinearLayout(this);listBox.setOrientation(LinearLayout.VERTICAL);scroll.addView(listBox,new ScrollView.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT,ViewGroup.LayoutParams.WRAP_CONTENT));
        root.addView(scroll,new LinearLayout.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT,0,1f));

        Button browse=new Button(this);browse.setText("CİHAZDAN BAŞKA DOSYA AÇ");browse.setTextSize(13);browse.setTextColor(Color.WHITE);browse.setAllCaps(false);
        browse.setBackground(round(Color.rgb(11,59,96),14));browse.setOnClickListener(v->browse());
        LinearLayout.LayoutParams bp=new LinearLayout.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT,dp(54));bp.topMargin=dp(6);root.addView(browse,bp);
        return root;
    }

    private void renderRecents(){
        recycleThumbnails(listBox);
        listBox.removeAllViews();
        List<RecentFileStore.Entry> entries=RecentFileStore.list(this);
        if(entries.isEmpty()){
            LinearLayout empty=new LinearLayout(this);empty.setOrientation(LinearLayout.VERTICAL);empty.setGravity(Gravity.CENTER);empty.setPadding(dp(16),dp(72),dp(16),dp(72));
            TextView icon=text("▱",44,Color.rgb(25,181,165),false);icon.setGravity(Gravity.CENTER);
            TextView title=text("Henüz son açılan dosya yok",17,Color.WHITE,true);title.setGravity(Gravity.CENTER);
            TextView hint=text("İlk DWG veya DXF dosyanızı açtıktan sonra burada önizlemesi görünecek.",13,Color.rgb(160,181,194),false);hint.setGravity(Gravity.CENTER);hint.setPadding(dp(14),dp(8),dp(14),0);
            empty.addView(icon);empty.addView(title);empty.addView(hint);listBox.addView(empty);
            return;
        }
        for(RecentFileStore.Entry entry:entries)listBox.addView(row(entry));
    }

    private View row(RecentFileStore.Entry entry){
        LinearLayout card=new LinearLayout(this);card.setOrientation(LinearLayout.HORIZONTAL);card.setGravity(Gravity.CENTER_VERTICAL);
        card.setPadding(dp(10),dp(10),dp(10),dp(10));card.setBackground(round(Color.rgb(14,31,40),12));
        LinearLayout.LayoutParams cp=new LinearLayout.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT,dp(98));cp.bottomMargin=dp(10);card.setLayoutParams(cp);

        FrameLayout preview=new FrameLayout(this);preview.setBackground(round(Color.rgb(25,37,44),9));
        ImageView image=new ImageView(this);image.setScaleType(ImageView.ScaleType.CENTER_CROP);preview.addView(image,new FrameLayout.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT,ViewGroup.LayoutParams.MATCH_PARENT));
        Bitmap thumb=RecentFileStore.thumbnail(this,entry);
        if(thumb!=null){image.setImageBitmap(thumb);image.setTag(thumb);}else{
            TextView fallback=text(entry.typeLabel(),16,Color.rgb(25,181,165),true);fallback.setGravity(Gravity.CENTER);preview.addView(fallback,new FrameLayout.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT,ViewGroup.LayoutParams.MATCH_PARENT));
        }
        LinearLayout.LayoutParams pp=new LinearLayout.LayoutParams(dp(112),dp(76));card.addView(preview,pp);

        LinearLayout info=new LinearLayout(this);info.setOrientation(LinearLayout.VERTICAL);info.setPadding(dp(12),0,dp(6),0);
        TextView name=text(entry.name,15,Color.WHITE,true);name.setMaxLines(2);name.setEllipsize(android.text.TextUtils.TruncateAt.END);
        TextView meta=text(entry.typeLabel()+"  •  "+RecentFileStore.accessLabel(entry.lastAccessMs),12,Color.rgb(153,180,194),false);meta.setPadding(0,dp(5),0,0);
        info.addView(name);info.addView(meta);card.addView(info,new LinearLayout.LayoutParams(0,ViewGroup.LayoutParams.WRAP_CONTENT,1f));

        TextView arrow=text("›",28,Color.rgb(25,181,165),false);arrow.setGravity(Gravity.CENTER);card.addView(arrow,new LinearLayout.LayoutParams(dp(28),ViewGroup.LayoutParams.MATCH_PARENT));
        card.setOnClickListener(v->openRecent(entry));
        card.setOnLongClickListener(v->{showRecentActions(entry);return true;});
        return card;
    }

    private void showRecentActions(RecentFileStore.Entry entry){
        new androidx.appcompat.app.AlertDialog.Builder(this)
            .setTitle(entry.name)
            .setItems(new String[]{"Paylaş","Son açılanlardan kaldır"},(dialog,which)->{
                if(which==0)shareRecent(entry);
                else new androidx.appcompat.app.AlertDialog.Builder(this)
                    .setTitle("Son açılanlardan kaldırılsın mı?")
                    .setMessage(entry.name)
                    .setPositiveButton("KALDIR",(d,w)->{RecentFileStore.remove(this,entry.uri);renderRecents();})
                    .setNegativeButton("İPTAL",null)
                    .show();
            })
            .setNegativeButton("KAPAT",null)
            .show();
    }

    private void shareRecent(RecentFileStore.Entry entry){
        Uri uri=Uri.parse(entry.uri);
        if(!canRead(uri)){
            RecentFileStore.remove(this,entry.uri);renderRecents();
            Toast.makeText(this,"Bu dosyaya erişim artık yok. Cihazdan yeniden seçin.",Toast.LENGTH_LONG).show();
            return;
        }
        Intent share=new Intent(Intent.ACTION_SEND);
        share.setType("application/octet-stream");
        share.putExtra(Intent.EXTRA_STREAM,uri);
        share.setClipData(ClipData.newRawUri(entry.name,uri));
        share.addFlags(Intent.FLAG_GRANT_READ_URI_PERMISSION);
        try{
            startActivity(Intent.createChooser(share,"Dosyayı paylaş"));
        }catch(android.content.ActivityNotFoundException e){
            Toast.makeText(this,"Bu dosyayı paylaşabilecek bir uygulama bulunamadı.",Toast.LENGTH_LONG).show();
        }
    }


    private void openRecent(RecentFileStore.Entry entry){
        Uri uri=Uri.parse(entry.uri);
        if(!canRead(uri)){
            RecentFileStore.remove(this,entry.uri);renderRecents();
            Toast.makeText(this,"Bu dosyaya erişim artık yok. Cihazdan yeniden seçin.",Toast.LENGTH_LONG).show();return;
        }
        Intent data=new Intent();data.setData(uri);data.addFlags(Intent.FLAG_GRANT_READ_URI_PERMISSION);setResult(RESULT_OK,data);finish();
    }

    private boolean canRead(Uri uri){
        try(android.os.ParcelFileDescriptor fd=getContentResolver().openFileDescriptor(uri,"r")){return fd!=null;}catch(Exception e){return false;}
    }

    private void browse(){
        Intent i=new Intent(Intent.ACTION_OPEN_DOCUMENT);i.setType("*/*");i.addCategory(Intent.CATEGORY_OPENABLE);
        i.addFlags(Intent.FLAG_GRANT_READ_URI_PERMISSION|Intent.FLAG_GRANT_PERSISTABLE_URI_PERMISSION);
        i.putExtra(Intent.EXTRA_MIME_TYPES,new String[]{"application/acad","application/x-autocad","application/dwg","image/vnd.dwg","application/dxf","application/x-dxf","image/vnd.dxf","application/octet-stream"});
        startActivityForResult(i,PICK_FILE);
    }

    @Override protected void onActivityResult(int requestCode,int resultCode,Intent data){
        super.onActivityResult(requestCode,resultCode,data);
        if(requestCode!=PICK_FILE||resultCode!=RESULT_OK||data==null||data.getData()==null)return;
        Uri uri=data.getData();int flags=data.getFlags()&(Intent.FLAG_GRANT_READ_URI_PERMISSION|Intent.FLAG_GRANT_WRITE_URI_PERMISSION);
        try{getContentResolver().takePersistableUriPermission(uri,flags&Intent.FLAG_GRANT_READ_URI_PERMISSION);}catch(Exception ignored){}
        Intent result=new Intent();result.setData(uri);result.addFlags(Intent.FLAG_GRANT_READ_URI_PERMISSION);setResult(RESULT_OK,result);finish();
    }

    @Override protected void onDestroy(){
        recycleThumbnails(listBox);super.onDestroy();
    }

    private void recycleThumbnails(View view){
        if(view instanceof ImageView){Object tag=view.getTag();if(tag instanceof Bitmap){Bitmap b=(Bitmap)tag;if(!b.isRecycled())b.recycle();}}
        if(view instanceof ViewGroup){ViewGroup g=(ViewGroup)view;for(int i=0;i<g.getChildCount();i++)recycleThumbnails(g.getChildAt(i));}
    }

    private TextView text(String value,float sp,int color,boolean bold){
        TextView t=new TextView(this);t.setText(value);t.setTextSize(sp);t.setTextColor(color);t.setIncludeFontPadding(false);
        if(bold)t.setTypeface(android.graphics.Typeface.DEFAULT,android.graphics.Typeface.BOLD);return t;
    }
    private GradientDrawable round(int color,int radiusDp){GradientDrawable d=new GradientDrawable();d.setColor(color);d.setCornerRadius(dp(radiusDp));return d;}
    private int dp(int value){return Math.round(value*getResources().getDisplayMetrics().density);}
}
