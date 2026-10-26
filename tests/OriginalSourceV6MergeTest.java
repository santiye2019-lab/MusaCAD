/* SPDX-License-Identifier: GPL-3.0-or-later */
package com.musa.cad;

import java.io.IOException;
import java.util.Arrays;

/* Pure JDK, no Android runtime or external DWG engine. Exact original v6
 * primitive words, colors, text payload and zero-area bounds survive. */
public final class OriginalSourceV6MergeTest {
    private static void require(boolean yes,String message){
        if(!yes)throw new AssertionError(message);
    }
    private static float[] packet(int n,float minX,float minY,
            float maxX,float maxY,float... primitives) {
        float[] p=new float[7+primitives.length];
        p[0]=6f;p[1]=n;p[2]=minX;p[3]=minY;
        p[4]=maxX;p[5]=maxY;p[6]=1f;
        System.arraycopy(primitives,0,p,7,primitives.length);
        return p;
    }
    private static void reject(float[] a,float[] b,int max,String message)
            throws Exception {
        float[] before=Arrays.copyOf(a,a.length);
        try{
            OriginalSourceV6Merge.join(a,b,max);
            throw new AssertionError("Unexpected acceptance: "+message);
        }catch(IOException expected){}
        require(Arrays.equals(before,a),"Source mutated on failed merge: "+message);
    }
    public static void main(String[] args)throws Exception {
        // Real v6 line ACI 1 and direct RGB24 (-258, RGB24) with precise WCS.
        float[] direct=packet(2,1f,1f,100f,80f,
            257f,1f,1f,4f,4f,
            -258f,1193046f,11f,22f,100f,80f);
        float[] block=packet(2,-20f,-10f,7f,9f,
            263f,-20f,9f,-2f,7f,
            -258f,16711935f,2f,-10f,7f,8f);
        float[] originalDirect=Arrays.copyOf(direct,direct.length);
        float[] originalBlock=Arrays.copyOf(block,block.length);
        float[] merged=OriginalSourceV6Merge.join(direct,block,256);
        require(merged.length==direct.length+block.length-7,"Lossless extent");
        require(merged[0]==6f&&merged[1]==4f&&merged[6]==1f,
                "Partial, never accepted whole drawing");
        require(merged[2]==-20f&&merged[3]==-10f&&
                merged[4]==100f&&merged[5]==80f,"Bounds union");
        for(int i=7;i<direct.length;i++)
            require(merged[i]==direct[i],"Original direct color and XY changed");
        for(int i=7;i<block.length;i++)
            require(merged[direct.length+i-7]==block[i],
                    "Original INSERT source color and XY changed");
        require(OriginalSourceV6Merge.count(merged,256)==4,
                "Both real source family packets parsed");
        require(Arrays.equals(direct,originalDirect)&&
                Arrays.equals(block,originalBlock),"Caller packet mutation");
        float[] empty=packet(0,0f,0f,0f,0f);
        float[] result=OriginalSourceV6Merge.join(direct,empty,256);
        require(Arrays.equals(direct,result)&&result!=direct,
                "Zero-emission INSERT must not fabricate source geometry");
        reject(direct,block,merged.length-1,"merged memory budget");
        float[] wrongHeader=Arrays.copyOf(block,block.length);
        wrongHeader[6]=0f;
        reject(direct,wrongHeader,256,"must never claim complete DWG");
        float[] invalid=Arrays.copyOf(block,block.length);
        invalid[1]=3f;
        reject(direct,invalid,256,"missing source primitive");
        float[] brokenRgb=Arrays.copyOf(block,block.length);
        brokenRgb[14]=16777216f;
        reject(direct,brokenRgb,256,"unrepresentable RGB24");
        float[] badCoords=Arrays.copyOf(block,block.length);
        badCoords[8]=Float.NaN;
        reject(direct,badCoords,256,"unverified source coordinate");
        float[] brokenRecord=Arrays.copyOf(block,block.length-1);
        reject(direct,brokenRecord,256,"truncated original line");
        float[] fakeGeometry=packet(0,0f,0f,0f,0f,257f,1f,1f,2f,2f);
        reject(direct,fakeGeometry,256,"fake empty source");

        // A text source record is retained verbatim: TEXT v6 type6 includes
        // nine metadata words followed by packed three-byte UTF8 words.
        float[] text=packet(1,10f,20f,10f,20f,
                1543f,10f,20f,1f,0f,0f,1f,0f,0f,3f,65793f);
        float[] joinedText=OriginalSourceV6Merge.join(text,block,256);
        require(joinedText[1]==3f&&joinedText[7]==1543f&&
                joinedText[17]==65793f,"Original packed text preserved");
        float[] circle=packet(1,1f,1f,100f,100f,
                1031f,0f,0f,5f,0f,0f,5f,0f,6.2831855f);
        float[] mix=OriginalSourceV6Merge.join(circle,direct,256);
        require(mix[1]==3f&&OriginalSourceV6Merge.count(mix,256)==3,
                "Curves, lines and bounds remain source-intact");
        System.out.println("PASS lossless partial v6 combined source UI stream"
            +" ACI RGB TEXT curve bounds fail atomic, no full DWG claim");
    }
}
