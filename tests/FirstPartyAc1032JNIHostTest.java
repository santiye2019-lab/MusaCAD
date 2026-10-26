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
        long[] report=new long[25];
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
        require(report[13]+report[16]==report[0]&&
                report[14]+report[15]==report[13]&&
                report[15]+report[16]==report[18]&&
                report[14]>=1L&&report[14]<=report[17]&&
                report[17]==(long)raw[1]&&
                report[19]==0L,
                "Native v6 source record census must match one signed-off"
                +" preview packet and MUST NOT certify full DWG display");
        require(report[20]+report[21]+report[22]+report[23]+
                   report[24]==report[0]&&
                report[21]+report[22]+report[23]<=report[16]&&
                report[20]<=report[13]+report[16],
                "Original R2018 TEXT/MTEXT candidates and unrendered BLOCK INSERT must"
                +" be authenticated without falsely counting source TEXT as other");
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
    /** Original real 2018/Text.dwg is MTEXT H1C0, not OT1 TEXT.
     * A source Unicode/single-line glyph preview is never full formatting. */
    private static void verifyMText(String path)throws IOException{
        long[] report=new long[25];
        float[] words=readOriginalNative(path,1_048_576,report);
        require(words!=null&&words.length>18&&words[0]==6f&&
                words[1]>0f&&words[6]==1f,
                "Authentic R2018 MTEXT-only source needs partial native packet");
        require(report[0]>0L&&report[13]>=1L&&
                report[14]>=1L&&report[14]<=report[17]&&
                report[17]==(long)words[1]&&report[19]==0L&&
                report[20]>=1L,
                "Verified original source MTEXT admission is independent"
                +" of exact final font/layout or colors");
        boolean found=false;
        for(int i=7;i<words.length;){
            int code=Math.round(words[i++]);
            int type;
            if(code<0){
                type=(-code-1)>>>8;
                require(i<words.length&&Float.isFinite(words[i]),
                        "Truncated v6 RGB");
                i++;
            }else type=code>>>8;
            if(type==1){require(i+4<=words.length,"LINE");i+=4;}
            else if(type==3){require(i+2<=words.length,"POINT");i+=2;}
            else if(type==4){require(i+8<=words.length,"CURVE");i+=8;}
            else if(type==2||type==5){
                require(i<words.length,"POLY");int n=Math.abs(Math.round(words[i++]));
                require(n>=2&&n<=100000&&n<=(words.length-i)/2,
                        "Invalid native POLY vertices");
                i+=2*n;
            }else if(type==6){
                require(i+9<=words.length,"Truncated original MTEXT record");
                int at=i;
                int bytes=Math.round(words[i+8]);
                require(bytes>0&&bytes<2048&&
                       ((long)bytes+2L)/3L<=(long)words.length-i-9L,
                       "Unbounded original MTEXT UTF-8 data");
                int n=(bytes+2)/3;
                byte[] utf8=new byte[bytes];
                i+=9;
                for(int wi=0;wi<n;wi++){
                    float f=words[i++];
                    require(f>=0f&&f<=16777215f&&f==Math.rint(f),
                            "Packed TEXT not exactly representable in v6");
                    int packed=Math.round(f);
                    for(int b=0;b<3;b++){
                        int offset=wi*3+b;
                        if(offset<bytes)utf8[offset]=(byte)((packed>>>(b*8))&255);
                    }
                }
                String value=new String(utf8,java.nio.charset.StandardCharsets.UTF_8);
                if(value.startsWith("This a multiline text")){
                    // Authentic H1C0 is source BYLAYER: its ACI7 is only
                    // a provisional preview fallback, not resolved layer color.
                    require(code==6*256+7,
                            "Real original MTEXT BYLAYER cannot claim direct RGB/ACI");
                    require(near(words[at],7.33298)&&
                            near(words[at+1],14.1158)&&
                            near(words[at+2],0.2)&&
                            near(words[at+3],0.0)&&
                            near(words[at+4],0.0)&&
                            near(words[at+5],-0.2)&&
                            near(words[at+6],0.0)&&
                            near(words[at+7],3.0),
                            "Original MTEXT H1C0 WCS/nominal size not retained");
                    found=true;
                }
            }else throw new AssertionError("Invalid MTEXT v6 record "+type);
        }
        require(found,"Original authentic source MTEXT Unicode/geometry"
             +" absent from Android JNI transport");
    }

    public static void main(String[] args)throws Exception{
        require(args.length==4,"Usage shared-lib Line.dwg Point.dwg Text.dwg");
        System.load(args[0]);
        float[] line=read(args[1]);
        verifyLine(line);
        float[] point=read(args[2]);
        verifyPoint(point);
        verifyMText(args[3]);
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
        // Old 13-field adapter must not silently receive an incomplete
        // subset of the now mandatory 20-slot original-source census.
        rejected=false;
        try {readOriginalNative(args[1],4096,new long[13]);}
        catch(IOException expected){rejected=true;}
        require(rejected,"Legacy audit shape must fail closed, not truncate");
        rejected=false;
        try {readOriginalNative(args[1],4096,new long[20]);}
        catch(IOException expected){rejected=true;}
        require(rejected,"Previous 20-slot audit must refuse silent family loss");
        System.out.println("PASS FIRST-PARTY Android-signature JNI:"
                +" original R2018 LINE and POINT XYZ, partial flag,"
                +" original ACI and provisional true R2018 MTEXT glyphs,"
                +" source-record vs emitted-primitive census,"
                +" zero-area POINT framing, file and capacity refusal");
    }
}
