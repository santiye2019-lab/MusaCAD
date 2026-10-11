package com.musa.cad;

import android.accounts.Account;
import android.content.Context;
import android.content.SharedPreferences;
import com.google.android.gms.auth.GoogleAuthUtil;
import com.google.android.gms.auth.UserRecoverableAuthException;
import com.google.android.gms.auth.api.signin.GoogleSignIn;
import com.google.android.gms.auth.api.signin.GoogleSignInAccount;
import java.io.File;
import java.util.List;
import java.util.concurrent.atomic.AtomicLong;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;

/**
 * Explicitly opt-in Drive-only transport. Does not claim Gandalf server indexing.
 * Google authorization must have been granted interactively first.
 * OAuth access tokens are held in memory for the duration of a transfer only.
 */
final class MusaGoogleDriveSync {
    private static final String PREF="musacad_drive_sync_v1";
    private static final AtomicLong LATEST=new AtomicLong();
    private static final ExecutorService QUEUE=Executors.newSingleThreadExecutor(r->{
        Thread t=new Thread(r,"MusaCAD-drive-sync");t.setPriority(Thread.MIN_PRIORITY);return t;
    });
    static boolean enabled(Context context){
        return context.getSharedPreferences(PREF,0).getBoolean("enabled",false);
    }
    static void setEnabled(Context context,boolean enabled){
        context.getSharedPreferences(PREF,0).edit().putBoolean("enabled",enabled).apply();
    }
    static void submit(Context context,List<MusaAiProjectSync.Entry> projects,
                       MusaAiProjectSync.Listener listener){
        if(!enabled(context)||projects==null||projects.isEmpty()||projects.size()>4)return;
        Context app=context.getApplicationContext();
        List<MusaAiProjectSync.Entry> snapshot=new java.util.ArrayList<>(projects);
        final long revision=LATEST.incrementAndGet();
        QUEUE.execute(()->{
            if(revision!=LATEST.get()||!enabled(app))return;
            GoogleSignInAccount signed=GoogleSignIn.getLastSignedInAccount(app);
            Account account=signed==null?null:signed.getAccount();
            if(account==null||!GoogleSignIn.hasPermissions(signed,
                new com.google.android.gms.common.api.Scope(MusaGoogleDriveTransport.SCOPE))){
                notify(listener,snapshot.get(0),-1,"Google Drive izni gerekli");return;
            }
            String token=null;
            try{
                token=GoogleAuthUtil.getToken(app,account,"oauth2:"+MusaGoogleDriveTransport.SCOPE);
                SharedPreferences pref=app.getSharedPreferences(PREF,0);
                for(MusaAiProjectSync.Entry entry:snapshot){
                    if(!enabled(app)||revision!=LATEST.get())return;
                    File file=entry.file;
                    if(file==null||!file.isFile())continue;
                    long size=file.length();
                    long modified=file.lastModified();
                    if(size<1||size>512L*1024*1024){
                        notify(listener,entry,-1,"Drive aktarımı için dosya boyutu uygun değil");continue;
                    }
                    String fingerprint=entry.id+":"+size+":"+modified;
                    if(fingerprint.equals(pref.getString("done_"+entry.id,null))
                        &&!pref.getString("file_"+entry.id,"").isEmpty()){
                        notify(listener,entry,99,"Drive dosyası henüz yeniden doğrulanmadı");
                        continue;
                    }
                    notify(listener,entry,0,"Google Drive aktarımı hazırlanıyor");
                    String session=MusaGoogleDriveTransport.begin(token,entry.name,size,null);
                    MusaGoogleDriveTransport.Upload result=MusaGoogleDriveTransport.send(
                        token,session,file,p->{
                            if(revision==LATEST.get()&&enabled(app))
                                notify(listener,entry,Math.min(99,p),"Google Drive yükleme • %"+Math.min(99,p));
                        },()->revision!=LATEST.get()||!enabled(app));
                    if(result.size!=size)throw new java.io.IOException("Drive size mismatch");
                    if(file.length()!=size||file.lastModified()!=modified)
                        throw new java.io.IOException("Local drawing changed during upload");
                    if(revision!=LATEST.get()||!enabled(app))return;
                    // No local file is ever deleted. Avoid saving OAuth tokens or upload session URLs.
                    pref.edit().putString("done_"+entry.id,fingerprint)
                        .putString("file_"+entry.id,result.id).apply();
                    notify(listener,entry,100,"Google Drive'a aktarıldı; AI sunucu indeksi ayrı");
                }
            }catch(UserRecoverableAuthException ex){
                if(revision==LATEST.get())notify(listener,snapshot.get(0),-1,
                    "Google Drive erişimi için hesap izni yeniden gerekli");
            }catch(Exception ex){
                if(revision==LATEST.get())notify(listener,snapshot.get(0),-1,
                    "Google Drive aktarımı başarısız; tekrar deneyin");
            }finally{
                token=null;
            }
        });
    }
    private static void notify(MusaAiProjectSync.Listener listener,
                               MusaAiProjectSync.Entry entry,int percent,String message){
        if(listener!=null)listener.status(entry.id,percent,message);
    }
    private MusaGoogleDriveSync(){}
}
