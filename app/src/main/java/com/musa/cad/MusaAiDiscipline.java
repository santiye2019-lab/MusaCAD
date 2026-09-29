package com.musa.cad;

import java.util.*;

/** Shared discipline taxonomy for MusaCAD AI project review, takeoff and reports. */
public enum MusaAiDiscipline {
    ARCHITECTURAL("MIM","Mimari"),
    STRUCTURAL("STA","Statik"),
    MECHANICAL("MEK","Mekanik"),
    ELECTRICAL("ELK","Elektrik"),
    LANDSCAPE("PEY","Peyzaj"),
    INFRASTRUCTURE("ALT","Altyapı"),
    ELEVATOR("ASN","Asansör"),
    FIRE_SAFETY("YNG","Yangın ve Can Güvenliği"),
    UNKNOWN("GEN","Genel");

    public final String code,label;
    MusaAiDiscipline(String code,String label){this.code=code;this.label=label;}

    public static MusaAiDiscipline fromQuery(String raw){
        String q=MusaAiDrawingIndex.normalize(raw);
        if(q.isEmpty())return UNKNOWN;
        if(has(q,"mimari","architect"))return ARCHITECTURAL;
        if(has(q,"statik","structural","kolon","kiris","perde","doseme","temel","radye","kazik",
            "zimbalama","modal analiz","modal","kat otelemesi","goreli kat otelemesi","story drift",
            "burulma","torsion","yumusak kat","soft story","zayif kat","weak story",
            "guclu kolon","strong column","zayif kiris","weak beam","sarilma bolgesi","confinement",
            "transfer kiris","transfer doseme","transfer kat","konsol","cantilever"))return STRUCTURAL;
        if(has(q,"mekanik","hvac","vrf","pis su","temiz su","havalandirma","isitma","sogutma"))return MECHANICAL;
        if(has(q,"elektrik","kuvvetli akim","zayif akim","kablo","pano","aydinlatma","topraklama","jenerator","ups"))return ELECTRICAL;
        if(has(q,"peyzaj","bitkilendirme","sulama","sert zemin","yesil alan"))return LANDSCAPE;
        if(has(q,"altyapi","alt yapi","kanalizasyon","rog ar","rogar","telekom","isale","sebek e","sebeke"))return INFRASTRUCTURE;
        if(has(q,"asansor","elevator","lift","kuyu dibi","kuyu ustu"))return ELEVATOR;
        if(has(q,"yangin","sprinkler","hidrant","duman","basinclandirma","itfaiye"))return FIRE_SAFETY;
        return UNKNOWN;
    }

    /** Conservative discipline inference from CAD layer/text or discovery/estimate description. */
    public static MusaAiDiscipline classify(String raw){
        String q=MusaAiDrawingIndex.normalize(raw);
        if(q.isEmpty())return UNKNOWN;
        if(has(q,"yangin","sprink","hidrant","fkc","fire","duman","basinclandirma","itfaiye"))return FIRE_SAFETY;
        if(has(q,"asansor","elevator","lift","kuyu","ray","kabin","makine dairesi"))return ELEVATOR;
        if(has(q,"elektrik","elk","kablo","tava","pano","priz","armat ur","armatur","aydinlat","toprak","paratoner","jenerator","ups","data","cctv","zayif akim"))return ELECTRICAL;
        if(has(q,"mekanik","mek","pis su","atik su","temiz su","sihhi","vrf","hvac","havaland","kanal","fan","klima","isitma","sogutma","kazan","hidrofor","pompa","dogalgaz","dogal gaz","boyler"))return MECHANICAL;
        if(has(q,"statik","beton","betonarme","donati","kolon","kiris","perde","doseme","temel","radye","fore kazik","kazik",
            "zimbalama","modal analiz","kat otelemesi","goreli kat otelemesi","burulma","yumusak kat","zayif kat",
            "guclu kolon","zayif kiris","sarilma bolgesi","transfer kiris","transfer doseme","transfer kat","konsol"))return STRUCTURAL;
        if(has(q,"peyzaj","bitki","agac","sulama","cim","sert zemin","yumusak zemin","bordur","peyz"))return LANDSCAPE;
        if(has(q,"altyapi","kanalizasyon","rogar","yagmur suyu","drenaj","telekom","dogalgaz hatti","icme suyu","kaz i","kazi","dolgu"))return INFRASTRUCTURE;
        if(has(q,"mimari","mim","duvar","kapi","pencere","mahal","seramik","boya","asma tavan","cephe","cati","merdiven","rampa"))return ARCHITECTURAL;
        return UNKNOWN;
    }

    public static List<MusaAiDiscipline> engineering(){
        return Arrays.asList(ARCHITECTURAL,STRUCTURAL,MECHANICAL,ELECTRICAL,LANDSCAPE,INFRASTRUCTURE,ELEVATOR,FIRE_SAFETY);
    }

    private static boolean has(String q,String... terms){
        for(String t:terms)if(q.contains(MusaAiDrawingIndex.normalize(t)))return true;
        return false;
    }
}
