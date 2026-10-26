/* SPDX-License-Identifier: GPL-3.0-or-later */
package com.musa.cad;
import java.io.IOException;
import java.util.Arrays;

/** Native+Java acceptance: same real R2018 DWG source is read by BOTH
 * Android JNI entrypoints, the exact Android scene-v6 merge is applied,
 * and every original word (ACI, RGB24, Unicode and WCS) remains intact.
 *
 * This is a PARTIAL source preview, never a paint or editable DWG claim.
 * Prints actual inserted LINE coverage, including zero where there is
 * no proven source geometry. */
final class FirstPartyDwgPreview {
    private static native float[] readOriginalNative(
            String path,int maximum,long[] audit)throws IOException;
    private static native float[] readAllInsertLinesNative(
            String path,int maximum,long[] audit)throws IOException;
    private static void check(boolean v,String s){
        if(!v)throw new AssertionError(s);
    }
    private static void one(String file)throws Exception {
        final int limit=2*1024*1024;
        long[] baseAudit=new long[25],insertAudit=new long[11];
        float[] main=readOriginalNative(file,limit,baseAudit);
        float[] inserts=readAllInsertLinesNative(file,limit,insertAudit);
        int direct=OriginalSourceV6Merge.count(main,limit);
        int children=OriginalSourceV6Merge.count(inserts,limit);
        check(direct>0&&main[6]==1f,"Original source is not a partial 2B scene");
        check(children==(int)insertAudit[5]&&inserts[6]==1f&&
              insertAudit[9]==1&&insertAudit[10]==0,
              "Original INSERT source may not claim full layer/paint");
        float[] combined=OriginalSourceV6Merge.join(main,inserts,limit);
        check(OriginalSourceV6Merge.count(combined,limit)==direct+children,
                "Actual combined Android scene primitive count mismatch");
        check(combined[0]==6f&&combined[6]==1f,
                "Merged preview is always incomplete");
        for(int j=7;j<main.length;j++)
            check(combined[j]==main[j],"Direct original v6 word changed");
        for(int j=7;j<inserts.length;j++)
            check(combined[main.length+j-7]==inserts[j],
                  "World-transformed INSERT source v6 word changed");
        check(main[1]==direct&&inserts[1]==children&&
              baseAudit[17]==direct,
              "Census differs from actually transferred 2B primitives");
        System.out.println("PROOF real original "+file+
            " direct="+direct+
            " verifiedInserts="+insertAudit[2]+
            " unsupportedInserts="+insertAudit[3]+
            " authenticatedChildLINEs="+insertAudit[4]+
            " emittedChildLINEs="+children+
            " combined="+(direct+children)+
            " PARTIAL=1 FINAL_PAINT=0");
    }
    public static void main(String[] args)throws Exception {
        check(args.length>=2,"JNI combined .so and genuine DWG required");
        System.load(args[0]);
        for(int i=1;i<args.length;i++){
            try{
                one(args[i]);
            }catch(IOException unsupported){
                if(i==1)throw unsupported;
                System.out.println("UNSUPPORTED extra genuine source "+
                    args[i]+": "+unsupported.getMessage());
            }
        }
    }
}
