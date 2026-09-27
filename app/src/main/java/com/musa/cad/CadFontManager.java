package com.musa.cad;

import android.content.Context;
import android.database.Cursor;
import android.graphics.Typeface;
import android.net.Uri;
import android.provider.OpenableColumns;
import java.io.*;
import java.util.*;

/** Central font catalog/resolver for CAD TEXT/MTEXT rendering and user-imported fonts. */
public final class CadFontManager {
    private static final String PREFS="musacad_fonts";
    private static final String KEY_HINT="default_hint";
    private static final String KEY_SHX="default_shx";
    private static final String DIR="cad_fonts";
    private static final String[] SYSTEM_DIRS={"/system/fonts","/product/fonts","/system/product/fonts","/system_ext/fonts","/vendor/fonts"};
    private static final String[] BUILTIN={
        "sans-serif","sans-serif-condensed","sans-serif-medium","serif","monospace"
    };
    private static final String[] COMMON_SHX={
        "simplex.shx","txt.shx","complex.shx","romans.shx","romand.shx","romant.shx","romanc.shx",
        "script.shx","scripts.shx","scriptc.shx","gothicg.shx","gothice.shx","gothici.shx",
        "iso.shx","isoct.shx","isocp.shx"
    };
    private static File importDir;

    public static final class Choice {
        public final String displayName,hint;
        public final boolean shx,imported;
        Choice(String displayName,String hint,boolean shx,boolean imported){
            this.displayName=displayName;this.hint=hint;this.shx=shx;this.imported=imported;
        }
        public String styleName(){return styleNameFor(hint,shx);}
        @Override public String toString(){return displayName+(shx?" • SHX":"");}
    }

    public static synchronized void init(Context context){
        if(context==null)return;
        File dir=new File(context.getFilesDir(),DIR);
        if(!dir.isDirectory())dir.mkdirs();
        importDir=dir;
    }

    public static List<Choice> choices(Context context){
        init(context);
        LinkedHashMap<String,Choice> out=new LinkedHashMap<>();
        for(String family:BUILTIN)add(out,new Choice(family,family,false,false));
        for(String shx:COMMON_SHX)add(out,new Choice(shx,shx,true,false));
        for(File dir:systemFontDirs()){
            File[] files=dir.listFiles((d,n)->isTtfOrOtf(n));
            if(files==null)continue;
            Arrays.sort(files,Comparator.comparing(File::getName,String.CASE_INSENSITIVE_ORDER));
            for(File file:files)add(out,new Choice(stripExtension(file.getName()),file.getName(),false,false));
        }
        File[] imported=importDir==null?null:importDir.listFiles();
        if(imported!=null){
            Arrays.sort(imported,Comparator.comparing(File::getName,String.CASE_INSENSITIVE_ORDER));
            for(File file:imported){
                if(!file.isFile()||!isTtfOrOtf(file.getName()))continue;
                add(out,new Choice(stripExtension(file.getName())+" • Kullanıcı",file.getName(),false,true));
            }
        }
        return Collections.unmodifiableList(new ArrayList<>(out.values()));
    }

    public static Choice defaultChoice(Context context){
        init(context);
        String hint=context.getSharedPreferences(PREFS,Context.MODE_PRIVATE).getString(KEY_HINT,"sans-serif");
        boolean shx=context.getSharedPreferences(PREFS,Context.MODE_PRIVATE).getBoolean(KEY_SHX,false);
        Choice found=findChoice(context,hint,shx);
        return found==null?new Choice("sans-serif","sans-serif",false,false):found;
    }

    public static void setDefaultChoice(Context context,Choice choice){
        if(context==null||choice==null)return;
        context.getSharedPreferences(PREFS,Context.MODE_PRIVATE).edit()
            .putString(KEY_HINT,choice.hint).putBoolean(KEY_SHX,choice.shx).apply();
    }

    public static Choice findChoice(Context context,String hint,boolean shx){
        String key=norm(hint);
        for(Choice c:choices(context))if(c.shx==shx&&norm(c.hint).equals(key))return c;
        if(!key.isEmpty())return new Choice(stripExtension(cleanName(hint)),cleanName(hint),shx,false);
        return null;
    }

    public static Choice importFont(Context context,Uri uri)throws IOException{
        if(context==null||uri==null)throw new IOException("Yazı tipi seçilmedi");
        init(context);
        String name=queryName(context,uri);
        if(!isTtfOrOtf(name))throw new IOException("Yalnız TTF veya OTF yazı tipi seçin");
        name=safeFileName(name);
        File target=uniqueTarget(importDir,name);
        try(InputStream in=context.getContentResolver().openInputStream(uri);OutputStream out=new FileOutputStream(target)){
            if(in==null)throw new IOException("Yazı tipi okunamadı");
            byte[] buf=new byte[64*1024];int n;long total=0;
            while((n=in.read(buf))!=-1){total+=n;if(total>32L*1024L*1024L)throw new IOException("Yazı tipi dosyası çok büyük");out.write(buf,0,n);}
        }catch(IOException e){target.delete();throw e;}
        try{Typeface.createFromFile(target);}catch(RuntimeException e){target.delete();throw new IOException("Geçerli bir TTF/OTF dosyası değil",e);}
        Choice choice=new Choice(stripExtension(target.getName())+" • Kullanıcı",target.getName(),false,true);
        setDefaultChoice(context,choice);
        return choice;
    }

