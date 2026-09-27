package com.musa.cad;

import java.io.*;
import java.net.URLDecoder;
import java.net.URLEncoder;
import java.nio.charset.StandardCharsets;
import java.util.ArrayList;
import java.util.Collections;
import java.util.List;
import java.util.Locale;

/** MusaCAD-private DXF 999 comments that preserve placed raster-image geometry. */
public final class CadImageMetadata {
    public static final String PREFIX="MUSACAD_IMAGE_V1|";

    public static String encode(CadImageOverlay image){
        if(image==null)throw new IllegalArgumentException("image");
        return PREFIX+escape(image.key)+"|"+escape(image.name)+"|"+
            number(image.centerX)+"|"+number(image.centerY)+"|"+number(image.width)+"|"+number(image.height)+"|"+number(image.rotationDegrees);
    }

    public static CadImageOverlay decode(String value){
        if(value==null||!value.startsWith(PREFIX))return null;
        String[] p=value.substring(PREFIX.length()).split("\\|",-1);
        if(p.length!=7)return null;
        try{
            String key=unescape(p[0]),name=unescape(p[1]);
            float x=Float.parseFloat(p[2]),y=Float.parseFloat(p[3]),w=Float.parseFloat(p[4]),h=Float.parseFloat(p[5]),rotation=Float.parseFloat(p[6]);
            return new CadImageOverlay(key,name,x,y,w,h,rotation);
        }catch(Exception ignored){return null;}
    }

    public static boolean isMetadata(String value){return value!=null&&value.startsWith(PREFIX);}

    public static List<CadImageOverlay> read(File file)throws IOException{
        if(file==null||!file.isFile())return Collections.emptyList();
        ArrayList<CadImageOverlay> out=new ArrayList<>();
        try(BufferedReader in=new BufferedReader(new InputStreamReader(new FileInputStream(file),StandardCharsets.ISO_8859_1))){
            String code,value;
            while((code=in.readLine())!=null&&(value=in.readLine())!=null){
                if("999".equals(code.trim())){
                    CadImageOverlay image=decode(value.trim());
                    if(image!=null)out.add(image);
                }
            }
        }
        return out;
    }

    public static void writeComments(BufferedWriter out,List<CadImageOverlay> images)throws IOException{
        if(out==null||images==null)return;
        for(CadImageOverlay image:images){
            if(image==null)continue;
            out.write("999");out.newLine();out.write(encode(image));out.newLine();
        }
    }

    private static String escape(String value){
        try{return URLEncoder.encode(value==null?"":value,StandardCharsets.UTF_8.name()).replace("+","%20");}
        catch(Exception e){return "";}
    }
    private static String unescape(String value)throws Exception{return URLDecoder.decode(value,StandardCharsets.UTF_8.name());}
    private static String number(float value){return String.format(Locale.US,"%.8g",value);}
    private CadImageMetadata(){}
}
