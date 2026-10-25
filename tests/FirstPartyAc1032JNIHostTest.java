/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Host-only same-FQCN JNI invocation oracle. DOES NOT replace the actual
 * Android FirstPartyDwgPreview.java and NEVER enters app builds.
 * Exercises JNI with original AutoCAD 2018 DWG bytes via JVM host mode.
 */
package com.musa.cad;

import java.io.IOException;

final class FirstPartyDwgPreview {
    private static native float[] readOriginalNative(
            String inputPath,int maxFloatWords,long[] readiness) throws IOException;

    private static void require(boolean value,String label){
        if(!value)throw new AssertionError(label);
    }
    private static boolean near(float got,double reference){
        return Float.isFinite(got)&&Math.abs(got-reference)<0.0002;
    }
    private static float[] read(String path)throws IOException{
        long[] report=new long[13];
        float[] raw=readOriginalNative(path,1_048_576,report);
        require(raw!=null&&raw.length>=10,"Missing full JNI packet");
        require(report[0]>0&&report[1]>=1&&report[2]>=1,
                "Original indexed source and linked color/layer evidence absent");
        require(report[1]+report[8]==report[0]&&
                report[2]+report[7]==report[1]&&
                report[3]+report[4]+report[5]+report[6]==report[2],
                "Corrupt original DWG provenance count partition");
        require(report[9]>=1&&report[9]<=report[2]&&report[10]==0L,
                "Narrow LAYER CMC must NOT certify complete display");
        require(report[11]>=0L&&report[11]<=report[6],
                "Verified direct RGB must be a subset of source complex styles");
        require(report[12]>=0L&&report[12]<=report[6]-report[11],
                "Source entity-hidden evidence must exclude direct RGB source");
        require(raw[0]==6f&&raw[1]>0f&&raw[6]==1f,
                "Incomplete scene must be explicitly marked");
        for(float x:raw)require(Float.isFinite(x),"Invalid float");
        return raw;
    }
    private static void verifyLine(float[] words){
        boolean found=false;
        for(int i=7;i<words.length;){
            int code=Math.round(words[i++]);
            require(code>0&&(code&255)>=1&&(code&255)<=255,
                    "Invalid directly encoded or provisional ACI");
            int type=code>>>8;
            if(type==1){
                require(i+4<=words.length,"Truncated LINE");
                if(near(words[i],21.6078)&&near(words[i+1],2.81192)&&
                   near(words[i+2],11.0307)&&near(words[i+3],16.6375))
                    found=true;
                i+=4;
            }else if(type==3){
                require(i+2<=words.length,"Truncated POINT");i+=2;
            }else if(type==4){
                require(i+8<=words.length,"Truncated CURVE");i+=8;
            }else if(type==2){
                require(i<words.length,"Truncated POLY count");
                int n=Math.abs(Math.round(words[i++]));
                require(n>=2&&n<100001&&i+2*n<=words.length,"Bad POLY");
                i+=2*n;
            }else throw new AssertionError("Unknown v6 record type "+code);
        }
        require(found,"Original LINE source coordinates absent from JNI");
    }
    private static void verifyPoint(float[] words){
        boolean found=false;
        for(int i=7;i<words.length;){
            int code=Math.round(words[i++]);
            require(code>0&&(code&255)>=1&&(code&255)<=255,
                    "Invalid directly encoded or provisional ACI");
            int type=code>>>8;
            if(type==1){
                require(i+4<=words.length,"Truncated LINE");i+=4;
            }else if(type==3){
                require(i+2<=words.length,"Truncated POINT");
                if(near(words[i],27.928)&&near(words[i+1],8.65308))
                    found=true;
                i+=2;
            }else if(type==4){
                require(i+8<=words.length,"Truncated CURVE");i+=8;
            }else if(type==2){
                require(i<words.length,"Truncated POLY count");
                int n=Math.abs(Math.round(words[i++]));
                require(n>=2&&n<100001&&i+2*n<=words.length,"Bad POLY");
                i+=2*n;
            }else throw new AssertionError("Unknown v6 record type "+code);
        }
        require(found,"Original POINT source coordinates absent from JNI");
    }
    public static void main(String[] args)throws Exception{
        require(args.length==3,"Usage shared-lib Line.dwg Point.dwg");
        System.load(args[0]);
        float[] line=read(args[1]);
        verifyLine(line);
        float[] point=read(args[2]);
        verifyPoint(point);
        require(point[2]==point[4]&&point[3]==point[5],
                "Original POINT.dwg geometry must report honest zero-area source bounds");
        float[] pointFrame=NativeSceneViewportBounds.forScene(
                point[2],point[3],point[4],point[5]);
        require(pointFrame!=null&&pointFrame[2]>pointFrame[0]&&
                pointFrame[3]>pointFrame[1],
                "Android renderer must not reject source-verified POINT.dwg zero-area bounds");
        boolean rejected=false;
        long[] untouched=new long[12];
        java.util.Arrays.fill(untouched,919L);
        try {readOriginalNative(args[1],7,untouched);}catch(IOException expected){
            rejected=true;
        }
        require(rejected,"Insufficient float budget must fail closed");
        for(long n:untouched)require(n==919L,"Failed read mutated caller audit");
        rejected=false;
        try {readOriginalNative(args[1]+".missing",4096,untouched);}
        catch(IOException expected){rejected=true;}
        require(rejected,"Unknown file must fail closed");
        for(long n:untouched)require(n==919L,"Missing source mutated audit");
        rejected=false;
        try {readOriginalNative(args[1],4096,new long[11]);}
        catch(IOException expected){rejected=true;}
        require(rejected,"Inconsistent audit array must fail closed");
        System.out.println("PASS FIRST-PARTY Android-signature JNI:"
                +" original R2018 LINE and POINT XYZ, partial flag,"
                +" original ACI and POINT viewport framing, file and capacity refusal");
    }
}