    public static Typeface resolveTypeface(String hint,boolean shx,int style){
        String value=cleanName(hint);
        File file=shx?findSubstituteByBase(value):findFontFile(value);
        if(file!=null){
            try{return Typeface.create(Typeface.createFromFile(file),style);}catch(RuntimeException ignored){}
        }
        if(shx)return Typeface.create(CadFontPolicy.shxFallbackFamily(value),style);
        return Typeface.create(CadFontPolicy.androidFallbackFamily(value),style);
    }

    public static boolean isAvailable(String hint,boolean shx){
        String value=cleanName(hint);
        if(value.isEmpty())return true;
        if(shx)return isCommonShx(value)||findSubstituteByBase(value)!=null;
        if(findFontFile(value)!=null)return true;
        String family=CadFontPolicy.androidFallbackFamily(value);
        return "sans-serif".equals(family)||"sans-serif-condensed".equals(family)||
            "serif".equals(family)||"monospace".equals(family);
    }

    public static String styleNameFor(String hint,boolean shx){return CadFontPolicy.dxfStyleName(hint);}
    public static String dxfFontFile(String hint,boolean shx){return CadFontPolicy.dxfFontFile(hint,shx);}

    public static int userFontCount(Context context){
        init(context);File[] files=importDir==null?null:importDir.listFiles();if(files==null)return 0;int count=0;
        for(File file:files)if(file.isFile()&&isTtfOrOtf(file.getName()))count++;
        return count;
    }

    private static File findSubstituteByBase(String hint){
        String wanted=CadFontPolicy.key(hint);if(wanted.isEmpty())return null;
        if(importDir!=null){File found=findByBase(importDir,wanted);if(found!=null)return found;}
        for(File dir:systemFontDirs()){File found=findByBase(dir,wanted);if(found!=null)return found;}
        return null;
    }

    private static File findByBase(File dir,String wanted){
        File[] files=dir==null?null:dir.listFiles();if(files==null)return null;
        for(File f:files)if(f.isFile()&&isTtfOrOtf(f.getName())&&CadFontPolicy.key(f.getName()).equals(wanted))return f;
        return null;
    }

    private static File findFontFile(String hint){
        String name=new File(cleanName(hint)).getName();
        if(name.isEmpty())return null;
        File direct=new File(hint);
        if(direct.isFile())return direct;
        if(importDir!=null){
            File imported=new File(importDir,name);if(imported.isFile())return imported;
            File match=findIgnoreCase(importDir,name);if(match!=null)return match;
        }
        for(File dir:systemFontDirs()){
            File exact=new File(dir,name);if(exact.isFile())return exact;
            File match=findIgnoreCase(dir,name);if(match!=null)return match;
        }
        return null;
    }

    private static File findIgnoreCase(File dir,String name){
        File[] files=dir.listFiles();if(files==null)return null;
        for(File f:files)if(f.isFile()&&f.getName().equalsIgnoreCase(name))return f;
        return null;
    }

    private static List<File> systemFontDirs(){
        ArrayList<File> out=new ArrayList<>();
        for(String path:SYSTEM_DIRS){File d=new File(path);if(d.isDirectory()&&d.canRead())out.add(d);}
        return out;
    }

    private static void add(Map<String,Choice> out,Choice c){
        String key=(c.shx?"S:":"F:")+norm(c.hint);
        if(!out.containsKey(key))out.put(key,c);
    }

    private static boolean isTtfOrOtf(String name){return CadFontPolicy.isTtfOrOtf(name);}
    private static boolean isCommonShx(String name){
        String n=norm(new File(name).getName());
        for(String common:COMMON_SHX)if(n.equals(norm(common)))return true;
        return false;
    }

    private static String queryName(Context context,Uri uri){
        try(Cursor c=context.getContentResolver().query(uri,new String[]{OpenableColumns.DISPLAY_NAME},null,null,null)){
            if(c!=null&&c.moveToFirst()){String n=c.getString(0);if(n!=null&&!n.trim().isEmpty())return n.trim();}
        }catch(Exception ignored){}
        String last=uri.getLastPathSegment();return last==null?"font.ttf":last;
    }

    private static File uniqueTarget(File dir,String name){
        File f=new File(dir,name);if(!f.exists())return f;
        String base=stripExtension(name),ext=extension(name);
        for(int i=2;i<1000;i++){f=new File(dir,base+"_"+i+ext);if(!f.exists())return f;}
        return new File(dir,base+"_"+System.currentTimeMillis()+ext);
    }

    private static String safeFileName(String name){
        String n=new File(name==null?"":name).getName().replaceAll("[^A-Za-z0-9._ -]","_").trim();
        return n.isEmpty()?"font.ttf":n;
    }
    private static String cleanName(String s){return s==null?"":s.trim();}
    private static String norm(String s){return cleanName(s).toLowerCase(Locale.ROOT);}
    private static String stripExtension(String s){String n=new File(cleanName(s)).getName();int dot=n.lastIndexOf('.');return dot>0?n.substring(0,dot):n;}
    private static String extension(String s){int dot=s==null?-1:s.lastIndexOf('.');return dot>=0?s.substring(dot):"";}
    private CadFontManager(){}
}
