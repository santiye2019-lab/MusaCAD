package com.musa.cad;

import java.util.*;

/** Small deterministic helpers for speech-to-text results before they enter MusaCAD AI. */
public final class MusaAiVoiceText {
    public static String best(Collection<String> candidates){
        if(candidates==null)return "";
        for(String raw:candidates){
            String s=clean(raw);
            if(!s.isEmpty())return s;
        }
        return "";
    }

    public static String clean(String raw){
        if(raw==null)return "";
        String s=raw.replace('\r',' ').replace('\n',' ').replaceAll("\\s+"," ").trim();
        if(s.length()>1000)s=s.substring(0,1000).trim();
        return s;
    }

    private MusaAiVoiceText(){}
}
