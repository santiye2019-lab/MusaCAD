/* SPDX-License-Identifier: GPL-3.0-or-later */
package com.musa.cad;
import java.io.IOException;

/** Real original R2018 source ACI evidence through the same Android JNI
 * entrypoint, independent of the numeric ACI6 non-source renderer.
 * Companion DXF oracle H1BB/H8A ACI4 is pinned in native CMake gold test. */
final class FirstPartyDwgPreview {
    private static native long[] readCircleLayerAciEvidenceNative(
        String path,int maxRows,long[] audit)throws IOException;
    private static void check(boolean b,String message){
        if(!b)throw new AssertionError(message);
    }
    public static void main(String[] args)throws Exception{
        check(args.length==2,"Need JNI .so and authentic R2018 DWG");
        System.load(args[0]);
        long[] audit=new long[10];
        long[] rows=readCircleLayerAciEvidenceNative(args[1],256,audit);
        check(rows!=null&&rows.length>0&&rows.length==6L*audit[7],
              "Original DWG source ACI rows absent or malformed");
        check(audit[0]>0&&audit[1]>0&&audit[2]>0&&
              audit[2]+audit[3]==audit[1],
              "Original indexed INSERT partition");
        check(audit[5]>0&&audit[7]>0&&audit[7]+audit[6]==audit[5],
              "Original indexed CIRCLE partition");
        check(audit[8]==1&&audit[9]==0,"Source-only never final paint");
        int match=0;
        for(int i=0;i<rows.length;i+=6){
            check(rows[i]>0&&rows[i+1]>0&&rows[i+2]>0&&
                  rows[i+3]>0&&rows[i+4]>=1&&rows[i+4]<=255&&
                  rows[i+5]>=0&&rows[i+5]<=65535,
                  "Corrupt original source ACI record");
            if(rows[i]==0x1c8&&rows[i+1]==0x1b8&&
               rows[i+2]==0x1bb&&rows[i+3]==0x8a){
                check(rows[i+4]==4,"Independent R2018 DXF H8A ACI4 mismatch");
                match++;
            }
        }
        check(match==1,"Genuine H1C8/H1BB/H8A ACI4 source chain");
        boolean refused=false;
        try{
            readCircleLayerAciEvidenceNative(args[1],0,new long[10]);
        }catch(IOException expected){refused=true;}
        check(refused,"Zero-row budget must fail closed");
        refused=false;
        try{
            readCircleLayerAciEvidenceNative(args[1],256,new long[9]);
        }catch(IOException expected){refused=true;}
        check(refused,"Wrong audit array must fail closed");
        refused=false;
        try{
            readCircleLayerAciEvidenceNative(args[1]+".missing",256,
                                             new long[10]);
        }catch(IOException expected){refused=true;}
        check(refused,"Missing original file must fail closed");
        System.out.println("PASS original R2018 CIRCLE JNI source ACI4; "+
            audit[7]+" rows, unresolved="+audit[6]+", final paint=0");
    }
}
