package com.musa.cad;

import java.io.*;
import java.net.URLDecoder;
import java.net.URLEncoder;
import java.nio.charset.StandardCharsets;
import java.util.*;
import java.util.Locale;

/** Versioned MusaCAD-private DXF comments for raster placement persistence. */
public final class CadImageMetadata {
    public static final String PREFIX="MUSACAD_IMAGE_V1|";

    public static String encode(CadImagePlacement p){
        return PREFIX+esc(p.uri)+"|"+esc(p.name)+"|"+num(p.centerX)+"|"+num(p.centerY)+"|"+num(p.width)+"|"+num(p.height)+"|"+num(p.rotationDegrees);
    }
    public static CadImagePlacement decode(String value){
        if(value==null||!value.startsWith(PREFIX))return null;
        String[] p=value.substring(PREFIX.length()).split("\\|",-1);if(p.length!=7)return null;
        try{return new CadImagePlacement(unesc(p[0]),unesc(p[1]),Float.parseFloat(p[2]),Float.parseFloat(p[3]),Float.parseFloat(p[4]),Float.parseFloat(p[5]),Float.parseFloat(p[6]));}
        catch(Exception ignored){return null;}
    }
    public static boolean isMetadata(String value){return value!=null&&value.startsWith(PREFIX);}
    public static List<CadImagePlacement> read(File file)throws IOException{
        if(file==null||!file.isFile())return Collections.emptyList();ArrayList<CadImagePlacement> out=new ArrayList<>();
        try(BufferedReader in=new BufferedReader(new InputStreamReader(new FileInputStream(file),StandardCharsets.ISO_8859_1),64*1024)){
            String code,value;while((code=in.readLine())!=null&&(value=in.readLine())!=null)if("999".equals(code.trim())){CadImagePlacement p=decode(value.trim());if(p!=null)out.add(p);}
        }
        return out;
    }
    public static void writeComments(BufferedWriter out,List<CadImagePlacement> placements)throws IOException{
        if(out==null||placements==null)return;for(CadImagePlacement p:placements){if(p==null)continue;out.write("999");out.newLine();out.write(encode(p));out.newLine();}
    }
    private static String esc(String v){try{return URLEncoder.encode(v==null?"":v,StandardCharsets.UTF_8.name()).replace("+","%20");}catch(Exception e){return "";}}
    private static String unesc(String v)throws Exception{return URLDecoder.decode(v,StandardCharsets.UTF_8.name());}
    private static String num(float v){return String.format(Locale.US,"%.8g",v);}
    private CadImageMetadata(){}
}
