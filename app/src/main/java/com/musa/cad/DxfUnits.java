package com.musa.cad;

import java.util.List;

/** AutoCAD $INSUNITS metadata used for automatic model-space measurements. */
public final class DxfUnits {
    public static int parse(List<String> tags){
        if(tags==null)return 0;String section="";
        for(int i=0;i+1<tags.size();i+=2){
            int code=code(tags.get(i));if(code==Integer.MIN_VALUE)continue;String value=tags.get(i+1).trim();
            if(code==0&&"SECTION".equals(value)){
                if(i+3<tags.size()&&code(tags.get(i+2))==2)section=tags.get(i+3).trim();
                continue;
            }
            if(code==0&&"ENDSEC".equals(value)){section="";continue;}
            if("HEADER".equals(section)&&code==9&&"$INSUNITS".equalsIgnoreCase(value)){
                for(int j=i+2;j+1<tags.size();j+=2){int c=code(tags.get(j));if(c==9||c==0)break;if(c==70)return unitCode(tags.get(j+1));}
            }
        }
        return 0;
    }

    /** Parse one streaming HEADER record containing multiple group-9 variables. */
    public static int parseHeaderRecord(List<String> tags){
        if(tags==null)return 0;
        for(int i=0;i+1<tags.size();i+=2){
            if(code(tags.get(i))==9&&"$INSUNITS".equalsIgnoreCase(tags.get(i+1).trim())){
                for(int j=i+2;j+1<tags.size();j+=2){int c=code(tags.get(j));if(c==9)break;if(c==70)return unitCode(tags.get(j+1));}
            }
        }
        return 0;
    }

    public static boolean known(int code){return code>=1&&code<=24;}

    public static String symbol(int code){
        switch(code){
            case 1:return "in"; case 2:return "ft"; case 3:return "mi"; case 4:return "mm"; case 5:return "cm"; case 6:return "m";
            case 7:return "km"; case 8:return "µin"; case 9:return "mil"; case 10:return "yd"; case 11:return "Å"; case 12:return "nm";
            case 13:return "µm"; case 14:return "dm"; case 15:return "dam"; case 16:return "hm"; case 17:return "Gm"; case 18:return "AU";
            case 19:return "ly"; case 20:return "pc"; case 21:return "US ft"; case 22:return "US in"; case 23:return "US yd"; case 24:return "US mi";
            default:return "";
        }
    }

    public static String name(int code){
        switch(code){
            case 1:return "inç"; case 2:return "fit"; case 3:return "mil"; case 4:return "milimetre"; case 5:return "santimetre"; case 6:return "metre";
            case 7:return "kilometre"; case 8:return "mikroinç"; case 9:return "mil (0,001 inç)"; case 10:return "yard"; case 11:return "angstrom";
            case 12:return "nanometre"; case 13:return "mikrometre"; case 14:return "desimetre"; case 15:return "dekametre"; case 16:return "hektometre";
            case 17:return "gigametre"; case 18:return "astronomik birim"; case 19:return "ışık yılı"; case 20:return "parsek";
            case 21:return "US survey fit"; case 22:return "US survey inç"; case 23:return "US survey yard"; case 24:return "US survey mil";
            default:return "belirtilmemiş";
        }
    }

    private static int unitCode(String raw){try{int n=Integer.parseInt(raw.trim());return known(n)?n:0;}catch(Exception e){return 0;}}
    private static int code(String raw){try{return Integer.parseInt(raw.trim());}catch(Exception e){return Integer.MIN_VALUE;}}
    private DxfUnits(){}
}
