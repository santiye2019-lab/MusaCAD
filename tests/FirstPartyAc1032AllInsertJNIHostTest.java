/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Host JVM invokes the EXACT JNI class + symbol Android uses.
 * It checks actual original 2018 DWG source v6 words and audits;
 * this does not certify full layers, visibility, paint or DWG editing. */
package com.musa.cad;
import java.io.IOException;
final class FirstPartyDwgPreview {
    private static native float[] readAllInsertLinesNative(
            String path,int maxWords,long[] audit)throws IOException;
    private static void require(boolean yes,String why){
        if(!yes)throw new AssertionError(why);
    }
    public static void main(String[] args)throws Exception{
        require(args.length==2,"Provide JNI host .so and original 2018 DWG");
        System.load(args[0]);
        long[] audit=new long[11];
        float[] raw=readAllInsertLinesNative(args[1],1_048_576,audit);
        require(raw!=null&&raw.length>=7&&raw.length<=1048576,
                "Actual original R2018 aggregate source missing");
        require(audit[0]>0&&audit[0]<=1_000_000L,"Index");
        require(audit[1]>=3&&audit[1]<=4096,"Authentic H190,H1C8,H756 candidates");
        require(audit[2]>=3&&audit[3]>=0&&audit[2]+audit[3]==audit[1],
                "Independently verified originals and unsupported partition");
        require(audit[4]>=audit[5]&&audit[4]==audit[5]+audit[6],
                "Authenticated LINE source to admitted and omitted partition");
        require(audit[7]==raw.length&&audit[9]==1&&audit[10]==0,
                "Always-partial packet, never final paint");
        require((audit[5]>0)==(audit[8]==1),"Bounds only with emitted segments");
        require(raw[0]==6f&&raw[1]==(float)audit[5]&&raw[6]==1f,
                "NativeScene v6 compatibility and partial marker");
        int count=0;
        for(int i=7;i<raw.length;){
            float code=raw[i++];
            require(Float.isFinite(code)&&code==Math.rint(code),"Bad v6 type");
            if(code<0f){
                require(code==-258f&&i<raw.length,"Only original direct RGB LINE");
                float rgb=raw[i++];
                require(rgb>=0f&&rgb<=16777215f&&rgb==Math.rint(rgb),
                        "RGB exact 24-bit float");
            }else{
                require(code>=257f&&code<=511f&&code==Math.rint(code),
                        "Only original directly encoded ACI LINE");
            }
            require(i+4<=raw.length,"Truncated source WCS line");
            for(int c=0;c<4;c++)require(Float.isFinite(raw[i+c]),
                                         "Non-finite real WCS coordinates");
            require(raw[i]!=raw[i+2]||raw[i+1]!=raw[i+3],
                    "Float-collapsed INSERT segment cannot be painted");
            i+=4;
            count++;
        }
        require(count==audit[5],"Exact visible LINE primitive count");
        if(count==0)require(raw.length==7,"Do not fabricate empty 2D scene");
        System.out.println("PASS real R2018 multi-INSERT Android JNI: "+
                "verified="+audit[2]+" original LINE="+audit[4]+
                " visible source-only LINE="+count+
                " omitted="+audit[6]+" partial=1 paint=0");
    }
}
