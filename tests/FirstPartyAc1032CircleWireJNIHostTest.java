/* SPDX-License-Identifier: GPL-3.0-or-later */
package com.musa.cad;
import java.io.IOException;

/** Same-class JNI host acceptance for REAL unchanged R2018 source CIRCLE.
 * Color code ACI6 is purely diagnostic, never sourced from original DWG. */
final class FirstPartyDwgPreview {
    private static native float[] readCircleGeometryEvidenceNative(
        String file,int budget,long[] evidence)throws IOException;
    private static void verify(boolean value,String label){
        if(!value)throw new AssertionError(label);
    }
    public static void main(String[] args)throws Exception{
        verify(args.length==2,"Need JNI .so and genuine original DWG");
        System.load(args[0]);
        long[] audit=new long[11];
        float[] raw=readCircleGeometryEvidenceNative(args[1],2*1024*1024,audit);
        verify(raw!=null&&raw.length>=139&&
               raw.length==7+132L*audit[8],"Positive original CIRCLE geometry");
        verify(audit[0]>0&&audit[1]>=3&&audit[2]>=3&&
               audit[2]+audit[3]==audit[1],"Original INSERT coverage partition");
        verify(audit[4]>=1&&audit[5]>=1&&audit[6]>=0&&
               audit[5]+audit[6]==audit[4],"Original CIRCLE numeric evidence");
        verify(audit[7]>=0&&audit[7]<audit[5],
               "Genuine H1BB owner/color must remain UNRESOLVED");
        verify(audit[8]==audit[5]&&audit[9]==raw.length&&audit[10]==1,
               "Every numeric CIRCLE is wireframed; synthetic hue only");
        verify(raw[0]==6f&&raw[1]==(float)audit[8]&&raw[6]==1f,
               "Partial v6 scene never claims final paint");
        int n=0;
        for(int at=7;at<raw.length;at+=132){
            verify(raw[at]==518f&&raw[at+1]==65f,
                   "Diagnostic ACI6 poly, not source DWG color");
            verify(raw[at+2]==raw[at+130]&&
                   raw[at+3]==raw[at+131],"Closed WCS ellipse chord");
            for(int j=0;j<65;j++){
                float x=raw[at+2+j*2],y=raw[at+3+j*2];
                verify(Float.isFinite(x)&&Float.isFinite(y),
                       "Non-finite WCS affine CIRCLE");
                verify(x>=raw[2]&&x<=raw[4]&&
                       y>=raw[3]&&y<=raw[5],"Bounds exclude WCS point");
            }
            n++;
        }
        verify(n==audit[8],"Real source numeric CIRCLE count");
        System.out.println("PASS original R2018 numeric CIRCLE JNI: "+
            n+" transformed non-source-magenta wireframes; "+
            audit[7]+" unknown owners/layers; full visibility=0");
    }
}
