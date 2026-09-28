package com.musa.cad;

import android.app.Activity;
import android.content.ActivityNotFoundException;
import android.content.Intent;
import android.speech.RecognizerIntent;
import java.lang.ref.WeakReference;
import java.util.*;

/**
 * Launches the device speech-recognition surface for MusaCAD AI.
 *
 * MusaCAD itself does not record or persist microphone audio. Recognition is
 * delegated to the Android speech service selected on the device. Offline
 * recognition is requested when the service supports it, but is not guaranteed.
 */
public final class MusaAiVoiceInput {
    public interface Callback {
        void onText(String text);
        void onStatus(String text);
    }

    public static final int REQUEST_CODE=49071;
    private static WeakReference<Callback> pending=new WeakReference<>(null);

    public static boolean launch(Activity activity,Callback callback){
        if(activity==null||callback==null)return false;
        Intent intent=new Intent(RecognizerIntent.ACTION_RECOGNIZE_SPEECH);
        intent.putExtra(RecognizerIntent.EXTRA_LANGUAGE_MODEL,RecognizerIntent.LANGUAGE_MODEL_FREE_FORM);
        intent.putExtra(RecognizerIntent.EXTRA_LANGUAGE,"tr-TR");
        intent.putExtra(RecognizerIntent.EXTRA_LANGUAGE_PREFERENCE,"tr-TR");
        intent.putExtra(RecognizerIntent.EXTRA_ONLY_RETURN_LANGUAGE_PREFERENCE,false);
        intent.putExtra(RecognizerIntent.EXTRA_MAX_RESULTS,5);
        intent.putExtra(RecognizerIntent.EXTRA_PREFER_OFFLINE,true);
        intent.putExtra(RecognizerIntent.EXTRA_PROMPT,"MusaCAD AI • Komutunuzu söyleyin");
        pending=new WeakReference<>(callback);
        try{
            activity.startActivityForResult(intent,REQUEST_CODE);
            return true;
        }catch(ActivityNotFoundException e){
            clear(callback);
            callback.onStatus("Bu telefonda kullanılabilir bir konuşma tanıma hizmeti bulunamadı.");
            return false;
        }catch(Exception e){
            clear(callback);
            callback.onStatus("Sesli giriş başlatılamadı.");
            return false;
        }
    }

    public static boolean handleActivityResult(int requestCode,int resultCode,Intent data){
        if(requestCode!=REQUEST_CODE)return false;
        Callback callback=pending.get();
        pending.clear();
        if(callback==null)return true;
        if(resultCode!=Activity.RESULT_OK){
            callback.onStatus("Sesli giriş iptal edildi.");
            return true;
        }
        ArrayList<String>results=data==null?null:data.getStringArrayListExtra(RecognizerIntent.EXTRA_RESULTS);
        String text=MusaAiVoiceText.best(results);
        if(text.isEmpty())callback.onStatus("Konuşma anlaşılamadı. Tekrar deneyin.");
        else callback.onText(text);
        return true;
    }

    public static void clear(Callback callback){
        Callback current=pending.get();
        if(callback==null||current==callback)pending.clear();
    }

    private MusaAiVoiceInput(){}
}
