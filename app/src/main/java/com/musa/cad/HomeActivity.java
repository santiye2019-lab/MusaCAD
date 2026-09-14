package com.musa.cad;

import android.content.*;
import android.graphics.*;
import android.graphics.drawable.GradientDrawable;
import android.net.Uri;
import android.os.Bundle;
import android.text.*;
import android.view.*;
import android.widget.*;
import androidx.appcompat.app.AppCompatActivity;
import androidx.core.view.*;
import androidx.core.graphics.Insets;
import com.google.android.material.button.MaterialButton;
import java.io.File;
import java.text.DateFormat;
import java.util.*;

/** Light local-file home with recent cards and favorites. */
public class HomeActivity extends AppCompatActivity {
    private LinearLayout cards;private boolean favorites;private String query="";
    private MaterialButton recentTab,favoriteTab;
    private int dp(float value){return (int)(value*getResources().getDisplayMetrics().density+.5f);}
    private TextView text(String value,int size,int color){TextView t=new TextView(this);t.setText(value);t.setTextSize(size);t.setTextColor(color);return t;}
    private GradientDrawable rounded(int color){GradientDrawable d=new GradientDrawable();d.setColor(color);d.setCornerRadius(dp(14));return d;}
    private MaterialButton button(String label,int icon){MaterialButton b=new MaterialButton(this);b.setText(label);b.setAllCaps(false);b.setTextSize(13);b.setTextColor(Color.WHITE);b.setBackgroundTintList(android.content.res.ColorStateList.valueOf(Color.rgb(21,151,160)));b.setCornerRadius(dp(12));if(icon!=0){b.setIconResource(icon);b.setIconTint(android.content.res.ColorStateList.valueOf(Color.WHITE));}return b;}
    @Override public void onCreate(Bundle state){super.onCreate(state);
        WindowCompat.setDecorFitsSystemWindows(getWindow(),false);
        LinearLayout root=new LinearLayout(this);root.setId(R.id.homeRoot);root.setOrientation(LinearLayout.VERTICAL);root.setBackgroundColor(0xfff3f5f7);setContentView(root);
        ViewCompat.setOnApplyWindowInsetsListener(root,(v,insets)->{Insets bars=insets.getInsets(WindowInsetsCompat.Type.systemBars()|WindowInsetsCompat.Type.displayCutout());v.setPadding(bars.left,bars.top,bars.right,bars.bottom);return insets;});
        WindowCompat.getInsetsController(getWindow(),root).setAppearanceLightStatusBars(true);WindowCompat.getInsetsController(getWindow(),root).setAppearanceLightNavigationBars(true);ViewCompat.requestApplyInsets(root);
        LinearLayout header=new LinearLayout(this);header.setPadding(dp(20),dp(12),dp(16),dp(12));header.setGravity(Gravity.CENTER_VERTICAL);header.setBackgroundColor(Color.WHITE);
        TextView title=text("MUSA CAD",24,0xff162b3b);title.setTypeface(null,Typeface.BOLD);header.addView(title,new LinearLayout.LayoutParams(0,dp(44),1));
        TextView badge=text("Çizimlerim",13,0xff1597a0);badge.setGravity(Gravity.CENTER);header.addView(badge);root.addView(header);
        LinearLayout tabs=new LinearLayout(this);tabs.setPadding(dp(12),dp(4),dp(12),dp(4));tabs.setBackgroundColor(Color.WHITE);
        recentTab=button("Son dosyalar",R.drawable.ic_folder);recentTab.setId(R.id.recentTab);favoriteTab=button("Favoriler",R.drawable.ic_star);favoriteTab.setId(R.id.favoriteTab);
        tabs.addView(recentTab,new LinearLayout.LayoutParams(0,dp(52),1));tabs.addView(favoriteTab,new LinearLayout.LayoutParams(0,dp(52),1));root.addView(tabs);
        recentTab.setOnClickListener(v->{favorites=false;refresh();});favoriteTab.setOnClickListener(v->{favorites=true;refresh();});
        EditText search=new EditText(this);search.setId(R.id.searchDrawings);search.setSingleLine(true);search.setTextSize(14);search.setTextColor(0xff203345);search.setHintTextColor(0xff718294);search.setHint("Dosya adına göre ara");search.setPadding(dp(16),0,dp(16),0);search.setBackground(rounded(Color.WHITE));
        LinearLayout.LayoutParams searchParams=new LinearLayout.LayoutParams(-1,dp(48));searchParams.setMargins(dp(12),dp(12),dp(12),dp(6));root.addView(search,searchParams);
        search.addTextChangedListener(new TextWatcher(){public void beforeTextChanged(CharSequence s,int start,int count,int after){}public void onTextChanged(CharSequence s,int start,int before,int count){query=s.toString();refresh();}public void afterTextChanged(Editable s){}});
        ScrollView scroll=new ScrollView(this);cards=new LinearLayout(this);cards.setId(R.id.recentCards);cards.setOrientation(LinearLayout.VERTICAL);cards.setPadding(dp(12),dp(4),dp(12),dp(12));scroll.addView(cards);root.addView(scroll,new LinearLayout.LayoutParams(-1,0,1));
        LinearLayout bottom=new LinearLayout(this);bottom.setPadding(dp(16),dp(8),dp(16),dp(8));bottom.setBackgroundColor(Color.WHITE);MaterialButton open=button("Dosya aç",R.drawable.ic_folder);open.setId(R.id.homeOpenButton);bottom.addView(open,new LinearLayout.LayoutParams(-1,dp(54)));open.setOnClickListener(v->pick());root.addView(bottom);
        refresh();
    }
    @Override protected void onResume(){super.onResume();if(cards!=null)refresh();}
    private void pick(){Intent i=new Intent(Intent.ACTION_OPEN_DOCUMENT);i.setType("*/*");i.addCategory(Intent.CATEGORY_OPENABLE);startActivityForResult(i,20);}
    @Override protected void onActivityResult(int request,int result,Intent data){super.onActivityResult(request,result,data);if(request==20&&result==RESULT_OK&&data!=null&&data.getData()!=null){Uri uri=data.getData();try{getContentResolver().takePersistableUriPermission(uri,Intent.FLAG_GRANT_READ_URI_PERMISSION);}catch(SecurityException ignored){}open(uri);}}
    private void open(Uri uri){startActivity(new Intent(this,MainActivity.class).setData(uri).addFlags(Intent.FLAG_GRANT_READ_URI_PERMISSION));}
    private void refresh(){
        if(cards==null)return;cards.removeAllViews();
        recentTab.setAlpha(favorites?.55f:1f);favoriteTab.setAlpha(favorites?1f:.55f);
        int count=0;String filter=query.toLowerCase(Locale.forLanguageTag("tr"));
        for(RecentDrawings.Entry e:RecentDrawings.read(this)){
            if(favorites&&!e.favorite||!e.name.toLowerCase(Locale.forLanguageTag("tr")).contains(filter))continue;count++;
            LinearLayout card=new LinearLayout(this);card.setGravity(Gravity.CENTER_VERTICAL);card.setPadding(dp(10),dp(12),dp(8),dp(12));card.setBackground(rounded(Color.WHITE));card.setElevation(dp(1));
            LinearLayout.LayoutParams cp=new LinearLayout.LayoutParams(-1,-2);cp.setMargins(0,dp(5),0,dp(5));cards.addView(card,cp);
            ImageView image=new ImageView(this);image.setScaleType(ImageView.ScaleType.FIT_CENTER);image.setBackground(rounded(0xff12181e));image.setClipToOutline(true);Bitmap bitmap=BitmapFactory.decodeFile(new File(getFilesDir(),e.thumbnail).getPath());if(bitmap!=null)image.setImageBitmap(bitmap);else image.setImageResource(R.drawable.ic_folder);card.addView(image,new LinearLayout.LayoutParams(dp(68),dp(68)));
            LinearLayout labels=new LinearLayout(this);labels.setOrientation(LinearLayout.VERTICAL);labels.setPadding(dp(12),0,dp(4),0);TextView name=text(e.name,15,0xff203345);name.setMaxLines(2);name.setEllipsize(TextUtils.TruncateAt.MIDDLE);labels.addView(name);
            TextView date=text(DateFormat.getDateTimeInstance(DateFormat.SHORT,DateFormat.SHORT).format(new Date(e.time)),11,0xff718294);date.setPadding(0,dp(6),0,0);labels.addView(date);card.addView(labels,new LinearLayout.LayoutParams(0,-2,1));
            ImageButton star=new ImageButton(this);star.setImageResource(R.drawable.ic_star);star.setImageTintList(android.content.res.ColorStateList.valueOf(e.favorite?0xff1597a0:0xffa7b4c0));star.setBackgroundColor(Color.TRANSPARENT);star.setContentDescription(e.favorite?"Favoriden çıkar":"Favoriye ekle");card.addView(star,new LinearLayout.LayoutParams(dp(48),dp(48)));star.setOnClickListener(v->{RecentDrawings.toggleFavorite(this,e.uri);refresh();});
            card.setOnClickListener(v->open(Uri.parse(e.uri)));
        }
        if(count==0){TextView empty=text(query.isEmpty()?(favorites?"Henüz favori çiziminiz yok.\nBir dosyanın yıldızına dokunun.":"Çizimlerin burada görünecek.\nBaşlamak için Dosya aç'a dokun."):"Aramaya uygun çizim bulunamadı.",15,0xff718294);empty.setGravity(Gravity.CENTER);empty.setPadding(dp(20),dp(60),dp(20),dp(20));cards.addView(empty);}
    }
}
