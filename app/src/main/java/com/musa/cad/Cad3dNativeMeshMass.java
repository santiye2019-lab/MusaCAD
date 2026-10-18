package com.musa.cad;

/**
 * Opt-in Android JNI cross-check for the SAME portable C11 mesh-volume engine
 * used by desktop/iOS adapters. The known working Cad3dSignedVolume path
 * remains authoritative until matching real DWG/phone acceptance is complete.
 *
 * On host JVM, a missing native library is not an error: return the original
 * Java result with a clear native-unavailable status. This does not certify
 * an arbitrary mesh as ACIS/B-Rep 3DSOLID or a billable quantity.
 */
public final class Cad3dNativeMeshMass {
    private Cad3dNativeMeshMass(){}
    private static volatile boolean loadAttempted;
    private static volatile boolean libraryReady;

    private static synchronized boolean ensureLibrary() {
        if(!loadAttempted) {
            loadAttempted=true;
            try {
                System.loadLibrary("musacad_dwg");
                libraryReady=true;
            } catch(LinkageError | SecurityException unavailable) {
                libraryReady=false;
            }
        }
        return libraryReady;
    }

    /** Native return [signedVolume, centroidX, centroidY, centroidZ], or null. */
    private static native double[] measureNative(float[] xyz, int[] faces);

    public static final class Comparison {
        public final Cad3dSignedVolume.Result javaResult;
        public final double[] nativeResult;
        public final boolean nativeAvailable, agreesWithJava;
        public final String status;
        private Comparison(Cad3dSignedVolume.Result javaResult, double[] nativeResult,
                           boolean nativeAvailable, boolean agreesWithJava, String status){
            this.javaResult=javaResult;
            this.nativeResult=nativeResult==null?null:nativeResult.clone();
            this.nativeAvailable=nativeAvailable;
            this.agreesWithJava=agreesWithJava;
            this.status=status;
        }
    }

    private static boolean close(double actual,double expected){
        if(!Double.isFinite(actual)||!Double.isFinite(expected))return false;
        return Math.abs(actual-expected)<=
            Math.max(1e-9,1e-12*Math.max(Math.abs(actual),Math.abs(expected)));
    }

    /** Not a parity approval. Strictly compares output to the proven Java path. */
    public static boolean agrees(Cad3dSignedVolume.Result original,double[] candidate){
        return original!=null&&original.available&&candidate!=null&&candidate.length==4&&
            close(candidate[0],original.signedVolume)&&
            close(candidate[1],original.centroidX)&&
            close(candidate[2],original.centroidY)&&
            close(candidate[3],original.centroidZ);
    }

    /**
     * Experimental cross-check only, OFF the UI thread. Does not change
     * Android's existing volume/mass outputs. Calls JNI only after the
     * independently audited index topology passed Java preflight.
     */
    public static Comparison compare(float[] xyz,int[] faces,
                                     Cad3dMeshTopology.Result topology){
        Cad3dSignedVolume.Result original=Cad3dSignedVolume.measure(xyz,faces,topology);
        String vm=System.getProperty("java.vm.name","");
        if("Dalvik".equalsIgnoreCase(vm)||"ART".equalsIgnoreCase(vm))
            return new Comparison(original,null,false,false,
                    "native_authoritative_no_independent_oracle");
        if(!original.available)
            return new Comparison(original,null,false,false,"mesh_not_accepted");
        if(xyz.length/3>1000000||faces.length/3>Cad3dMeshTopology.MAX_AUDITED_TRIANGLES)
            return new Comparison(original,null,false,false,"native_input_limit");
        if(!ensureLibrary())
            return new Comparison(original,null,false,false,"native_unavailable");
        try {
            double[] nativeValues=measureNative(xyz,faces);
            if(nativeValues==null||nativeValues.length!=4)
                return new Comparison(original,null,true,false,"native_rejected");
            boolean match=agrees(original,nativeValues);
            return new Comparison(original,nativeValues,true,match,
                                  match?"cross_check_passed":"native_mismatch");
        } catch(LinkageError unavailable) {
            libraryReady=false;
            return new Comparison(original,null,false,false,"native_unavailable");
        }
    }
}
