package com.musa.cad;

import java.io.IOException;
import java.util.Arrays;

/**
 * Bounded, lossless concatenation of TWO independently decoded original
 * AC1032 partial v6 source packets. Does not resolve block ownership,
 * model/paper space, layer off/freeze, XREF, BYLAYER/BYBLOCK or draw order.
 * Primarily used by the opt-in Android 2B diagnostic, NEVER normal CAD open.
 *
 * Checks every full record before copying and rejects any malformed/partial
 * record. All original color codes, Unicode-packed words and world positions
 * are preserved exactly. No fallback geometry/colors are generated.
 */
final class OriginalSourceV6Merge {
    private OriginalSourceV6Merge(){}

    static int count(float[] packet,int maximumWords)throws IOException{
        if(packet==null||packet.length<7||packet.length>maximumWords||
           maximumWords<7||packet[0]!=6f||packet[6]!=1f||
           !Float.isFinite(packet[1])||packet[1]<0f||
           packet[1]!=Math.rint(packet[1])||
           packet[1]>packet.length-7)
            throw new IOException("Kismi DWG v6 basligi gecersiz");
        int declared=(int)packet[1];
        for(int j=2;j<=5;j++)
            if(!Float.isFinite(packet[j]))
                throw new IOException("DWG kaynak koordinati sonlu degil");
        if(declared>0&&(packet[2]>packet[4]||packet[3]>packet[5]))
            throw new IOException("DWG kaynak siniri ters");
        if(declared==0&&packet.length!=7)
            throw new IOException("Bos DWG kaynak paketinde sahte veri");
        int p=7,n=0;
        while(p<packet.length){
            float encodedFloat=packet[p++];
            if(!Float.isFinite(encodedFloat)||
               encodedFloat!=Math.rint(encodedFloat)||
               encodedFloat<Integer.MIN_VALUE||
               encodedFloat>Integer.MAX_VALUE)
                throw new IOException("DWG v6 tur alani bozuk");
            int code=(int)encodedFloat;
            int type;
            if(code<0){
                type=(-code-1)>>>8;
                if(type<1||type>6||code!=-(type*256+2)||
                   p>=packet.length)
                    throw new IOException("DWG v6 RGB kodu bozuk");
                float rgb=packet[p++];
                if(!Float.isFinite(rgb)||rgb<0f||rgb>16777215f||
                   rgb!=Math.rint(rgb))
                    throw new IOException("DWG v6 RGB24 bozuk");
            }else{
                type=code>>>8;
                if(type<1||type>6||code<=0|| (code&255)>255)
                    throw new IOException("DWG v6 ACI kodu bozuk");
            }
            int needed;
            if(type==1)needed=4;
            else if(type==3)needed=2;
            else if(type==4)needed=8;
            else if(type==2||type==5){
                if(p>=packet.length)throw new IOException("DWG cokgen uzunlugu yok");
                float vertices=packet[p++];
                if(!Float.isFinite(vertices)||vertices!=Math.rint(vertices)||
                   vertices==Integer.MIN_VALUE||
                   Math.abs(vertices)>(packet.length-p)/2f||
                   Math.abs(vertices)>100000f||
                   Math.abs(vertices)<(type==5?3f:2f))
                    throw new IOException("DWG cokgen verisi gecersiz");
                needed=(int)Math.abs(vertices)*2;
            }else{
                if(packet.length-p<9)
                    throw new IOException("DWG metin basligi kesik");
                float bytes=packet[p+8];
                if(!Float.isFinite(bytes)||bytes!=Math.rint(bytes)||
                   bytes<=0f||bytes>1048576f)
                    throw new IOException("DWG metin uzunlugu gecersiz");
                int packed=((int)bytes+2)/3;
                if(packed>packet.length-p-9)
                    throw new IOException("DWG metin baytlari eksik");
                needed=9+packed;
            }
            if(needed>packet.length-p)
                throw new IOException("DWG v6 kaynak nesnesi eksik");
            for(int j=0;j<needed;j++)
                if(!Float.isFinite(packet[p+j]))
                    throw new IOException("DWG v6 kaynak nesnesi sonlu degil");
            if(type==6){
                for(int j=9;j<needed;j++){
                    float packed=packet[p+j];
                    if(packed<0f||packed>16777215f||
                       packed!=Math.rint(packed))
                        throw new IOException("DWG kaynak UTF8 bayti bozuk");
                }
            }
            p+=needed;
            n++;
            if(n>declared)throw new IOException("DWG kaynak nesnesi fazladan");
        }
        if(n!=declared)
            throw new IOException("DWG kaynak nesnesi sayaci uyusmuyor");
        return n;
    }

    static float[] join(float[] primary,float[] instances,int maxWords)
            throws IOException{
        int a=count(primary,maxWords);
        int b=count(instances,maxWords);
        if(a==0)throw new IOException("Ana DWG kaynak sahnesi bos");
        if(b==0)return Arrays.copyOf(primary,primary.length);
        long total=(long)primary.length+instances.length-7L;
        if(total>maxWords||total>Integer.MAX_VALUE||
           (long)a+b>16777215L)
            throw new IOException("Birlesik kaynak geometri bellek sinirini asti");
        float[] merged=Arrays.copyOf(primary,(int)total);
        System.arraycopy(instances,7,merged,primary.length,instances.length-7);
        merged[1]=(float)(a+b);
        merged[2]=Math.min(primary[2],instances[2]);
        merged[3]=Math.min(primary[3],instances[3]);
        merged[4]=Math.max(primary[4],instances[4]);
        merged[5]=Math.max(primary[5],instances[5]);
        merged[6]=1f; // Always source-only incomplete, never final paint.
        if(count(merged,maxWords)!=a+b)
            throw new IOException("Birlesik kaynak DWG paketi bozuk");
        return merged;
    }
    /** Append genuine WCS numeric CIRCLE evidence to the already partial
     * direct+INSERT LINE streams. All 65-point ACI6 outlines are explicitly
     * NON-SOURCE-color diagnostics; no owner, layer or viewport is inferred.
     * No packet is changed on failure. */
    static float[] joinWithDiagnosticCircles(float[] direct,float[] lines,
            float[] numericCircles,int maxWords)throws IOException{
        int n=count(numericCircles,maxWords);
        if(n>1024||numericCircles.length!=7+132*n)
            throw new IOException("Sayisal CIRCLE tani kaydi boyutu gecersiz");
        int at=7;
        for(int k=0;k<n;k++,at+=132){
            if(numericCircles[at]!=518f||numericCircles[at+1]!=65f)
                throw new IOException("CIRCLE orijinal renk gibi sunulamaz");
            if(numericCircles[at+2]!=numericCircles[at+130]||
               numericCircles[at+3]!=numericCircles[at+131])
                throw new IOException("CIRCLE WCS tel-kafesi kapanmadi");
            for(int j=0;j<65;j++){
                float x=numericCircles[at+2+2*j];
                float y=numericCircles[at+3+2*j];
                if(!Float.isFinite(x)||!Float.isFinite(y)||
                   x<numericCircles[2]||x>numericCircles[4]||
                   y<numericCircles[3]||y>numericCircles[5])
                    throw new IOException("CIRCLE WCS noktasi kaynak sinirini asti");
            }
        }
        float[] partial=join(direct,lines,maxWords);
        return join(partial,numericCircles,maxWords);
    }

}
