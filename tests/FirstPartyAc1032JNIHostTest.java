/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Host-only same-FQCN JNI invocation oracle. DOES NOT replace the actual
 * Android FirstPartyDwgPreview.java and NEVER enters app builds.
 * Exercises JNI with original AutoCAD 2018 DWG bytes via JVM host mode.
 */
package com.musa.cad;

import java.io.IOException;

final class FirstPartyDwgPreview {
    private static native float[] readOriginalNative(
            String inputPath,int maxFloatWords) throws IOException;

    private static void require(boolean value,String label){
        if(!value)throw new AssertionError(label);
    }
    private static boolean near(float got,double reference){
        return Float.isFinite(got)&&Math.abs(got-reference)<0.0002;
    }
    private static float[] read(String path)throws IOException{
        float[] raw=readOriginalNative(path,1_048_576);
        require(raw!=null&&raw.length>=10,"Missing full JNI packet");
        require(raw[0]==6f&&raw[1]>0f&&raw[6]==1f,
                "Incomplete scene must be explicitly marked");
        for(float x:raw)require(Float.isFinite(x),"Invalid float");
        return raw;
    }
    private static void verifyLine(float[] words){
        boolean found=false;
        for(int i=7;i<words.length;){
            int code=Math.round(words[i++]);
            if(code==263){
                require(i+4<=words.length,"Truncated LINE");
                if(near(words[i],21.6078)&&near(words[i+1],2.81192)&&
                   near(words[i+2],11.0307)&&near(words[i+3],16.6375))
                    found=true;
                i+=4;
            }else if(code==775){
                require(i+2<=words.length,"Truncated POINT");i+=2;
            }else if(code==1031){
                require(i+8<=words.length,"Truncated CURVE");i+=8;
            }else if(code==519){
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
            if(code==263){
                require(i+4<=words.length,"Truncated LINE");i+=4;
            }else if(code==775){
                require(i+2<=words.length,"Truncated POINT");
                if(near(words[i],27.928)&&near(words[i+1],8.65308))
                    found=true;
                i+=2;
            }else if(code==1031){
                require(i+8<=words.length,"Truncated CURVE");i+=8;
            }else if(code==519){
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
        boolean rejected=false;
        try {readOriginalNative(args[1],7);}catch(IOException expected){
            rejected=true;
        }
        require(rejected,"Insufficient float budget must fail closed");
        rejected=false;
        try {readOriginalNative(args[1]+".missing",4096);}
        catch(IOException expected){rejected=true;}
        require(rejected,"Unknown file must fail closed");
        System.out.println("PASS FIRST-PARTY Android-signature JNI:"
                +" original R2018 LINE and POINT XYZ, partial flag,"
                +" file and capacity refusal");
    }
}
